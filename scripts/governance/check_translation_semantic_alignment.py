#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    issues: list[str] = []

    # Language-specific root README semantic checks.
    rules: dict[str, dict[str, list[str]]] = {
        "README.es.md": {
            "required": [],
            "banned_regex": [],
            "required_headings": [],
        },
        "README.pt-BR.md": {
            "required": [],
            "banned_regex": [],
            "required_headings": [],
        },
        "README.ru.md": {
            "required": [],
            "banned_regex": [],
            "required_headings": [],
        },
        "README.zh-CN.md": {
            "required": [],
            "banned_regex": [],
            "required_headings": [],
        },
    }

    for rel, cfg in sorted(rules.items()):
        path = root / rel
        if not path.exists():
            issues.append(f"missing translation readme: {rel}")
            continue
        text = path.read_text(encoding="utf-8")
        for token in cfg["required"]:
            if token not in text:
                issues.append(f"{rel}: missing required semantic token: {token}")
        for pat in cfg["banned_regex"]:
            if re.search(pat, text, flags=re.IGNORECASE):
                issues.append(f"{rel}: contains banned over-strong phrase /{pat}/")
        for heading in cfg.get("required_headings", []):
            if heading not in text:
                issues.append(f"{rel}: missing required section heading: {heading}")

    if issues:
        print("translation semantic alignment check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("translation semantic alignment check PASSED")
    print(f"- files checked: {len(rules)} root translation READMEs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
