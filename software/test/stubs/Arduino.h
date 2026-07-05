// Minimal Arduino.h stub for host-side unit tests (-DTESTING builds).
// Only add what a pure-logic header actually needs — anything requiring
// real hardware belongs behind the engine/adapter split, not in here.
#pragma once

#include <stdint.h>
#include <stdlib.h>

// flash/section attributes are meaningless on the host
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef FLASHMEM
#define FLASHMEM
#endif
#ifndef FASTRUN
#define FASTRUN
#endif

#ifndef constrain
#define constrain(amt, low, high) \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

inline long random(long howbig) { return rand() % (howbig ? howbig : 1); }
inline long random(long howsmall, long howbig) {
  return howsmall + random(howbig - howsmall);
}
