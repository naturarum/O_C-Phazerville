# Development Environment Setup (macOS)

This fork's primary target is **Teensy 3.2** (the classic o_C module,
256KB flash / 64KB RAM). Everything below also works for T4.x targets.

## Toolchain

```sh
brew install platformio        # CLI; or use the VSCode PlatformIO extension
```

The PlatformIO project lives in `software/`. First build downloads the
toolchain and the pinned Teensy framework automatically.

## Build / flash loop

```sh
cd software
pio run -e T32dev              # build (dev env for T3.2)
pio run -e T32dev -t upload    # build + flash via Teensy loader
```

- Clean build ≈ 1–2 min; incremental ≈ 10–30 s; flash ≈ 5 s.
- Flash/RAM usage prints at the end of every build. `T32dev` sits near the
  256KB ceiling — watch the number when adding code.
- Don't use `software/build.sh` for the inner loop (it cleans + builds all
  default envs).
- Cross-check T4 before pushing: `pio run -e T40`.

Envs: `T32dev` (T3.2 development: Calibr8or + Hemisphere, debug menu, some
audio-toy applets disabled via `DISABLE_APPLET_*`), `T32`/`T32_vor` (upstream
release sets), `custom` (takes `$CUSTOM_BUILD_FLAGS`, used by the build bot),
`T40`/`T41`/… (Teensy 4.x).

## Seeing the module screen on your computer

[Phazerville Screen Capture](https://github.com/PaulStoffregen/Phazerville-Screen-Capture)
(macOS builds in its Releases) mirrors the OLED over USB. Works with any
build using the default `USB_MIDI` type — nothing to enable. Don't switch the
env to `USB_MIDI_SERIAL`, or the capture tool won't find the device (it
connects via Teensy's Seremu HID serial).

## Driving the module from your desk (serial "secret menu")

`T32dev` enables `PRINT_DEBUG`. Connect with `pio device monitor`
(PlatformIO understands Teensy HID serial) and type:

| Key | Action |
|---|---|
| `z` | help / show toggles |
| `I` `D` `L` | toggle app ISR / display redraw / app loop |
| `+` `-` | UP / DOWN button |
| `[` `]` | left / right encoder press (`{` `}` = long press) |
| `,` `.` | left encoder turn CCW / CW |
| `<` `>` | right encoder turn CCW / CW |

Any other byte requests a screen capture frame, so keep the capture tool and
the monitor to one connection at a time.

Combined with screen capture this is a poor-man's emulator: drive the UI and
watch the screen without touching the module.

## Debug printing

`SERIAL_PRINTLN(...)` (see `OC_debug.h`) prints when `PRINT_DEBUG` is
defined. **Never print from the 60µs core ISR** — it will blow the timing
budget; print from `loop()`/UI code, or accumulate counters and read them via
the debug stats screen (Up+Down in the app menu → cycle stats).

## Unit tests (host-side, no hardware)

```sh
git submodule update --init software/test/gtest   # once
cd software/test && make
```

Engine/logic classes (see `software/src/engines/README.md`) are plain C++
testable on the host. Add `oc_test_<name>.cpp` and it is picked up
automatically by the Makefile wildcard.
