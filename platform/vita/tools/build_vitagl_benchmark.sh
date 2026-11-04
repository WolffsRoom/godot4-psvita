#!/bin/sh
# Run under WSL. Builds the two renderer benchmark VPKs:
#   benchmark_gxm.vpk     native libvita2d/GXM bridge   (GDBG00001, "Bench GXM")
#   benchmark_vitagl.vpk  VitaGL bridge                 (GDBV00001, "Bench VitaGL")
# Both carry the same libgodot.so (GXM 2D + 3D prototype) and the same
# tests/vitagl_benchmark project; only eboot.bin differs.
#
# Usage: build_vitagl_benchmark.sh <vitagl-root> <output-dir> [vitagl-bridge-root]
#   vitagl-root         patched VitaGL used by the runtime and the GXM loader
#   vitagl-bridge-root  optional VitaGL for the VitaGL loader, e.g. the same
#                       commit/patch built with NO_DEBUG=1 and no LOG_ERRORS
# Environment: VITA_RUNTIME_SO=/path/libgodot.so reuses an existing runtime
# (it must be a vita_gxm3d=yes build) instead of running SCons.
set -eu
engine=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
vitagl=$(CDPATH= cd -- "$1" && pwd)
mkdir -p "$2"
work=$(CDPATH= cd -- "$2" && pwd)
bridge_vitagl=$(CDPATH= cd -- "${3:-$1}" && pwd)
export VITASDK=${VITASDK:-/usr/local/vitasdk}
build_id=bench-$(date -u +%Y%m%dT%H%M%SZ)
cd "$engine"
if [ -n "${VITA_RUNTIME_SO:-}" ]; then
    cp "$VITA_RUNTIME_SO" "$work/libgodot.so"
else
    python3 -m SCons platform=vita arch=arm32 target=template_release optimize=size \
        disable_path_overrides=no vita_shared=yes vita_gxm3d=yes vita_gl_root="$vitagl" -j"$(nproc)"
    cp bin/libgodot_gxm3d.so "$work/libgodot.so"
fi
"$VITASDK/bin/arm-vita-eabi-strip" --strip-unneeded "$work/libgodot.so"
python3 platform/vita/tools/verify_runtime.py "$work/libgodot.so" --report "$work/runtime-validation.json"
game="$engine/platform/vita/tests/vitagl_benchmark"
build_loader() { # <dir> <title-id> <name> <vitagl-root> [extra cmake args]
    dir=$1; title=$2; name=$3; root=$4; shift 4
    cmake -S platform/vita/loader -B "$work/$dir" \
        -DVITA_RUNTIME="$work/libgodot.so" \
        -DVITA_GAME_DATA="$game" \
        -DVITA_GL_ROOT="$root" \
        -DVITA_GXM3D=ON \
        -DVITA_TITLEID="$title" -DVITA_APP_NAME="$name" \
        -DVITA_RUNTIME_BUILD_ID="$build_id-$dir" "$@"
    cmake --build "$work/$dir" -j"$(nproc)"
}
build_loader loader-gxm GDBG00001 "Bench GXM" "$vitagl"
build_loader loader-vitagl GDBV00001 "Bench VitaGL" "$bridge_vitagl" -DVITA_VITAGL_BRIDGE=ON
cp "$work/loader-gxm/godot_loader.vpk" "$work/benchmark_gxm.vpk"
cp "$work/loader-vitagl/godot_loader.vpk" "$work/benchmark_vitagl.vpk"
python3 - "$work" "$build_id" <<'EOF'
import hashlib, json, sys, zipfile
from pathlib import Path
work = Path(sys.argv[1])
manifest = {"build_id": sys.argv[2], "packages": {}}
for name, marker in (("benchmark_gxm.vpk", None), ("benchmark_vitagl.vpk", "vitagl.cfg")):
    path = work / name
    with zipfile.ZipFile(path) as vpk:
        assert vpk.testzip() is None, name
        names = set(vpk.namelist())
        assert {"eboot.bin", "libgodot.so", "gxm2d.cfg", "gxm3d.cfg", "game_data/main.gd"} <= names, name
        assert (marker in names) if marker else ("vitagl.cfg" not in names), name
        manifest["packages"][name] = {
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "eboot_sha256": hashlib.sha256(vpk.read("eboot.bin")).hexdigest(),
            "libgodot_sha256": hashlib.sha256(vpk.read("libgodot.so")).hexdigest(),
        }
runtimes = {p["libgodot_sha256"] for p in manifest["packages"].values()}
assert len(runtimes) == 1, "both VPKs must carry the same libgodot.so"
(work / "benchmark-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
EOF
echo "VPKs: $work/benchmark_gxm.vpk $work/benchmark_vitagl.vpk"
