# Writing Your First Applet

Hemisphere applets live on half the screen, with 2 CV ins, 2 CV outs and
2 digital ins. They're the quickest way to get an idea running.

## 1. Scaffold

```sh
cd software
python3 tools/new_applet.py MyApplet --categories CAT_MODULATOR
```

This copies `src/applets/Boilerplate.h`, allocates a **free applet ID**
(IDs are stored in saved presets — never reuse or renumber one after
release), and registers the applet in `src/applets/_config.h` behind an
`#ifndef DISABLE_APPLET_MyApplet` guard.

## 2. Implement

The interface (see `API-notes.md` at the repo root for the long version):

- `Controller()` — logic, runs every 60µs tick. No prints, no blocking.
- `View()` — draw your half-screen with `gfxPrint`/`gfxLine`/`gfxCursor`…
- `OnEncoderMove(int dir)` / `OnButtonPress()` — UI.
- `OnDataRequest()` / `OnDataReceive(uint64_t)` — pack ALL persistent state
  into 64 bits (use the `Pack`/`Unpack` helpers). This is the hard limit on
  T3.2; design your parameter ranges around it.
- `Start()`, `SetHelp()`.

I/O: `Clock(ch)`, `Gate(ch)`, `In(ch)`, `DetentedIn(ch)`; `Out(ch, raw)`
(128 = 1 semitone), `ClockOut(ch)`, `GateOut(ch, on)`, `Quantize(...)`.

Keep any non-trivial DSP/logic in `src/engines/` (see its README) so it can
be unit-tested on the host: `python3 tools/new_engine.py MyCore` and
`cd test && make`.

## 3. Build & try

```sh
pio run -e T32dev -t upload    # or just `pio run -e T32dev` to size-check
pio run -e T40                 # make sure T4.x still builds before pushing
```

T32dev sits near the 256KB flash ceiling — check the printed size. If you
need room, disable applets you don't use with `-DDISABLE_APPLET_<Class>`
in your env.

Use the serial secret menu + screen capture to drive and watch the module
from your desk (see [Environment-Setup](Environment-Setup.md)).

## 4. Document & ship

- Add `docs/<MyApplet>.md` (a docs page per applet is the convention).
- PR checklist: builds on `T32dev` **and** `T40`; host tests pass;
  applet ID unchanged since first release; docs page exists.
- Consider offering it upstream to djphazer once it's stable — see
  `docs/Development-Philosophy.md`.
