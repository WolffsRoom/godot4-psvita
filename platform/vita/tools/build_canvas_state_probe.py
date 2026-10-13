#!/usr/bin/env python3
"""Build the CanvasData probe against an identified existing UBO probe library."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

PLATFORM = Path(__file__).resolve().parents[1]
ENGINE = PLATFORM.parents[1]


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ubo-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--vitasdk", type=Path, default=os.environ.get("VITASDK", "/usr/local/vitasdk"))
    args = parser.parse_args()
    dependency = args.ubo_build.resolve()
    output = args.output.resolve()
    sdk = args.vitasdk.resolve()
    previous = json.loads((dependency / "manifest.json").read_text())
    library = dependency / "vitagl/libvitaGL.a"
    header = dependency / "vitagl/source/vitaGL.h"
    if previous["status"] != "built" or digest(library) != previous["vitagl_sha256"] or digest(header) != previous["vitagl_header_sha256"]:
        parser.error("VitaGL dependency does not match the completed build manifest")
    output.mkdir(parents=True, exist_ok=False)
    build_id = "canvas-state-" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    source = PLATFORM / "tools/canvas_state_probe"
    inputs = [Path(__file__), source / "main.cpp", source / "CMakeLists.txt",
              PLATFORM / "tools/ubo_binding_probe/sdk_shims.c", PLATFORM / "cmake/VitaGL.cmake",
              PLATFORM / "canvas_uniforms_vita.h"]
    manifest = {
        "build_id": build_id, "status": "building", "hardware_result": "not_run",
        "dependency_build_id": previous["build_id"],
        "dependency_manifest_sha256": digest(dependency / "manifest.json"),
        "vitagl_sha256": digest(library), "vitagl_header_sha256": digest(header),
        "vitasdk": str(sdk), "inputs_sha256": {str(p.relative_to(ENGINE)): digest(p) for p in inputs},
    }
    manifest_path = output / "manifest.json"
    def save():
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    save()
    env = os.environ.copy()
    env["VITASDK"] = str(sdk)
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    try:
        with (output / "build.log").open("w") as log:
            for command in (["cmake", "-S", source, "-B", output / "probe",
                             f"-DVITA_GL_ROOT={library.parent}", f"-DVITA_PROBE_BUILD_ID={build_id}"],
                            ["cmake", "--build", output / "probe", "--parallel", "4"]):
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        # Refuse to label a build whose inputs changed while compiling.
        for path, expected in manifest["inputs_sha256"].items():
            if digest(ENGINE / path) != expected:
                raise RuntimeError(f"Input changed during build: {path}")
        if digest(library) != manifest["vitagl_sha256"]:
            raise RuntimeError("VitaGL changed during build")
        vpk = output / "canvas_state_probe.vpk"
        shutil.copy2(output / "probe/canvas_state_probe.vpk", vpk)
        with zipfile.ZipFile(vpk) as archive:
            if archive.testzip() is not None or not {"eboot.bin", "sce_sys/param.sfo"} <= set(archive.namelist()):
                raise RuntimeError("Invalid VPK")
        manifest["vpk_sha256"] = digest(vpk)
        manifest["compiler"] = subprocess.check_output([sdk / "bin/arm-vita-eabi-g++", "--version"], text=True).splitlines()[0]
        manifest["link_command"] = (output / "probe/CMakeFiles/canvas_state_probe.dir/link.txt").read_text().strip()
        manifest["status"] = "built"
        save()
    except Exception as error:
        manifest["status"] = "failed"
        manifest["error"] = str(error)
        save()
        raise
    print(f"Built {vpk}\nBuild ID: {build_id}\nOn-device pixel validation still required.")


if __name__ == "__main__":
    main()
