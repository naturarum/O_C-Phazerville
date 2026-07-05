#!/usr/bin/env python3
"""Scaffold a pure-logic engine class + its host-side unit test.

Engines hold an app's musical/DSP core: plain integer math, no OC:: calls,
no graphics — so they compile and test on the host. See src/engines/README.md.

Usage:
  python3 tools/new_engine.py TU_Channel
"""

import argparse
import re
import sys
from pathlib import Path

SW = Path(__file__).resolve().parent.parent

ENGINE_TEMPLATE = '''// {name}: __ONE_LINE_DESCRIPTION__
//
// Pure logic — no OC::, no gfx, host-testable (see engines/README.md).
#pragma once

#include <stdint.h>

namespace engines {{

class {name} {{
public:
  void Init() {{
  }}

  // called every core ISR tick (16.666kHz / 60us) from the app adapter
  void Tick() {{
  }}

private:
}};

}} // namespace engines
'''

TEST_TEMPLATE = '''#include "gtest/gtest.h"

#include "Arduino.h" // host stubs
#include "engines/{name}.h"

TEST({name}, Init) {{
  engines::{name} e;
  e.Init();
  // EXPECT_EQ(...);
}}
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    args = ap.parse_args()

    name = args.name
    if not re.fullmatch(r"[A-Za-z]\w+", name):
        print("error: name must be a valid C++ identifier", file=sys.stderr)
        sys.exit(1)

    engine = SW / "src" / "engines" / f"{name}.h"
    test = SW / "test" / f"oc_test_{name.lower()}.cpp"
    for p in (engine, test):
        if p.exists():
            print(f"error: {p} already exists", file=sys.stderr)
            sys.exit(1)

    engine.write_text(ENGINE_TEMPLATE.format(name=name))
    test.write_text(TEST_TEMPLATE.format(name=name))

    print(f"created  src/engines/{name}.h")
    print(f"created  test/oc_test_{name.lower()}.cpp")
    print("\nnext: cd test && make   (test is picked up automatically)")


if __name__ == "__main__":
    main()
