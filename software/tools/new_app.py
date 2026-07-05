#!/usr/bin/env python3
"""Scaffold a new full-width app (modern OC_APP_CLASS style, v2.0 main).

Creates src/apps/<ClassName>.h, registers it in src/apps/_config.h (include +
AppContainer entry, both behind #ifdef ENABLE_APP_*), adds the flag to the
T32dev env in platformio.ini, and stamps a docs page.

Usage:
  python3 tools/new_app.py MyNewApp --name "My New App" --twocc MN
"""

import argparse
import re
import sys
from pathlib import Path

SW = Path(__file__).resolve().parent.parent
APPS = SW / "src" / "apps"
CONFIG = APPS / "_config.h"
PIO = SW / "platformio.ini"
DOCS = SW.parent / "docs"

TEMPLATE = '''// Copyright (c) 2026, __YOUR_NAME__
//
// MIT licensed — see licensing notes in the repository root.

// {cls}: __ONE_LINE_DESCRIPTION__
//
// Convention: keep the musical/DSP core in src/engines/ as a plain class
// (host-testable, no OC:: / gfx calls) and keep this file a thin adapter.
// See src/engines/README.md.

#include "HSApplication.h"

enum {upper}_SETTINGS {{
  {upper}_SETTING_PARAM1,
  {upper}_SETTING_COUNT
}};

class {cls}Settings : public settings::SettingsBase<{cls}Settings, {upper}_SETTING_COUNT> {{
  SETTINGS_ARRAY_DECLARE() {{{{
    {{ 0, 0, 127, "Param1", NULL, settings::STORAGE_TYPE_U8 }},
  }}}};
}};
SETTINGS_ARRAY_DEFINE({cls}Settings);

OC_APP_CLASS({cls}, TWOCCS("{twocc}"), "{name}", "{boring}"),
  public HSApplication {{
public:
  OC_APP_INTERFACE_DECLARE({cls}, {cls}Settings::storageSize());

  {cls}Settings settings;

  void Start() {{
  }}

  void Resume() {{
  }}

  void Controller() {{
    // main logic, called from the 60us ISR — keep it fast, no prints
  }}

  void View() const {{
    gfxHeader("{name}");
  }}

  // UI handlers, dispatched below
  void OnUpButtonPress() {{}}
  void OnDownButtonPress() {{}}
  void OnLeftEncoderMove(int direction) {{}}
  void OnRightEncoderMove(int direction) {{ cursor += direction; }}

private:
  int cursor = 0;
}};

// ---- OC::AppBase interface ----

void {cls}::Init() {{ BaseStart(); }}

void {cls}::Process(OC::IOFrame *ioframe) {{ BaseController(ioframe); }}

void {cls}::Loop() {{}}

FLASHMEM
void {cls}::DrawMenu() const {{ BaseView(); }}

void {cls}::DrawScreensaver() const {{ BaseScreensaver(); }}
void {cls}::DrawDebugInfo() const {{}}

FLASHMEM
void {cls}::GetIOConfig(OC::IOConfig &ioconfig) const {{
  using namespace OC;
  ioconfig.digital_inputs[DIGITAL_INPUT_1].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_2].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_3].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_4].set("");
  ioconfig.outputs[0].set("A", OUTPUT_MODE_PITCH);
  ioconfig.outputs[1].set("B", OUTPUT_MODE_PITCH);
  ioconfig.outputs[2].set("C", OUTPUT_MODE_PITCH);
  ioconfig.outputs[3].set("D", OUTPUT_MODE_PITCH);
}}

size_t {cls}::SaveAppData(util::StreamBufferWriter &stream_buffer) const {{
  settings.Save(stream_buffer);
  return stream_buffer.written();
}}

size_t {cls}::RestoreAppData(util::StreamBufferReader &stream_buffer) {{
  settings.Restore(stream_buffer);
  return stream_buffer.read();
}}

void {cls}::HandleAppEvent(OC::AppEvent event) {{
  if (event == OC::APP_EVENT_RESUME) Resume();
}}

FLASHMEM
void {cls}::HandleButtonEvent(const UI::Event &event) {{
  if (event.control == OC::CONTROL_BUTTON_UP && event.type == UI::EVENT_BUTTON_PRESS)
    OnUpButtonPress();
  if (event.control == OC::CONTROL_BUTTON_DOWN && event.type == UI::EVENT_BUTTON_PRESS)
    OnDownButtonPress();
}}

void {cls}::HandleEncoderEvent(const UI::Event &event) {{
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}}
'''


