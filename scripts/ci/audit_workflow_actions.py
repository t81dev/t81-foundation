#!/usr/bin/env python3
"""
Audit GitHub workflow `uses:` references for pinning hygiene.

Usage:
  python3 scripts/ci/audit_workflow_actions.py
  python3 scripts/ci/audit_workflow_actions.py --markdown-out docs/audits/2026-02-workflow-action-audit.md
"""

from __future__ import annotations

import argparse
import pathlib
import re
from dataclasses import dataclass


# Match both:
#   uses: owner/action@ref
#   - uses: owner/action@ref
USES_PATTERN = re.compile(r"^\s*(?:-\s*)?uses:\s*([^\s#]+)")
PINNED_SHA_PATTERN = re.compile(r"^[^@]+@[0-9a-fA-F]{40}$")
PINNED_DOCKER_DIGEST_PATTERN = re.compile(r"^docker://[^@]+@sha256:[0-9a-fA-F]{64}$")
TAG_PATTERN = re.compile(r"^[^@]+@.+$")


@dataclass(frozen=True)
class UseRef:
    file: pathlib.Path
    line_no: int
    ref: str

    @property
    def is_pinned_sha(self) -> bool:
        return bool(PINNED_SHA_PATTERN.match(self.ref) or PINNED_DOCKER_DIGEST_PATTERN.match(self.ref))

    @property
    def is_tag(self) -> bool:
        return bool(TAG_PATTERN.match(self.ref)) and not self.is_pinned_sha


def collect_uses(repo_root: pathlib.Path) -> list[UseRef]:
    refs: list[UseRef] = []
    workflow_globs = [".github/workflows/*.yml", ".github/workflows/*.yaml"]
    for pattern in workflow_globs:
        for wf in sorted(repo_root.glob(pattern)):
            with wf.open("r", encoding="utf-8") as handle:
                for i, line in enumerate(handle, start=1):
                    match = USES_PATTERN.match(line)
                    if not match:
                        continue
                    refs.append(UseRef(file=wf, line_no=i, ref=match.group(1)))
    return refs


def build_markdown(refs: list[UseRef], repo_root: pathlib.Path) -> str:
    pinned = [r for r in refs if r.is_pinned_sha]
    tagged = [r for r in refs if r.is_tag]
    unknown = [r for r in refs if not r.is_pinned_sha and not r.is_tag]

    lines: list[str] = []
    lines.append("# Workflow Action Pinning Audit")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Total `uses:` references: **{len(refs)}**")
    lines.append(f"- Pinned to immutable SHA/digest: **{len(pinned)}**")
    lines.append(f"- Tag/major-version references: **{len(tagged)}**")
    lines.append(f"- Unclassified references: **{len(unknown)}**")
    lines.append("")
    lines.append("## Tag/Major References (Migration Candidates)")
    lines.append("")
    lines.append("| Workflow | Line | Reference |")
    lines.append("| --- | ---: | --- |")
    for ref in tagged:
        rel = ref.file.relative_to(repo_root).as_posix()
        lines.append(f"| `{rel}` | {ref.line_no} | `{ref.ref}` |")
    if not tagged:
        lines.append("| n/a | n/a | n/a |")
    lines.append("")
    lines.append("## SHA-Pinned References")
    lines.append("")
    lines.append("| Workflow | Line | Reference |")
    lines.append("| --- | ---: | --- |")
    for ref in pinned:
        rel = ref.file.relative_to(repo_root).as_posix()
        lines.append(f"| `{rel}` | {ref.line_no} | `{ref.ref}` |")
    if not pinned:
        lines.append("| n/a | n/a | n/a |")
    lines.append("")
    if unknown:
        lines.append("## Unclassified References")
        lines.append("")
        lines.append("| Workflow | Line | Reference |")
        lines.append("| --- | ---: | --- |")
        for ref in unknown:
            rel = ref.file.relative_to(repo_root).as_posix()
            lines.append(f"| `{rel}` | {ref.line_no} | `{ref.ref}` |")
        lines.append("")
    lines.append("## Recommendation")
    lines.append("")
    lines.append("- Keep all workflow references pinned to immutable SHAs/digests.")
    lines.append("- Use Dependabot for GitHub Actions to roll forward pinned SHAs through reviewable PRs.")
    lines.append("- Re-run this audit after workflow edits to prevent tag regressions.")
    lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root (default: current directory)",
    )
    parser.add_argument(
        "--markdown-out",
        default="",
        help="Optional path to write markdown report",
    )
    parser.add_argument(
        "--max-tagged",
        type=int,
        default=None,
        help="Fail if tag/major-version references exceed this threshold",
    )
    parser.add_argument(
        "--max-unknown",
        type=int,
        default=None,
        help="Fail if unclassified references exceed this threshold",
    )
    args = parser.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    refs = collect_uses(repo_root)
    pinned = sum(1 for r in refs if r.is_pinned_sha)
    tagged = sum(1 for r in refs if r.is_tag)
    unknown = len(refs) - pinned - tagged

    print(f"workflow action audit: total={len(refs)} pinned_sha={pinned} tagged={tagged} unknown={unknown}")

    if args.markdown_out:
        md_path = pathlib.Path(args.markdown_out)
        if not md_path.is_absolute():
            md_path = repo_root / md_path
        md_path.parent.mkdir(parents=True, exist_ok=True)
        md_path.write_text(build_markdown(refs, repo_root), encoding="utf-8")
        print(f"wrote markdown report: {md_path.relative_to(repo_root)}")

    if args.max_tagged is not None and tagged > args.max_tagged:
        print(f"ERROR: tagged references {tagged} exceed allowed maximum {args.max_tagged}")
        return 1
    if args.max_unknown is not None and unknown > args.max_unknown:
        print(f"ERROR: unknown references {unknown} exceed allowed maximum {args.max_unknown}")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
