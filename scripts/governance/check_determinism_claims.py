#!/usr/bin/env python3
"""
Governance Check: Determinism Claim Boundaries

Ensures that any external summary (READMEs/quickstarts) claiming
"determinism" or "deterministic" capabilities explicitly acknowledges
the 'Determinism Surface Registry' as the bounding condition.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


# Match any casing of deterministic or determinism
CLAIM_PATTERN = re.compile(r"(?i)determinism|deterministic")
# Match English or common translation references to the registry
BOUNDARY_PATTERNS = [
    re.compile(r"(?i)determinism surface registry"),
    re.compile(r"(?i)registro de superficie de determinismo"), # ES
    re.compile(r"(?i)registro de superfície de determinismo"), # PT
    re.compile(r"(?i)реєстр|реестр.*детерминизм"), # RU approximation
    re.compile(r"(?i)确定性表面注册表|确定性平面注册表"), # ZH-CN approximation
]
# Canonical File
REGISTRY_FILE = "docs/governance/DETERMINISM_SURFACE_REGISTRY.md"


def check_file(path: Path) -> list[str]:
    content = path.read_text(encoding="utf-8")
    
    # If the file doesn't make determinism claims, it's exempt from the requirement
    if not CLAIM_PATTERN.search(content):
        return []

    # If it makes claims, it MUST include the boundary reference
    has_boundary = any(pat.search(content) for pat in BOUNDARY_PATTERNS)
    # OR explicitly link the registry file
    has_link = REGISTRY_FILE in content

    if not (has_boundary or has_link):
        return [f"{path}: claims determinism but omits 'Determinism Surface Registry' boundary language or link."]
    
    return []


def main() -> int:
    parser = argparse.ArgumentParser(description="Enforce determinism boundary claims in documentation.")
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Path to repository root",
    )
    args = parser.parse_args()

    # Targets defined by the R-01 mitigation plan
    targets = [
        "README.md",
        "README.es.md",
        "README.pt-BR.md",
        "README.ru.md",
        "README.zh-CN.md",
        "docs/user-guide/quickstart/INSTALL.md"
    ]

    errors: list[str] = []
    
    for relative_target in targets:
        target_path = args.repo_root / relative_target
        if not target_path.exists():
            # Soft skip if a specific translation is currently missing
            continue
        
        try:
            errors.extend(check_file(target_path))
        except Exception as exc:
            errors.append(f"{target_path}: failed to read file ({exc})")

    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        print("\nAll determinism claims must be strictly bounded by the Determinism Surface Registry.", file=sys.stderr)
        return 1
    
    print("determinism claims boundary check: pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
