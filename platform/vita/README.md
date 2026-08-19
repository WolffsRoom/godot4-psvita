# Godot 4.x — PlayStation Vita platform (bring-up skeleton)

This directory is a **work-in-progress skeleton** for porting Godot 4.x to the
PS Vita, adapted from the Godot 3.x Vita port
(Stary2001 / EIREXE / PVR_PSP2) and using the Godot 4.x platform conventions
(split `OS` / `DisplayServer`, `RenderingDevice` vs. GL Compatibility renderer,
`target=template_release`, `arch=arm32`).

It does **not** build to completion yet. It is meant to be iterated on. The
files fall into three buckets:

| File | Status | Notes |
|------|--------|-------|
| `detect.py` | usable | Try `scons platform=vita` to get *real* compile/link errors. |
| `SCsub`, `platform_config.h`, `platform_gl.h` | usable | Build wiring + VitaGL include. |
| `godot_vita.cpp` | usable | Entry point, clock setup, `Main::setup/start`. |
| `os_vita.cpp/.h` | skeleton | OS_Unix subclass: paths, dlopen, run loop. |
| `display_server_vita.cpp/.h` | skeleton | Window (fixed 960x544), input pump, GL swap. TODOs for touch/IME. |
| `gl_manager_vita.cpp/.h` | skeleton | Thin VitaGL context wrapper (replaces 3.x ContextEGL_Vita). |
| `audio_driver_vita.cpp/.h` | port target | sceAudioOut worker thread. |
| `joypad_vita.cpp/.h` | port target | SceCtrl -> Input joy_button/joy_axis. |

---

## ⚠️ The blocking issue: GLES3 vs. the Vita GPU

Godot 4.x removed the GLES2 renderer. The **only** low-level backend that can
run on Vita-class hardware is the **OpenGL Compatibility renderer**
(`drivers/gles3`), and it targets **real OpenGL ES 3.0 / GLSL ES 3.00**:

- `drivers/gles3/shader_gles3.cpp` emits `#version 300 es` (no ES2 fallback).
- Even the **2D canvas** shader (`drivers/gles3/shaders/canvas.glsl`) uses
  `layout(std140) uniform` blocks (UBOs) and `uint` — both GLSL ES 3.00 only.

The Vita GPU is a **PowerVR SGX543MP4+**, a GLES2-class part. **VitaGL** and
**gl4es4vita** are GL wrappers over GXM that expose roughly GL 1.x/2.x /
GLES2 semantics — **neither exposes a real GLES 3.0 driver.** So the renderer,
not the platform layer, is the wall.

**gl4es4vita is not suitable** (fixed-function GL 1.x/2.x). Use **VitaGL**.

### ES3 API surface actually used by `drivers/gles3`

"uses" = occurrences of each entry point across the driver (measured on this
`godot-master` tree). Every one of these is **ES3 core / not ES2**. The last
column was resolved **against VitaGL source** (`vitaGL-nosplash`, on this
machine) — not guessed.

| ES3 feature | GL entry points | uses | needed by | in VitaGL (from source) |
|-------------|-----------------|-----:|-----------|-------------------------|
| Uniform Buffer Objects | `glBindBufferBase` (35), `glBindBufferRange` (3), `glUniformBlockBinding`, `glGetUniformBlockIndex` | 40 | **2D + 3D** (all materials) | ⚠️ present but **gated behind `HAVE_GLSL_UBOS` (off by default)**, string-based/experimental |
| Vertex Array Objects | `glGenVertexArrays` (18), `glBindVertexArray` (76) | 94 | **2D + 3D** | ✅ implemented |
| Instanced drawing | `glDrawArraysInstanced` (4), `glDrawElementsInstanced` (5), `glVertexAttribDivisor` (9) | 18 | **2D + 3D** | ✅ implemented |
| Integer vertex attribs | `glVertexAttribIPointer` (8) | 8 | 2D batching, 3D | ❌ **absent** (not even in header) |
| Buffer mapping | `glMapBufferRange` (6), `glUnmapBuffer` (6) | 12 | **2D + 3D** streaming | ✅ implemented |
| MRT | `glDrawBuffers` (8) | 8 | 3D (deferred-ish passes) | ❌ **absent** |
| Framebuffer blit | `glBlitFramebuffer` (7) | 7 | 3D, post | ✅ implemented |
| 3D / array textures + immutable storage | `glTexImage3D` (9), `glTexStorage2D` (4), `glTexSubImage3D` (2), `glCompressedTexImage3D` | 16 | 3D, some 2D | ❌ **absent** |
| MSAA renderbuffers | `glRenderbufferStorageMultisample` (2) | 2 | AA | ❌ absent (use GXM MSAA at init) |
| Transform feedback | `glBeginTransformFeedback` (7), `glTransformFeedbackVaryings` (2) | 9 | GPU particles | ❌ **absent (no GXM equivalent)** |
| Sync objects | `glFenceSync` (2), `glClientWaitSync` | 3 | frame pacing | ❌ absent |
| Indexed queries | `glGetStringi` (2), `glReadBuffer` | 3 | init, readback | ⚠️ `glGetStringi` ✅, `glReadBuffer` ❌ (easy to shim) |
| **GLSL ES 3.00** | `#version 300 es`, `layout(std140)`, `in/out`, `uint`, integer ops | shaders | **2D + 3D** | ❌ **translator targets GLSL ES 1.00** — see below |

### Phase-0 findings — CONFIRMED ON REAL HARDWARE (PS Vita, 2026-08-01)

