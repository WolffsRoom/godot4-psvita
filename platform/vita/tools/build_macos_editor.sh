#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS with Xcode command line tools." >&2
    exit 2
fi

source_root="$(cd "$(dirname "$0")/../../.." && pwd)"
output="${1:-$source_root/../_build/editor-macos}"
jobs="${BUILD_JOBS:-6}"
mkdir -p "$output"
cd "$source_root"
export GODOT_VERSION_STATUS=dev7 BUILD_NAME=vita_editor

python3 -m SCons platform=macos target=editor arch=x86_64 optimize=size debug_symbols=no \
    vulkan=no metal=no lto=none -j"$jobs"
python3 -m SCons platform=macos target=editor arch=arm64 optimize=size debug_symbols=no \
    vulkan=no lto=none -j"$jobs"

app="$output/Godot4Vita.app"
rm -rf "$app"
cp -R misc/dist/macos_tools.app "$app"
mkdir -p "$app/Contents/MacOS"
lipo -create bin/godot.macos.editor.x86_64 bin/godot.macos.editor.arm64 -output "$app/Contents/MacOS/Godot4Vita"
chmod +x "$app/Contents/MacOS/Godot4Vita"
/usr/libexec/PlistBuddy -c 'Set :CFBundleExecutable Godot4Vita' "$app/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Set :CFBundleName Godot4Vita' "$app/Contents/Info.plist"
/usr/bin/codesign --force --deep --sign - "$app"
"$app/Contents/MacOS/Godot4Vita" --headless --version
echo "$app"
