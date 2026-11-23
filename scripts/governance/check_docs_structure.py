#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

BOOK_DIRS = [
    "book/book-en",
    "book/book-cn",
    "book/book-es",
    "book/book-pt",
    "book/book-ru",
]
REQUIRED_PREFIXES = {f"{n:02d}" for n in range(1, 16)}


def chapter_prefixes(path: Path) -> set[str]:
    out: set[str] = set()
    for p in path.glob("*.md"):
        m = re.match(r"^(\d{2})_", p.name)
        if m:
            out.add(m.group(1))
    return out


ACTIVE_DOC_DIRS = [
    "docs/governance",
    "docs/research",
    "docs/reference",
    "docs/user-guide",
    "docs/explanation",
    "docs/how-to",
    "docs/status",
]


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    issues: list[str] = []

    if (root / "book").is_dir():
        for rel in BOOK_DIRS:
            path = root / rel
            if not path.is_dir():
                issues.append(f"missing directory: {rel}")
                continue
            found = chapter_prefixes(path)
            missing = sorted(REQUIRED_PREFIXES - found)
            if missing:
                issues.append(f"{rel}: missing chapter prefixes {', '.join(missing)}")
    else:
        for rel in ACTIVE_DOC_DIRS:
            if not (root / rel).is_dir():
                issues.append(f"missing active docs directory: {rel}")

    if issues:
        print("docs structure check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("docs structure check PASSED")
    if (root / "book").is_dir():
        print("- verified chapter prefix coverage 01..15 across book language directories")
    else:
        print(f"- verified {len(ACTIVE_DOC_DIRS)} active docs/ directories")
    return 0


if __name__ == "__main__":
    sys.exit(main())
