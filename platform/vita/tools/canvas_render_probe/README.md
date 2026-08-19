# Canvas render probe — Godot 4.x Vita port, Phase 0 (neutral proof)

Answers one question, without committing to path A or B and without touching
Godot or VitaGL source:

> **Can VitaGL compile and render a shader shaped like Godot's 2D canvas
> material?**

## What it does

Hand-writes the GLSL **ES 1.00** equivalent of what
`drivers/gles3/shaders/canvas.glsl` does at its core — `mvp * vertex`, texture
sample, modulate multiply — then:

1. compiles + links it through VitaGL (logging compile/link status),
2. uploads a known 2×2 texture `(255,128,64,255)`,
3. draws a fullscreen textured quad modulated by `(1, 0.5, 1, 1)`,
4. **reads back the center pixel** and checks it equals the expected
   `(255, 64, 64)` — proving the draw actually happened (no display capture
   needed).

The shader is deliberately the *down-emit* of Godot 4's real one:
`#version 300 es`→(none), `in/out`→`attribute/varying`,
`layout(std140)` UBO→plain uniforms, `texture()`/frag-out→`texture2D()`/
`gl_FragColor`, `uvec4` integer attribs→dropped.

## Build & run

```bash
export VITASDK=/usr/local/vitasdk
cd platform/vita/tools/canvas_render_probe
cmake -B build -S . && cmake --build build
```

Install `build/canvas_render_probe.vpk`, run it (shows the quad ~3s, then
exits), and read:

```
ux0:/data/canvas_render_probe.txt
```

Requires VitaGL built with shader-compiler support and `libshacccg.suprx`
installed/decrypted on the console.

## Reading the result

- **`RESULT: PASS`** → VitaGL renders a Godot-shaped 2D shader. **Path B**
  (down-emit ES 1.00 from `drivers/gles3`) is viable — scope that fork next.
- **`RESULT: FAIL (shader compile)`** → even the ES 1.00 down-emit is rejected;
  the shader-compiler story is worse than expected.
- **`RESULT: FAIL` with "pixel is background"** → it compiled/linked but drew
  nothing; a GXM/VitaGL draw-state issue to dig into before investing.

Pair this with `../es3_probe/` (which probes the raw ES3 API surface). Together
they bound the renderer effort precisely.
