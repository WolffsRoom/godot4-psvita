"""Independent acceptance checks for an editor-exported Vita example VPK."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import zipfile


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
            assert hashlib.sha256(archive.read(name)).hexdigest() == digest, name
        assert archive.read('gxm2d.cfg') == b'native-canvas-v2\n'
        assert archive.read('capunlocker.cfg') in (b'disabled\n', b'optional\n', b'required\n')
        sfo = read_sfo(archive.read('sce_sys/param.sfo'))
        assert sfo['TITLE'] == 'Godot Vita Game'
        assert sfo['TITLE_ID'] == 'GDOT40001'
        assert sfo['APP_VER'] == '01.00' and sfo['CATEGORY'] == 'gd'
        assert sfo['ATTRIBUTE2'] == 12, 'Extended 365 MiB USER_RW budget is missing'
        assert 'game_data/project.binary' in names
        assert 'game_data/main.tscn.remap' in names
        assert any(name.endswith('.ctex') for name in names), 'No imported texture'
        for name in names:
            if name.startswith('game_data/') and not name.endswith('/'):
                output = args.extracted_project / name.removeprefix('game_data/')
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(archive.read(name))
    print('PASS: ZIP CRC, unique safe paths, runtime hashes, SFO, scene remap and imported texture')


if __name__ == '__main__':
    main()