Both probes were run on a real Vita (VitaGL built locally with
`NO_SPLASHSCREEN=1 HAVE_GLSL_UBOS=1`; GL_VENDOR "Rinnegatamante",
GL_RENDERER "SGX543MP4+", GL_VERSION "OpenGL ES 2.0 VitaGL", GLSL "1.00 ES").

**`canvas_render_probe` → PASS.** A hand-written GLSL **ES 1.00** down-emit of
Godot's 2D canvas core (mvp × vertex, texture sample, modulate) compiled,
linked, AND rendered correctly (center pixel read back = expected value). This
is the green light for Path B on 2D.

**`es3_probe` → API all PASS, GLSL 3.00 link CRASHES:**

| Check | Result on hardware |
|-------|--------------------|
| `vao.genBind` (VAOs) | ✅ PASS |
| `ubo.bindBufferBase` (UBO *API* binding) | ✅ PASS |
| `buffer.mapRange` (`glMapBufferRange`) | ✅ PASS |
| `draw.instanced` (`glDrawElementsInstanced` + divisor) | ✅ PASS |
| `#version 300 es` vertex compile | ✅ PASS (VitaGL strips `#version`, compiles body as Cg) |
| `#version 300 es` fragment compile | ✅ PASS (same caveat) |
| `#version 300 es` **`glLinkProgram`** | 💥 **HARD CRASH** (process dies; coredump) |

So: the **modern GL *API* surface works** (VAO / UBO-bind / map-range /
instancing are all usable — this VitaGL is newer than the `vitaGL-nosplash`
source first audited). The wall is purely the **shader language**: linking a
program built from Godot's real `#version 300 es` shaders (std140 UBO block +
`in/out` + `flat out uint` + `gl_VertexID`) **crashes VitaGL's Cg linker**. The
per-stage compiles "succeed" only because `#version` is stripped; the link,
where Cg must reconcile the ES3 constructs, faults.

The contrast is the whole story: canvas ES 1.00 (attribute/varying/plain
uniforms) links and renders; the ES 3.00 program crashes at link.

**Verdict — Path B is the route, and it is viable for 2D:**
Fork Godot's `drivers/gles3` shader emission to a **GLSL ES 1.00 dialect**:
`in/out`→`attribute/varying`, fragment out→`gl_FragColor`, `texture()`→
`texture2D()`, `layout(std140) uniform Block{...}`→plain `uniform`s (do NOT rely
on VitaGL's in-shader UBO — the *API* bind works but the in-shader block is what
crashes the linker), and drop integer varyings/attribs (`uint`/`gl_VertexID`).
Subsystems with no GXM path (GPU particles via transform feedback, MRT-dependent
3D effects) stay disabled. Still a large effort, but no longer speculative: the
one thing that had to be true on hardware — that a Godot-shaped shader can be
made to compile+render through VitaGL — is proven.

## Phase 0 — DONE (probes under `tools/`)

`tools/es3_probe/` and `tools/canvas_render_probe/` are the homebrews used for
the hardware results above. See `tools/BUILDING.md` for the exact build recipe.

## Phase 1 — platform layer (this skeleton)

`scons platform=vita arch=arm32 target=template_release` and drive the
compile/link errors down. Expect: (a) OS_Unix virtuals that need Vita overrides,
(b) missing GL ES3 symbols from `platform_gl.h` — that missing-symbol list *is*
the Phase-2 backlog.

## Phase 2 — renderer (shader down-emit STARTED)

Wire `RasterizerGLES3::make_current()` to the live VitaGL context (already
stubbed in `display_server_vita.cpp`) and resolve the ES3 gaps.

**Shader down-emit — initial patch landed** in `drivers/gles3/shader_gles3.cpp`
(`_vita_downemit()`, guarded by `#ifdef VITA_ENABLED`, called at the two
stage-compile sites). It rewrites each assembled GLSL ES 3.00 stage into the
GLSL ES 1.00 dialect VitaGL accepts:
- drop `#version` + ES3-only `precision … sampler` lines
- strip `layout(...)` qualifiers
- flatten `std140` UBO blocks → plain `uniform`s (the in-shader block is what
  crashes VitaGL's linker; integer math inside is fine)
- fragment `out vec4 NAME;` → `#define NAME gl_FragColor`
- token-safe `#define` preamble: `in`→attribute/varying (per stage),
  `out`→varying (vertex), `texture`→`texture2D`, `textureLod`→`texture2DLod`

Integer/bitwise math is intentionally preserved (hardware-proven to pass through
to Cg via `tools/dialect_probe`).

**Not yet done (needs Phase 1 to build first, then on-device iteration):**
1. Polyfill ES3 builtins with no Cg equivalent: `unpackHalf2x16`,
   `unpackUnorm4x8`, `floatBitsToUint`, …
2. Integer **vertex attributes** (`uvec4 attrib_*`): VitaGL has no
   `glVertexAttribIPointer`, so the rasterizer must upload these as float
   attribs and the shader reinterpret them (vertex-format change, not shader
   text).
3. Decide on `flat` varyings (left as-is for now).
4. Disable transform-feedback (GPU particles) and MRT-dependent paths.

## Build (once it compiles)

```bash
export VITASDK=/usr/local/vitasdk
scons platform=vita arch=arm32 target=template_release use_vitagl=yes -j$(nproc)
```

Requires VitaSDK + a VitaGL built and installed into
`$VITASDK/arm-vita-eabi/lib` (plus `vitashark`, `SceShaccCgExt`).
