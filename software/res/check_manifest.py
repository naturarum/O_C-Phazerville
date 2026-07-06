#!/usr/bin/env python3
"""CI consistency check: build_manifest.json <-> the _config.h files.

Fails (exit 1) with a precise diff when an app flag or applet exists in code
but not in the manifest, or vice versa, or when an applet's numeric ID
disagrees. Run from software/: python3 res/check_manifest.py
"""

import json
import re
import sys
from pathlib import Path

SW = Path(__file__).resolve().parent.parent
manifest = json.loads((SW / "res" / "build_manifest.json").read_text())

errors = []

# --- apps: every ENABLE_APP_* consumed by apps/_config.h must be listed ---
apps_cfg = (SW / "src" / "apps" / "_config.h").read_text()
code_flags = set(re.findall(r"#ifdef (ENABLE_APP_\w+)", apps_cfg))
manifest_flags = {a["define"] for a in manifest["apps"]}

for f in sorted(code_flags - manifest_flags):
    errors.append(f"app flag {f} is in src/apps/_config.h but missing from the manifest")
for f in sorted(manifest_flags - code_flags):
    errors.append(f"app flag {f} is in the manifest but not in src/apps/_config.h")

# --- applets: DeclareApplet entries must match id + guard ---
applets_cfg = (SW / "src" / "applets" / "_config.h").read_text()
code_applets = {
    m.group(1): int(m.group(2))
    for m in re.finditer(r",\s*DeclareApplet<(\w+),\s*(\d+),", applets_cfg)
}
manifest_applets = {a["id"]: a["applet_id"] for a in manifest["applets"]}

for name in sorted(set(code_applets) - set(manifest_applets)):
    errors.append(f"applet {name} is registered in code but missing from the manifest")
for name in sorted(set(manifest_applets) - set(code_applets)):
    errors.append(f"applet {name} is in the manifest but not registered in code")
for name in sorted(set(code_applets) & set(manifest_applets)):
    if code_applets[name] != manifest_applets[name]:
        errors.append(f"applet {name}: id {code_applets[name]} in code vs "
                      f"{manifest_applets[name]} in manifest")
    if f"DISABLE_APPLET_{name}" not in applets_cfg:
        errors.append(f"applet {name} lacks a DISABLE_APPLET_ guard "
                      f"(run tools/gen_applet_guards.py)")

if errors:
    print("manifest inconsistencies:")
    for e in errors:
        print(f"  - {e}")
    sys.exit(1)

print(f"manifest OK: {len(manifest_flags)} apps, {len(manifest_applets)} applets in sync")
