#!/usr/bin/env python3
"""Enforce canonical numeric alias usage in v1 migration-surface tests.

Policy scope:
- Files matching `tests/cpp/v1*_*.cpp`.
- Within those files, direct canonical type names (`t81::T81BigInt`,
  `t81::T81Fraction`) are disallowed; use `t81::v1::CanonicalBigInt` and
  `t81::v1::CanonicalFraction` instead.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


FORBIDDEN_PATTERNS = (
    re.compile(r"\bt81::T81BigInt\b"),
    re.compile(r"\bt81::T81Fraction\b"),
)

ALIAS_PATTERNS = (
    re.compile(r"\bt81::v1::CanonicalBigInt\b"),
    re.compile(r"\bt81::v1::CanonicalFraction\b"),
)

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
        description="Enforce canonical numeric alias usage in v1 migration tests."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="Repository root to scan (default: infer from script location).",
    )
    return parser.parse_args()


def iter_target_files(root: Path) -> list[Path]:
    target_root = root / "tests/cpp"
    if not target_root.exists():
        return []
    files: list[Path] = []
    for path in target_root.rglob("v1*_*.cpp"):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        if any(part in EXCLUDED_DIRS for part in rel.parts):
            continue
        files.append(path)
    return sorted(files, key=lambda p: p.relative_to(root).as_posix())


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

    forbidden_violations: list[tuple[str, int, str]] = []
    missing_alias_files: list[str] = []

    for path in iter_target_files(root):
        rel = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="ignore")

        has_alias = any(p.search(text) for p in ALIAS_PATTERNS)
        if not has_alias:
            missing_alias_files.append(rel)

        lines = text.splitlines()
        for lineno, line in enumerate(lines, start=1):
            for pattern in FORBIDDEN_PATTERNS:
                if pattern.search(line):
                    forbidden_violations.append((rel, lineno, pattern.pattern))

    if forbidden_violations or missing_alias_files:
        print("v1 canonical numeric alias usage policy violation(s):")
        for rel, lineno, pattern in forbidden_violations:
            print(
                f"  - {rel}:{lineno}: disallowed direct canonical type usage "
                f"(matches /{pattern}/)"
            )
        for rel in missing_alias_files:
            print(
                f"  - {rel}: missing canonical alias usage; expected at least one of "
                "`t81::v1::CanonicalBigInt` or `t81::v1::CanonicalFraction`"
            )
        return 1

    print("v1 canonical numeric alias usage policy: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
