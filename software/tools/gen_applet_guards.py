#!/usr/bin/env python3
"""One-shot (idempotent) codemod: wrap every DeclareApplet registry entry in
src/applets/_config.h with #ifndef DISABLE_APPLET_<Class> guards.

Only the Registry entries are guarded, not the #include lines: Teensy builds
link with --gc-sections, so an applet absent from the Registry is dropped from
the binary. This keeps the diff against upstream minimal.

Usage: python3 tools/gen_applet_guards.py [--check]
  --check  exit 1 if any entry is unguarded (for CI), change nothing
"""

import re
import sys
from pathlib import Path

CONFIG = Path(__file__).resolve().parent.parent / "src" / "applets" / "_config.h"

ENTRY_RE = re.compile(r"^\s*,\s*DeclareApplet<\s*(\w+)\s*,")
GUARD_RE = re.compile(r"^#ifndef DISABLE_APPLET_(\w+)\s*$")


def main() -> int:
    check_only = "--check" in sys.argv
    lines = CONFIG.read_text().splitlines(keepends=True)

    out = []
    unguarded = []
    i = 0
    while i < len(lines):
        line = lines[i]
        m = ENTRY_RE.match(line)
        if m:
            name = m.group(1)
            prev = out[-1] if out else ""
            already = GUARD_RE.match(prev.strip("\n") or "")
            if already and already.group(1) == name:
                out.append(line)  # guarded on the line above; keep as-is
            else:
                unguarded.append(name)
                out.append(f"#ifndef DISABLE_APPLET_{name}\n")
                out.append(line)
                # close the guard immediately after the entry line
                out.append("#endif\n")
        else:
            out.append(line)
        i += 1

    if check_only:
        if unguarded:
            print("Unguarded applet entries:", ", ".join(unguarded))
            return 1
        print("All applet entries guarded.")
        return 0

    if unguarded:
        CONFIG.write_text("".join(out))
        print(f"Guarded {len(unguarded)} applet entries:")
        print(", ".join(unguarded))
    else:
        print("Nothing to do — all entries already guarded.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
