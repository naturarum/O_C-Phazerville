// Copyright (c) 2026, Phazerville fork
//
// MIT licensed — see licensing notes in the repository root.

// Enveloop: a 4-channel Bitwig-style multi-segment envelope generator (MSEG).
//
// Each output is an independent envelope of up to 8 segments, each with its
// own time and a continuous log<->lin<->exp curve. A selectable point range
// is the sustain/loop region (hold, gate-loop, or free-run LFO). Triggered
// from TR1-4 or another channel's end-of-cycle, for chaining.
//
// The DSP core lives in src/engines/MsegEnvelope.h (host-tested); this file
// is the adapter (settings, I/O, UI). See src/engines/README.md.

#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "OC_menus.h"
#include "HSUtils.h"
#include "util/util_settings.h"
#include "engines/MsegEnvelope.h"

namespace enveloop {

static constexpr int kChannels = DAC_CHANNEL_COUNT;
static constexpr int kMaxPoints = mseg::MsegEnvelope::kMaxPoints;      // 9
static constexpr int kMaxSegments = mseg::MsegEnvelope::kMaxSegments;  // 8

const char* const loop_mode_names[] = { "1shot", "Hold", "Loop", "LFO" };
const char* const retrig_mode_names[] = { "Hard", "Soft" };
const char* const trigger_source_names[] = {
  "TR1", "TR2", "TR3", "TR4", "EOC1", "EOC2", "EOC3", "EOC4"
};
const char* const cv_source_names[] = { "off", "CV1", "CV2", "CV3", "CV4" };

enum EnveloopSetting {
  ENVL_LEVEL0, ENVL_LEVEL1, ENVL_LEVEL2, ENVL_LEVEL3, ENVL_LEVEL4,
  ENVL_LEVEL5, ENVL_LEVEL6, ENVL_LEVEL7, ENVL_LEVEL8,
  ENVL_TIME0, ENVL_TIME1, ENVL_TIME2, ENVL_TIME3,
  ENVL_TIME4, ENVL_TIME5, ENVL_TIME6, ENVL_TIME7,
  ENVL_CURVE0, ENVL_CURVE1, ENVL_CURVE2, ENVL_CURVE3,
  ENVL_CURVE4, ENVL_CURVE5, ENVL_CURVE6, ENVL_CURVE7,
  ENVL_NUM_POINTS,
  ENVL_LOOP_START,
  ENVL_LOOP_END,
  ENVL_LOOP_MODE,
  ENVL_RETRIG_MODE,
  ENVL_TIME_SCALE,
  ENVL_AMPLITUDE,
  ENVL_TRIGGER_SOURCE,
  ENVL_CV_TIME,
  ENVL_CV_AMP,
  ENVL_SETTING_LAST
};

class EnveloopChannel : public settings::SettingsBase<EnveloopChannel, ENVL_SETTING_LAST> {
public:
  // EEPROM: 9+8+8 U8 = 25 B, + Points/Loop*/scale/amp (U8) + 5 U4 ~= 30.5 B.
  SETTINGS_ARRAY_DECLARE() {{
    { 0,   0, 255, "Lvl 1", NULL, settings::STORAGE_TYPE_U8 },
    { 255, 0, 255, "Lvl 2", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 3", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 4", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 5", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 6", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 7", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 8", NULL, settings::STORAGE_TYPE_U8 },
    { 0,   0, 255, "Lvl 9", NULL, settings::STORAGE_TYPE_U8 },
    { 24, 0, 255, "Time 1", NULL, settings::STORAGE_TYPE_U8 },
    { 96, 0, 255, "Time 2", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 3", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 4", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 5", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 6", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 7", NULL, settings::STORAGE_TYPE_U8 },
    { 40, 0, 255, "Time 8", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 1", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 2", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 3", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 4", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 5", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 6", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 7", NULL, settings::STORAGE_TYPE_U8 },
    { 128, 0, 255, "Crv 8", NULL, settings::STORAGE_TYPE_U8 },
    { 3, 2, kMaxPoints, "Points", NULL, settings::STORAGE_TYPE_U8 },
    { 1, 0, kMaxPoints - 1, "Loop start", NULL, settings::STORAGE_TYPE_U8 },
    { 1, 0, kMaxPoints - 1, "Loop end", NULL, settings::STORAGE_TYPE_U8 },
    { mseg::HOLD_POINT, 0, mseg::LOOP_MODE_LAST - 1, "Loop mode", loop_mode_names, settings::STORAGE_TYPE_U4 },
    { mseg::RETRIG_HARD, 0, mseg::RETRIG_MODE_LAST - 1, "Retrigger", retrig_mode_names, settings::STORAGE_TYPE_U4 },
    { 0, 0, 13, "Time scale", NULL, settings::STORAGE_TYPE_U8 },
    { 255, 0, 255, "Amplitude", NULL, settings::STORAGE_TYPE_U8 },
    { 0, 0, kChannels * 2 - 1, "Trigger", trigger_source_names, settings::STORAGE_TYPE_U4 },
    { 0, 0, kChannels, "CV->time", cv_source_names, settings::STORAGE_TYPE_U4 },
    { 0, 0, kChannels, "CV->amp", cv_source_names, settings::STORAGE_TYPE_U4 },
  }};

  void InitChannel() {
    InitDefaults();
    engine_.Init();
    apply_value(ENVL_NUM_POINTS, engine_.num_points());
    for (int i = 0; i < kMaxPoints; ++i)
      apply_value(ENVL_LEVEL0 + i, engine_.point_level(i) >> 8);
    apply_value(ENVL_LOOP_START, engine_.loop_start());
    apply_value(ENVL_LOOP_END, engine_.loop_end());
    dirty_ = true;
  }

  uint8_t num_points() const { return values_[ENVL_NUM_POINTS]; }
  uint8_t level_byte(int i) const { return values_[ENVL_LEVEL0 + i]; }
  uint8_t time_byte(int s) const { return values_[ENVL_TIME0 + s]; }
  uint8_t curve_byte(int s) const { return values_[ENVL_CURVE0 + s]; }
  uint8_t loop_start_v() const { return values_[ENVL_LOOP_START]; }
  uint8_t loop_end_v() const { return values_[ENVL_LOOP_END]; }
  mseg::LoopMode loop_mode() const { return (mseg::LoopMode)values_[ENVL_LOOP_MODE]; }
  mseg::RetrigMode retrig_mode() const { return (mseg::RetrigMode)values_[ENVL_RETRIG_MODE]; }
  uint8_t time_scale() const { return values_[ENVL_TIME_SCALE]; }
  uint8_t amplitude() const { return values_[ENVL_AMPLITUDE]; }
  int trigger_source() const { return values_[ENVL_TRIGGER_SOURCE]; }
  int cv_time_src() const { return values_[ENVL_CV_TIME]; }
  int cv_amp_src() const { return values_[ENVL_CV_AMP]; }

  void MarkDirty() { dirty_ = true; }
  mseg::MsegEnvelope& engine() { return engine_; }
  const mseg::MsegEnvelope& engine() const { return engine_; }
  uint8_t eoc_pulse() const { return eoc_pulse_; }
  bool triggered_display() const { return triggered_display_; }

  int BuildEnabledSettings(EnveloopSetting *dest) const {
    EnveloopSetting *p = dest;
    *p++ = ENVL_NUM_POINTS;
    *p++ = ENVL_LOOP_MODE;
    *p++ = ENVL_LOOP_START;
    *p++ = ENVL_LOOP_END;
    *p++ = ENVL_RETRIG_MODE;
    *p++ = ENVL_TIME_SCALE;
    *p++ = ENVL_AMPLITUDE;
    *p++ = ENVL_TRIGGER_SOURCE;
    *p++ = ENVL_CV_TIME;
    *p++ = ENVL_CV_AMP;
    return p - dest;
  }

  void PushConfig() {
    engine_.set_num_points(num_points());
    for (int i = 0; i < kMaxPoints; ++i) {
      uint8_t b = level_byte(i);
      engine_.set_point_level(i, (b << 8) | b);   // 255 -> 65535 exact
    }
    for (int s = 0; s < kMaxSegments; ++s) {
      engine_.set_time(s, time_byte(s));
      engine_.set_curve(s, curve_byte(s));
    }
    engine_.set_loop(loop_start_v(), loop_end_v());
    engine_.set_loop_mode(loop_mode());
    engine_.set_retrig_mode(retrig_mode());
  }

  void Update(OC::IOFrame *ioframe, uint32_t triggers, uint8_t eoc_mask,
              const int32_t *cvs, int ch, bool manual_gate) {
    if (dirty_) { PushConfig(); dirty_ = false; }

    int shift = time_scale();
    if (cv_time_src()) shift += cvs[cv_time_src() - 1] >> 9;
    engine_.set_time_shift((uint8_t)constrain(shift, 0, 13));

    uint32_t amp = (amplitude() << 8) | amplitude();
    if (cv_amp_src()) {
      int32_t a = (int32_t)amp + (cvs[cv_amp_src() - 1] << 4);
      amp = (uint32_t)constrain(a, 0, 65535);
    }
    engine_.set_amplitude((uint16_t)amp);

    bool triggered = false, gate_high = false;
    int ts = trigger_source();
    if (ts < OC::DIGITAL_INPUT_LAST) {
      triggered = triggers & DIGITAL_INPUT_MASK(ts);
      gate_high = OC::DigitalInputs::read_immediate((OC::DigitalInput)ts);
    } else {
      int src = ts - OC::DIGITAL_INPUT_LAST;   // EOC of channel 0..3
      triggered = eoc_mask & (0x1 << src);
      gate_high = false;
    }
    if (manual_gate) { gate_high = true; if (!gate_prev_) triggered = true; }

    uint8_t gate_flags = 0;
    if (triggered) gate_flags |= mseg::GATE_RISING;
    if (gate_high) gate_flags |= mseg::GATE_HIGH;
    else if (gate_prev_) gate_flags |= mseg::GATE_FALLING;
    gate_prev_ = gate_high;

    uint16_t value = engine_.Tick(gate_flags);
    ioframe->outputs.set_unipolar_value(ch, value);

    eoc_pulse_ = engine_.event_flags() & mseg::EVENT_EOC;
    triggered_display_ = triggered || gate_high;
  }

private:
  mseg::MsegEnvelope engine_;
  bool dirty_ = true;
  bool gate_prev_ = false;
  uint8_t eoc_pulse_ = 0;
  bool triggered_display_ = false;
};

}  // namespace enveloop

SETTINGS_ARRAY_DEFINE(enveloop::EnveloopChannel);

// Guard the app's EEPROM footprint (shared T3.2 app pool is ~956 bytes for
// ALL enabled apps combined; keep our slice modest).
static_assert(enveloop::kChannels * enveloop::EnveloopChannel::storageSize() < 200,
              "Enveloop EEPROM footprint too large");

using namespace enveloop;

OC_APP_CLASS(Enveloop, TWOCCS("EL"), "Enveloop", "MSEG") {
public:
  OC_APP_INTERFACE_DECLARE(Enveloop, kChannels * EnveloopChannel::storageSize());

  enum EditMode { MODE_EDIT, MODE_SETTINGS };
  enum Field { FIELD_LEVEL, FIELD_TIME, FIELD_CURVE, FIELD_LAST };

  void Start() {
    for (int ch = 0; ch < kChannels; ++ch) channels_[ch].InitChannel();
    Resume();
  }

  void Resume() {
    for (int ch = 0; ch < kChannels; ++ch) channels_[ch].MarkDirty();
    RebuildCursor();
  }

  void RebuildCursor() {
    num_enabled_ = channels_[selected_channel_].BuildEnabledSettings(enabled_);
    cursor_.Init(0, num_enabled_ - 1);
  }

private:
  static constexpr int kTopY = 22;
  static constexpr int kBotY = 63;
  static constexpr int kH = 42;

  EnveloopChannel channels_[kChannels];
  int selected_channel_ = 0;
  int sel_point_ = 0;
  Field field_ = FIELD_LEVEL;
  bool loop_edit_ = false;
  bool manual_gate_ = false;
  EditMode edit_mode_ = MODE_EDIT;

  menu::ScreenCursor<menu::kScreenLines> cursor_;
  EnveloopSetting enabled_[ENVL_SETTING_LAST];
  int num_enabled_ = 0;

  EnveloopChannel& sel() { return channels_[selected_channel_]; }
  const EnveloopChannel& sel() const { return channels_[selected_channel_]; }

  void ClampSelPoint() {
    int last = sel().num_points() - 1;
    if (sel_point_ > last) sel_point_ = last;
    if (sel_point_ < 0) sel_point_ = 0;
    if (field_ != FIELD_LEVEL && sel_point_ == last) field_ = FIELD_LEVEL;
  }

  void NextChannel(int dir) {
    selected_channel_ = (selected_channel_ + kChannels + dir) % kChannels;
    ClampSelPoint();
    RebuildCursor();
  }

  void OnRightPress() {
    if (edit_mode_ == MODE_EDIT) {
      int last = sel().num_points() - 1;
      field_ = (Field)((field_ + 1) % FIELD_LAST);
      if (sel_point_ == last) field_ = FIELD_LEVEL;  // no segment past last pt
    } else {
      cursor_.toggle_editing();
    }
  }

  void OnLeftPress() {
    if (edit_mode_ == MODE_EDIT) AddPoint();
  }

  void AddPoint() {
    auto &ch = sel();
    int n = ch.num_points();
    if (n >= kMaxPoints) return;
    int at = sel_point_;                 // split the segment to the right
    if (at >= n - 1) at = n - 2;
    // shift levels/times/curves right of `at` up by one
    for (int i = n; i > at + 1; --i)
      ch.apply_value(ENVL_LEVEL0 + i, ch.get_value(ENVL_LEVEL0 + i - 1));
    for (int s = kMaxSegments - 1; s > at; --s) {
      ch.apply_value(ENVL_TIME0 + s, ch.get_value(ENVL_TIME0 + s - 1));
      ch.apply_value(ENVL_CURVE0 + s, ch.get_value(ENVL_CURVE0 + s - 1));
    }
    // new point at midpoint level of the split segment
    int mid = (ch.level_byte(at) + ch.level_byte(at + 2 <= n ? at + 1 : at)) / 2;
    ch.apply_value(ENVL_LEVEL0 + at + 1, mid);
    ch.apply_value(ENVL_TIME0 + at, ch.time_byte(at));
    ch.apply_value(ENVL_NUM_POINTS, n + 1);
    ch.MarkDirty();
    sel_point_ = at + 1;
  }

  void DeletePoint() {
    if (edit_mode_ != MODE_EDIT) return;
    auto &ch = sel();
    int n = ch.num_points();
    if (n <= 2) return;
    int at = sel_point_;
    for (int i = at; i < n - 1; ++i)
      ch.apply_value(ENVL_LEVEL0 + i, ch.get_value(ENVL_LEVEL0 + i + 1));
    for (int s = at; s < kMaxSegments - 1; ++s) {
      ch.apply_value(ENVL_TIME0 + s, ch.get_value(ENVL_TIME0 + s + 1));
      ch.apply_value(ENVL_CURVE0 + s, ch.get_value(ENVL_CURVE0 + s + 1));
    }
    ch.apply_value(ENVL_NUM_POINTS, n - 1);
    ch.MarkDirty();
    ClampSelPoint();
  }

  void OnLeftEncoder(int dir) {
    if (edit_mode_ != MODE_EDIT) return;
    auto &ch = sel();
    if (loop_edit_) {
      ch.change_value(ENVL_LOOP_START, dir);
      ch.MarkDirty();
    } else {
      sel_point_ = constrain(sel_point_ + dir, 0, ch.num_points() - 1);
      ClampSelPoint();
    }
  }

  void OnRightEncoder(int dir) {
    auto &ch = sel();
    if (edit_mode_ == MODE_SETTINGS) {
      if (cursor_.editing()) {
        ch.change_value(enabled_[cursor_.cursor_pos()], dir);
        ch.MarkDirty();
      } else {
        cursor_.Scroll(dir);
      }
      return;
    }
    if (loop_edit_) {
      ch.change_value(ENVL_LOOP_END, dir);
      ch.MarkDirty();
      return;
    }
    switch (field_) {
      case FIELD_LEVEL: ch.change_value(ENVL_LEVEL0 + sel_point_, dir); break;
      case FIELD_TIME:  ch.change_value(ENVL_TIME0 + sel_point_, dir); break;
      case FIELD_CURVE: ch.change_value(ENVL_CURVE0 + sel_point_, dir); break;
      default: break;
    }
    ch.MarkDirty();
  }

  void DrawEditor() const {
    const auto &ch = sel();

    gfxPrint(0, 13, "P");
    gfxPrint(sel_point_ + 1);
    int last = ch.num_points() - 1;
    if (field_ == FIELD_LEVEL || sel_point_ == last) {
      gfxPrint(26, 13, "Lv ");
      gfxPrint(ch.level_byte(sel_point_));
    } else if (field_ == FIELD_TIME) {
      gfxPrint(26, 13, "Tm ");
      gfxPrint(ch.time_byte(sel_point_));
    } else {
      gfxPrint(26, 13, "Cv ");
      gfxPrint(ch.curve_byte(sel_point_) - 128);
    }
    if (loop_edit_) gfxPrint(96, 13, "LP");

    uint16_t vals[128];
    uint8_t px[kMaxPoints];
    uint8_t lx0, lx1, phx;
    ch.engine().RenderPreview(vals, 128, px, lx0, lx1, phx);

    if (ch.loop_mode() != mseg::ONE_SHOT && lx1 > lx0)
      gfxInvert(lx0, kTopY, lx1 - lx0, kH);

    for (int x = 0; x < 128; ++x) {
      int h = (vals[x] * (kH - 1)) >> 16;
      gfxPixel(x, kBotY - h);
    }
    for (int i = 0; i < ch.num_points(); ++i) {
      int b = ch.level_byte(i);
      int y = kBotY - ((((b << 8) | b) * (kH - 1)) >> 16);
      if (i == sel_point_) gfxRect(px[i] < 1 ? 0 : px[i] - 1, y < 1 ? 0 : y - 1, 3, 3);
      else gfxPixel(px[i], y);
    }
    if (ch.engine().active()) gfxDottedLine(phx, kTopY, phx, kBotY, 2);
  }

  void DrawSettingsList() const {
    const auto &ch = sel();
    menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> list(cursor_);
    menu::SettingsListItem item;
    while (list.available()) {
      const int i = list.Next(item);
      const EnveloopSetting s = enabled_[i];
      item.DrawDefault(ch.get_value(s), EnveloopChannel::value_attributes(s));
    }
  }
};

// ================= interface implementation =================

void Enveloop::Init() { Start(); }

void Enveloop::Process(OC::IOFrame *ioframe) {
  uint32_t triggers = ioframe->digital_inputs.triggered();
  int32_t cvs[kChannels];
  for (int i = 0; i < kChannels; ++i) cvs[i] = ioframe->cv.values[i];

  uint8_t eoc_mask = 0;
  for (int ch = 0; ch < kChannels; ++ch)
    if (channels_[ch].eoc_pulse()) eoc_mask |= (0x1 << ch);

  for (int ch = 0; ch < kChannels; ++ch) {
    bool mg = manual_gate_ && (ch == selected_channel_);
    channels_[ch].Update(ioframe, triggers, eoc_mask, cvs, ch, mg);
  }
}

void Enveloop::Loop() {}

FLASHMEM
void Enveloop::DrawMenu() const {
  menu::QuadTitleBar::Draw();
  for (int ch = 0; ch < kChannels; ++ch) {
    menu::QuadTitleBar::SetColumn(ch);
    graphics.print((char)('1' + ch));
    menu::QuadTitleBar::DrawGateIndicator(ch, channels_[ch].triggered_display() ? 255 : 0);
  }
  menu::QuadTitleBar::Selected(selected_channel_);

  if (edit_mode_ == MODE_EDIT) DrawEditor();
  else DrawSettingsList();
}

void Enveloop::DrawScreensaver() const {
  uint16_t vals[64];
  uint8_t px[kMaxPoints];
  uint8_t lx0, lx1, phx;
  for (int ch = 0; ch < kChannels; ++ch) {
    int ox = (ch & 1) * 64;
    int oy = (ch >> 1) * 32;
    channels_[ch].engine().RenderPreview(vals, 60, px, lx0, lx1, phx);
    for (int x = 0; x < 60; ++x) {
      int h = (vals[x] * 28) >> 16;
      gfxPixel(ox + 2 + x, oy + 30 - h);
    }
    if (channels_[ch].engine().active())
      gfxDottedLine(ox + 2 + phx, oy + 2, ox + 2 + phx, oy + 30, 2);
  }
}

void Enveloop::DrawDebugInfo() const {}

void Enveloop::GetIOConfig(OC::IOConfig &ioconfig) const {
  using namespace OC;
  ioconfig.outputs[0].set("Env1", OUTPUT_MODE_UNI);
  ioconfig.outputs[1].set("Env2", OUTPUT_MODE_UNI);
  ioconfig.outputs[2].set("Env3", OUTPUT_MODE_UNI);
  ioconfig.outputs[3].set("Env4", OUTPUT_MODE_UNI);
}

size_t Enveloop::SaveAppData(util::StreamBufferWriter &stream_buffer) const {
  for (int ch = 0; ch < kChannels; ++ch) channels_[ch].Save(stream_buffer);
  return stream_buffer.written();
}

size_t Enveloop::RestoreAppData(util::StreamBufferReader &stream_buffer) {
  for (int ch = 0; ch < kChannels; ++ch) {
    channels_[ch].Restore(stream_buffer);
    channels_[ch].MarkDirty();
  }
  RebuildCursor();
  return stream_buffer.read();
}

void Enveloop::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

FLASHMEM
void Enveloop::HandleButtonEvent(const UI::Event &event) {
  if (event.type == UI::EVENT_BUTTON_PRESS) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:   NextChannel(1);  break;
      case OC::CONTROL_BUTTON_DOWN: NextChannel(-1); break;
      case OC::CONTROL_BUTTON_R:    OnRightPress();  break;
      case OC::CONTROL_BUTTON_L:    OnLeftPress();   break;
    }
  } else if (event.type == UI::EVENT_BUTTON_LONG_PRESS) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:   loop_edit_ = !loop_edit_; break;
      case OC::CONTROL_BUTTON_DOWN: manual_gate_ = !manual_gate_; break;
      case OC::CONTROL_BUTTON_R:
        edit_mode_ = (edit_mode_ == MODE_EDIT) ? MODE_SETTINGS : MODE_EDIT;
        loop_edit_ = false;
        break;
      case OC::CONTROL_BUTTON_L:    DeletePoint(); break;
    }
  }
}

void Enveloop::HandleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoder(event.value);
  else if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoder(event.value);
}
