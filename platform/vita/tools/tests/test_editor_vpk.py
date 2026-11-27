"""Independent acceptance checks for an editor-exported Vita example VPK."""
import argparse
import hashlib
import json
import re
from pathlib import Path, PurePosixPath
import struct
import zipfile


# LiveArea images and template.xml are replaced by the preset, so their
# runtime-template hashes no longer apply in the exported VPK.
# External-data and customizable-file checks adapted from
# WolffsRoom/godot4-psvita#9 by gtyire40-cpu to the v0.3 data layout.
CUSTOMIZABLE_TEMPLATE_FILES = {
    'sce_sys/icon0.png',
    'sce_sys/livearea/contents/bg0.png',
    'sce_sys/livearea/contents/startup.png',
    'sce_sys/livearea/contents/template.xml',
}
RESERVED_NAMES = {'CON', 'PRN', 'AUX', 'NUL', *(f'COM{i}' for i in range(1, 10)), *(f'LPT{i}' for i in range(1, 10))}


def read_sfo(data):
    magic, version, keys, values, count = struct.unpack_from('<5I', data)
    assert magic == 0x46535000 and version == 0x101
    result = {}
    for index in range(count):
        key, kind, length, capacity, offset = struct.unpack_from('<HHIII', data, 20 + index * 16)
        assert length <= capacity and offset % 4 == 0
        name = data[keys + key:].split(b'\0', 1)[0].decode()
        value = data[values + offset:values + offset + length]
        assert len(value) == length
        result[name] = struct.unpack('<I', value)[0] if kind == 0x404 else value.rstrip(b'\0').decode()
    return result


def read_external_data(vpk_path, folder):
    """v0.3 split export: <folder>/game.pck (PCK) or <folder>/game_data/ (loose)."""
    root = vpk_path.parent / folder
    assert root.is_dir() and not root.is_symlink(), f'External data folder missing: {root}'
    files = []
    for path in root.rglob('*'):
        assert not path.is_symlink(), f'Symlink in external data: {path}'
        if path.is_file():
            relative = path.relative_to(root).as_posix()
            assert ':' not in relative and '\\' not in relative
            assert '..' not in PurePosixPath(relative).parts
            files.append((relative, path))
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('vpk', type=Path)
    parser.add_argument('extracted_project', type=Path)
    args = parser.parse_args()
    assert not args.extracted_project.exists(), 'Use a fresh output folder'
    with zipfile.ZipFile(args.vpk) as archive:
        assert archive.testzip() is None
        names = archive.namelist()
        assert len(names) == len(set(names))
        for name in names:
            path = PurePosixPath(name)
            assert not path.is_absolute() and '..' not in path.parts and ':' not in name and '\\' not in name
            assert not name.startswith('game_data/addons/psvita/')
        manifest = json.loads(archive.read('vita-template.json'))
        for name, digest in manifest['sha256'].items():
            if name in CUSTOMIZABLE_TEMPLATE_FILES:
                continue
            assert hashlib.sha256(archive.read(name)).hexdigest() == digest, name
        assert archive.read('gxm2d.cfg') == b'native-canvas-v2\n'
        assert archive.read('capunlocker.cfg') in (b'disabled\n', b'optional\n', b'required\n')
        if 'shacc.cfg' in names:
            assert archive.read('shacc.cfg') in (b'disabled\n', b'safe\n', b'experimental\n')
        if 'pck_prefetch.cfg' in names:
            assert archive.read('pck_prefetch.cfg') in (b'enabled\n', b'disabled\n')
        sfo = read_sfo(archive.read('sce_sys/param.sfo'))
        assert sfo['TITLE_ID'] == 'GDOT40001'
        assert sfo['APP_VER'] == '01.00' and sfo['CATEGORY'] == 'gd'
        assert sfo['ATTRIBUTE2'] == 12, 'Extended 365 MiB USER_RW budget is missing'

        embedded = [name for name in names if name.startswith('game_data/') and not name.endswith('/')]
        external = []
        if 'game_data_folder.cfg' in names:
            folder = archive.read('game_data_folder.cfg').decode('utf-8')
            assert re.fullmatch(r'[A-Za-z0-9_-]{1,64}', folder) and folder.upper() not in RESERVED_NAMES, folder
            if 'game_data_owner.cfg' in names:
                assert archive.read('game_data_owner.cfg').decode('utf-8') == sfo['TITLE_ID']
            if not embedded:
                external = read_external_data(args.vpk, folder)
        assert embedded or external, 'VPK has no game data'
        assert not (embedded and external), 'Game data must not be both embedded and external'

        # Map everything to game_data/<relative> to check one layout.
        game = {}
        for name in embedded:
            game[name] = lambda name=name: archive.read(name)
        for relative, path in external:
            if relative == 'game.pck':
                game['game_data/game.pck'] = path.read_bytes
            elif relative.startswith('game_data/'):
                game[relative] = path.read_bytes
        if 'game_data/game.pck' in game:
            assert len(game['game_data/game.pck']()) > 0, 'PCK is empty'
            layout = 'PCK'
        else:
            assert 'game_data/project.binary' in game
            assert 'game_data/main.tscn.remap' in game
            assert any(name.endswith('.ctex') for name in game), 'No imported texture'
            layout = 'loose resources'
        for name, read in game.items():
            output = args.extracted_project / name.removeprefix('game_data/')
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(read())
    where = 'external' if external else 'embedded'
    print(f'PASS: ZIP CRC, unique safe paths, runtime hashes, SFO and {where} {layout} game data')


if __name__ == '__main__':
    main()
