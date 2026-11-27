# Integrated Linux editor

Based on the Linux editor guide proposed by gtyire40-cpu in
WolffsRoom/godot4-psvita#8, adapted to the v0.3 build and packaging tools.

The Linux editor is a native x86_64 Godot 4.8-dev7 editor build with PS Vita
registered as a built-in export platform. It loads the portable `vita/`
support folder beside the executable, so exporting a project needs no addon,
Python, SCons or VitaSDK.

## Running

Extract the release ZIP and keep `vita/` next to `Godot4Vita`:

```sh
./Godot4Vita
./Godot4Vita --editor --path /path/to/project
```

Display drivers: the editor is built with X11 and, when the build host has
`wayland-scanner`, Wayland. `--display-driver x11` works on a Wayland desktop
through XWayland. If `--display-driver wayland` answers "Unknown display
driver", that binary was built without Wayland.

Renderer: the desktop editors are built without Vulkan, D3D12 or Metal.
Projects saved with Forward+ or Mobile open with the Compatibility renderer and
print a warning; the PS Vita export requires Compatibility anyway (use
**Prepare settings for PS Vita** or set Rendering > Renderer > Rendering Method
to `gl_compatibility`). Editors from v0.3 and older stop with "Video driver not
found" on such projects; start them with `--rendering-driver opengl3` instead.

## Building

Requirements: Linux x86_64 with Godot's Linux/*BSD build dependencies, Python 3,
SCons and `wayland-scanner` (Debian/Ubuntu: `libwayland-bin`). No VitaSDK is
needed; the runtime template comes from an existing distribution.

```sh
platform/vita/tools/build_linux_editor.sh "$PWD/_build/editor-linux"
python3 platform/vita/tools/package_editor_distribution.py --platform linux \
  --arch x86_64 --payload _build/editor-linux/Godot4Vita \
  --support /path/to/portable-support \
  --output releases/Godot4Vita-v0.3-Linux-x86_64.zip
```

The build script compiles a staged copy (`VITA_LINUX_EDITOR_BUILD_DIR`,
default `/tmp/godot4vita-linux-editor`) so SCons output stays out of the
checkout. It stops when `wayland-scanner` is missing; set
`VITA_EDITOR_ALLOW_NO_WAYLAND=1` for an X11-only build. The packager keeps the
executable bit in the ZIP even when it runs on Windows.

## Validation

```sh
./Godot4Vita --headless --editor --path Exemplo \
  --script "$PWD/platform/vita/editor/test_integrated_preset.gd"
./Godot4Vita --headless --editor --path Exemplo \
  --export-release "PS Vita" /tmp/GodotVita-example.vpk
python3 platform/vita/tools/tests/test_editor_vpk.py \
  /tmp/GodotVita-example.vpk /tmp/GodotVita-example-extracted
```

`validate_integrated_editor.py` targets the Windows distribution. A VPK that
passes these checks still needs a test on a physical Vita.
