# Enveloop

A 4-channel Bitwig-style multi-segment envelope generator (MSEG).

|  |  |
|---|---|
| Build flag | `ENABLE_APP_ENVELOOP` |
| Storage id | `EL` |
| Outputs | 4 independent unipolar envelopes (one per DAC channel) |

Each output is an independent envelope of up to **8 segments** (9 points).
Every segment has its own **time** and a continuous **curve** that morphs from
logarithmic through linear to exponential. A selectable point range is the
**loop / sustain region**, so one engine covers AD, ADSR, complex multi-stage
shapes, gated sustain-loops, and free-running LFOs.

## Editing (graphical page)

The envelope is drawn across the screen with the loop region shaded and a
playhead line while running.

| Control | Action |
|---|---|
| Left encoder | select a point |
| Right encoder | edit the active field of the selected point |
| Right button | cycle the active field: Level → Time → Curve |
| Left button | add a point (splits the segment to the right) |
| Left button (long) | delete the selected point |
| Up / Down button | previous / next channel |
| Up button (long) | loop-edit overlay — encoders move loop start / end |
| Down button (long) | manual gate (audition the selected channel) |
| Right button (long) | toggle the settings page |

## Settings page

Per channel: number of points, loop mode, loop start/end, retrigger mode,
time scale, amplitude, trigger source, and CV assignments.

- **Loop mode**: `1shot` (run once), `Hold` (classic sustain at loop start
  while gated), `Loop` (Bitwig sustain-loop: cycle the region while gated,
  release past it on gate-off), `LFO` (free-running).
- **Retrigger**: `Hard` restarts from the first level; `Soft` restarts from the
  current output value (click-free).
- **Trigger**: TR1–4, or the end-of-cycle of another channel (`EOC1`–`EOC4`)
  for chaining envelopes.
- **CV → time / amp**: assign a CV input to scale segment times or amplitude.

## Notes

- The DSP core is `src/engines/MsegEnvelope.h` (pure integer, host-tested in
  `software/test/oc_test_mseg.cpp`); this app is a thin adapter.
- Segment timing reuses the shared `peaks::lut_env_increments` table
  (0.5 ms – 8 s per segment, extended to ~65 s via the time-scale shift).
