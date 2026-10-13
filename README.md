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

## Status (October 2026)

This experimental **Godot 4.8-dev7 PlayStation Vita runtime** has progressed
from engine boot/logo bring-up to a **playable Godot 4 2D game**, [Dodge the
Creeps](platform/vita/tests/dodge_the_creeps/). A real-Vita tester reported
the game **fully functional** after native GXM rendering, joypad mapping,
HUD, animation and worker-thread fixes.

| Milestone | Current state |
|---|---|
| ARM template_release, PIC libgodot.so and kuBridge loader | Working on real Vita |
| Native GXM ColorRects, sprites, 2D game and UI | Working in Dodge the Creeps; partial Canvas coverage |
| X/START, D-pad, analog controls and sound | Working in the reference demo |
| Game score, collision, enemies, game-over UI | Confirmed playable on physical Vita |
| Native worker-pool crash at score ~8 | Coredump decoded and patch applied; newer build reported playable |
| Six-second Godot Vita chewing animation and optional a5 LiveArea | Compiled and packaged; animated transition still needs a device test |
| GPUParticles2D, arbitrary 2D custom shaders and 3D | Not fully supported |

The native GXM renderer bypasses the incomplete GLES3-to-Cg path for supported
Canvas2D primitives. This result does **not** imply that arbitrary Godot 4.8
projects, effects, shaders or particle scenes can run on Vita.

The updated demo adds an in-game settings overlay (speed, master volume,
FPS cap) and progressive color changes with score, host-tested but awaiting
fresh Vita testing. It also offers an optional 180-frame/30-FPS loading animation. Its
[assets](platform/vita/demo_assets/dodge_the_creeps/) and [build documentation]
(platform/vita/BUILDING.md) are included. Details are summarized in the
[October 2026 technical update](platform/vita/UPDATES_2026_10.md).

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

Requires [VitaSDK](https://vitasdk.org), Python 3/SCons, CMake and a
**matching, validated patched VitaGL** build. See the full
[build guide](platform/vita/BUILDING.md).

Build an engine shared object from the repository root:

    python3 -m SCons platform=vita arch=arm32 target=template_release       optimize=size disable_path_overrides=no vita_shared=yes       vita_gl_root=/absolute/path/to/patched/vitagl -j6

Then package the GXM Dodge the Creeps demo with optional native loading:

    python3 platform/vita/tools/package_runtime.py       --runtime bin/libgodot.so       --ubo-build /absolute/path/to/validated-ubo-build       --output /absolute/path/to/new-vpk-build       --backend gxm2d --scene dodge_the_creeps       --livearea-dir platform/vita/demo_assets/dodge_the_creeps/LiveArea       --boot-animation platform/vita/demo_assets/dodge_the_creeps/boot_anim.dat

For a fresh clone, import the example in a Godot 4.8 host executable first
to create .godot/imported resources. The Vita engine cannot import arbitrary
source PNG, OGG and font assets at runtime. VitaSDK+matching VitaGL dependencies
are required for packaging; see BUILDING.md. VPKs are generated outputs and
not guaranteed to run without real-hardware testing.

The independent, physically tested game lives in the sibling local workspace
DodgeCreepsVita; this fork remains the engine source and reference fixture.

---

## Layout of the Vita-specific work

| Path | Purpose |
|---|---|
| platform/vita/ | Vita OS, inputs, audio, graphics and native platform code |
| platform/vita/rasterizer_gxm_vita.cpp | Native GXM canvas subset |
| platform/vita/loader/ | Shared-object loader and optional boot_splash.c |
| platform/vita/demo_assets/dodge_the_creeps/ | a5 LiveArea and original 180-frame boot package |
| platform/vita/tests/dodge_the_creeps/ | Playable sample and host regression tests |
| platform/vita/tests/colorrect/, canvas_features/ | Renderer validation fixtures |
| platform/vita/tools/ | ELF verification, probes and package creation |
| platform/vita/vitagl/ | Patched VitaGL source notes |

The original upstream README is preserved as
[README.godot.md](README.godot.md).

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
