// MsegEnvelope: a Bitwig-style multi-segment envelope generator.
//
// Pure integer logic — no OC::, no gfx, host-testable (see engines/README.md).
// The app adapter (apps/Enveloop.h) owns one instance per channel, pushes
// config on edits, and calls Tick() once per 60us core ISR tick.
//
// Model: 2..9 points define 1..8 segments. Each point has a 16-bit level;
// each segment has a time (index into the shared peaks time LUT) and a
// continuous curve. A selectable point range [loop_start, loop_end] is the
// sustain/loop region; LoopMode decides what happens there.
//
// Curve: a purpose-built Schlick bias, integer-only and ENDPOINT-EXACT in
// both directions (curve==kLinearCurve is a straight line; every segment
// actually reaches its target level). tideslite's WarpPhase was evaluated
// but maps full-scale asymmetrically (65025 vs 65535 across the linear
// point), so it can't land exactly on segment endpoints — which matters for
// envelopes. Timing reuses peaks::lut_env_increments (already in flash).
#pragma once

#include <stdint.h>
#include "../src/extern/peaks_resources.h"  // peaks::lut_env_increments[257]

namespace mseg {

enum GateFlags : uint8_t {
  GATE_HIGH    = 0x01,
  GATE_RISING  = 0x02,
  GATE_FALLING = 0x04,
};

enum EventFlags : uint8_t {
  EVENT_NONE      = 0x00,
  EVENT_EOC       = 0x01,  // envelope finished (one-shot / release end)
  EVENT_LOOP_WRAP = 0x02,  // wrapped from loop_end back to loop_start
};

enum LoopMode : uint8_t {
  ONE_SHOT,     // run all segments, park at last level, EOC
  HOLD_POINT,   // classic sustain: hold at loop_start while gate high,
                // release through remaining segments on gate fall
  LOOP_GATE,    // Bitwig sustain-loop: wrap loop_end->loop_start while gate
                // high, release past loop_end on gate fall
  LOOP_ALWAYS,  // free-running LFO: always wrap loop_end->loop_start
  LOOP_MODE_LAST
};

enum RetrigMode : uint8_t {
  RETRIG_HARD,  // gate rising: restart from level[0]
  RETRIG_SOFT,  // gate rising: restart from current output (click-free)
  RETRIG_MODE_LAST
};

class MsegEnvelope {
public:
  static constexpr int kMaxPoints = 9;
  static constexpr int kMaxSegments = kMaxPoints - 1;
  static constexpr uint16_t kLinearCurve = 128;  // U8 curve setting, 0..255

  void Init() {
    num_points_ = 3;
    for (int i = 0; i < kMaxPoints; ++i) level_[i] = 0;
    level_[1] = 65535;
    for (int i = 0; i < kMaxSegments; ++i) { time_[i] = 40; SetCurveByte(i, kLinearCurve); }
    time_[0] = 24;   // ~10ms attack
    time_[1] = 96;   // ~250ms decay
    time_shift_ = 0;
    amplitude_ = 65535;
    loop_start_ = 1;
    loop_end_ = 1;
    loop_mode_ = HOLD_POINT;
    retrig_mode_ = RETRIG_HARD;
    Reset();
  }

  // ---- configuration (app pushes on dirty; tests call directly) ----
  void set_num_points(uint8_t n) {
    num_points_ = n < 2 ? 2 : (n > kMaxPoints ? kMaxPoints : n);
    ClampLoop();
  }
  void set_point_level(uint8_t i, uint16_t lvl) { if (i < kMaxPoints) level_[i] = lvl; }
  void set_time(uint8_t seg, uint8_t t) { if (seg < kMaxSegments) time_[seg] = t; }
  // curve byte: 0..255, 128 == linear. <128 fast-start (log), >128 slow-start (exp).
  void set_curve(uint8_t seg, uint8_t c) { if (seg < kMaxSegments) SetCurveByte(seg, c); }
  void set_time_shift(uint8_t sh) { time_shift_ = sh > 13 ? 13 : sh; }
  void set_amplitude(uint16_t a) { amplitude_ = a; }
  void set_loop(uint8_t start_pt, uint8_t end_pt) {
    loop_start_ = start_pt;
    loop_end_ = end_pt;
    ClampLoop();
  }
  void set_loop_mode(LoopMode m) { loop_mode_ = m; }
  void set_retrig_mode(RetrigMode m) { retrig_mode_ = m; }

  uint8_t num_points() const { return num_points_; }
  uint8_t num_segments() const { return num_points_ - 1; }
  uint16_t point_level(uint8_t i) const { return level_[i]; }
  uint8_t loop_start() const { return loop_start_; }
  uint8_t loop_end() const { return loop_end_; }

