# Godot4Vita v0.3 self test

One VPK that runs every v0.3 device test in sequence, with no input. It
replaces the four-VPK test kit (benchmark, GXM3D test, showcase with and
without prefetch).

| Stage | Source | What runs | Time |
| --- | --- | --- | --- |
| 1. `bench` | `tests/vitagl_benchmark` | 8 renderer phases (2D sprites, UI, SubViewport, 3D boxes, spheres, vertex lighting, transparency), 2 s warm-up and 10 s measured each, vsync off | ~1 min 40 s |
| 2. `gxm3d` | `tests/gxm3d_tux` | the Tux / Pixel Plush scene for 12 s (sprite frames, sorted transparency, Label3D, tint, skinning), then 7 benchmark phases, vsync off | ~1 min 30 s |
| 3. `showcase` | `tests/v0_3_showcase` | the guided tour, 12 reads of the 1.5 MiB `pck_probe.bin` alternating prefetch active / paused, 45 s of movement with the audio loop, first person, 2D card, giant scale; vsync and tilt shift on | ~1 min 50 s |

Each stage is a copy of the original scene in its own folder. The copies only
add a `selftest` mode: the runner sets the `selftest` meta before adding the
scene, the scene then ignores the pad and emits `selftest_finished(summary)`.
The original test projects are unchanged.

START skips the current stage. At the end the app shows a summary, writes
`selftest_report.txt` (summary, all `[selftest]` lines and a JSON copy) to
`ux0:data/g4v03selftest/logs/` and to `user://`, and quits by itself after
25 s (Cross quits at once). The clean exit makes the runtime write its shutdown
telemetry, including `[vita-io] PCK prefetch requests=... hits=...`.

## Prefetch A/B in one run

The prefetch worker starts at boot when CapUnlocker is available and
`pck_prefetch.cfg` is `enabled`. For the comparison the showcase sets
`GODOT_VITA_PCK_PREFETCH_PAUSE=1` with `OS.set_environment()` before a
"paused" read, which makes `vita_pck_prefetch.cpp` bypass the read-ahead
window, and back to `0` afterwards. The order is balanced (off, on, on, off,
...) so neither mode always reads first. Every switch logs
`[vita-io] PCK prefetch paused=<0|1> requests=... hits=...`, so the hits of
each mode can be read from the log.

The A/B only means something with CapUnlocker installed: without it the
prefetch never starts and the report says the comparison is not valid.

## Export

`export_presets.cfg` builds an embedded-PCK VPK (`G4VT00003`, data folder
`g4v03selftest`), CapUnlocker optional, prefetch on, RGBA4444 textures (the
editor default). The PCK is required: the prefetch only applies to PCK reads.

Host smoke test (no Vita): `godot --headless --path . -- --quick` runs all
stages with short benchmark phases. The showcase stage keeps its real timing.
