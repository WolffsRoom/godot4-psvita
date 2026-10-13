<p align="center"><img src="Logo.png" alt="Godot 4 Vita" width="300"></p>

# Godot 4 Vita

An experimental **Godot 4.8-dev7 port for PlayStation Vita**. It uses a native **GXM 2D canvas backend**, VitaGL integration and a PIC `libgodot.so` loaded by a small homebrew eboot.

## Status — October 2026

| Feature | Status |
|---|---|
| Engine boot, sound, input and native GXM 2D | Working on real Vita |
| [Dodge the Creeps](https://github.com/WolffsRoom/DodgeTheCreeps-PSVita) gameplay, HUD, collision, controls | Reported functional on physical Vita |
| D-pad/analog/X/START and thread-pool crash fix | Implemented and hardware-tested in the demo |
| Configurable `a5` LiveArea and animated native boot logo | Build verified; latest animation/handoff needs Vita testing |
| Full Canvas2D, GPU particles, custom shaders and 3D | Incomplete/not supported |

This is **not** a general Godot export template. The GXM backend supports a subset of 2D drawing operations; sample-specific sprite fallbacks cover unsupported effects.

## Building

Requires [VitaSDK](https://vitasdk.org), SCons, CMake, Python 3 and a **matching patched VitaGL** build. Follow [BUILDING.md](platform/vita/BUILDING.md) for the dependency setup and validation.

```bash
# Engine shared object (from repository root)
python3 -m SCons platform=vita arch=arm32 target=template_release \
  optimize=size disable_path_overrides=no vita_shared=yes \
  vita_gl_root=/path/to/validated/vitagl -j6

# Example: package the native GXM demo with the a5 LiveArea and loading animation
python3 platform/vita/tools/package_runtime.py \
  --runtime bin/libgodot.so --ubo-build /path/to/validated-ubo-build \
  --output /path/to/new-output --backend gxm2d --scene dodge_the_creeps \
  --livearea-dir platform/vita/demo_assets/dodge_the_creeps/LiveArea \
  --boot-animation platform/vita/demo_assets/dodge_the_creeps/boot_anim.dat
```

Import the demo using a **Godot 4.8 host editor** before packaging: Vita cannot create its `.godot/imported/` texture/audio/font assets on-device. Compiled runtimes, test logs and VPKs are not source dependencies.

## Reference

- [Dodge the Creeps PS Vita](https://github.com/WolffsRoom/DodgeTheCreeps-PSVita) — standalone playable game, build scripts and assets.
- [GXM backend](platform/vita/GXM_BACKEND.md) · [October technical update](platform/vita/UPDATES_2026_10.md) · [Development roadmap](platform/vita/ROADMAP.md).
- [Godot Engine](https://github.com/godotengine/godot) · [VitaGL](https://github.com/Rinnegatamante/vitagl) · [VitaSDK](https://vitasdk.org) · [kuBridge](https://github.com/TheOfficialFloW/kuBridge) · [SonicMastr's Godot Vita 3.x](https://github.com/SonicMastr/godot-vita).

Godot's original README remains in [README.godot.md](README.godot.md). The Godot Engine and port additions follow the [MIT license](LICENSE.txt); bundled components retain their own licenses.
