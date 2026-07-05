# New App / Applet Checklist

The short version — details in [dev/Writing-A-Full-App.md](dev/Writing-A-Full-App.md)
and [dev/Writing-Your-First-Applet.md](dev/Writing-Your-First-Applet.md).

## Full app

1. `python3 tools/new_app.py ClassName --name "Display Name" --twocc XY`
   (from `software/`; TWOCC must be unique forever — the script checks)
2. Engine first: `python3 tools/new_engine.py ClassNameCore`, write tests,
   `cd test && make`
3. Implement the adapter (`src/apps/ClassName.h`): Controller/View/UI +
   settings
4. `pio run -e T32dev` — watch the Flash % (ceiling is 256KB!) and the
   EEPROM `static_assert` in `src/apps/_config.h`
5. `pio run -e T40` — T4.x must keep building
6. Flash to hardware, test, save/power-cycle/restore
7. Fill in `docs/ClassName.md`

## Applet

1. `python3 tools/new_applet.py ClassName --categories CAT_...`
   (allocates a free applet ID — never renumber after release)
2. State must pack into 64 bits (`OnDataRequest`/`OnDataReceive`)
3. Steps 4–7 as above

## Registration touch-points (what the scripts edit)

| File | Edit |
|---|---|
| `src/apps/_config.h` | `#include` + `AppContainer` entry, both `#ifdef ENABLE_APP_X` |
| `src/applets/_config.h` | `#include` (BEFORE the `#undef applet_name` block) + `DeclareApplet<>` inside `#ifndef DISABLE_APPLET_X` |
| `platformio.ini` | `-DENABLE_APP_X` in the envs that should carry it |
| `docs/` | one page per app/applet |
