#!/usr/bin/env python3
"""Keep ARCHITECTURE target table in sync with CMake target declarations."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ARCH_TABLE_ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|")
CMAKE_TARGET = re.compile(
    r"\b(?:add_(?:library|executable)|pybind11_add_module)\(\s*([A-Za-z0-9_+-]+)"
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def architecture_targets(architecture_md: Path) -> set[str]:
    targets: set[str] = set()
    for line in read_text(architecture_md).splitlines():
        match = ARCH_TABLE_ROW.match(line)
        if match:
            targets.add(match.group(1))
    return targets


def cmake_targets(cmake_files: list[Path]) -> set[str]:
    targets: set[str] = set()
    for cmake in cmake_files:
        text = read_text(cmake)
        for match in CMAKE_TARGET.finditer(text):
            targets.add(match.group(1))
    return targets


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    architecture_md = root / "docs/explanation/ARCHITECTURE.md"
    cmake_files = [root / "CMakeLists.txt", root / "benchmarks/CMakeLists.txt"]

    if not architecture_md.exists():
        print(f"error: missing {architecture_md}")
        return 1
    for cmake in cmake_files:
        if not cmake.exists():
            print(f"error: missing {cmake}")
            return 1

    arch_targets = architecture_targets(architecture_md)
    declared_targets = cmake_targets(cmake_files)

    missing = sorted(t for t in arch_targets if t not in declared_targets)
    if missing:
        print("ARCHITECTURE.md references targets not declared in CMake:")
        for target in missing:
            print(f"  - {target}")
        return 1

    print(f"architecture target sync: ok ({len(arch_targets)} documented targets)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
