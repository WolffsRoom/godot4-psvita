#!/usr/bin/env python3
"""Create a portable Godot4Vita editor ZIP for Windows, Linux or macOS."""

from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import shutil
import stat
import tempfile
import zipfile

VERSION = "v0.3"
GUIDES = (
    "LEIA-ME.md",
    "TROFEUS.md",
    "RGBA4444.md",
    "LICENSE-Godot.txt",
    "COPYRIGHT.txt",
    "AUTHORS.md",
)


def copy_support(source: Path, destination: Path) -> None:
    template = source / "vita" / "templates" / "vita_release.zip"
    if not template.is_file():
        raise FileNotFoundError(f"Missing required Vita template: {template}")
    shutil.copytree(source / "vita", destination / "vita", dirs_exist_ok=True)
    for name in GUIDES:
        candidate = source / name
        if candidate.is_file():
            shutil.copy2(candidate, destination / name)
    for directory in ("Exemplo", "Exports"):
        candidate = source / directory
        if candidate.is_dir():
            shutil.copytree(
                candidate,
                destination / directory,
                dirs_exist_ok=True,
                ignore=shutil.ignore_patterns(".godot", "*.import", "*.uid", "*.partial-*", "*.previous-*"),
            )


def archive_tree(staging: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(staging.rglob("*")):
            if path.is_file():
                relative = path.relative_to(staging).as_posix()
                info = zipfile.ZipInfo.from_file(path, relative)
                if path.name == "Godot4Vita":
                    # Packaging may run on Windows, whose filesystem does not
                    # carry the executable bit required after Linux extraction.
                    info.create_system = 3
                    info.external_attr = (stat.S_IFREG | 0o755) << 16
                with path.open("rb") as source:
                    archive.writestr(info, source.read(), zipfile.ZIP_DEFLATED, compresslevel=6)
    with zipfile.ZipFile(destination) as archive:
        if archive.testzip() is not None:
            raise RuntimeError(f"Corrupt archive produced: {destination}")
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_suffix(destination.suffix + ".sha256").write_text(
        f"{digest}  {destination.name}\n", encoding="utf-8"
    )


def package_distribution(platform: str, arch: str, payload: Path, support: Path, destination: Path) -> None:
    labels = {"windows": "Windows", "linux": "Linux", "macos": "macOS"}
    if platform not in labels:
        raise ValueError(f"Unsupported host platform: {platform}")
    if not payload.exists():
        raise FileNotFoundError(payload)
    root_name = f"Godot4Vita-{VERSION}-{labels[platform]}-{arch}"
    with tempfile.TemporaryDirectory(prefix="godot4vita-editor-") as temporary:
        staging = Path(temporary)
        root = staging / root_name
        if platform == "macos":
            if payload.suffix != ".app" or not payload.is_dir():
                raise ValueError("macOS payload must be a Godot4Vita.app bundle")
            app = root / "Godot4Vita.app"
            shutil.copytree(payload, app)
            copy_support(support, app / "Contents" / "MacOS")
            resources = app / "Contents" / "Resources"
            resources.mkdir(parents=True, exist_ok=True)
            for name in GUIDES:
                source = support / name
                if source.is_file():
                    shutil.copy2(source, resources / name)
        else:
            root.mkdir(parents=True)
            executable_name = "Godot4Vita.exe" if platform == "windows" else "Godot4Vita"
            shutil.copy2(payload, root / executable_name)
            copy_support(support, root)
        archive_tree(staging, destination)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True, choices=("windows", "linux", "macos"))
    parser.add_argument("--arch", required=True)
    parser.add_argument("--payload", required=True, type=Path)
    parser.add_argument("--support", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    package_distribution(args.platform, args.arch, args.payload.resolve(), args.support.resolve(), args.output.resolve())
    print(args.output.resolve())


if __name__ == "__main__":
    main()
