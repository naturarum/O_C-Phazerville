# Upstream Tracking

This fork tracks [djphazer/O_C-Phazerville](https://github.com/djphazer/O_C-Phazerville).

## Branch model

- **`main`** — pristine mirror of `upstream/main`. Never commit here;
  fast-forward only (`git fetch upstream && git checkout main && git merge --ff-only upstream/main`).
- **`trunk`** — the canonical fork branch (default; releases and custom
  builds come from here).
- `feat/*` — feature branches off `trunk`. Anything intended for upstream
  gets rebased onto `upstream/main` before opening the PR.

## Merge cadence

Monthly (or when something interesting lands upstream):

```sh
git fetch upstream
git checkout trunk && git merge upstream/main
```

Post-merge ritual:
1. `pio run -e T32dev` — T3.2 still fits & builds (this is the fork's raison d'être)
2. `pio run -e T40`
3. `cd test && make`
4. `python3 tools/gen_applet_guards.py` — wraps any NEW upstream applets
   (idempotent), then `--check` passes
5. Check no new upstream TWOCC collides with fork apps (`grep -rn 'TWOCCS("' software/src/apps/`)

Conflict hot-spots: `src/apps/_config.h`, `src/applets/_config.h`,
`platformio.ini` (fork edits are bracketed with `; === FORK ===` / `// === FORK ===`),
and `.github/workflows/firmware.yml`.

## Cherry-picks from upstream/T32

The upstream `T32` branch carries T3.2-specific fixes. Record picks here:

| SHA | Subject | Picked into |
|---|---|---|
| `d8a3287c` | fixes for new toolchain in Teensyduino 1.62 | trunk (2026-07-05, adapted — FOURCC uses had moved to OC_global_settings.h/OC_storage.h) |

## Fork-only changes worth offering upstream

- T3.2 buildability fixes on v2.0 main (PhzConfig stubs, Read_ID_Voltage stub,
  USBHost guards, MIDIMapping settings accessors)
- `DISABLE_APPLET_*` registry guards + `tools/gen_applet_guards.py`
- Serial UI-simulation keys (PRINT_DEBUG)
- Revived `software/test/` harness
