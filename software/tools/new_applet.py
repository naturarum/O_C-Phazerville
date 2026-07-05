#!/usr/bin/env python3
"""Scaffold a new Hemisphere applet from applets/Boilerplate.h.

Copies the boilerplate, allocates the lowest free applet ID (IDs are stored
in user presets — collisions corrupt saved state), and registers the applet
in src/applets/_config.h with a DISABLE_APPLET_* guard.

Usage:
  python3 tools/new_applet.py MyApplet --categories CAT_MODULATOR,CAT_UTILITY
"""

import argparse
import re
import sys
from pathlib import Path

SW = Path(__file__).resolve().parent.parent
APPLETS = SW / "src" / "applets"
CONFIG = APPLETS / "_config.h"
BOILERPLATE = APPLETS / "Boilerplate.h"

CATS = ("CAT_MODULATOR CAT_SEQUENCER CAT_CLOCKING CAT_QUANTIZER "
        "CAT_UTILITY CAT_MIDI CAT_LOGIC CAT_OTHER").split()


def die(msg):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("classname")
    ap.add_argument("--categories", default="CAT_OTHER",
                    help="comma-separated: " + ", ".join(CATS))
    args = ap.parse_args()

    cls = args.classname
    if not re.fullmatch(r"[A-Za-z]\w+", cls):
        die("class name must be a valid C++ identifier")
    header = APPLETS / f"{cls}.h"
    if header.exists():
        die(f"{header} already exists")

    cats = [c.strip() for c in args.categories.split(",")]
    for c in cats:
        if c not in CATS:
            die(f"unknown category {c}")
    cats_expr = " | ".join(cats)

    config = CONFIG.read_text()

    # allocate lowest free ID
    used = set(map(int, re.findall(r"DeclareApplet<\s*\w+\s*,\s*(\d+)", config)))
    applet_id = next(i for i in range(1, 256) if i not in used)

    # 1. header from boilerplate
    src = BOILERPLATE.read_text().replace("MyApplet", cls)
    header.write_text(src)

    # 2. include — must land BEFORE the `#undef applet_name` line, while the
    # static-name macro hack is still in effect
    undef_anchor = "\n#undef applet_name"
    if undef_anchor not in config:
        die("could not find `#undef applet_name` anchor in applets/_config.h")
    config = config.replace(undef_anchor,
                            f'#include "{cls}.h"\n{undef_anchor}', 1)

    entry = (f"#ifndef DISABLE_APPLET_{cls}\n"
             f"    , DeclareApplet<{cls}, {applet_id}, {cats_expr}>\n"
             f"#endif\n")
    anchor = config.rfind("\n>{};")
    if anchor < 0:
        die("could not find registry terminator `>{};` in applets/_config.h")
    anchor += 1  # insert before the `>` line, after the trailing newline
    config = config[:anchor] + entry + config[anchor:]
    CONFIG.write_text(config)

    print(f"created  src/applets/{cls}.h (from Boilerplate.h)")
    print(f"edited   src/applets/_config.h (id {applet_id}, {cats_expr})")
    print("\nnext: implement Controller/View, then pio run -e T32dev")
    print("      applet IDs live in saved presets — never renumber after release")


if __name__ == "__main__":
    main()
