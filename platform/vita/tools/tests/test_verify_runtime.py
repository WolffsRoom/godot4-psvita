#!/usr/bin/env python3
"""Exercise packaging rejection paths using a known compatible runtime ELF."""
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_runtime import verify


class Binary:
    def __init__(self, data):
        self.data = data

    def read_bytes(self):
        return self.data


class Text:
    def __init__(self, data):
        self.data = data

    def read_text(self):
        return self.data


def rejects(binary, imports, expected):
    try:
        verify(binary, imports)
    except ValueError as error:
        assert expected in str(error), str(error)
        return
    raise AssertionError(f"Validator accepted invalid input: {expected}")


def main():
    data = Path(sys.argv[1]).read_bytes()
    imports = Path(__file__).resolve().parents[2] / "loader/default_dynlib.c"
    report = verify(Binary(data), imports)
    missing = report["imports"][0]
    table = imports.read_text().replace(f'"{missing}"', '"deliberately_missing"')
    rejects(Binary(data), Text(table), "Imports missing from loader")
    invalid_machine = bytearray(data)
    struct.pack_into("<H", invalid_machine, 18, 62)  # x86-64, not ARM.
    rejects(Binary(bytes(invalid_machine)), imports, "ARM ET_DYN")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    sections = [struct.unpack_from("<IIIIIIIIII", data, header[6] + i * header[11]) for i in range(header[12])]
    # First nonempty SHT_REL section. R_ARM_COPY is unsupported by so_util.c.
    relocation_section = next(section for section in sections if section[1] == 9 and section[5])
    offset = relocation_section[4] + 4
    invalid_relocation = bytearray(data)
    info = struct.unpack_from("<I", data, offset)[0]
    struct.pack_into("<I", invalid_relocation, offset, (info & ~255) | 20)
    rejects(Binary(bytes(invalid_relocation)), imports, "Unsupported relocation type 20")
    print("PASS: compatible runtime accepted; missing import, wrong architecture and unsupported relocation rejected")


if __name__ == "__main__":
    main()
