#!/usr/bin/env python3
"""Build an identified UBO probe from a clean, pinned VitaGL snapshot (Linux/WSL).

Never modifies the supplied Git checkout or installs libraries in VitaSDK.
Each output directory must be new; failed builds retain their log for inspection.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
from datetime import datetime, timezone


PLATFORM = Path(__file__).resolve().parents[1]
ENGINE = PLATFORM.parents[1]


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vitagl-repo", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path, help="New build directory")
    parser.add_argument("--vitasdk", type=Path, default=os.environ.get("VITASDK", "/usr/local/vitasdk"))
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    repo, output, sdk = args.vitagl_repo.resolve(), args.output.resolve(), args.vitasdk.resolve()
    for tool in ("git", "make", "cmake", "pkg-config"):
        if not shutil.which(tool):
            parser.error(f"Missing tool: {tool}")
    if not (sdk / "bin/arm-vita-eabi-gcc").is_file():
        parser.error(f"VitaSDK compiler not found in {sdk}")
    lock_path = PLATFORM / "vitagl/dependency.json"
    lock = json.loads(lock_path.read_text())
    commit = lock["commit"]
    # Resolve the exact object before creating output; no fetching or mutable refs.
    actual = subprocess.check_output(["git", "-C", str(repo), "rev-parse", f"{commit}^{{commit}}"], text=True).strip()
    if actual != commit:
        parser.error("VitaGL commit does not match dependency.json")
    output.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env["VITASDK"] = str(sdk)
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    env.pop("MAKEFLAGS", None)
    env.pop("MFLAGS", None)
    source = output / "vitagl"
    source.mkdir()
    build_id = "ubo-" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    manifest = {
        "build_id": build_id,
        "status": "building",
        "hardware_result": "not_run",
        "vitagl_commit": commit,
        "make_options": lock["make_options"],
        "vitasdk": str(sdk),
        "engine_commit": subprocess.check_output(["git", "-C", str(ENGINE), "rev-parse", "HEAD"], text=True).strip(),
        "inputs_sha256": {},
    }
    manifest_path = output / "manifest.json"

    def save_manifest():
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")

    save_manifest()
    with (output / "build.log").open("w") as log:
        def run(command, cwd=output):
            log.write("\n$ " + repr([str(arg) for arg in command]) + "\n")
            log.flush()
            subprocess.run(command, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)

        try:
            archive = output / "vitagl-base.tar"
            run(["git", "-C", repo, "archive", "--format=tar", f"--output={archive}", commit])
            with tarfile.open(archive) as tar:
                tar.extractall(source, filter="data")
            # Normalize Windows checkout line endings without changing source semantics.
            patch = PLATFORM / "vitagl" / lock["patch"]
            normalized = output / "vitagl-applied.patch"
            normalized.write_text(patch.read_text(), newline="\n")
            run(["git", "apply", "--check", normalized], cwd=source)
            run(["git", "apply", normalized], cwd=source)
            run(["python3", PLATFORM / "tools/tests/test_vitagl_ubo_binding.py", source])
            run(["make", f"-j{args.jobs}", f"VGL_GIT_HASH={commit[:12]}",
                 *[f"{key}={value}" for key, value in lock["make_options"].items()]], cwd=source)
            library = source / "libvitaGL.a"
            manifest["vitagl_sha256"] = sha256(library)
            probe_source = PLATFORM / "tools/ubo_binding_probe"
            probe_build = output / "probe"
            run(["cmake", "-S", probe_source, "-B", probe_build,
                 f"-DVITA_GL_ROOT={source}", f"-DVITA_PROBE_BUILD_ID={build_id}"])
            run(["cmake", "--build", probe_build, "--parallel", str(args.jobs)])
            vpk = output / "ubo_binding_probe.vpk"
            shutil.copy2(probe_build / "ubo_binding_probe.vpk", vpk)
            manifest["vpk_sha256"] = sha256(vpk)
            manifest["compiler"] = subprocess.check_output([str(sdk / "bin/arm-vita-eabi-gcc"), "--version"], text=True).splitlines()[0]
            manifest["cmake"] = subprocess.check_output(["cmake", "--version"], text=True).splitlines()[0]
            inputs = [lock_path, patch, Path(__file__), PLATFORM / "cmake/VitaGL.cmake",
                      PLATFORM / "tools/tests/test_vitagl_ubo_binding.py", *probe_source.glob("*.c"),
                      probe_source / "CMakeLists.txt"]
            manifest["inputs_sha256"] = {str(p.relative_to(ENGINE)): sha256(p) for p in inputs}
            manifest["vitagl_header_sha256"] = sha256(source / "source/vitaGL.h")
            link_file = probe_build / "CMakeFiles/ubo_binding_probe.dir/link.txt"
            link = link_file.read_text()
            manifest["link_command"] = link.strip()
            manifest["sdk_libraries_sha256"] = {}
            for name in sorted(set(re.findall(r"(?:^|\s)-l([\w+.-]+)", link))):
                path = sdk / "arm-vita-eabi/lib" / f"lib{name}.a"
                if path.is_file():
                    manifest["sdk_libraries_sha256"][name] = sha256(path)
            manifest["status"] = "built"
            save_manifest()
        except Exception as error:
            manifest["status"] = "failed"
            manifest["error"] = str(error)
            save_manifest()
            raise
    print(f"Built {vpk}\nManifest: {manifest_path}\nHardware validation still required.")


if __name__ == "__main__":
    main()
