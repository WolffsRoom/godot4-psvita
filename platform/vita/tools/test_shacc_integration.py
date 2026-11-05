#!/usr/bin/env python3
# Godot4Vita v0.3 export integration test: no Split Memories dependencies.
# Usage: python test_shacc_integration.py EDITOR TEMPLATE OUTPUT_FOLDER
import hashlib
import json
import subprocess
import sys
import zipfile
from pathlib import Path

editor, template, output = map(Path, sys.argv[1:4])
output.mkdir(parents=True, exist_ok=True)
proj = output / "project"
proj.mkdir(exist_ok=True)
(proj/"project.godot").write_text('''config_version=5
[application]
config/name="Shader Test"
run/main_scene="res://main.tscn"
[display]
window/size/viewport_width=960
window/size/viewport_height=544
[rendering]
renderer/rendering_method="gl_compatibility"
renderer/rendering_method.mobile="gl_compatibility"
[threading]
worker_pool/max_threads=2
''', encoding="utf-8")
(proj/"main.gdshader").write_text('''shader_type canvas_item;
uniform float intensity = 0.75;
void fragment() { COLOR = vec4(UV.x, UV.y, intensity, 1.0); }
''', encoding="utf-8")
(proj/"main.tscn").write_text('''[gd_scene load_steps=3 format=3]
[ext_resource type="Shader" path="res://main.gdshader" id="1"]
[sub_resource type="ShaderMaterial" id="Mat_1"]
shader = ExtResource("1")
[node name="Main" type="Control"]
layout_mode = 3
anchors_preset = 15
anchor_right = 1.0
anchor_bottom = 1.0
[node name="FX" type="ColorRect" parent="."]
material = SubResource("Mat_1")
layout_mode = 1
anchors_preset = 15
anchor_right = 1.0
anchor_bottom = 1.0
''', encoding="utf-8")
with zipfile.ZipFile(template) as z:
    manifest = json.loads(z.read("vita-template.json"))
    assert z.testzip() is None and manifest["runtime_shacc"] == "v1"
    assert manifest["external_game_data"] is True
run = subprocess.run([str(editor),"--headless","--editor","--path",str(proj),"--quit"],
                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,timeout=110)
assert run.returncode == 0, run.stdout.decode(errors="replace")[-1100:]
results = []
for mode, separate in (("disabled", False), ("safe", False), ("safe", True), ("experimental", False)):
    dest = output / (mode+("-separate" if separate else "-embedded"))
    dest.mkdir(exist_ok=True)
    preset = '''[preset.0]
name="PS Vita"
platform="PS Vita"
runnable=true
advanced_options=false
dedicated_server=false
custom_features=""
export_filter="all_resources"
include_filter=""
exclude_filter=""
export_path="../out/Test.vpk"
patches=PackedStringArray()
encryption_include_filters=""
encryption_exclude_filters=""
encrypt_pck=false
encrypt_directory=false
script_export_mode=0
[preset.0.options]
data/separate_folder=%s
data/package_format="PCK"
data/folder_name="shacctest"
performance/capunlocker="optional"
performance/pck_prefetch=true
textures/gpu_format="RGBA4444"
rendering/shader_compiler="%s"
livearea/style="a5"
application/title="Shader Smoke"
application/title_id="GDST00001"
application/version="01.00"
custom_template/release="%s"
''' % (str(separate).lower(), mode, str(template.resolve()).replace("\\","/"))
    (proj/"export_presets.cfg").write_text(preset,encoding="utf-8")
    vpk = dest / "Test.vpk"
    done = subprocess.run([str(editor),"--headless","--path",str(proj),
                          "--export-release","PS Vita",str(vpk)],
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=140)
    text = done.stdout.decode(errors="replace")
    (dest/"export.log").write_text(text,encoding="utf-8")
    assert done.returncode == 0, (mode,separate,text[-1800:])
    with zipfile.ZipFile(vpk) as z:
        assert z.testzip() is None
        assert z.read("shacc.cfg") == (mode+"\n").encode()
        assert z.read("game_data_folder.cfg")==b"shacctest"
        assert all(not x.lower().endswith(".suprx") for x in z.namelist())
        if separate:
            assert "game_data/game.pck" not in z.namelist()
            pck = dest/"shacctest/game.pck"
            assert pck.is_file() and pck.read_bytes()[:4] == b"GDPC"
        else:
            assert "game_data/game.pck" in z.namelist()
    results.append(dict(mode=mode,separate=separate,vpk_bytes=vpk.stat().st_size,
                        sha256=hashlib.sha256(vpk.read_bytes()).hexdigest()))
(output/"shacc-export-results.json").write_text(json.dumps(results,indent=2),encoding="utf-8")
print(json.dumps(results,indent=2))
