"""Independent acceptance checks for an editor-exported Vita example VPK."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import zipfile


CUSTOMIZABLE_TEMPLATE_FILES = {
    'sce_sys/icon0.png',
    'sce_sys/livearea/contents/bg0.png',
    'sce_sys/livearea/contents/startup.png',
    'sce_sys/livearea/contents/template.xml',
}


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


def read_external_game_data(vpk_path, folder, title_id):
    assert re.fullmatch(r"[A-Za-z0-9_-]{1,64}", folder), "Unsafe external data folder name"
    assert folder.upper() not in {
        "CON", "PRN", "AUX", "NUL",
        *(f"COM{index}" for index in range(1, 10)),
        *(f"LPT{index}" for index in range(1, 10)),
    }, "Reserved external data folder name"
    root = vpk_path.parent / folder
    assert not root.is_symlink(), "External data folder must not be a symlink"
    assert (root / ".vita-export-owner").read_bytes() == title_id.encode(), "External data owner mismatch"
    game_root = root / "game_data"
    assert game_root.is_dir() and not game_root.is_symlink(), "External game data is missing"

    files = []
    for path in game_root.rglob("*"):
        assert not path.is_symlink(), f"Symlink in external game data: {path}"
        if path.is_file():
            relative = path.relative_to(root).as_posix()
            safe_path = PurePosixPath(relative)
            assert not safe_path.is_absolute() and ".." not in safe_path.parts
            assert ":" not in relative and "\\" not in relative
            files.append((relative, path))
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('vpk', type=Path)
    parser.add_argument('extracted_project', type=Path)
    args = parser.parse_args()
    assert not args.extracted_project.exists(), 'Use a fresh output folder'
    external_game_files = []
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
        sfo = read_sfo(archive.read('sce_sys/param.sfo'))
        assert sfo['TITLE'] == 'Godot Vita Game'
        assert sfo['TITLE_ID'] == 'GDOT40001'
        assert sfo['APP_VER'] == '01.00' and sfo['CATEGORY'] == 'gd'
        assert sfo['ATTRIBUTE2'] == 12, 'Extended 365 MiB USER_RW budget is missing'
        embedded_game_files = [
            name for name in names if name.startswith('game_data/') and not name.endswith('/')
        ]
        if 'game_data_folder.cfg' in names:
            assert not embedded_game_files, 'Game data must not be both embedded and external'
            folder = archive.read('game_data_folder.cfg').decode('utf-8')
            external_game_files = read_external_game_data(args.vpk, folder, sfo['TITLE_ID'])
        else:
            assert embedded_game_files, 'VPK has no game data'

        game_names = set(embedded_game_files)
        game_names.update(relative for relative, _path in external_game_files)
        if 'game_data/game.pck' in game_names:
            if external_game_files:
                pck = next(path for relative, path in external_game_files if relative == 'game_data/game.pck')
                assert pck.stat().st_size > 0, 'External PCK is empty'
            else:
                assert len(archive.read('game_data/game.pck')) > 0, 'Embedded PCK is empty'
            layout = 'PCK'
        else:
            assert 'game_data/project.binary' in game_names
            assert 'game_data/main.tscn.remap' in game_names
            assert any(name.endswith('.ctex') for name in game_names), 'No imported texture'
            layout = 'loose resources'

        for name in embedded_game_files:
            output = args.extracted_project / name.removeprefix('game_data/')
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(archive.read(name))
    for relative, source in external_game_files:
        output = args.extracted_project / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(source.read_bytes())
    print(f'PASS: ZIP CRC, unique safe paths, runtime hashes, SFO and {layout} game data')


if __name__ == '__main__':
    main()
