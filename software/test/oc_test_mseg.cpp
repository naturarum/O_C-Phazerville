#include "gtest/gtest.h"

#include "Arduino.h"  // host stubs
#include "engines/MsegEnvelope.h"
#include "src/extern/peaks_resources.h"

using mseg::MsegEnvelope;
using mseg::GateFlags;

namespace {

// run one rising edge, then hold gate high, ticking until pred() or cap.
int TickUntil(MsegEnvelope &e, uint8_t gate, std::function<bool()> pred, int cap) {
  for (int i = 0; i < cap; ++i) {
    e.Tick(gate);
    if (pred()) return i + 1;
    gate &= ~GateFlags::GATE_RISING;  // rising only on first tick
    gate &= ~GateFlags::GATE_FALLING;
  }
  return -1;
}

int ExpectedTicks(uint8_t time_byte, uint8_t shift) {
  uint32_t inc = peaks::lut_env_increments[time_byte] >> shift;
  if (!inc) inc = 1;
  // ticks for phase (starting 0) to first wrap past 2^32
  return (int)((0x100000000ull + inc - 1) / inc);
}

MsegEnvelope MakeTwoPoint(uint8_t time_byte, uint8_t curve = MsegEnvelope::kLinearCurve) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(2);
  e.set_point_level(0, 0);
  e.set_point_level(1, 65535);
  e.set_time(0, time_byte);
  e.set_curve(0, curve);
  e.set_time_shift(0);
  e.set_loop_mode(mseg::ONE_SHOT);
  e.Reset();
  return e;
}

}  // namespace

// 1. Segment timing matches the LUT (± a couple ticks), incl. time_shift.
TEST(Mseg, SegmentTiming) {
  for (uint8_t t : {uint8_t(0), uint8_t(64), uint8_t(128), uint8_t(200)}) {
    auto e = MakeTwoPoint(t);
    int got = TickUntil(e, GateFlags::GATE_RISING | GateFlags::GATE_HIGH,
                        [&] { return e.event_flags() & mseg::EVENT_EOC; },
                        2000000);
    int exp = ExpectedTicks(t, 0);
    EXPECT_NEAR(got, exp, 2) << "time byte " << (int)t;
  }
}

TEST(Mseg, TimeShiftSlowsByPowerOfTwo) {
  auto e = MakeTwoPoint(128);
  e.set_time_shift(3);  // /8
  e.Reset();
  int got = TickUntil(e, GateFlags::GATE_RISING | GateFlags::GATE_HIGH,
                      [&] { return e.event_flags() & mseg::EVENT_EOC; }, 20000000);
  EXPECT_NEAR(got, ExpectedTicks(128, 3), 2);
}

// 2. Endpoints are exact for any curve.
TEST(Mseg, CurveEndpointsExact) {
  for (uint8_t c : {uint8_t(0), uint8_t(40), uint8_t(128), uint8_t(220), uint8_t(255)}) {
    auto e = MakeTwoPoint(180, c);
    // first tick: value should be >= start (0)
    e.Tick(GateFlags::GATE_RISING | GateFlags::GATE_HIGH);
    EXPECT_GE(e.value(), 0);
    // tick to EOC: on the EOC tick, value parks at the last level (65535)
    TickUntil(e, GateFlags::GATE_HIGH,
              [&] { return e.event_flags() & mseg::EVENT_EOC; }, 2000000);
    EXPECT_EQ(e.value(), 65535) << "curve " << (int)c;
  }
}

// 3. Linear curve == straight line within 1 LSB via the preview; curves monotonic.
TEST(Mseg, CurveMonotonicAndLinearDetent) {
  uint16_t vals[128];
  uint8_t px[MsegEnvelope::kMaxPoints];
  uint8_t l0, l1, ph;

  auto lin = MakeTwoPoint(180, MsegEnvelope::kLinearCurve);
  lin.RenderPreview(vals, 128, px, l0, l1, ph);
  for (int x = 0; x < 128; ++x) {
    uint16_t expect = (uint16_t)((uint32_t)x * 65535 / 127);
    EXPECT_NEAR(vals[x], expect, 8) << "x " << x;  // within a few LSB
  }

  auto fast = MakeTwoPoint(180, 0);    // fast start
  fast.RenderPreview(vals, 128, px, l0, l1, ph);
  for (int x = 1; x < 128; ++x) EXPECT_GE(vals[x], vals[x - 1]) << "rising monotonic x " << x;
  EXPECT_EQ(vals[0], 0u);
  EXPECT_EQ(vals[127], 65535u);

  // falling segment with extreme curve stays non-increasing
  MsegEnvelope down;
  down.Init();
  down.set_num_points(2);
  down.set_point_level(0, 65535);
  down.set_point_level(1, 0);
  down.set_time(0, 180);
  down.set_curve(0, 255);
  down.RenderPreview(vals, 128, px, l0, l1, ph);
  for (int x = 1; x < 128; ++x) EXPECT_LE(vals[x], vals[x - 1]) << "falling monotonic x " << x;
  EXPECT_EQ(vals[0], 65535u);
  EXPECT_EQ(vals[127], 0u);
}

