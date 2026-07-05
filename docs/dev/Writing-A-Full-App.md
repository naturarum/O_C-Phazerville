# Writing a Full-Width App

Full apps own the whole screen, all 4 outputs, all inputs, and get real
EEPROM storage (not just 64 bits). Use one when the idea needs an
integrated multi-channel UI — otherwise prefer an applet.

## 1. Scaffold

```sh
cd software
python3 tools/new_app.py MyNewApp --name "My New App" --twocc MN
```

This generates a modern `OC_APP_CLASS`-style app in `src/apps/MyNewApp.h`,
registers it in `src/apps/_config.h` (include + `AppContainer` entry, both
behind `#ifdef ENABLE_APP_MY_NEW_APP`), enables the flag in `T32dev`, and
stamps `docs/MyNewApp.md`.

The **TWOCC** (two-character code, e.g. `"MN"`) keys your app's EEPROM
chunk. It must be unique forever — the script checks against the tree, and
changing it later orphans users' saved state.

## 2. The pieces

- **Settings**: a `settings::SettingsBase` subclass declares parameters
  (min/max/default/storage type). `SETTINGS_ARRAY_DECLARE/DEFINE` handle
  serialization; `Save`/`Restore` stream it to EEPROM.
- **EEPROM budget**: ~2KB total on T3.2 shared by ALL enabled apps.
  `_config.h` has a `static_assert` that fails the build on overflow.
  Budget tens of bytes, not hundreds.
- **`Controller()`** runs in the 60µs ISR via `Process()`; **`Loop()`** is
  for non-time-critical work (MIDI parsing, file IO).
- **`GetIOConfig()`** labels the inputs/outputs for the global IO screen.
- Keep the core in `src/engines/` (host-testable); the app file is the
  adapter. See `src/engines/README.md`.

Reference apps: `apps/TheDarkestTimeline.h` (clean modern structure),
`apps/QQ.h` (per-channel settings menus with `update_enabled_settings()`),
`apps/Piqued.h` (envelope engine reuse, `GetIOConfig`).

## 3. Flash reality on Teensy 3.2

Every app is optional by design — that's how a 256KB module carries a
90-applet ecosystem. Your app should:

- gate everything behind its `ENABLE_APP_*` flag (the scaffold does this);
- build into `T32dev` without overflowing (disable applets to make room);
- build on `T40` too (`pio run -e T40`).

Record the app's flash cost (build with and without the flag, diff the
`Flash:` line) in your PR description — it feeds the custom-build tool's
size estimates.
