# Dodge the Creeps — Godot 4 Vita GXM fixture

This is an adapted copy of the official Godot demo, used to validate the
experimental Godot 4.8-dev7 PlayStation Vita native GXM Canvas renderer.

## Status

- Gameplay and original UI with native controls and sound were confirmed
  functional on physical Vita in October 2026.
- The later build adds a **Settings** overlay and smoothly changing score
  background, validated by host tests and packaged for hardware evaluation.
- The optional LiveArea a5 + chewing Godot logo loading animation is also
  compiled/packaged but still needs confirmation of the animation itself
  on physical Vita.

## New settings overlay

- Triangle (PS Vita) / F1 (host): open or close.
- D-pad: navigate and change selections; Cross to confirm; Circle to exit.
- Character speed: 250, 400, 550 px/s.
- Master volume: 0, 25, 50, 75, 100%.
- FPS cap: 30, 45, 60 (ceiling, not guaranteed performance).
- Persisted via ConfigFile in user://dodge_vita_settings.cfg.
- Pauses gameplay while changing settings; score-based background colors
  blend progressively, with a palette milestone every five points.

## Regression tests

Import demo assets with a matching Godot 4.8 host editor and run:

    godot --headless --path platform/vita/tests/dodge_the_creeps        --script res://host_settings_smoke.gd

Other fixtures: host_smoke.gd, host_joypad_matrix.gd,
host_visual_smoke.gd, host_score10_smoke.gd. The game remains separate from
the production shell-based loader and is not a general compatibility proof.

The independent packaged Vita release lives in SDKVita/DodgeCreepsVita;
GitHub contains the reproducible engine fixture instead.

## Audio, controller and death-animation update

- Predecoded 48 kHz stereo PCM 16-bit WAV assets replace the in-game OGG
  decoding path to reduce CPU spikes. The soundtrack explicitly loops
  across its entire valid sample range.
- Default high-frequency diagnostic logging has been disabled in main.gd.
- Triangle now has an input-event handler and a polled-state fallback;
  the on-screen hint appears only when Start is visible and no longer
  references PC keyboard shortcuts.
- On collision, the player spins/fades with a six-sprite burst and the
  music fades out before showing Game Over.
- Run host_audio_death_smoke.gd for input fallback, audio format, loop
  boundaries, visibility, animation and restart-screen regression checks.
  This update needs real PS Vita testing for crackling and visuals.

## Input/audio hardware regression

The subsequent fix polls D-pad, Cross and Circle in the always-processing HUD
even when Godot pauses the game and swallows GUI input events; controls also
repeat when held. The audio driver's buffer size is raised to 2048 samples
and the soundtrack/player gains were reduced to improve Vita playback.
host_vita_dpad_paused_smoke.gd covers controls under forced loss of _input.
New sound quality awaits hardware verification.
