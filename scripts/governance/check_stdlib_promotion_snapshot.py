#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SNAPSHOT = REPO_ROOT / "docs/status/STDLIB_PROMOTION_SNAPSHOT.md"

EXPECTED_MODULES = {
    "std.core",
    "std.math",
    "std.io",
    "std.collections",
    "std.text",
    "std.bytes",
    "std.symbol",
    "std.sys",
    "std.async",
    "std.tensor",
    "std.agent",
}

ALLOWED_STATUS = {"stable", "bounded", "experimental"}


def main() -> int:
    if not SNAPSHOT.exists():
        print("stdlib promotion snapshot check FAILED")
        print("- missing snapshot: docs/status/STDLIB_PROMOTION_SNAPSHOT.md")
        return 1

    text = SNAPSHOT.read_text(encoding="utf-8")
    module_rows: dict[str, str] = {}
    for line in text.splitlines():
        if not line.startswith("|"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) < 4:  # Updated to handle 4-column table
            continue
        module = parts[1].strip("`")  # Module is in column 1 (0-indexed)
        status = parts[2].strip().lower()  # Status is in column 2
        if module in EXPECTED_MODULES:
            module_rows[module] = status

    issues: list[str] = []
    missing = sorted(EXPECTED_MODULES - set(module_rows.keys()))
    if missing:
        issues.append("missing module rows: " + ", ".join(missing))

    for module, status in sorted(module_rows.items()):
        if status not in ALLOWED_STATUS:
            issues.append(f"invalid status '{status}' for {module}")

    if "Date:" not in text:
        issues.append("snapshot missing Date field")
    if "Baseline:" not in text:
        issues.append("snapshot missing Baseline field")

    if issues:
        print("stdlib promotion snapshot check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("stdlib promotion snapshot check PASSED")
    print(f"- modules validated: {len(module_rows)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
