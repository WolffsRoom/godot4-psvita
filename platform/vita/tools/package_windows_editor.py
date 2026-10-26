"""Assemble the portable editor support files beside Godot-Vita.exe."""
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[4]
SOURCE = ROOT / "godot-master/platform/vita/editor/addons/psvita"
OUT = ROOT / "editor-vita-integrado"
SUPPORT = OUT / "vita"
SUPPORT.mkdir(parents=True, exist_ok=True)
for name in ("platform.gd", "defaults_inspector.gd", "trophy_tools.gd", "trophy_panel.gd"):
    text = (SOURCE / name).read_text(encoding="utf-8")
    text = text.replace("const TEMPLATE =", "static var TEMPLATE =")
    text = text.replace("const DEFAULT_DIR =", "static var DEFAULT_DIR =")
    text = re.sub(r'"res://addons/psvita/([^"\n]*)"',
                  lambda m: 'OS.get_executable_path().get_base_dir().path_join("vita/' + m[1] + '")', text)
    # Native defaults are resolved beside the exe each time, keeping presets portable.
    if name == "defaults_inspector.gd":
        text = text.replace("Preenche Release, Icon, Background e Startup com o template e as imagens padrão incluídas.",
                            "Restaura os padrões incluídos no editor. Campos vazios usam esses arquivos automaticamente.")
        text = text.replace('preset.set("custom_template/release", VitaPlatform.TEMPLATE)',
                            'preset.set("custom_template/release", "")')
        text = text.replace('preset.set(key, VitaPlatform.default_asset_path(key))', 'preset.set(key, "")')
    (SUPPORT / name).write_text(text, encoding="utf-8")
for name in ("defaults", "previews"):
    shutil.copytree(SOURCE / name, SUPPORT / name, dirs_exist_ok=True)
shutil.copy2(SOURCE / "iconGodotPSV.png", SUPPORT / "iconGodotPSV.png")
(SUPPORT / "templates").mkdir(exist_ok=True)
shutil.copy2(ROOT / "editor-vita/ProjetoVita/addons/psvita/templates/vita_release.zip",
             SUPPORT / "templates/vita_release.zip")
for source, destination in (("LICENSE.txt", "LICENSE-Godot.txt"), ("COPYRIGHT.txt", "COPYRIGHT.txt"), ("AUTHORS.md", "AUTHORS.md")):
    shutil.copy2(ROOT / "godot-master" / source, OUT / destination)
(OUT / "_sc_").touch()
print(OUT)
