"""Create a Linux-native tarball while preserving the editor executable mode."""
import argparse
import hashlib
import os
from pathlib import Path
import tarfile

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SOURCE = Path(os.environ.get("VITA_EDITOR_OUTPUT_DIR", ROOT.parent / "editor-vita-linux"))


def archive_filter(member: tarfile.TarInfo) -> tarfile.TarInfo | None:
    if any(part in {".godot", "editor_data", "Exports", "logs"} for part in Path(member.name).parts):
        return None
    if any(".previous-" in part or ".partial-" in part for part in Path(member.name).parts):
        return None
    return member


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_SOURCE.parent / "Godot-Vita-4.8-dev7-Linux-x86_64.tar.gz",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    output = args.output.resolve()
    if source == output or source in output.parents:
        parser.error("Archive output must be outside the distribution directory")
    for required in ("Godot4Vita", "_sc_", "vita/platform.gd", "vita/templates/vita_release.zip"):
        if not (source / required).is_file():
            parser.error(f"Required distribution file not found: {source / required}")
    if not (source / "Godot4Vita").stat().st_mode & 0o111:
        parser.error(f"Editor binary is not executable: {source / 'Godot4Vita'}")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(output, "w:gz") as archive:
        archive.add(source, arcname=source.name, filter=archive_filter)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(output.suffix + ".sha256").write_text(
        f"{digest}  {output.name}\n", encoding="ascii"
    )
    print(output)
    print(digest)


if __name__ == "__main__":
    main()
