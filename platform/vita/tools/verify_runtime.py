#!/usr/bin/env python3
"""Validate the ELF contract supported by loader/so_util.c before packaging."""
import argparse
import json
from pathlib import Path
import re
import struct


def verify(runtime, imports_file):
    data = runtime.read_bytes()
    if data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("Expected little-endian ELF32")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    if header[1:3] != (3, 40):
        raise ValueError("Expected ARM ET_DYN shared object")
    if not header[7] & 0x400:
        raise ValueError("Runtime must use the hard-float ARM ABI")
    phoff, shoff = header[5], header[6]
    sections = [struct.unpack_from("<IIIIIIIIII", data, shoff + i * header[11]) for i in range(header[12])]
    def section_bytes(section):
        return data[section[4]:section[4] + section[5]]
    def string(table, offset):
        return table[offset:table.index(b"\0", offset)].decode("utf-8")
    names = section_bytes(sections[header[13]])
    named = {string(names, section[0]): section for section in sections}
    required = {".hash", ".dynsym", ".dynstr", ".dynamic", ".rel.dyn", ".rel.plt", ".init_array"}
    if required - named.keys():
        raise ValueError(f"Missing loader sections: {sorted(required - named.keys())}")
    loads = []
    for i in range(header[10]):
        segment = struct.unpack_from("<IIIIIIII", data, phoff + i * header[9])
        if segment[0] == 1:
            loads.append(segment)
    if len(loads) != 2 or loads[0][2] != 0 or loads[0][6] != 5 or loads[1][6] != 6:
        raise ValueError("Loader requires two PT_LOAD segments: RX at vaddr 0, then RW")
    strings = section_bytes(named[".dynstr"])
    symbol_data = section_bytes(named[".dynsym"])
    symbols = [struct.unpack_from("<IIIBBH", symbol_data, offset)
               for offset in range(0, named[".dynsym"][5], 16)]
    if not any(symbol[5] and string(strings, symbol[0]) == "main" for symbol in symbols):
        raise ValueError("Exported main entry is missing")
    imports = set()
    relocations = set()
    for section_name in (".rel.dyn", ".rel.plt"):
        reloc = section_bytes(named[section_name])
        for offset in range(0, len(reloc), 8):
            _, info = struct.unpack_from("<II", reloc, offset)
            kind, index = info & 255, info >> 8
            relocations.add(kind)
            if kind not in (2, 21, 22, 23):
                raise ValueError(f"Unsupported relocation type {kind}")
            symbol = symbols[index]
            if kind != 23 and symbol[5] == 0:
                imports.add(string(strings, symbol[0]))
    provided = set(re.findall(r'\{\s*"([^"\n]+)"\s*,', imports_file.read_text()))
    missing = imports - provided
    if missing:
        raise ValueError(f"Imports missing from loader: {sorted(missing)}")
    dynamic = section_bytes(named[".dynamic"])
    if any(struct.unpack_from("<II", dynamic, i)[0] == 1 for i in range(0, len(dynamic), 8)):
        raise ValueError("DT_NEEDED libraries are not packaged by this runtime build")
    return {"result": "PASS", "imports": sorted(imports), "relocation_types": sorted(relocations),
            "load_segments": [{"vaddr": s[2], "file_size": s[4], "memory_size": s[5], "flags": s[6]} for s in loads]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("--imports", type=Path, default=Path(__file__).resolve().parents[1] / "loader/default_dynlib.c")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    report = verify(args.runtime, args.imports)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: ELF layout, relocations, main and {len(report['imports'])} imports supported by the loader")
