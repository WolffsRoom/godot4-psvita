#!/usr/bin/env bash
set -euo pipefail

source_root="$(cd "$(dirname "$0")/../../.." && pwd)"
work="${VITA_LINUX_EDITOR_BUILD_DIR:-/tmp/godot4vita-linux-editor}"
output="${1:-$source_root/../_build/editor-linux}"

# Wayland support is compiled only when wayland-scanner is installed
# (Debian/Ubuntu: libwayland-bin). Without it the editor is X11-only.
if ! command -v wayland-scanner >/dev/null 2>&1 && [ "${VITA_EDITOR_ALLOW_NO_WAYLAND:-0}" != 1 ]; then
    echo "wayland-scanner not found: install libwayland-bin (or set VITA_EDITOR_ALLOW_NO_WAYLAND=1 for an X11-only build)." >&2
    exit 1
fi

mkdir -p "$work" "$output"
tar -C "$source_root" --exclude=.git --exclude=bin --exclude='*.o' --exclude='*.a' --exclude='*.so' --exclude='*.gen.*' --exclude='.scons*' -cf - . | tar -C "$work" -xf -
cd "$work"
export GODOT_VERSION_STATUS=dev7 BUILD_NAME=vita_editor
python3 -m SCons platform=linuxbsd target=editor arch=x86_64 optimize=size debug_symbols=no \
    x11=yes wayland=yes vulkan=no opengl3=yes lto=none -j"${BUILD_JOBS:-6}"
cp bin/godot.linuxbsd.editor.x86_64 "$output/Godot4Vita"
chmod +x "$output/Godot4Vita"
"$output/Godot4Vita" --headless --version
echo "$output/Godot4Vita"
