# Integrated Linux editor

The Linux editor is a native x86-64 Godot editor build with PS Vita registered
as a built-in export platform. It loads the portable `vita/` support directory
beside the executable, so users do not need to install the project addon,
Python, SCons, or VitaSDK to export a project.

## Requirements to build

- Linux x86-64 with the compiler and system development packages required by
  Godot's Linux/*BSD editor build.
- Python 3 and SCons.
- A validated `vita_release.zip` runtime template built with
  `tools/make_editor_template.py`.

The bundled runtime template is a separate build artifact and is not generated
by the desktop editor build.

## Build and package

From the engine checkout:

```sh
platform/vita/tools/build_linux_editor.sh
python3 platform/vita/tools/package_linux_editor.py \
  --template /absolute/path/to/vita_release.zip
python3 platform/vita/tools/archive_linux_editor.py
```

The editor build uses a staged copy under `/tmp/godot-vita-linux-editor` by
default, keeping generated SCons files out of the source checkout. Set
`VITA_EDITOR_BUILD_DIR` or `VITA_EDITOR_OUTPUT_DIR` to change the build or
distribution paths; `VITA_EDITOR_JOBS` changes the default six parallel build
jobs. The portable folder is `editor-vita-linux` beside the engine checkout;
the archive is `Godot-Vita-4.8-dev7-Linux-x86_64.tar.gz`.
Both output paths can also be changed with `--output`.
The portable folder includes an `Exemplo/` project, trophy and texture-format
guides, the runtime template and all editor support files. Imported caches and
old export outputs are not part of the archive.

The build requests the Linux/*BSD X11 and Wayland drivers. Wayland is only
included when `wayland-scanner` is available; check the build output for a
warning that says Wayland support was disabled. The resulting editor can run with
`--display-driver x11`, `--display-driver wayland`, or `--headless`, depending
on the drivers that were compiled in. XWayland is needed to run the X11 driver
from a Wayland-only desktop session.

## Use

Extract the tarball and keep the `vita/` directory next to `Godot4Vita`.
Launch the editor or open the bundled example:

```sh
./Godot4Vita
./Godot4Vita --editor --path Exemplo
```

Projects can then add a PS Vita export preset in **Project > Export**. Export
the same tested Godot 4.8-dev7 project format and runtime template as the
Windows editor. The build host does not need VitaSDK for desktop export.

## Validation and limitations

Run the staged editor headlessly against the integrated-preset test, then
export the example project before distributing a new build:

```sh
"../editor-vita-linux/Godot4Vita" --headless --editor \
  --path ../editor-vita-linux/Exemplo \
  --script "$PWD/platform/vita/editor/test_integrated_preset.gd"
"../editor-vita-linux/Godot4Vita" --headless --editor \
  --path ../editor-vita-linux/Exemplo \
  --export-release "PS Vita" /tmp/GodotVita-example.vpk
python3 platform/vita/tools/tests/test_editor_vpk.py \
  /tmp/GodotVita-example.vpk /tmp/GodotVita-example-extracted
```

Inspect the resulting VPK and its configured external game-data folder with
`tools/tests/test_editor_vpk.py`. The
`validate_integrated_editor.py` report helper currently targets the Windows
distribution and must not be used as Linux validation. An export that passes
desktop checks still requires testing on a prepared physical Vita.

This is a portable Linux x86-64 distribution, not a `.deb` or Flatpak. Runtime
requirements are those of the matching Godot Linux editor build. The Vita
runtime remains experimental and retains its documented rendering and
compatibility limitations.
