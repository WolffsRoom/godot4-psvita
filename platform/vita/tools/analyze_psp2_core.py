#!/usr/bin/env python3
"""Print the crash thread, registers and loaded module ranges from a Vita core."""

from __future__ import annotations

import argparse
import gzip
import struct
from pathlib import Path


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def notes(data: bytes):
    phoff = u32(data, 28)
    phentsize, phnum = struct.unpack_from("<HH", data, 42)
    for index in range(phnum):
        base = phoff + index * phentsize
        kind, offset, _, _, size = struct.unpack_from("<IIIII", data, base)
        if kind != 4:  # PT_NOTE
            continue
        cursor, end = offset, offset + size
        while cursor + 12 <= end:
            namesz, descsz, note_type = struct.unpack_from("<III", data, cursor)
            cursor += 12
            name = data[cursor : cursor + namesz].rstrip(b"\0").decode("ascii", "replace")
            cursor = (cursor + namesz + 3) & ~3
            desc = data[cursor : cursor + descsz]
            cursor = (cursor + descsz + 3) & ~3
            yield note_type, name, desc


def parse_threads(desc: bytes):
    count = u32(desc, 4)
    cursor = 8
    result = []
    for _ in range(count):
        size = u32(desc, cursor)
        thread_id = u32(desc, cursor + 4)
        name = desc[cursor + 8 : cursor + 40].split(b"\0", 1)[0].decode("ascii", "replace")
        stop_reason = u32(desc, cursor + 0x74)
        pc = u32(desc, cursor + 0x9C)
        result.append((thread_id, name, stop_reason, pc))
        cursor += size or 0xC8
    return result


def parse_registers(desc: bytes):
    count = u32(desc, 4)
    cursor = 8
    result = {}
    for _ in range(count):
        size = u32(desc, cursor)
        thread_id = u32(desc, cursor + 4)
        regs = struct.unpack_from("<16I", desc, cursor + 8)
        dfsr = u32(desc, cursor + 0x170)
        dfar = u32(desc, cursor + 0x174)
        result[thread_id] = (regs, dfsr, dfar)
        cursor += size or 0x178
    return result


def parse_modules(desc: bytes):
    count = u32(desc, 4)
    cursor = 8
    result = []
    for _ in range(count):
        size = u32(desc, cursor)
        name = desc[cursor + 0x24 : cursor + 0x44].split(b"\0", 1)[0].decode("ascii", "replace")
        segment_count = u32(desc, cursor + 0x4C)
        segments = []
        seg_cursor = cursor + 0x50
        for _ in range(segment_count):
            seg_size, attributes, base, mem_size, alignment = struct.unpack_from("<IiIII", desc, seg_cursor)
            segments.append((base, mem_size, attributes))
            seg_cursor += seg_size or 20
        result.append((name, segments))
        cursor += size or (seg_cursor - cursor + 16)
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("core", type=Path)
    args = parser.parse_args()
    packed = args.core.read_bytes()
    data = gzip.decompress(packed) if packed[:2] == b"\x1f\x8b" else packed
    parsed = {kind: desc for kind, _, desc in notes(data)}
    threads = parse_threads(parsed[0x1003])
    registers = parse_registers(parsed[0x1004])
    modules = parse_modules(parsed[0x1005])
    print(f"core={args.core} packed={len(packed)} raw={len(data)}")
    for thread_id, name, stop_reason, pc in threads:
        if stop_reason == 0:
            continue
        regs, dfsr, dfar = registers[thread_id]
        print(f"crash thread=0x{thread_id:08x} name={name!r} stop=0x{stop_reason:08x}")
        print(" ".join(f"r{i}=0x{value:08x}" for i, value in enumerate(regs[:13])))
        print(f"sp=0x{regs[13]:08x} lr=0x{regs[14]:08x} pc=0x{regs[15]:08x} info_pc=0x{pc:08x}")
        print(f"DFSR=0x{dfsr:08x} DFAR=0x{dfar:08x}")
        for module_name, segments in modules:
            for base, size, attributes in segments:
                if base <= regs[15] < base + size:
                    print(f"module={module_name} base=0x{base:08x} offset=0x{regs[15]-base:08x} size=0x{size:x} attrs=0x{attributes:x}")
    print("modules:")
    for name, segments in modules:
        ranges = ", ".join(f"0x{base:08x}+0x{size:x}" for base, size, _ in segments)
        print(f"  {name}: {ranges}")


if __name__ == "__main__":
    main()
