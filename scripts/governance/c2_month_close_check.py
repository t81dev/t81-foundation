#!/usr/bin/env python3
"""Run C2 month-close governance checks and emit an auditable summary."""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md"


@dataclass
class CheckResult:
    name: str
    command: list[str]
    returncode: int
    output: str

    @property
    def status(self) -> str:
        return "PASS" if self.returncode == 0 else "FAIL"


def run_command(name: str, command: list[str]) -> CheckResult:
    proc = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True)
    output = (proc.stdout or "") + (proc.stderr or "")
    return CheckResult(name=name, command=command, returncode=proc.returncode, output=output)


def run_link_target_sweep() -> CheckResult:
    root = REPO_ROOT.resolve()
    scan = [
        root / "docs/status",
        root / "docs/governance",
        root / "docs/product",
        root / "docs/records/audits",
    ]
    pattern = re.compile(r"\[[^\]]+\]\(([^)#]+)")
    missing: list[tuple[pathlib.Path, str]] = []
    for base in scan:
        for md in base.rglob("*.md"):
            text = md.read_text(encoding="utf-8")
            for match in pattern.finditer(text):
                link = match.group(1).strip()
                if not link or "://" in link or link.startswith("#"):
                    continue
                target = (md.parent / link).resolve()
                if not target.exists():
                    missing.append((md.relative_to(root), link))

    if missing:
        lines = [f"MISSING {md} -> {link}" for md, link in missing]
        output = "\n".join(lines) + "\n"
        return CheckResult(
            name="Markdown link-target sweep",
            command=["internal:link-target-sweep"],
            returncode=1,
            output=output,
        )

    return CheckResult(
        name="Markdown link-target sweep",
        command=["internal:link-target-sweep"],
        returncode=0,
        output="link-target sweep passed\n",
    )


def extract_snapshot_timestamp(snapshot_path: pathlib.Path) -> str:
    if not snapshot_path.exists():
        return "unavailable (snapshot file not found)"
    for line in snapshot_path.read_text(encoding="utf-8").splitlines():
        if line.startswith("Generated (UTC):"):
            return line.split(":", 1)[1].strip()
    return "unavailable (timestamp line not found)"


def format_report(results: list[CheckResult], snapshot_timestamp: str) -> str:
    generated = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%d %H:%M:%SZ")
    overall_ok = all(r.returncode == 0 for r in results)
    overall = "PASS" if overall_ok else "FAIL"

    lines = [
        "# C2 Month-Close Check Report (2026-03-31 Runbook)",
        "",
        f"Generated (UTC): {generated}",
        f"Generator: `scripts/governance/c2_month_close_check.py`",
        f"Overall: {overall}",
        "",
        "## Summary",
        "",
        "| Check | Status |",
        "| :--- | :--- |",
    ]
    for result in results:
        lines.append(f"| {result.name} | {result.status} |")

    lines.extend(
        [
            "",
            "## Derived Fields",
            "",
            f"- Promotion snapshot timestamp: {snapshot_timestamp}",
            "",
            "## Command Outputs",
            "",
        ]
    )

    for result in results:
        lines.append(f"### {result.name}")
        lines.append("")
        lines.append(f"- Status: {result.status}")
        lines.append(f"- Command: `{' '.join(result.command)}`")
        lines.append("")
        lines.append("```text")
        lines.append(result.output.strip() or "(no output)")
        lines.append("```")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default=str(DEFAULT_OUTPUT.relative_to(REPO_ROOT)),
        help="Markdown report path (repo-relative or absolute).",
    )
    parser.add_argument(
        "--snapshot-path",
        default="docs/status/archive/T81LANG_PROMOTION_GATE_SNAPSHOT.md",
        help="Promotion snapshot file path.",
    )
    args = parser.parse_args()

    output_path = pathlib.Path(args.output)
    if not output_path.is_absolute():
        output_path = (REPO_ROOT / output_path).resolve()

    snapshot_path = pathlib.Path(args.snapshot_path)
    if not snapshot_path.is_absolute():
        snapshot_path = (REPO_ROOT / snapshot_path).resolve()

    results = [
        run_command(
            "Governance hygiene check",
            [sys.executable, "scripts/governance/check_docs_governance_hygiene.py"],
        ),
        run_command(
            "Promotion gate snapshot refresh",
            [sys.executable, "scripts/governance/t81lang_promotion_gate_snapshot.py"],
        ),
        run_link_target_sweep(),
    ]

    report = format_report(results, extract_snapshot_timestamp(snapshot_path))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report, encoding="utf-8")

    print(f"Wrote report: {output_path.relative_to(REPO_ROOT)}")
    for result in results:
        print(f"{result.name}: {result.status}")

    return 0 if all(r.returncode == 0 for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
