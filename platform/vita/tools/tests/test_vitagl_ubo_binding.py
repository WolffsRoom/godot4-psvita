#!/usr/bin/env python3
"""Regression guard for VitaGL indexed UBO binding semantics required by Godot 4."""
from __future__ import annotations

import re
import sys
from pathlib import Path


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\bvoid\s+{re.escape(name)}\s*\([^)]*\)\s*\{{", text)
    if not match:
        raise AssertionError(f"function {name} not found")
    start = match.end() - 1
    depth = 0
    for pos in range(start, len(text)):
        ch = text[pos]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : pos]
    raise AssertionError(f"unterminated function {name}")


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} <vitaGL-source-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = root / "source" / "custom_shaders.c"
    text = source.read_text(encoding="utf-8", errors="replace")

    failures: list[str] = []
    for name in ("glBindBufferBase", "glBindBufferRange"):
        body = function_body(text, name)
        if "uniform_array_unit = buffer;" not in body:
            failures.append(
                f"{name} does not update the generic GL_UNIFORM_BUFFER binding "
                "(missing `uniform_array_unit = buffer;`)"
            )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print("PASS: glBindBufferBase/glBindBufferRange update the generic UBO binding")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
