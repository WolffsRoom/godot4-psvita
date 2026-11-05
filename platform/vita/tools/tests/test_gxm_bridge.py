#!/usr/bin/env python3
"""Check the vita_gxm_* contract between the engine, both loader bridges and the dynlib table.

libgodot.so imports every function declared in platform/vita/gxm_bridge.h from
the loader. A declaration without a definition in a bridge, or without an entry
in loader/default_dynlib.c, makes the loader fail to link or abort at boot.

The previous version of this test drove an early libvita2d-only bridge through
a host fake. The bridge now talks to sceGxm directly (shader patcher, render
targets, readback), so that fake no longer compiled; the behaviour tests that
still apply live in test_texture_format.c and test_gxm_bridge_vgl.py.
"""
from pathlib import Path
import re
import sys

platform = Path(__file__).resolve().parents[2]
header = (platform / 'gxm_bridge.h').read_text(encoding='utf-8')
declared = sorted(set(re.findall(r'\b(vita_gxm_\w+)\s*\(', header)))
assert declared, 'no vita_gxm_* declarations found in gxm_bridge.h'

definition = re.compile(r'^[A-Za-z_][\w \t\*]*?\b(vita_gxm_\w+)\s*\([^;{]*\)\s*\{', re.M)


def defined_in(path):
    return set(definition.findall((platform / path).read_text(encoding='utf-8')))


bridges = {
    'loader/gxm_bridge.c': defined_in('loader/gxm_bridge.c'),
    'loader/gxm_bridge_vgl.c': defined_in('loader/gxm_bridge_vgl.c'),
}
dynlib = set(re.findall(r'"(vita_gxm_\w+)"', (platform / 'loader/default_dynlib.c').read_text(encoding='utf-8')))

errors = []
for name in declared:
    for path, names in bridges.items():
        if name not in names:
            errors.append(f'{name} is declared in gxm_bridge.h but not defined in {path}')
    if name not in dynlib:
        errors.append(f'{name} is declared in gxm_bridge.h but missing from loader/default_dynlib.c')
for name in sorted(dynlib - set(declared)):
    errors.append(f'{name} is exported by default_dynlib.c but not declared in gxm_bridge.h')

if errors:
    print('\n'.join(errors), file=sys.stderr)
    sys.exit(1)
print(f'PASS: {len(declared)} vita_gxm_* functions declared, defined in both bridges and exported by the loader')