// 4. HOLD_POINT parks at the sustain point, releases on gate fall.
TEST(Mseg, HoldPointParkAndRelease) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(4);
  e.set_point_level(0, 0);
  e.set_point_level(1, 50000);
  e.set_point_level(2, 30000);   // sustain here
  e.set_point_level(3, 0);
  for (int s = 0; s < 3; ++s) e.set_time(s, 128);
  e.set_loop(2, 2);              // hold at point 2
  e.set_loop_mode(mseg::HOLD_POINT);
  e.Reset();

  // reach and hold at point 2
  TickUntil(e, GateFlags::GATE_RISING | GateFlags::GATE_HIGH,
            [&] { return !e.active() || e.value() == 30000; }, 2000000);
  // park: value stays at 30000 over a long hold, no EOC
  for (int i = 0; i < 100000; ++i) {
    e.Tick(GateFlags::GATE_HIGH);
    ASSERT_EQ(e.value(), 30000u);
    ASSERT_FALSE(e.event_flags() & mseg::EVENT_EOC);
  }
  // release -> traverse final segment -> EOC, park at 0
  int got = TickUntil(e, GateFlags::GATE_FALLING,
                      [&] { return e.event_flags() & mseg::EVENT_EOC; }, 2000000);
  EXPECT_GT(got, 0);
  EXPECT_EQ(e.value(), 0u);
}

// 5. LOOP_GATE wraps while gate high, releases past loop_end on gate fall.
TEST(Mseg, LoopGateEntryExit) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(5);
  uint16_t lv[5] = {0, 40000, 20000, 60000, 0};
  for (int i = 0; i < 5; ++i) e.set_point_level(i, lv[i]);
  for (int s = 0; s < 4; ++s) e.set_time(s, 120);
  e.set_loop(1, 3);   // loop segments between points 1..3
  e.set_loop_mode(mseg::LOOP_GATE);
  e.Reset();

  // gate held: expect at least two loop wraps
  int wraps = 0;
  uint8_t gate = GateFlags::GATE_RISING | GateFlags::GATE_HIGH;
  for (int i = 0; i < 2000000 && wraps < 3; ++i) {
    e.Tick(gate);
    gate = GateFlags::GATE_HIGH;
    if (e.event_flags() & mseg::EVENT_LOOP_WRAP) ++wraps;
    ASSERT_FALSE(e.event_flags() & mseg::EVENT_EOC) << "no EOC while looping";
  }
  EXPECT_GE(wraps, 2);

  // release -> must reach EOC (leaves the loop)
  int got = TickUntil(e, GateFlags::GATE_FALLING,
                      [&] { return e.event_flags() & mseg::EVENT_EOC; }, 2000000);
  EXPECT_GT(got, 0);
  EXPECT_EQ(e.value(), 0u);
}

// 6. LOOP_ALWAYS runs forever, no EOC.
TEST(Mseg, LoopAlwaysIsLfo) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(3);
  e.set_point_level(0, 0);
  e.set_point_level(1, 65535);
  e.set_point_level(2, 0);
  e.set_time(0, 100);
  e.set_time(1, 100);
  e.set_loop(0, 2);
  e.set_loop_mode(mseg::LOOP_ALWAYS);
  e.Reset();

  int wraps = 0;
  uint8_t gate = GateFlags::GATE_RISING | GateFlags::GATE_HIGH;
  for (int i = 0; i < 2000000 && wraps < 3; ++i) {
    e.Tick(gate);
    gate = GateFlags::GATE_HIGH;
    ASSERT_FALSE(e.event_flags() & mseg::EVENT_EOC);
    if (e.event_flags() & mseg::EVENT_LOOP_WRAP) ++wraps;
  }
  EXPECT_GE(wraps, 3);
  EXPECT_TRUE(e.active());
}

