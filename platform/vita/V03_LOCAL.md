# v0.3 local work

This branch remains local-only until hardware validation is complete. Do not
publish these artifacts as v0.3 yet.

## Development showcase

`platform/vita/tests/v0_3_showcase` presents only changes that are implemented
locally: the optional PCK prefetch path, Dome Keeper-inspired audio isolation,
multiplatform desktop editors and the combined GXM 2D/3D renderer work carried
over from v0.2. It labels prefetch as experimental and does not claim a measured
speedup before the hardware checklist below is complete.

The release-style VPK uses the standard Godot4Vita icon and LiveArea, embeds
the game as a PCK, requests optional CapUnlocker access and enables the prefetch
configuration. The presentation uses the project splash, Branding Medium and
native Vita button glyphs. Its deterministic desktop capture tours each test
object before unlocking movement, first-person view and the integrated 2D
diagnostics.

The revised Neon PCK Harbor showcase includes a deterministic 1.5 MiB probe
inside `game.pck`, reads it in 64 KiB chunks, reports bytes/time on screen and
plays a bundled PCM WAV concurrently. This verifies the packaged FileAccess and
audio paths literally; only the CPU3 affinity and performance gain remain
hardware-only claims.

The expanded harbor also includes a deliberately large background character.
Its scale can be changed within safe bounds with D-pad Up/Down, providing a
repeatable geometry/fill-rate stress step without allowing an extreme scale to
make the scene unusable.

The v0.3 scene now uses a reusable `LinuxPixelPlush` built directly in Godot
from low-segment primitive meshes. Its Tux-inspired black, white and yellow
silhouette follows the Godot Pixel Plush proportions, includes a procedural
idle/wave animation and serves as the scalable background performance probe.

The release artwork is rendered by `release_meeting/release_meeting.tscn`:
Godot Pixel Plush and Linux Pixel Plush walk in from opposite sides, meet and
wave before returning to their starting positions for a seamless six-second
loop. Frames are captured as RGBA PNGs and encoded with a transparent GIF
palette, so the animation works on both light and dark release pages.

### Vita launch correction

The first hardware package exposed an inconsistent embedded-data mount path:
the runtime searched for `app0:/game_data/game.pck`, while Vita application
files and the loader use the `app0:game_data/...` form. The embedded PCK and
loose-project fallbacks now use the Vita-native path. The loader also records
the engine return code when startup stops before the main loop, making a future
early exit distinguishable from a crash.

## Audio isolation

The default `godot3` audio layout remains the Dome Keeper-compatible path: BGM
at the project mix rate, 512 frames with the default 15 ms latency, firmware
volume/ALC and a normal-priority dedicated mixer. The optional `main48` layout
keeps its high-priority mixer. Neither audio mode is pinned to CPU3.

Music-heavy projects should follow the other Dome Keeper lesson as well: load
long tracks on demand instead of preloading the entire soundtrack.

## Experimental PCK prefetch

When all of the following are true, the runtime starts one PCK read-ahead
worker pinned to `SCE_KERNEL_CPU_MASK_SYSTEM`:

- CapUnlocker access was successfully probed by the loader;
- the export preset has `performance/pck_prefetch` enabled;
- a 384 KiB uncached staging buffer can be allocated;
- the project is running from an unencrypted, non-sparse PCK.

After two consecutive reads in one packed resource, the worker requests up to
three 128 KiB blocks following the current position. Reads fully contained in
the published window are copied from the staging buffer. Random seeks,
encrypted packs, unavailable CPU3 access, allocation failures and thread
startup failures use the original synchronous path.

Only reads are prefetched. Saves and other writes are untouched.

At shutdown the runtime writes this telemetry to Godot's stderr log:

```text
[vita-io] PCK prefetch requests=... hits=... hit_bytes=... disk_bytes=... unused_bytes=...
```

## Hardware validation checklist

1. Confirm `[vita-audio] ... dedicated_user_core=1` and the absence of audio
   underruns or distortion.
2. Confirm `[vita-io] ... cpu3_affinity_rc=0x00000000` with CapUnlocker.
3. Compare the same PCK with prefetch enabled and disabled.
4. Record boot time, scene transition time, worst frame time and telemetry.
5. Test SD2Vita and an official memory card separately.
6. Run repeated transitions and verify resource hashes/logs before publishing.



## Split Memories issue #3: export and diagnostics

The v0.3 Vita loader and both runtime entry points share a project data
namespace derived from app0:game_data_folder.cfg. The exporter writes
that file for both embedded and separate-folder VPKs.

- Separate Folder ON: one VPK and a sibling `<foldername>/` containing exactly
  `game.pck` and `logs/`. Transfer the entire folder to
  `ux0:data/<foldername>/`.
- Separate Folder OFF: one VPK containing game_data/game.pck.
- Backward compatibility: legacy <foldername>/game_data/game.pck and
  loose game_data layouts remain readable.
- All new startup diagnostics (including loader static constructors),
  engine logs, stdout and stderr are in ux0:data/<foldername>/logs/.
- Export ownership metadata stays inside the VPK, so it is not copied into the
  game's data directory. Older `.vita-export-owner` outputs are migrated on
  their next successful export.
- Missing external PCK files are reported as errors instead of
  silently initializing an empty project.

The native GXM runtime uses godot_vita_gxm3d.cpp, not
godot_vita.cpp. Both entry points must remain in sync.

For Split Memories the initial screen-texture transition is bypassed
only on native Vita until the white-screen fault is isolated by
post-rebuild engine/stderr logs. This is a game-side workaround, not
proof of a native shader fix.

Regression tests: platform/vita/tools/test_split_export.py,
and Split Memories Vita Port/Issue-3-Diagnostics/export-contract-results.json.
