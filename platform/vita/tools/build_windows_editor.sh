#!/usr/bin/env bash
set -euo pipefail
SOURCE="$(cd "$(dirname "$0")/../../.." && pwd)"
WORK="${VITA_EDITOR_BUILD_DIR:-/tmp/godot-vita-windows-editor}"
mkdir -p "$WORK"
# Isolate generated headers and objects from the Vita runtime checkout.
tar -C "$SOURCE" --exclude=.git --exclude=bin --exclude='*.o' --exclude='*.a' --exclude='*.so' --exclude='*.gen.*' --exclude='.scons*' -cf - . | tar -C "$WORK" -xf -
cd "$WORK"
export GODOT_VERSION_STATUS=dev7 BUILD_NAME=vita_editor
scons platform=windows target=editor arch=x86_64 optimize=size debug_symbols=no \
    vulkan=no d3d12=no opengl3=yes lto=none -j6
mkdir -p "$SOURCE/../editor-vita-integrado"
cp bin/godot.windows.editor.x86_64.exe "$SOURCE/../editor-vita-integrado/Godot4Vita.exe"
