<p align="center">
  <img src="Logo.png" alt="godot4-vita" width="320">
</p>

# godot4-vita

**Work-in-progress port of [Godot Engine](https://godotengine.org) 4.x (4.8-dev) to the PlayStation Vita.**

This is a fork of Godot 4.x that boots and runs on real Vita hardware through
[VitaGL](https://github.com/Rinnegatamante/vitagl) + a homebrew shared-object
loader. It is **not** a finished, playable port yet — see **Status** below — but
the engine boots, initializes all servers, compiles its shaders, and renders on
device. The 3.x port by [SonicMastr](https://github.com/SonicMastr/godot-vita)
is used as a reference (kept under `../godot-vita-3.x` in the original tree).

---

## Status

| Milestone | State |
|---|---|
| Whole engine compiles for Vita (ARM, `template_release`) | ✅ ~1650 files, 0 errors |
| so-loader architecture (thin eboot + PIC `libgodot.so`) | ✅ sidesteps the `-Wl,-q` Thumb-veneer link wall |
| Boots on real hardware (loads `.so`, resolves imports, calls `main`) | ✅ |
| `Main::setup` — servers + renderer up | ✅ |
| Main loop running, no crash | ✅ |
| **Godot boot logo renders on device** | ✅ |
| GLES3→Cg shader down-emit (254 shaders compile) | ✅ |
| Rendering a 2D scene (Control / ColorRect / Label) | 🚧 blocked on the **canvas 2D shader** |
| 3D scene shader | ❌ (large; not required for 2D) |

**What blocks 2D scene content right now:** Godot 4's `canvas.glsl` packs batch
data into **integer vertex attributes** (`in uvec4`), integer `flat` varyings,
and ES3 pack/unpack builtins. The PowerVR SGX / VitaGL GLES2 path does not have
these natively, so the canvas shader needs rasterizer-level work (upload integer
attributes as float + reinterpret) plus builtin polyfills. That is the current
focus.

---

## How it works

Godot 4's *Compatibility* renderer targets **GLSL ES 3.00**; the Vita GPU
(PowerVR SGX543MP4+) and VitaGL are GLES2-class. Two bridges make it run:

1. **so-loader.** A 31 MB `-Wl,-q` executable overflows the Thumb branch range,
   so the engine is built as a **position-independent `libgodot.so`** and a small
   eboot (`platform/vita/loader/`, based on TheOfficialFloW's `so_util`) loads it
   at runtime, relocates it, resolves its ~650 imports against VitaGL / newlib /
   the media libs, and calls the exported `main`.

2. **Shader down-emit.** `drivers/gles3/shader_gles3.cpp` (`_vita_downemit`,
   guarded by `VITA_ENABLED`) rewrites each assembled ES3 shader into the
   GLSL-ES-1.00-ish dialect VitaGL's Cg translator accepts: `in/out`→
   `attribute/varying`, type-aware `texture()`/`textureLod()`, `*=` matrix →
   `vglMul`, `uint`/`uvec`/non-square matrix mappings, comment stripping, etc.

VitaGL itself needed a handful of fixes (see `platform/vita/vitagl/`), the most
important being a **`char newline[128]` → `[2048]`** stack buffer in its GLSL
translator that Godot 4's long varying names overflowed — the root cause of a
long chain of "random" memory-corruption crashes.

---

## Building

Requires [VITASDK](https://vitasdk.org) and a patched VitaGL.

```bash
# 1) VitaGL (see platform/vita/vitagl/README.md — apply the patch first)
make -C vitagl -j4 NO_SPLASHSCREEN=1 HAVE_GLSL_UBOS=1 HAVE_SHARK_LOG=1 LOG_ERRORS=2

# 2) Engine as a PIC shared object (template_release)
scons platform=vita arch=arm32 target=template_release optimize=size disable_path_overrides=no
#    then relink the objects/archives as: -shared -nostdlib -fPIC \
#    -Wl,--unresolved-symbols=ignore-all -Wl,-soname,libgodot.so

# 3) Loader eboot + VPK (bundles libgodot.so + a loose test project)
cmake -B build platform/vita/loader && make -C build
```

A prebuilt preview VPK that boots and renders the Godot logo lives at
`platform/vita/dist/godot4-vita-preview.vpk`.

Handy tools live in `platform/vita/tools/` (a `libvcp`-based PSP2 coredump parser
that was essential for the crash debugging).

---

## Layout of the Vita-specific work

```
platform/vita/            the port: OS_Vita, DisplayServer, GL manager, audio,
                          input, POSIX shims, platform_gl.h ES3 token/shims
platform/vita/loader/     the so-loader eboot + game_data test project
platform/vita/vitagl/     VitaGL patch + build notes
platform/vita/tools/      coredump parser and helpers
platform/vita/dist/       prebuilt preview VPK
drivers/gles3/shader_gles3.cpp        the GLSL ES3 -> Cg down-emit
drivers/gles3/shaders/tonemap_inc.glsl  reduced for the Vita bring-up
```

Everything else is upstream Godot 4.8-dev. Upstream's own README is preserved as
[`README.godot.md`](README.godot.md).

---

## Credits

- [Godot Engine](https://github.com/godotengine/godot) — MIT.
- [VitaGL](https://github.com/Rinnegatamante/vitagl) — the GL backend.
- so-loader (`so_util`) from TheOfficialFloW's `gtasa_vita`.
- [SonicMastr/godot-vita](https://github.com/SonicMastr/godot-vita) — the 3.x port used as reference.

## License

The Godot Engine portions remain under the MIT license (see
[`LICENSE.txt`](LICENSE.txt)). The Vita port additions are provided under the
same terms.