  // ---- hot path: one call per 60us tick ----
  uint16_t Tick(uint8_t gate_flags) {
    events_ = EVENT_NONE;
    gate_high_ = gate_flags & GATE_HIGH;

    if (gate_flags & GATE_RISING) Trigger();
    if (gate_flags & GATE_FALLING) Release();

    if (state_ == RUNNING) {
      uint32_t prev = phase_;
      phase_ += increment_;
      if (phase_ < prev) AdvanceSegment();  // phase wrapped -> segment done
      if (state_ == RUNNING) ComputeValue();
      // (HOLDING/IDLE keep value_ frozen at the current level)
    }

    if (amplitude_ == 65535) return value_;
    return (uint16_t)(((uint32_t)value_ * amplitude_) >> 16);
  }

  uint8_t event_flags() const { return events_; }
  uint16_t value() const { return value_; }
  bool active() const { return state_ != IDLE; }
  uint8_t current_segment() const { return segment_; }

  void Reset() {
    state_ = IDLE;
    segment_ = 0;
    phase_ = 0;
    a_level_ = level_[0];
    value_ = level_[0];
    released_ = false;
    events_ = EVENT_NONE;
  }

  // ---- UI helper: never call from the ISR ----
  // Fills values[width] (0..65535), point_x[num_points] pixel positions,
  // the loop region x-range, and the current playhead x. Returns width.
  uint16_t RenderPreview(uint16_t *values, uint8_t width,
                         uint8_t *point_x,
                         uint8_t &loop_x0, uint8_t &loop_x1,
                         uint8_t &playhead_x) const {
    if (width < 2) width = 2;
    const int nseg = num_segments();

    // segment pixel widths proportional to duration (ticks ~ 2^32/increment),
    // min 2px so short segments stay visible/grabbable.
    uint32_t dur[kMaxSegments];
    uint64_t total = 0;
    for (int s = 0; s < nseg; ++s) {
      uint32_t inc = Increment(s);
      dur[s] = inc ? (uint32_t)(0xFFFFFFFFull / inc) : 1;
      total += dur[s];
    }
    if (total == 0) total = 1;

    int min_w = 2;
    int avail = width - 1 - min_w * nseg;
    if (avail < 0) avail = 0;

    int x = 0;
    point_x[0] = 0;
    for (int s = 0; s < nseg; ++s) {
      int seg_w = min_w + (int)((uint64_t)dur[s] * avail / total);
      int x_end = (s == nseg - 1) ? (width - 1) : (x + seg_w);
      if (x_end <= x) x_end = x + 1;
      if (x_end > width - 1) x_end = width - 1;
      for (int px = x; px < x_end; ++px) {
        uint32_t t = (uint32_t)(px - x) * 65535 / (x_end - x);
        uint16_t frac = Curve((uint16_t)t, curve_c_[s]);
        values[px] = Lerp(level_[s], level_[s + 1], frac);
      }
      x = x_end;
      point_x[s + 1] = (uint8_t)x;
    }
    values[width - 1] = level_[nseg];

    loop_x0 = point_x[loop_start_ < num_points_ ? loop_start_ : num_points_ - 1];
    loop_x1 = point_x[loop_end_ < num_points_ ? loop_end_ : num_points_ - 1];

    if (state_ == IDLE) {
      playhead_x = 0;
    } else {
      // map current (segment_, phase_) into the same x space
      int px = point_x[segment_];
      int seg_px = point_x[segment_ + 1] - point_x[segment_];
      playhead_x = (uint8_t)(px + ((uint64_t)(phase_ >> 16) * seg_px >> 16));
    }
    return width;
  }

private:
  enum State : uint8_t { IDLE, RUNNING, HOLDING };

  // Schlick bias, endpoint-exact & symmetric. t,return in 0..65535.
  // c_q16 == 0 is linear; c>0 bends one way, mirrored for c<0.
  static uint16_t Curve(uint16_t t, int32_t c_q16) {
    if (c_q16 == 0) return t;
    if (c_q16 > 0) return BiasPos(t, c_q16);
    return 65535 - BiasPos(65535 - t, -c_q16);
  }
  static uint16_t BiasPos(uint32_t t, int32_t c_q16) {
    // bias(t) = t / (c*(1-t) + 1); c>=0 keeps the denominator >= 1.0 (Q16),
    // so this is stable and endpoint-exact for all t.
    int64_t d = ((int64_t)c_q16 * (int32_t)(65535 - t)) / 65535 + 65536;
    uint32_t y = (uint32_t)(((uint64_t)t << 16) / (uint64_t)d);
    return y > 65535 ? 65535 : (uint16_t)y;
  }
  static uint16_t Lerp(uint16_t a, uint16_t b, uint16_t frac) {
    return (uint16_t)((int32_t)a + ((int64_t)((int32_t)b - a) * frac >> 16));
  }

