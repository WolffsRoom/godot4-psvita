"""Archive only distributable files; exclude editor state and import caches."""
import hashlib
import argparse
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, default=ROOT / "editor-vita-integrado")
source = parser.parse_args().source.resolve()
release = ROOT / "releases"
release.mkdir(exist_ok=True)
name = "Godot-Vita-4.8-dev7-Windows-x64"
destination = release / (name + ".zip")
files = [source / n for n in ("Godot4Vita.exe", "LEIA-ME.md", "TROFEUS.md", "RGBA4444.md", "LICENSE-Godot.txt", "COPYRIGHT.txt", "AUTHORS.md", "_sc_", "validation.json")]
for directory in ("vita", "Exemplo", "Exports"):
    for path in (source / directory).rglob("*"):
        if path.is_file() and not any(".previous-" in part or ".partial-" in part for part in path.parts) and (directory == "Exports" or (".godot" not in path.parts and path.suffix not in (".import", ".uid"))):
            files.append(path)
with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in sorted(files):
        archive.write(path, name + "/" + path.relative_to(source).as_posix())
with zipfile.ZipFile(destination) as archive:
    assert archive.testzip() is None
    assert not any("editor_data/" in n or "/Exemplo/.godot/" in n for n in archive.namelist())
digest = hashlib.sha256(destination.read_bytes()).hexdigest()
destination.with_suffix(".zip.sha256").write_text(digest + "  " + destination.name + "\n")
print(destination)
print(digest)
