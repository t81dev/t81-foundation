#!/usr/bin/env python3
"""
Audit workflow-level permissions posture in .github/workflows.

Reports whether each workflow declares explicit `permissions:` and summarizes
write scopes, to support least-privilege reviews.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from dataclasses import dataclass


PERMISSIONS_BLOCK = re.compile(r"^permissions:\s*$")
INDENTED_KEY = re.compile(r"^\s{2}([a-zA-Z-]+):\s*(read|write|none)\s*$")


@dataclass(frozen=True)
class WorkflowPermissions:
    path: pathlib.Path
    has_permissions: bool
    entries: dict[str, str]

    @property
    def write_keys(self) -> list[str]:
        return sorted(k for k, v in self.entries.items() if v == "write")


def parse_permissions(path: pathlib.Path) -> WorkflowPermissions:
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    has = False
    entries: dict[str, str] = {}
    i = 0
    while i < len(lines):
        if PERMISSIONS_BLOCK.match(lines[i]):
            has = True
            i += 1
            while i < len(lines):
                line = lines[i]
                if not line.startswith("  ") or not line.strip():
                    break
                m = INDENTED_KEY.match(line)
                if m:
                    entries[m.group(1)] = m.group(2)
                i += 1
            break
        i += 1
    return WorkflowPermissions(path=path, has_permissions=has, entries=entries)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--markdown-out", default="")
    parser.add_argument(
        "--max-missing",
        type=int,
        default=None,
        help="Fail if workflows missing explicit permissions exceed this threshold",
    )
    args = parser.parse_args()

    root = pathlib.Path(args.repo_root).resolve()
    workflows = sorted(
        [p for p in (root / ".github" / "workflows").iterdir() if p.suffix in {".yml", ".yaml"}]
    )
    parsed = [parse_permissions(p) for p in workflows]

    missing = [p for p in parsed if not p.has_permissions]
    write = [p for p in parsed if p.write_keys]

    print(
        f"workflow permissions audit: total={len(parsed)} explicit={len(parsed)-len(missing)} missing={len(missing)} write_scoped={len(write)}"
    )

    if args.markdown_out:
        out = pathlib.Path(args.markdown_out)
        if not out.is_absolute():
            out = root / out
        out.parent.mkdir(parents=True, exist_ok=True)
        lines: list[str] = []
        lines.append("# Workflow Permissions Audit")
        lines.append("")
        lines.append("## Summary")
        lines.append("")
        lines.append(f"- Total workflows: **{len(parsed)}**")
        lines.append(f"- Explicit `permissions` blocks: **{len(parsed)-len(missing)}**")
        lines.append(f"- Missing explicit `permissions`: **{len(missing)}**")
        lines.append(f"- Workflows with write scopes: **{len(write)}**")
        lines.append("")
        lines.append("## Workflow Matrix")
        lines.append("")
        lines.append("| Workflow | Explicit | Write Scopes |")
        lines.append("| --- | --- | --- |")
        for item in parsed:
            rel = item.path.relative_to(root).as_posix()
            explicit = "yes" if item.has_permissions else "no"
            writes = ", ".join(item.write_keys) if item.write_keys else "none"
            lines.append(f"| `{rel}` | {explicit} | `{writes}` |")
        lines.append("")
        lines.append("## Recommendation")
        lines.append("")
        lines.append("- Keep explicit `permissions` on every workflow.")
        lines.append("- Limit `write` scopes to workflows that mutate repo state or publish releases/artifacts requiring it.")
        lines.append("- Re-run this audit after workflow edits.")
        lines.append("")
        out.write_text("\n".join(lines), encoding="utf-8")
        print(f"wrote markdown report: {out.relative_to(root)}")

    if args.max_missing is not None and len(missing) > args.max_missing:
        print(f"ERROR: missing explicit permissions {len(missing)} exceed allowed maximum {args.max_missing}")
        for p in missing:
            print(f"  - {p.path.relative_to(root)}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
