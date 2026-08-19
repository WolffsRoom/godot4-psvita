# VitaGL ES3 probe — Godot 4.x Vita port, Phase 0

A tiny standalone homebrew that measures, on real hardware, how much of the
OpenGL ES 3.0 / GLSL ES 3.00 surface Godot 4's Compatibility renderer needs is
actually usable through VitaGL. This is the go/no-go test described in
`platform/vita/README.md`.

## What it checks

- **`glsl300es.*`** (decisive): compiles + links a `#version 300 es` shader
  pair with a `layout(std140)` UBO block, `in/out` qualifiers, and `uint`.
- `vao.genBind` — vertex array objects
- `ubo.bindBufferBase` — uniform buffer object binding
- `buffer.mapRange` — `glMapBufferRange`
- `draw.instanced` — `glDrawElementsInstanced` + `glVertexAttribDivisor`

Two ways things can fail, both informative:
1. **Link error** while building → VitaGL doesn't implement that entry point at
   all. The missing-symbol list is part of the result.
2. **Runtime FAIL** in `ux0:/data/es3_probe.txt` → the symbol exists but the
   feature doesn't work (compile error, GL error, or NULL map).

## Build

```bash
export VITASDK=/usr/local/vitasdk
cd platform/vita/tools/es3_probe
cmake -B build -S .
cmake --build build
```

Install `build/es3_probe.vpk` with VitaShell, run it once (it exits itself),
then read the report off the memory card:

```
ux0:/data/es3_probe.txt
```

(The same text is also sent to `sceClibPrintf`, visible over PrincessLog /
udcd-uvc if you have console capture set up.)

## Interpreting the result

- **`glsl300es.*` all PASS** → the shader language works; the port is mostly a
  platform-layer + API-gap effort. Good news.
- **`glsl300es.*` FAIL** → VitaGL's shader compiler (Cg/vitashark) doesn't take
  `#version 300 es` as-is. This is the expected outcome and means the core work
  is shader translation: either extend VitaGL, or down-emit an ES2-compatible
  dialect from `drivers/gles3/shader_gles3.cpp`. Plan the project around this.

Requires VitaGL built with GLSL / runtime-shader-compiler support
(`HAVE_GLSL_SUPPORT` / shark) — otherwise the shader test is meaningless.
