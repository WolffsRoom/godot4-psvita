#!/usr/bin/env bash
set -euo pipefail

SOURCE="$(cd "$(dirname "$0")/../../.." && pwd)"
WORK="${VITA_EDITOR_BUILD_DIR:-/tmp/godot-vita-linux-editor}"
OUTPUT="${VITA_EDITOR_OUTPUT_DIR:-$SOURCE/../editor-vita-linux}"
JOBS="${VITA_EDITOR_JOBS:-6}"

mkdir -p "$WORK" "$OUTPUT"
tar -C "$SOURCE" --exclude=.git --exclude=bin --exclude='*.o' --exclude='*.a' \
    --exclude='*.so' --exclude='*.gen.*' --exclude='.scons*' -cf - . | tar -C "$WORK" -xf -
cd "$WORK"

export GODOT_VERSION_STATUS=dev7 BUILD_NAME=vita_editor
scons platform=linuxbsd target=editor arch=x86_64 optimize=size debug_symbols=no \
    x11=yes wayland=yes vulkan=no opengl3=yes lto=none "-j$JOBS"

install -m 755 bin/godot.linuxbsd.editor.x86_64 "$OUTPUT/Godot4Vita"
