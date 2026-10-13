# CanvasData transport probe

This probe uses `../../canvas_uniforms_vita.h`, the same upload helper called
by `RasterizerCanvasGLES3` after binding each canvas batch's shader on Vita.
It tests explicit plain uniforms, matching the flattened `CanvasData` block.
It does not use the full Godot canvas shader, integer attributes, textures,
lighting, custom materials or batching.

The engine retains the CPU `StateBuffer` for the current canvas pass, then
uploads its named fields after a successful shader bind. Matrix uploads use
`GL_FALSE` (column-major, like the existing CPU matrix builders). The CPU field
`canvas_modulate` maps to GLSL `canvas_modulation`. SDF fields are selected by
name rather than their differing declaration order. Unused/optimized uniforms
are ignored by GL. Locations are looked up on each batch for now; caching is
deferred until correctness is established.

## Build

Reuse the exact library from the passing UBO build, without recompiling VitaGL:

```sh
python3 godot-master/platform/vita/tools/build_canvas_state_probe.py \
  --ubo-build _build/ubo-20261010-02 \
  --output _build/canvas-state-new-build
```

Run from the workspace root under Linux/WSL with Python 3.12+, CMake and VitaSDK.
The new output directory contains the VPK, log, manifest and ELF for diagnosis.
The builder checks library/header hashes against the dependency manifest.

## Hardware acceptance

Install `canvas_state_probe.vpk` (title ID `GDCS40001`, **Canvas State Probe**).
Run it and retrieve `ux0:/data/canvas_state_probe.txt`. The app exits after the
tests, so the log is the authoritative result; no manual screenshot is required.
Match `BUILD_ID` and `VITAGL_SHA256` with the manifest.

Three cases check pixels inside and outside the expected rectangle, with RGB
tolerance of 8 per channel and zero GL errors:

| Case | Inside sample | Expected RGB | Outside sample |
|---|---|---|---|
| A: translated rectangle | (160,160) | (255,64,64) | (560,160), black |
| B: new program, changed transform/color/time | (560,160) | (64,255,64) | (160,160), black |
| A: rebound, half-width rectangle, new state and boolean | (340,360) | (128,128,255) | (420,360), black |

All cases must PASS, followed by `RESULT: PASS`. This validates matrices,
modulation, pixel size, time, boolean upload and state refresh across programs.
It does not establish that every other CanvasData field works on the GPU;
normal/SDF/light fields are uploaded by the helper but inactive in this probe.
Other flattened blocks (lights, globals, materials) still require a transport
strategy before their corresponding renderer features are supported.

After PASS, the next integrated test is a Godot ColorRect using the real canvas
shader and vertex attributes. This VPK is a diagnostic app, not a rebuilt Godot
runtime.
