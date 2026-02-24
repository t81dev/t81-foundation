#!/usr/bin/env python3
"""Disallow new source-level usage of legacy core numeric types.

Policy:
- New code should use canonical numerics (`t81::T81BigInt`, `t81::T81Fraction`,
  and/or `t81::v1` numerics).
- Legacy compatibility types (`t81::core::BigInt`, `t81::core::Fraction`) are
  allowed only in a small compatibility allowlist.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


USAGE_RE = re.compile(r"\b(?:t81::core::|core::)(?:BigInt|Fraction)\b")
STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')

# Paths are repository-relative (POSIX style).
ALLOWLIST = {
    "include/t81/core/bigint.hpp",
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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Disallow new source-level usage of legacy core numeric types."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="Repository root to scan (default: infer from script location).",
    )
    return parser.parse_args()


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


def strip_comments_and_strings(text: str) -> str:
    # Remove string literals first so comment markers inside strings do not
    # alter comment stripping behavior.
    text = STRING_RE.sub('""', text)

    out: list[str] = []
    i = 0
    n = len(text)
    in_block = False

    while i < n:
        if in_block:
            end = text.find("*/", i)
            if end == -1:
                # Preserve line numbering even when an unterminated block
                # comment consumes the rest of the file.
                out.extend("\n" for _ in range(text.count("\n", i)))
                break
            # Preserve line numbering for removed block comments.
            out.extend("\n" for _ in range(text.count("\n", i, end)))
            i = end + 2
            in_block = False
            continue

        if text.startswith("/*", i):
            in_block = True
            i += 2
            continue

        if text.startswith("//", i):
            end = text.find("\n", i)
            if end == -1:
                break
            out.append("\n")
            i = end + 1
            continue

        out.append(text[i])
        i += 1

    return "".join(out)


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
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            cleaned = strip_comments_and_strings(f.read())
            for lineno, line in enumerate(cleaned.splitlines(), start=1):
                match = USAGE_RE.search(line)
                if match and rel not in ALLOWLIST:
                    violations.append((rel, lineno, match.group(0)))

    if violations:
        print("legacy core numeric type usage violation(s):")
        for rel, lineno, token in violations:
            print(f"  - {rel}:{lineno}: disallowed token \"{token}\"")
        print("use canonical numerics (`t81::T81BigInt`, `t81::T81Fraction`, `t81::v1`).")
        return 1

    print("legacy core numeric type usage policy: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
