# engines/ — pure-logic cores for apps and applets

Convention for all NEW apps in this fork: the musical/DSP logic lives here as
a plain class — integer math only, no `OC::` calls, no graphics, no
`HSApplication`/`HemisphereApplet` includes. The app or applet file is a thin
adapter: read inputs → tick the engine → write DAC values → draw.

Why: engine classes compile on the host, so their behavior is unit-tested in
`software/test/` (seconds, no hardware). Precedents in-tree:
`src/tideslite.h` (used by the EbbAndLfo applet) and
`src/src/extern/peaks_multistage_envelope.*` (used by the Piqued app).

Adding an engine:
1. `engines/MyEngine.h` — self-contained, `#include <stdint.h>` at most.
   If it needs `random()`/`constrain()`, the test harness provides them via
   `test/stubs/Arduino.h` (force-included); the firmware provides them anyway.
2. `test/oc_test_myengine.cpp` — gtest cases for the core behavior.
3. The app adapter includes the engine and maps I/O.
