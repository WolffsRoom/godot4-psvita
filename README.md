<p align="center">
  <img src="Logo.png" alt="Godot 4 Vita" width="300">
</p>

# Godot 4 Vita

An experimental port of **Godot 4.8-dev7 to PlayStation Vita**, focused on running 2D games on real hardware. It provides Vita input and audio, a native GXM canvas renderer, and a shared-library loader for the Godot runtime.

> **Development build:** This is not an official Godot release or a general-purpose Vita export template.

## Architecture

| Component | Implementation |
|---|---|
| **Engine** | Godot 4.8-dev7 compiled for ARM as `libgodot.so` (PIC) |
| **Loader** | Lightweight Vita eboot loads, relocates and starts the shared library |
| **2D graphics** | Native GXM backend via [libvita2d](https://github.com/xerpi/libvita2d); selected with `gxm2d.cfg` |
| **Compatibility renderer** | Experimental VitaGL path with GLES3-to-Cg shader adaptations |
| **Input and audio** | Native Vita controls (D-pad, analog sticks and buttons) and audio integration |
| **Display** | 960 × 544, matching the Vita's native screen resolution |

The shared-library approach avoids linking the entire Godot engine into one large eboot. The native GXM path draws supported Godot CanvasItem commands directly rather than relying on GLES3 shader translation.

## Implemented features

- **Playable 2D reference:** [Dodge the Creeps Vita](https://github.com/WolffsRoom/DodgeTheCreeps-PSVita), with gameplay, collisions, HUD, audio and controls confirmed on a physical Vita.
- **GXM canvas:** solid-color rectangles, textured sprites, texture regions, transforms, tinting, clipping and bitmap/atlas-based text within the implemented subset.
- **Controller support:** D-pad, analog movement and face/START buttons exposed through Godot's input system.
- **Stability:** a `WorkerThreadPool` index-check crash identified from a Vita coredump was corrected.
- **Packaging:** per-game assets, optional LiveArea and a native pre-engine loading animation.

The latest settings menu, color changes and animated loading screen in Dodge the Creeps have passed host/build checks; those revisions still need physical-Vita confirmation.

## Limitations

- **Incomplete Canvas 2D:** not every CanvasItem command, material, font mode or screen effect is implemented.
- **No general 3D support** or full custom-shader compatibility.
- **GPU particles are unsupported** in the native 2D path; the reference game uses a sprite-based fallback.
- **Assets must be imported before packaging:** the Vita runtime cannot generate `.godot/imported/` resources.
- Performance, memory use and compatibility must be verified **per game**.

See [GXM_BACKEND.md](platform/vita/GXM_BACKEND.md) for rendering details.

## Build

**Requirements:** Linux/WSL, [VitaSDK](https://vitasdk.org), Python 3, SCons, CMake, a compatible patched [VitaGL](https://github.com/Rinnegatamante/vitagl) build and Godot 4.8 on the host for importing project assets.

From the repository root, after preparing the VitaGL dependency:

```bash
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"

python3 -m SCons platform=vita arch=arm32 target=template_release \
  optimize=size disable_path_overrides=no vita_shared=yes \
  vita_gl_root=/path/to/validated/vitagl -j6
```

This generates `bin/libgodot.so`. Package the 2D reference project:

```bash
python3 platform/vita/tools/package_runtime.py \
  --runtime bin/libgodot.so \
  --ubo-build /path/to/validated-ubo-build \
  --output /path/to/new-build \
  --backend gxm2d --scene dodge_the_creeps
```

The `--ubo-build` directory must contain the matching VitaGL dependency and its validated manifest. The packaging tool checks ELF imports, dependency hashes, resources and VPK integrity.

**Optional LiveArea and loading animation:** append these flags to the packaging command:

```bash
--livearea-dir platform/vita/demo_assets/dodge_the_creeps/LiveArea \
--boot-animation platform/vita/demo_assets/dodge_the_creeps/boot_anim.dat
```

See [BUILDING.md](platform/vita/BUILDING.md) for complete setup, imported assets and device diagnostics. The standalone reference also has its own [build instructions](https://github.com/WolffsRoom/DodgeTheCreeps-PSVita#build).

## Repository structure

| Path | Purpose |
|---|---|
| `platform/vita/` | Vita platform, graphics, input, audio and OS integration |
| `platform/vita/rasterizer_gxm_vita.cpp` | Native GXM 2D renderer |
| `platform/vita/loader/` | Shared-library loader and optional animated boot screen |
| `platform/vita/tests/` | ColorRect, canvas and Dodge the Creeps test projects |
| `platform/vita/demo_assets/` | Example LiveArea and loading animation |
| `platform/vita/tools/` | Build probes, runtime verification and VPK packaging |
| `platform/vita/vitagl/` | VitaGL patches and dependency notes |

## Credits

- [Godot Engine](https://github.com/godotengine/godot) — upstream engine; original README: [README.godot.md](README.godot.md).
- [Dodge the Creeps Vita](https://github.com/WolffsRoom/DodgeTheCreeps-PSVita) — playable reference project.
- [VitaSDK](https://vitasdk.org), [VitaGL](https://github.com/Rinnegatamante/vitagl), [libvita2d](https://github.com/xerpi/libvita2d) and [kuBridge](https://github.com/TheOfficialFloW/kuBridge).
- [SonicMastr/godot-vita](https://github.com/SonicMastr/godot-vita) — Godot 3.x Vita port reference.

Godot engine code and port additions are provided under the [MIT license](LICENSE.txt); third-party components retain their respective licenses.
