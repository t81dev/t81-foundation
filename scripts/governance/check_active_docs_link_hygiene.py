#!/usr/bin/env python3
"""Prevent legacy docs/guides path references on the active docs surface."""

from __future__ import annotations

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

SCAN_ROOTS = [
    REPO_ROOT / "docs",
    REPO_ROOT / "scripts",
    REPO_ROOT / "spec",
    REPO_ROOT / "tooling",
    REPO_ROOT / "README.md",
]

EXCLUDED_PREFIXES = [
    REPO_ROOT / "docs/records",
    REPO_ROOT / "docs/records/archive",
    REPO_ROOT / "docs/records/status-history",
    REPO_ROOT / "docs/records/inventories",
]

EXCLUDED_FILES = {
    REPO_ROOT / "docs/status/DECISION_LOG.md",
    REPO_ROOT / "docs/status/CI_GATE_STATUS.md",
    REPO_ROOT / "docs/governance/MIGRATION_MAP.md",
    REPO_ROOT / "docs/index.md",
    REPO_ROOT / "scripts/governance/check_active_docs_link_hygiene.py",
}

FORBIDDEN_PATTERNS = [
    "docs/guides/",
    "../guides/",
    "../../guides/",
    "/t81-foundation/guides/",
]

TEXT_SUFFIXES = {
    ".md",
    ".html",
    ".py",
    ".sh",
    ".yml",
    ".yaml",
    ".json",
    ".js",
}


def should_skip(path: Path) -> bool:
    if path in EXCLUDED_FILES:
        return True
    return any(path.is_relative_to(prefix) for prefix in EXCLUDED_PREFIXES)


def iter_files() -> list[Path]:
    files: list[Path] = []
    for root in SCAN_ROOTS:
        if root.is_file():
            files.append(root)
            continue
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix and path.suffix not in TEXT_SUFFIXES:
                continue
            if should_skip(path):
                continue
            files.append(path)
    return files


def main() -> int:
    issues: list[str] = []
    for path in iter_files():
        rel = path.relative_to(REPO_ROOT)
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        for idx, line in enumerate(lines, 1):
            for pattern in FORBIDDEN_PATTERNS:
                if pattern in line:
                    issues.append(f"{rel}:{idx}: legacy guides path reference: {pattern}")

    if issues:
        print("active docs link hygiene check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("active docs link hygiene check PASSED")
    print("- no legacy docs/guides path references found on the active docs surface")
    return 0


if __name__ == "__main__":
    sys.exit(main())