// 7. Soft retrigger continues from current value; hard restarts from level[0].
TEST(Mseg, RetrigSoftVsHard) {
  // climb (single slow rising segment) until output crosses mid-scale.
  auto climb = [](MsegEnvelope &e) {
    uint8_t gate = GateFlags::GATE_RISING | GateFlags::GATE_HIGH;
    for (int i = 0; i < 2000000; ++i) {
      e.Tick(gate);
      gate = GateFlags::GATE_HIGH;
      if (e.value() >= 30000) return;
    }
  };

  auto soft = MakeTwoPoint(200);
  soft.set_point_level(0, 0);
  soft.set_point_level(1, 65535);
  soft.set_retrig_mode(mseg::RETRIG_SOFT);
  soft.Reset();
  climb(soft);
  uint16_t mid = soft.value();
  ASSERT_GE(mid, 30000);
  soft.Tick(GateFlags::GATE_RISING | GateFlags::GATE_HIGH);
  EXPECT_NEAR(soft.value(), mid, 2000) << "soft retrig stays near current value";

  auto hard = MakeTwoPoint(200);
  hard.set_point_level(0, 0);
  hard.set_point_level(1, 65535);
  hard.set_retrig_mode(mseg::RETRIG_HARD);
  hard.Reset();
  climb(hard);
  ASSERT_GE(hard.value(), 30000);
  hard.Tick(GateFlags::GATE_RISING | GateFlags::GATE_HIGH);
  EXPECT_LT(hard.value(), 2000) << "hard retrig jumps back toward level[0]";
}

// 8. EOC flag is latched only on the tick it happens.
TEST(Mseg, EocFlagLatch) {
  auto e = MakeTwoPoint(60);
  int got = TickUntil(e, GateFlags::GATE_RISING | GateFlags::GATE_HIGH,
                      [&] { return e.event_flags() & mseg::EVENT_EOC; }, 2000000);
  EXPECT_GT(got, 0);
  // next tick: flag cleared
  e.Tick(GateFlags::GATE_HIGH);
  EXPECT_FALSE(e.event_flags() & mseg::EVENT_EOC);
}

// 9. Degenerate configs don't hang or misbehave.
TEST(Mseg, DegenerateSingleSegment) {
  auto e = MakeTwoPoint(0);  // fastest single segment
  int got = TickUntil(e, GateFlags::GATE_RISING | GateFlags::GATE_HIGH,
                      [&] { return e.event_flags() & mseg::EVENT_EOC; }, 100000);
  EXPECT_GT(got, 0);
}

TEST(Mseg, DegenerateLoopClampAndAmplitude) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(4);
  e.set_loop(5, 2);          // start>points, end<start -> clamps
  EXPECT_LE(e.loop_start(), e.num_points() - 1);
  EXPECT_LE(e.loop_end(), e.num_points() - 1);
  EXPECT_LE(e.loop_start(), e.loop_end());

  // shrinking points clamps loop
  e.set_loop(3, 3);
  e.set_num_points(2);
  EXPECT_LE(e.loop_end(), e.num_points() - 1);

  // amplitude 0 -> output 0 but EOC still fires
  auto z = MakeTwoPoint(60);
  z.set_amplitude(0);
  z.Reset();
  bool eoc = false;
  for (int i = 0; i < 100000 && !eoc; ++i) {
    uint16_t v = z.Tick(GateFlags::GATE_HIGH | (i == 0 ? GateFlags::GATE_RISING : 0));
    EXPECT_EQ(v, 0u);
    if (z.event_flags() & mseg::EVENT_EOC) eoc = true;
  }
  EXPECT_TRUE(eoc);
}

// 10. Preview geometry.
TEST(Mseg, PreviewGeometry) {
  MsegEnvelope e;
  e.Init();
  e.set_num_points(4);
  e.set_time(0, 40);
  e.set_time(1, 200);   // much longer -> more pixels
  e.set_time(2, 40);
  e.set_loop(1, 2);
  uint16_t vals[128];
  uint8_t px[MsegEnvelope::kMaxPoints];
  uint8_t l0, l1, ph;
  e.RenderPreview(vals, 128, px, l0, l1, ph);
  EXPECT_EQ(px[0], 0);
  EXPECT_EQ(px[3], 127);
  for (int i = 1; i < 4; ++i) EXPECT_GT(px[i], px[i - 1]) << "monotonic point x " << i;
  EXPECT_EQ(l0, px[1]);
  EXPECT_EQ(l1, px[2]);
  // segment 1 (long) wider than segment 0 (short)
  EXPECT_GT(px[2] - px[1], px[1] - px[0]);
}
