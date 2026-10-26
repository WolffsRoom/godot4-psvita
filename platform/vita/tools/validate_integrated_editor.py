"""Check an actual VPK exported by the native Windows editor."""
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile
import xml.etree.ElementTree as ET
from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
OUT = ROOT / "editor-vita-integrado"
vpk = OUT / "Exports/Exemplo.vpk"
with zipfile.ZipFile(OUT / "vita/templates/vita_release.zip") as template, zipfile.ZipFile(vpk) as exported:
    assert exported.testzip() is None
    manifest = json.loads(template.read("vita-template.json"))
    for name in ("eboot.bin", "libgodot.so", "boot_anim.dat"):
        assert hashlib.sha256(exported.read(name)).hexdigest() == manifest["sha256"][name]
    for name, size in (("sce_sys/icon0.png", (128,128)), ("sce_sys/livearea/contents/bg0.png", (840,500)), ("sce_sys/livearea/contents/startup.png", (280,158))):
        raw = exported.read(name)
        assert raw[24:26] == bytes([8, 3])
        image = Image.open(io.BytesIO(raw))
        assert image.size == size
        image.verify()
    assert ET.fromstring(exported.read("sce_sys/livearea/contents/template.xml")).get("style") == "a5"
    assert exported.read("gxm_texture_format.cfg") == b"RGBA4444\n"
    assert manifest['external_game_data'] is True
    assert exported.read('game_data_folder.cfg') == b'exemplovita'
    assert not any(n.startswith('game_data/') for n in exported.namelist())
    game = OUT / 'Exports/exemplovita'
    assert (game/'project.binary').is_file()
    assert list(game.rglob('*.ctex'))
    assert not (game/'addons/psvita').exists()
    raw = exported.read("sce_sys/param.sfo")
    assert struct.unpack_from("<I", raw)[0] == 0x46535000
    assert b"GDOT40001" in raw
    key_table, data_table, count = struct.unpack_from("<3I", raw, 8)
    sfo_values = {}
    for index in range(count):
        key, kind, length, _capacity, offset = struct.unpack_from("<HHIII", raw, 20 + index * 16)
        name = raw[key_table + key:].split(b"\0", 1)[0].decode()
        value = raw[data_table + offset:data_table + offset + length]
        sfo_values[name] = struct.unpack("<I", value)[0] if kind == 0x404 else value.rstrip(b"\0").decode()
    assert sfo_values.get("ATTRIBUTE2") == 12
report = {
    "editor": "Godot4Vita.exe", "version": "4.8-dev7.vita_editor",
    "date": "2026-10-14", "native_export_without_project_addon": True,
    "vpk_crc": "passed", "runtime_hashes": "passed", "livearea": "a5",
    "assets": "indexed PNG8", "canonical_boot_animation": True,
    "gpu_texture_format": "RGBA4444", "gpu_texture_hardware_validation": "pending",
    "physical_vita_test": "not_performed",
    "external_game_data": True, "data_folder": "ux0:data/exemplovita",
    "export_contract_tests": json.loads((ROOT/"_build/split-export-results.json").read_text()),
    "editor_sha256": hashlib.sha256((OUT / "Godot4Vita.exe").read_bytes()).hexdigest(),
    "vpk_sha256": hashlib.sha256(vpk.read_bytes()).hexdigest(),
}
(OUT / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("PASS: native export VPK CRC, runtime hashes, PNG8, a5, boot animation and game resources")
