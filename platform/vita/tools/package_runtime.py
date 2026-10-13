#!/usr/bin/env python3
"""Validate and package an explicitly built engine with the ColorRect test scene.

Run the SCons vita_shared target first. This tool never falls back to the old
loader/libgodot.so and never edits the input binary or VitaGL installation.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

from verify_runtime import verify

PLATFORM = Path(__file__).resolve().parents[1]
ENGINE = PLATFORM.parents[1]


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--ubo-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--backend", choices=("vitagl", "gxm2d"), default="vitagl")
    parser.add_argument("--scene", choices=("colorrect", "canvas_features", "dodge_the_creeps"), default="colorrect")
    parser.add_argument("--livearea-dir", type=Path,
                        help="Optional LiveArea assets (icon0, pic0, bg0, startup, template)")
    parser.add_argument("--boot-animation", type=Path,
                        help="Optional native 960x544 DVAN1 frame sequence (boot_anim.dat)")
    parser.add_argument("--vitasdk", type=Path, default=os.environ.get("VITASDK", "/usr/local/vitasdk"))
    args = parser.parse_args()
    runtime, dependency, output, sdk = (p.resolve() for p in (args.runtime, args.ubo_build, args.output, args.vitasdk))
    livearea = args.livearea_dir.resolve() if args.livearea_dir else None
    boot_animation = args.boot_animation.resolve() if args.boot_animation else None
    livearea_targets = {
        "icon0.png": "sce_sys/icon0.png",
        "pic0.png": "sce_sys/pic0.png",
        "bg0.png": "sce_sys/livearea/contents/bg0.png",
        "startup.png": "sce_sys/livearea/contents/startup.png",
        "template.xml": "sce_sys/livearea/contents/template.xml",
    }
    if livearea:
        for name in livearea_targets:
            if not (livearea / name).is_file():
                parser.error(f"Missing LiveArea asset: {livearea / name}")
    if boot_animation:
        if not boot_animation.is_file() or boot_animation.read_bytes()[:24] != (
                b"DVAN" + struct.pack("<5I", 1, 960, 544, 30, 180)):
            parser.error("Expected a native DVAN1 960x544 30 FPS 180-frame boot animation")
    previous = json.loads((dependency / "manifest.json").read_text())
    library = dependency / "vitagl/libvitaGL.a"
    if previous["status"] != "built" or digest(library) != previous["vitagl_sha256"]:
        parser.error("VitaGL does not match the completed dependency manifest")
    if digest(library.parent / "source/vitaGL.h") != previous["vitagl_header_sha256"]:
        parser.error("VitaGL header does not match the dependency manifest")
    report = verify(runtime, PLATFORM / "loader/default_dynlib.c")
    if args.backend == "gxm2d" and "vita_gxm_init" not in report["imports"]:
        parser.error("The runtime does not contain the native GXM backend")
    vita2d = sdk / "arm-vita-eabi/lib/libvita2d.a"
    vita2d_header = sdk / "arm-vita-eabi/include/vita2d.h"
    native_hashes = {str(p): digest(p) for p in (vita2d, vita2d_header)}
    output.mkdir(parents=True, exist_ok=False)
    (output / "runtime-verification.json").write_text(json.dumps(report, indent=2) + "\n")
    copied_runtime = output / "libgodot.so"
    shutil.copy2(runtime, copied_runtime)
    # so_load allocates a RAM buffer for the ENTIRE file before mapping it.
    # Keep symbols in the input ELF for diagnosis, but do not ship its duplicate
    # static symbol/debug tables. Dynamic symbols/relocations and section names
    # required by so_util remain, and are verified again after stripping.
    subprocess.run([sdk / "bin/arm-vita-eabi-strip", "--strip-unneeded", copied_runtime], check=True)
    report = verify(copied_runtime, PLATFORM / "loader/default_dynlib.c")
    (output / "runtime-verification.json").write_text(json.dumps(report, indent=2) + "\n")
    build_id = args.backend + "-" + args.scene + "-" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    title_id = "GDGX40001" if args.backend == "gxm2d" else "GDCT40001"
    app_name = "Godot GXM ColorRect" if args.backend == "gxm2d" else "Godot ColorRect Test"
    if args.scene == "canvas_features":
        title_id = "GDGX40002"
        app_name = "Godot GXM Canvas"
    if args.scene == "dodge_the_creeps":
        title_id = "GDDC40001"
        app_name = "Dodge the Creeps Vita"
    scene = PLATFORM / "tests" / args.scene
    scene_files = sorted(p for p in scene.rglob("*") if p.is_file()
                         and ".godot/editor" not in p.as_posix()
                         and ".godot/shader_cache" not in p.as_posix())
    inputs = [*scene_files, *list((PLATFORM / "loader").glob("*.c")),
              *list((PLATFORM / "loader").glob("*.h")), PLATFORM / "loader/CMakeLists.txt",
              PLATFORM / "cmake/VitaGL.cmake", Path(__file__), PLATFORM / "tools/verify_runtime.py",
              PLATFORM / "canvas_uniforms_vita.h", PLATFORM / "detect.py", PLATFORM / "SCsub",
              PLATFORM / "os_vita.cpp", PLATFORM / "godot_vita.cpp",
              PLATFORM / "joypad_vita.cpp", PLATFORM / "joypad_vita.h",
              PLATFORM / "display_server_vita.cpp", PLATFORM / "gxm_bridge.h",
              PLATFORM / "rasterizer_gxm_vita.cpp", PLATFORM / "rasterizer_gxm_vita.h",
              PLATFORM / "licenses/libvita2d-LICENSE.txt",
              PLATFORM / "fonts/Lato-Regular.ttf", PLATFORM / "fonts/Lato-LICENSE.txt",
              PLATFORM / "fonts/vita_default_font.gen.h", ENGINE / "scene/theme/default_theme.cpp",
              ENGINE / "core/object/worker_thread_pool.cpp",
              ENGINE / "core/object/worker_thread_pool.h",
              ENGINE / "drivers/gles3/rasterizer_canvas_gles3.cpp",
              ENGINE / "drivers/gles3/rasterizer_canvas_gles3.h",
              ENGINE / "drivers/gles3/shader_gles3.cpp", ENGINE / "drivers/gles3/shader_gles3.h"]
    manifest = {
        "build_id": build_id, "status": "packaging", "hardware_result": "not_run",
        "title_id": title_id, "backend": args.backend, "scene": args.scene, "native_dependencies_sha256": native_hashes,
        "runtime_sha256": digest(copied_runtime),
        "source_runtime_sha256": digest(runtime), "source_runtime": str(runtime),
        "source_runtime_size": runtime.stat().st_size, "runtime_size": copied_runtime.stat().st_size,
        "vitagl_sha256": digest(library), "dependency_build_id": previous["build_id"],
        "dependency_manifest_sha256": digest(dependency / "manifest.json"),
        "branding": {
            "livearea": {name: digest(livearea / name) for name in livearea_targets} if livearea else None,
            "boot_animation_sha256": digest(boot_animation) if boot_animation else None,
        },
        "engine_commit": subprocess.check_output(["git", "-C", ENGINE, "rev-parse", "HEAD"], text=True).strip(),
        "inputs_sha256": {str(p.relative_to(ENGINE)): digest(p) for p in inputs},
        "runtime_verification": {"result": report["result"], "import_count": len(report["imports"])},
    }
    manifest_file = output / "manifest.json"
    def save():
        manifest_file.write_text(json.dumps(manifest, indent=2) + "\n")
    save()
    # Restrict the diff to relevant tracked source files, avoiding a scan of
    # generated/binary artifacts on mounted Windows workspaces.
    with (output / "engine-working-tree.patch").open("wb") as diff:
        subprocess.run(["git", "-C", ENGINE, "diff", "HEAD", "--",
                        "core/object/worker_thread_pool.cpp", "core/object/worker_thread_pool.h",
                        "drivers/gles3/rasterizer_canvas_gles3.cpp", "drivers/gles3/rasterizer_canvas_gles3.h",
                        "drivers/gles3/shader_gles3.cpp", "drivers/gles3/shader_gles3.h",
                        "platform/vita/detect.py", "platform/vita/SCsub", "platform/vita/godot_vita.cpp",
                        "platform/vita/loader/CMakeLists.txt", "platform/vita/loader/main.c",
                        "platform/vita/loader/so_util.c", "platform/vita/os_vita.cpp",
                        "platform/vita/display_server_vita.cpp"], stdout=diff, check=True)
    # Include untracked platform additions as source files, not only a git diff.
    for path in inputs:
        dest = output / "source" / path.relative_to(ENGINE)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
    env = os.environ.copy()
    env["VITASDK"] = str(sdk)
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    try:
        with (output / "build.log").open("w") as log:
            commands = [
                ["cmake", "-S", PLATFORM / "loader", "-B", output / "loader",
                 f"-DVITA_GL_ROOT={library.parent}", f"-DVITA_RUNTIME={copied_runtime}",
                 f"-DVITA_RUNTIME_BUILD_ID={build_id}", f"-DVITA_GAME_DATA={scene}",
                 f"-DVITA_TITLEID={title_id}", f"-DVITA_APP_NAME={app_name}",
                 f"-DVITA_GXM2D={'ON' if args.backend == 'gxm2d' else 'OFF'}",
                 f"-DVITA_LIVEAREA_DIR={livearea if livearea else ''}",
                 f"-DVITA_BOOT_ANIMATION={boot_animation if boot_animation else ''}",
                 f"-DVITA_FULL_GAME_DATA={'ON' if args.scene == 'dodge_the_creeps' else 'OFF'}"],
                ["cmake", "--build", output / "loader", "--parallel", "4"],
            ]
            for command in commands:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        vpk = output / "godot_colorrect.vpk"
        shutil.copy2(output / "loader/godot_loader.vpk", vpk)
        with zipfile.ZipFile(vpk) as archive:
            if archive.testzip() is not None:
                raise RuntimeError("VPK CRC check failed")
            if hashlib.sha256(archive.read("libgodot.so")).hexdigest() != manifest["runtime_sha256"]:
                raise RuntimeError("Wrong runtime in VPK")
            if archive.read("eboot.bin") != (output / "loader/godot_loader.self").read_bytes():
                raise RuntimeError("Wrong loader in VPK")
            if args.backend == "gxm2d":
                if archive.read("gxm2d.cfg") != b"native-canvas-v2\n":
                    raise RuntimeError("Missing or wrong GXM selector")
            elif "gxm2d.cfg" in archive.namelist():
                raise RuntimeError("Unexpected GXM selector in VitaGL package")
            for name in ("project.godot", "main.tscn", "main.gd"):
                if archive.read(f"game_data/{name}") != (scene / name).read_bytes():
                    raise RuntimeError(f"Wrong scene file in VPK: {name}")
            if args.scene == "dodge_the_creeps":
                for resource in scene_files:
                    relative = resource.relative_to(scene).as_posix()
                    if relative.startswith(".") and not relative.startswith(".godot/imported/") and relative != ".godot/uid_cache.bin":
                        continue
                    internal = "game_data/" + relative
                    if internal not in archive.namelist() or archive.read(internal) != resource.read_bytes():
                        raise RuntimeError("Missing or modified game resource: " + relative)
            if livearea:
                for name, inside in livearea_targets.items():
                    if archive.read(inside) != (livearea / name).read_bytes():
                        raise RuntimeError(f"Wrong LiveArea asset: {name}")
            if boot_animation and archive.read("boot_anim.dat") != boot_animation.read_bytes():
                raise RuntimeError("Wrong pre-engine boot animation")
            if title_id.encode() not in archive.read("sce_sys/param.sfo"):
                raise RuntimeError("Wrong title ID")
        for path, expected in manifest["inputs_sha256"].items():
            if digest(ENGINE / path) != expected:
                raise RuntimeError(f"Input changed during build: {path}")
        for name, before in manifest["branding"]["livearea"].items() if livearea else []:
            if digest(livearea / name) != before:
                raise RuntimeError(f"LiveArea changed during packaging: {name}")
        if boot_animation and digest(boot_animation) != manifest["branding"]["boot_animation_sha256"]:
            raise RuntimeError("Boot animation changed during packaging")
        if digest(library) != manifest["vitagl_sha256"]:
            raise RuntimeError("VitaGL changed during packaging")
        for path, expected in native_hashes.items():
            if digest(Path(path)) != expected:
                raise RuntimeError(f"Native dependency changed: {path}")
        manifest["vpk_sha256"] = digest(vpk)
        manifest["status"] = "built"
        save()
    except Exception as error:
        manifest["status"] = "failed"
        manifest["error"] = str(error)
        save()
        raise
    print(f"Built {vpk}\nBUILD_ID: {build_id}\nDevice boot and visual acceptance still required.")


if __name__ == "__main__":
    main()