def die(msg):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def existing_twoccs():
    codes = set()
    for f in APPS.glob("*.h"):
        codes.update(re.findall(r'TWOCCS\("(..)"\)', f.read_text()))
    return codes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("classname")
    ap.add_argument("--name", help="display name (default: class name)")
    ap.add_argument("--boring", help="short/boring name (default: display name)")
    ap.add_argument("--twocc", help="2-char storage id (default: first 2 letters)")
    ap.add_argument("--flag", help="ENABLE_APP_* flag (default: ENABLE_APP_<UPPER>)")
    args = ap.parse_args()

    cls = args.classname
    if not re.fullmatch(r"[A-Za-z]\w+", cls):
        die("class name must be a valid C++ identifier")
    header = APPS / f"{cls}.h"
    if header.exists():
        die(f"{header} already exists")

    name = args.name or cls
    boring = args.boring or name
    upper = re.sub(r"(?<!^)(?=[A-Z])", "_", cls).upper()
    flag = args.flag or f"ENABLE_APP_{upper}"
    twocc = args.twocc or cls[:2].upper()
    if len(twocc) != 2:
        die("--twocc must be exactly 2 characters")
    if twocc in existing_twoccs():
        die(f'TWOCC "{twocc}" is already taken — pick another with --twocc')

    config = CONFIG.read_text()
    if flag in config:
        die(f"{flag} already appears in _config.h")

    # 1. app header
    header.write_text(TEMPLATE.format(cls=cls, name=name, boring=boring,
                                      twocc=twocc, upper=upper))

    # 2. registration: include before `namespace OC {`, entry before `> app_container;`
    inc = f'#ifdef {flag}\n#include "apps/{cls}.h"\n#endif\n'
    anchor = "\nnamespace OC {"
    if anchor not in config:
        die("could not find `namespace OC {` anchor in _config.h")
    config = config.replace(anchor, f"\n{inc}{anchor}", 1)

    entry = f"#ifdef {flag}\n  , {cls}\n#endif\n"
    anchor2 = "> app_container;"
    if anchor2 not in config:
        die("could not find `> app_container;` anchor in _config.h")
    config = config.replace(anchor2, f"{entry}{anchor2}", 1)
    CONFIG.write_text(config)

    # 3. enable in T32dev (insert after the CUSTOM_BUILD line in the FORK block)
    pio = PIO.read_text()
    m = re.search(r"(\[env:T32dev\].*?)(    -DOC_VERSION_EXTRA)", pio, re.S)
    if m:
        pio = pio.replace(m.group(0), m.group(1) + f"    -D{flag}\n" + m.group(2), 1)
        PIO.write_text(pio)
        pio_note = "enabled in [env:T32dev]"
    else:
        pio_note = "!! could not find [env:T32dev] — add the flag to platformio.ini yourself"

    # 4. docs stub
    doc = DOCS / f"{cls}.md"
    if not doc.exists():
        doc.write_text(f"# {name}\n\n_One-line description._\n\n"
                       f"|  |  |\n|---|---|\n| Build flag | `{flag}` |\n"
                       f"| Storage id | `{twocc}` |\n\n## Controls\n\n## IO\n")

    print(f"created  src/apps/{cls}.h")
    print(f"edited   src/apps/_config.h (include + container entry, #ifdef {flag})")
    print(f"pio      {pio_note}")
    print(f"created  docs/{cls}.md")
    print(f"\nnext: pio run -e T32dev — the app appears in the menu as \"{name}\"")


if __name__ == "__main__":
    main()
