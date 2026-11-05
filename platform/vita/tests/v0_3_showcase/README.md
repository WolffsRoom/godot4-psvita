# Godot 4 Vita v0.3 development showcase

This self-contained project presents the changes currently being validated for
v0.3 without treating experimental work as final:

- optional sequential PCK read-ahead on the CapUnlocker system core;
- a Dome Keeper-inspired dedicated audio mixer layout, kept away from CPU3;
- portable PS Vita export editors for Windows, Linux and macOS;
- the v0.2 native GXM 2D/3D improvements exercised together with a rigged
  character, camera movement, transparent sprites and a sharp canvas pass.

The expanded Neon PCK Harbor scene contains a deterministic 1.5 MiB
`pck_probe.bin`, textured cargo wings, animated scanner rings, moving lights and
2D diagnostic holograms placed directly in the 3D world. Each probe reads the
blob in 64 KiB chunks through `FileAccess`, displays the byte count and elapsed
time, and therefore exercises `FileAccessPack` when launched from the exported
PCK. A bundled 22.05 kHz mono PCM chord loops through `AudioStreamWAV` at the
same time so storage, rendering and audio run together.

Startup is a guided presentation: logo/loading screen, then five camera stops
that identify an object and state exactly what it proves. Control is released
only after the tour; **Start** skips it.

On Vita, **Select** opens the 2D pipeline presentation and **Start** returns to
the playable 3D scene. Use the left stick to walk with acceleration/deceleration,
the right stick to orbit, L/R to zoom, **X** to jump, **Circle** to roll and
**Square** to switch between first- and third-person cameras. The VPK uses the standard
Godot4Vita icon and LiveArea, title ID `G4V030001`, an embedded PCK and optional
CapUnlocker/prefetch settings. **Triangle** runs another PCK probe.

For deterministic desktop GIF capture, launch with the user argument
`--capture`; it shows idle, forward movement, camera zoom-out, the 2D feature
card and a final 3D orbit, then exits.
