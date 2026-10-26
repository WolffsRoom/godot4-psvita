#!/bin/sh
# Run under WSL. Builds the experimental GXM 2D/3D runtime, the generic loader
# and the portable editor export template (vita_release.zip).
# Usage: build_release_template.sh <vitagl-root> <output-dir> [build-id]
set -eu
engine=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
vitagl=$1
work=$2
build_id=${3:-godot4-psvita-release}
export VITASDK=${VITASDK:-/usr/local/vitasdk}
mkdir -p "$work"
cd "$engine"
python3 -m SCons platform=vita arch=arm32 target=template_release optimize=size \
    disable_path_overrides=no vita_shared=yes vita_gxm3d=yes vita_gl_root="$vitagl" -j"$(nproc)"
cp bin/libgodot_gxm3d.so "$work/libgodot.so"
"$VITASDK/bin/arm-vita-eabi-strip" --strip-unneeded "$work/libgodot.so"
python3 platform/vita/tools/verify_runtime.py "$work/libgodot.so" --report "$work/runtime-validation.json"
cmake -S platform/vita/loader -B "$work/loader" \
    -DVITA_RUNTIME="$work/libgodot.so" \
    -DVITA_GAME_DATA="$engine/platform/vita/tests/mesh3d" \
    -DVITA_GL_ROOT="$vitagl" \
    -DVITA_GXM3D=ON -DVITA_TITLEID=GDVT00001 \
    -DVITA_RUNTIME_BUILD_ID="$build_id" \
    -DVITA_BOOT_ANIMATION="$engine/platform/vita/loader/boot_assets/custom_splash_960x544.dat"
cmake --build "$work/loader" -j"$(nproc)"
python3 platform/vita/tools/make_editor_template.py --rgba4444 "$work/loader/godot_loader.vpk" "$work/vita_release.zip"
