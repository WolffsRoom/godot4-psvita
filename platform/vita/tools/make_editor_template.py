"""Convert a matching generic loader/runtime VPK to a portable editor template.

Never use a game-specific loader with hardcoded branding or dependencies here.
Game files are deliberately excluded; the editor exports its current project.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import zlib


def png(width, height):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    # Small original geometric handheld icon/background; no game artwork.
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            inside = width // 5 < x < width * 4 // 5 and height // 3 < y < height * 2 // 3
            rows.extend((230, 243, 255, 255) if inside else (24, 67, 94, 255))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('vpk', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--rgba4444', action='store_true', help='Loader was built with the optional RGBA4444 upload path')
    args = parser.parse_args()
    with zipfile.ZipFile(args.vpk) as source:
        assert source.testzip() is None, 'Corrupt input VPK'
        # Generic native Godot Vita splash is part of the export template.
        # Refuse game-specific replacement animations to keep exports generic.
        if 'boot_anim.dat' in source.namelist():
            canonical = (Path(__file__).resolve().parents[1] /
                         "loader/boot_assets/custom_splash_960x544.dat").read_bytes()
            assert source.read('boot_anim.dat') == canonical, (
                'Export template requires the canonical Godot Vita boot animation'
            )
        names = ['eboot.bin', 'libgodot.so', 'gxm2d.cfg']
        if 'gxm3d.cfg' in source.namelist():
            names.append('gxm3d.cfg')
        if 'boot_anim.dat' in source.namelist():
            names.append('boot_anim.dat')
        names += [name for name in source.namelist() if name.startswith('licenses/')]
        files = {name: source.read(name) for name in names}
    assert files['libgodot.so'][:4] == b'\x7fELF'
    files['licenses/Godot-LICENSE.txt'] = (Path(__file__).resolve().parents[3] / 'LICENSE.txt').read_bytes()
    files['sce_sys/icon0.png'] = png(128, 128)
    files['sce_sys/livearea/contents/bg0.png'] = png(840, 500)
    files['sce_sys/livearea/contents/startup.png'] = png(280, 158)
    files['sce_sys/livearea/contents/template.xml'] = b'''<?xml version="1.0" encoding="utf-8"?>
<livearea style="a5" format-ver="01.00" content-rev="1">
  <livearea-background><image>bg0.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>'''
    manifest = {'format': 1, 'godot': '4.8-dev7', 'backend': 'gxm3d-experimental' if 'gxm3d.cfg' in files else 'gxm2d',
                'external_game_data': b'app0:game_data_folder.cfg\x00' in files['libgodot.so'],
                'texture_formats': ['RGBA8888', 'RGBA4444'] if args.rgba4444 else ['RGBA8888'],
                'source_vpk_sha256': hashlib.sha256(args.vpk.read_bytes()).hexdigest(),
                'sha256': {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}}
    files['vita-template.json'] = json.dumps(manifest, indent=2).encode()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED) as target:
        for name, data in files.items():
            target.writestr(name, data)
    print(args.output)


if __name__ == '__main__':
    main()
