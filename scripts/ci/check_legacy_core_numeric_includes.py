#!/usr/bin/env python3
"""Disallow new includes of legacy core numeric compatibility headers.

Policy:
- New code should include canonical headers (`t81/bigint.hpp`, `t81/fraction.hpp`).
- Legacy headers (`t81/core/bigint.hpp`, `t81/core/fraction.hpp`) are allowed only
  in a small compatibility allowlist.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
FORBIDDEN = {
    "t81/core/bigint.hpp",
    "t81/core/fraction.hpp",
}

# Paths are repository-relative (POSIX style).
ALLOWLIST = {
    "include/t81/t81.hpp",
    "include/t81/core/fraction.hpp",
    "src/core/bigint.cpp",
    "src/core/fraction.cpp",
    "tests/cpp/core_numeric_compat_test.cpp",
    "tests/cpp/core_bigint_compat_properties_test.cpp",
    "tests/cpp/core_fraction_compat_properties_test.cpp",
}

SCAN_SUFFIXES = {
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
}

EXCLUDED_DIRS = {
    ".git",
    "build",
    "build-cxx20",
    ".venv",
    "venv",
    "node_modules",
    "legacy",
}


def iter_source_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        if any(part in EXCLUDED_DIRS for part in rel.parts):
            continue
        if path.suffix in SCAN_SUFFIXES:
            files.append(path)
    return sorted(files, key=lambda p: p.relative_to(root).as_posix())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Disallow new includes of legacy core numeric compatibility headers."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="Repository root to scan (default: infer from script location).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = (
        args.root.resolve()
        if args.root is not None
        else Path(__file__).resolve().parents[2]
    )
    if not root.exists():
        print(f"error: root does not exist: {root}")
        return 1
    violations: list[tuple[str, int, str]] = []

    for path in iter_source_files(root):
        rel = path.relative_to(root).as_posix()
        with path.open("r", encoding="utf-8", errors="ignore") as handle:
            for lineno, line in enumerate(handle, start=1):
                match = INCLUDE_RE.match(line)
                if not match:
                    continue
                include = match.group(1)
                if include in FORBIDDEN and rel not in ALLOWLIST:
                    violations.append((rel, lineno, include))

    if violations:
        print("legacy numeric include policy violation(s):")
        for rel, lineno, include in violations:
            print(f"  - {rel}:{lineno}: disallowed include \"{include}\"")
        print("use canonical headers: \"t81/bigint.hpp\" and \"t81/fraction.hpp\"")
        return 1

    print("legacy numeric include policy: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
