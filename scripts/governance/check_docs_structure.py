#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

BOOK_DIRS = [
    "docs/book/book-en",
    "docs/book/book-cn",
    "docs/book/book-es",
    "docs/book/book-pt",
    "docs/book/book-ru",
]
REQUIRED_PREFIXES = {f"{n:02d}" for n in range(1, 16)}


def chapter_prefixes(path: Path) -> set[str]:
    out: set[str] = set()
    for p in path.glob("*.md"):
        m = re.match(r"^(\d{2})_", p.name)
        if m:
            out.add(m.group(1))
    return out


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    issues: list[str] = []

    for rel in BOOK_DIRS:
        path = root / rel
        if not path.is_dir():
            issues.append(f"missing directory: {rel}")
            continue
        found = chapter_prefixes(path)
        missing = sorted(REQUIRED_PREFIXES - found)
        if missing:
            issues.append(f"{rel}: missing chapter prefixes {', '.join(missing)}")

    if issues:
        print("docs structure check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("docs structure check PASSED")
    print("- verified chapter prefix coverage 01..15 across book language directories")
    return 0


if __name__ == "__main__":
    sys.exit(main())