  void SetCurveByte(uint8_t seg, uint8_t c) {
    // 128 -> 0 (linear); spread to ~±4.0 in Q16 at the extremes.
    curve_c_[seg] = ((int32_t)kLinearCurve - (int32_t)c) * 2064;
  }

  uint32_t Increment(int seg) const {
    uint32_t inc = peaks::lut_env_increments[time_[seg]] >> time_shift_;
    return inc ? inc : 1;
  }

  void ClampLoop() {
    if (loop_start_ > num_points_ - 1) loop_start_ = num_points_ - 1;
    if (loop_end_ > num_points_ - 1) loop_end_ = num_points_ - 1;
    if (loop_end_ < loop_start_) loop_end_ = loop_start_;
  }

  void EnterSegment(uint8_t seg) {
    segment_ = seg;
    phase_ = 0;
    a_level_ = level_[seg];
    increment_ = Increment(seg);
  }

  void Trigger() {
    released_ = false;
    if (retrig_mode_ == RETRIG_SOFT && state_ != IDLE) {
      segment_ = 0;
      phase_ = 0;
      a_level_ = value_;                 // start segment 0 from current output
      increment_ = Increment(0);
    } else {
      EnterSegment(0);
    }
    state_ = RUNNING;
  }

  void Release() {
    released_ = true;
    if (state_ == HOLDING) {
      // resume from the hold point through the remaining segments
      EnterSegment(loop_start_);
      state_ = RUNNING;
    }
  }

  // Called when the current segment's phase wraps. Decides the next state.
  void AdvanceSegment() {
    uint8_t arrived = segment_ + 1;       // point index we just reached
    const uint8_t last = num_points_ - 1;

    switch (loop_mode_) {
      case HOLD_POINT:
        if (arrived == loop_start_ && (gate_high_state() && !released_)) {
          state_ = HOLDING;
          value_ = level_[loop_start_];
          phase_ = 0;
          return;
        }
        break;
      case LOOP_GATE:
        if (loop_end_ > loop_start_ && arrived == loop_end_ &&
            gate_high_state() && !released_) {
          EnterSegment(loop_start_);
          events_ |= EVENT_LOOP_WRAP;
          return;
        }
        // degenerate loop (end<=start): behave like HOLD_POINT at loop_start
        if (loop_end_ <= loop_start_ && arrived == loop_start_ &&
            gate_high_state() && !released_) {
          state_ = HOLDING;
          value_ = level_[loop_start_];
          phase_ = 0;
          return;
        }
        break;
      case LOOP_ALWAYS:
        if (loop_end_ > loop_start_ && arrived == loop_end_) {
          EnterSegment(loop_start_);
          events_ |= EVENT_LOOP_WRAP;
          return;
        }
        break;
      case ONE_SHOT:
      default:
        break;
    }

    if (arrived >= last) {
      // reached the end
      state_ = IDLE;
      value_ = level_[last];
      phase_ = 0;
      events_ |= EVENT_EOC;
      return;
    }
    EnterSegment(arrived);
  }

  void ComputeValue() {
    uint16_t frac = Curve((uint16_t)(phase_ >> 16), curve_c_[segment_]);
    value_ = Lerp(a_level_, level_[segment_ + 1], frac);
  }

  // sustained gate level, cached from Tick's GATE_HIGH flag
  bool gate_high_state() const { return gate_high_; }

  // config
  uint8_t num_points_ = 3;
  uint16_t level_[kMaxPoints] = {0};
  uint8_t time_[kMaxSegments] = {0};
  int32_t curve_c_[kMaxSegments] = {0};
  uint8_t time_shift_ = 0;
  uint16_t amplitude_ = 65535;
  uint8_t loop_start_ = 1, loop_end_ = 1;
  LoopMode loop_mode_ = HOLD_POINT;
  RetrigMode retrig_mode_ = RETRIG_HARD;

  // state
  State state_ = IDLE;
  uint8_t segment_ = 0;
  uint32_t phase_ = 0;
  uint32_t increment_ = 1;
  uint16_t a_level_ = 0;      // start level of the current segment
  uint16_t value_ = 0;
  bool released_ = false;
  bool gate_high_ = false;
  uint8_t events_ = EVENT_NONE;
};

}  // namespace mseg
