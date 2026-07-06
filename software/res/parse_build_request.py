#!/usr/bin/env python3
"""Turn a custom-build request into validated build flags.

Input (first match wins):
  1. A line `oc-build/1 target=T32 apps=+CALIBR8OR,+SCENES applets=-DuoTET
     feats=+GRIDS2` anywhere in $ISSUE_BODY or $GH_COMMENT — the strict,
     web-configurator format. Every token is looked up in
     res/build_manifest.json; defines are emitted ONLY from the manifest, so
     arbitrary flag injection is structurally impossible.
  2. Legacy: a body containing `/buildthis` plus keyword aliases (the old
     discussion-bot format), resolved from the same manifest.

Output:
  stdout line 1: the CUSTOM_BUILD_FLAGS string
  If $GITHUB_OUTPUT is set, also writes CUSTOM_BUILD_FLAGS and BUILD_ENV.
  On invalid input: human-readable error on stdout, exit code 2.
"""

import json
import os
import re
import sys
from pathlib import Path

MANIFEST = Path(__file__).resolve().parent / "build_manifest.json"


def fail(msg):
    print(f"ERROR: {msg}")
    sys.exit(2)


def emit(env, flags):
    line = " ".join(flags)
    print(line)
    gh_out = os.environ.get("GITHUB_OUTPUT")
    if gh_out:
        with open(gh_out, "a") as f:
            f.write(f"CUSTOM_BUILD_FLAGS={line}\n")
            f.write(f"BUILD_ENV={env}\n")
    sys.exit(0)


def parse_strict(line, m):
    targets = {t["id"].upper(): t for t in m["targets"]}
    apps = {a["id"].upper(): a for a in m["apps"]}
    applets = {a["id"].upper(): a for a in m["applets"]}
    feats = {f["id"].upper(): f for f in m["features"]}

    flags = ["-DCUSTOM_BUILD"]
    target = targets["T32"]

    body = line[len("oc-build/1"):].strip()
    if not re.fullmatch(r"[\w+,=\s-]*", body):
        fail("request contains invalid characters")

    for field in body.split():
        if "=" not in field:
            fail(f"malformed field '{field}' (expected key=value)")
        key, _, val = field.partition("=")
        if key == "target":
            t = targets.get(val.upper())
            if not t:
                fail(f"unknown target '{val}' (valid: {', '.join(targets)})")
            target = t
        elif key in ("apps", "applets", "feats"):
            for tok in filter(None, val.split(",")):
                sign, name = tok[0], tok[1:].upper()
                if sign not in "+-":
                    fail(f"token '{tok}' must start with + or -")
                if key == "apps":
                    a = apps.get(name)
                    if not a:
                        fail(f"unknown app '{tok[1:]}'")
                    if sign == "+":
                        if not a.get("t32", True):
                            fail(f"app {a['id']} is not available on Teensy 3.2")
                        flags.append(f"-D{a['define']}")
                elif key == "feats":
                    ft = feats.get(name)
                    if not ft:
                        fail(f"unknown feature '{tok[1:]}'")
                    if sign == "+":
                        flags.append(f"-D{ft['define']}")
                else:  # applets — negative selection only
                    ap = applets.get(name)
                    if not ap:
                        fail(f"unknown applet '{tok[1:]}'")
                    if sign == "-":
                        flags.append(f"-DDISABLE_APPLET_{ap['id']}")
        else:
            fail(f"unknown field '{key}'")

    flags += [f"-D{d}" for d in target["defines"]]

    # auto-add gating dependencies (e.g. EnigmaJr needs ENABLE_APP_ENIGMA is
    # handled by code; nothing to add for now)
    # dedupe, stable order
    seen, out = set(), []
    for f in flags:
        if f not in seen:
            seen.add(f)
            out.append(f)
    emit(target["env"], out)


def parse_legacy(text, m):
    alias_map = {}
    for a in m["apps"]:
        for al in a.get("aliases", []):
            alias_map[al.upper()] = f"-D{a['define']}"
    for ft in m["features"]:
        for al in ft.get("aliases", []):
            alias_map[al.upper()] = f"-D{ft['define']}"
    # hardware keywords, preserved from the original bot
    hw = {
        "VOR": "-DVOR",
        "BUCHLA": "-DNORTHERNLIGHT", "NLM": "-DNORTHERNLIGHT",
        "NORTHERN": "-DNORTHERNLIGHT",
        "NLM_HOC": "-DNLM_hOC", "NLM_CARDOC": "-DNLM_cardOC",
        "NLM_2OC_L": "-DNORTHERNLIGHT_2OC_LEFTSIDE",
    }

    flags = ["-DCUSTOM_BUILD"]
    pewcount = 0
    for item in text.replace(",", " ").replace(";", " ").split():
        f = item.strip().upper()
        matched = False
        for alias, define in sorted({**alias_map, **hw}.items(),
                                    key=lambda kv: -len(kv[0])):
            if f.startswith(alias):
                if define not in flags:
                    flags.append(define)
                matched = True
                break
        if not matched and f.startswith("PEW"):
            pewcount += 1
    if pewcount > 2 and "-DPEWPEWPEW" not in flags:
        flags.append("-DPEWPEWPEW")
    emit("custom", flags)


def main():
    text = os.environ.get("ISSUE_BODY") or os.environ.get("GH_COMMENT") or ""
    m = json.loads(MANIFEST.read_text())

    for line in text.splitlines():
        line = line.strip()
        if line.startswith("oc-build/1"):
            return parse_strict(line, m)

    if "/buildthis" in text:
        return parse_legacy(text, m)

    fail("no `oc-build/1 ...` line and no `/buildthis` keyword found in the request")


if __name__ == "__main__":
    main()
