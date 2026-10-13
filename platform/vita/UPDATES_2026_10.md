# Godot 4 Vita — October 2026 development status

This is a technical record. Earlier plans are preserved in ROADMAP.md.

## Validated / implemented

- ARM Godot 4.8-dev7 shared-object loader, plus native GXM Canvas2D subset.
- Vita device 0 maps Godot joypad buttons and axes; fixed D-pad indices.
- Real-device gameplay, collision, HUD, audio, menus and score for the
  official Dodge the Creeps demo were reported fully functional.
- PSP2 crash coredump decoded as EXCEPTION_UNDEF at
  WorkerThreadPool::_process_task, with index 8 into an eight-element pool.
  The runtime passes ThreadData explicitly, validates indices, and limits
  the demo's work pool to two threads. The user subsequently confirmed
  normal gameplay on hardware.
- GPUParticles2D trail replaced with time-limited translucent Sprite2D
  ghost sprites for this demo. START background was removed.
- Optional game-specific a5 LiveArea and 180-frame Godot Vita loading
  animation can now be packaged into the engine's loader via CMake arguments.
  Playback is on a loader thread while the runtime is mapped; Vita2D context
  is destroyed before Godot's GXM renderer begins.
  **The animated boot handoff is build-verified only, pending device check.**

## Remaining limitations

- Partial Canvas command implementation; no general 3D, advanced shaders or
  full GPU particles; game-specific fallback rendering remains necessary.
- Not a drop-in export template for arbitrary Godot 4.8 games.
- No physical Vita graphical CI automation.
- A fresh checkout needs Godot host-imported texture/audio/font resources
  before packaging a game: Vita imports are not performed on device.

## Project layout

Engine Git repository: GodotPSV/godot-master.
Editable logo animation: GodotPSV/Load/animation/alternativa_mastigando.
Physical test logs: GodotPSV/Logs.
Generated packages and build dependencies: GodotPSV/vpk_out and _build.
Standalone port with its own VPKs: SDKVita/DodgeCreepsVita.

The private repository stores portable source code, tests, examples and
the 2.97-MB packed demonstration animation. It does not store VPK output,
coredumps or multi-gigabyte compiled SDK dependencies.


## Dodge demo: settings and adaptive background (2026-10-13)

A subsequent game-only iteration added an on-screen settings menu with
playback pausing, configurable movement speed, volume, 30/45/60 FPS cap and
ConfigFile persistence in user://. The score now drives a smoothly blended
background palette with a color milestone every five points.

Five Godot 4.8-dev7 host regressions pass (gameplay, joypad matrix, visuals,
score>=10 and settings/menu/color/persistence). A new VPK builds and passes
integrity checks, including custom a5 LiveArea and 180 native loading frames.
The settings UI and the animated handoff have not yet been verified on
physical hardware; please do not interpret the host tests as on-device proof.

## Vita D-pad and audio output regression

Real-hardware feedback after the first public VPK reported Settings opening but
its D-pad remaining unresponsive; music also sounded distorted or crackly.

- All Vita menu actions now poll the physical joypad during
  PROCESS_MODE_ALWAYS (including repeat-on-hold for D-pad). Focused
  Godot Controls no longer need to forward _input while SceneTree.paused.
- The native Vita audio driver now uses a 2048-frame output buffer
  (~42.7 ms at 48 kHz vs. ~21.3 ms), and requests a high-priority
  mixer thread. Dodge's music and SFX players use -8 dB gain in
  addition to the already headroom-limited PCM source.
- host_vita_dpad_paused_smoke.gd explicitly disables HUD _input and
  verifies D-pad navigation/value changes/repeat and Circle close.

Host regression/build verification does not prove that physical Vita audio
is now distortion-free. Validate on hardware before marking it fixed.
