"""Assemble a portable Linux editor distribution with the Vita export support files."""
import argparse
import os
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[3]
ADDON = ROOT / "platform/vita/editor/addons/psvita"
DEFAULT_OUTPUT = Path(os.environ.get("VITA_EDITOR_OUTPUT_DIR", ROOT.parent / "editor-vita-linux"))


def portable_script(source: Path, destination: Path) -> None:
    text = source.read_text(encoding="utf-8")
    text = text.replace("const TEMPLATE =", "static var TEMPLATE =")
    text = text.replace("const DEFAULT_DIR =", "static var DEFAULT_DIR =")
    text = re.sub(
        r'"res://addons/psvita/([^"\n]*)"',
        lambda match: 'OS.get_executable_path().get_base_dir().path_join("vita/'
        + match[1]
        + '")',
        text,
    )
    if source.name == "defaults_inspector.gd":
        text = text.replace(
            "Preenche Release, Icon, Background e Startup com o template e as imagens padrão incluídas.",
            "Restaura os padrões incluídos no editor. Campos vazios usam esses arquivos automaticamente.",
        )
        text = text.replace(
            'preset.set("custom_template/release", VitaPlatform.TEMPLATE)',
            'preset.set("custom_template/release", "")',
        )
        text = text.replace(
            'preset.set(key, VitaPlatform.default_asset_path(key))',
            'preset.set(key, "")',
        )
    destination.write_text(text, encoding="utf-8", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--template",
        type=Path,
        required=True,
        help="Validated vita_release.zip produced by make_editor_template.py",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=DEFAULT_OUTPUT / "Godot4Vita",
        help="Linux editor binary produced by build_linux_editor.sh",
    )
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    binary = args.binary.resolve()
    template = args.template.resolve()
    output = args.output.resolve()
    for path, label in ((binary, "Linux editor binary"), (template, "Vita runtime template")):
        if not path.is_file():
            parser.error(f"{label} not found: {path}")
    if not (ADDON / "platform.gd").is_file():
        parser.error(f"Vita export support files not found: {ADDON}")

    output.mkdir(parents=True, exist_ok=True)
    editor_binary = output / "Godot4Vita"
    if binary != editor_binary.resolve():
        shutil.copy2(binary, editor_binary)
    support = output / "vita"
    support.mkdir(exist_ok=True)
    for name in ("platform.gd", "defaults_inspector.gd", "trophy_tools.gd", "trophy_panel.gd"):
        portable_script(ADDON / name, support / name)
    for name in ("defaults", "previews"):
        shutil.copytree(ADDON / name, support / name, dirs_exist_ok=True)
    shutil.copy2(ADDON / "iconGodotPSV.png", support / "iconGodotPSV.png")
    templates = support / "templates"
    templates.mkdir(exist_ok=True)
    shutil.copy2(template, templates / "vita_release.zip")

    for source, destination in (
        (ROOT / "LICENSE.txt", "LICENSE-Godot.txt"),
        (ROOT / "COPYRIGHT.txt", "COPYRIGHT.txt"),
        (ROOT / "AUTHORS.md", "AUTHORS.md"),
        (ROOT / "platform/vita/editor/INTEGRATED_LINUX.md", "README.md"),
        (ROOT / "platform/vita/editor/TROFEUS.md", "TROFEUS.md"),
        (ROOT / "platform/vita/editor/RGBA4444.md", "RGBA4444.md"),
    ):
        if not source.is_file():
            parser.error(f"Required distribution file not found: {source}")
        shutil.copy2(source, output / destination)
    shutil.copytree(
        ROOT / "examples/vita_export/project",
        output / "Exemplo",
        dirs_exist_ok=True,
        ignore=shutil.ignore_patterns(".godot"),
    )
    (output / "_sc_").touch()
    print(output)


if __name__ == "__main__":
    main()
