#include "gtest/gtest.h"

#include "Arduino.h" // host stub: random()
#include "util/util_turing.h"

// With probability 0 the LSB is never toggled, so Clock() is a pure
// rotate-right within the configured length: after `length` clocks the
// register must cycle back to its starting bits.

TEST(TuringShiftRegister, LocksAtZeroProbability) {
  util::TuringShiftRegister tsr;
  tsr.Init();
  tsr.set_probability(0);
  tsr.set_length(8);

  const uint32_t start = tsr.get_shift_register() & 0xff;
  uint32_t out = 0;
  for (int i = 0; i < 8; ++i)
    out = tsr.Clock();

  EXPECT_EQ(start, out & 0xff);
}

TEST(TuringShiftRegister, OutputMaskedToLength) {
  util::TuringShiftRegister tsr;
  tsr.Init();
  tsr.set_probability(0);
  tsr.set_length(5);

  for (int i = 0; i < 64; ++i) {
    EXPECT_EQ(0u, tsr.Clock() >> 5) << "output must fit in `length` bits";
  }
}

TEST(TuringShiftRegister, NeverAllZeros) {
  util::TuringShiftRegister tsr;
  tsr.Init();
  tsr.set_probability(255); // always toggle: fastest way to churn bits
  tsr.set_length(3);

  bool nonzero_seen = false;
  for (int i = 0; i < 1000; ++i) {
    if (tsr.Clock())
      nonzero_seen = true;
  }
  EXPECT_TRUE(nonzero_seen);
  EXPECT_NE(0u, tsr.get_shift_register());
}
