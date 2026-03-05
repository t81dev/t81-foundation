#!/usr/bin/env python3
"""Enforce thin-wrapper discipline for legacy core numeric adapters.

Policy:
- `core/types/{bigint,fraction}.cpp` should remain compatibility adapters and
  must not introduce direct arithmetic implementation logic.
- New arithmetic work belongs in canonical paths (`t81::v1` / canonical headers).
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


TARGETS = (
    "core/types/bigint.cpp",
    "core/types/fraction.cpp",
)

# Tokens indicating arithmetic implementation work drifting into wrappers.
FORBIDDEN_TOKENS = (
    "T81BigInt::add(",
    "T81BigInt::sub(",
    "T81BigInt::mul(",
    "T81BigInt::div(",
    "T81BigInt::mod(",
    "T81BigInt::gcd(",
    "T81BigInt::pow(",
    "T81Fraction::add(",
    "T81Fraction::sub(",
    "T81Fraction::mul(",
    "T81Fraction::div(",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Enforce thin-wrapper discipline for core numeric adapters."
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

    for rel in TARGETS:
        path = root / rel
        if not path.exists():
            continue
        with path.open("r", encoding="utf-8", errors="ignore") as handle:
            for lineno, line in enumerate(handle, start=1):
                for token in FORBIDDEN_TOKENS:
                    if token in line:
                        violations.append((rel, lineno, token))

    if violations:
        print("core numeric wrapper thinness policy violation(s):")
        for rel, lineno, token in violations:
            print(f'  - {rel}:{lineno}: disallowed token "{token}"')
        print("move arithmetic implementation logic to canonical numeric paths")
        return 1

    print("core numeric wrapper thinness policy: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
