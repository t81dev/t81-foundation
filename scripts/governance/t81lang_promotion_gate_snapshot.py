#!/usr/bin/env python3
"""Generate a rerunnable T81Lang promotion-gate snapshot."""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
BACKLOG_FILE = REPO_ROOT / "docs/status/HARDENING_BACKLOG.md"
GATE_FILE = REPO_ROOT / "docs/status/T81LANG_PROMOTION_GATE.md"
MATRIX_FILE = REPO_ROOT / "docs/status/IMPLEMENTATION_MATRIX.md"
AUDIT_FILE = REPO_ROOT / "docs/records/audits/2026-03-governance-review.md"
REGISTRY_FILE = REPO_ROOT / "docs/governance/DETERMINISM_SURFACE_REGISTRY.md"
DCP_FILE = REPO_ROOT / "docs/product/DETERMINISTIC_CORE_PROFILE.md"


@dataclass
class CommandResult:
    name: str
    command: list[str]
    returncode: int
    output: str

    @property
    def status(self) -> str:
        return "Pass" if self.returncode == 0 else "Fail"


def run_command(name: str, command: list[str]) -> CommandResult:
    proc = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True)
    output = (proc.stdout or "") + (proc.stderr or "")
    return CommandResult(name=name, command=command, returncode=proc.returncode, output=output)


def parse_bg_statuses(backlog_text: str) -> dict[str, str]:
    statuses: dict[str, str] = {}
    for line in backlog_text.splitlines():
        if not line.startswith("|"):
            continue
        parts = [part.strip() for part in line.split("|")]
        if len(parts) < 5:
            continue
        item_id = parts[1].replace("*", "")
        status = parts[-2].replace("*", "")
        if re.match(r"^BG-\d+", item_id):
            statuses[item_id] = status
    return statuses


def bool_to_status(value: bool) -> str:
    return "Pass" if value else "Fail"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--t81-bin", default="build/t81")
    parser.add_argument("--fixtures-dir", default="tests/fixtures/t81lang_determinism")
    parser.add_argument(
        "--output",
        default="docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md",
        help="Path for generated snapshot markdown.",
    )
    args = parser.parse_args()

    build_dir = REPO_ROOT / args.build_dir
    t81_bin = REPO_ROOT / args.t81_bin
    fixtures_dir = REPO_ROOT / args.fixtures_dir
    output_path = REPO_ROOT / args.output

    commands: list[CommandResult] = [
        run_command(
            "Docs Governance Hygiene",
            [sys.executable, "scripts/governance/check_docs_governance_hygiene.py"],
        ),
        run_command(
            "Conformance + Semantics Slice",
            [
                "ctest",
                "--test-dir",
                str(build_dir),
                "-R",
                (
                    "t81lang_conformance_baseline_test|"
                    "t81lang_conformance_edge_semantics_test|"
                    "t81_semantic_analyzer_match_test|"
                    "t81_semantic_analyzer_loop_test|"
                    "t81_semantic_analyzer_diagnostic_precision_test|"
                    "t81_semantic_analyzer_diagnostic_location_test|"
                    "t81_semantic_analyzer_cascade_suppression_test"
                ),
                "--output-on-failure",
            ],
        ),
        run_command(
            "Compile Determinism Slice",
            [
                "ctest",
                "--test-dir",
                str(build_dir),
                "-R",
                (
                    "e2e_compile_determinism_test|"
                    "e2e_ast_ir_canonical_determinism_test|"
                    "e2e_enum_metadata_determinism_test"
                ),
                "--output-on-failure",
            ],
        ),
        run_command(
            "Axion Metadata Slice",
            [
                "ctest",
                "--test-dir",
                str(build_dir),
                "-R",
                (
                    "axion_policy_match_guard_test|"
                    "axion_policy_segment_event_test|"
                    "axion_match_metadata_test|"
                    "axion_enum_guard_test|"
                    "e2e_axion_trace_test"
                ),
                "--output-on-failure",
            ],
        ),
        run_command(
            "Repro Gate",
            [
                sys.executable,
                "scripts/ci/t81lang_repro_gate.py",
                "--t81-bin",
                str(t81_bin),
                "--fixtures-dir",
                str(fixtures_dir),
                "--workdir",
                "build/t81lang-repro-promotion-gate",
                "--hash-out",
                "build/t81lang-repro-promotion-gate/hash.txt",
            ],
        ),
    ]

    backlog_text = BACKLOG_FILE.read_text(encoding="utf-8")
    bg_statuses = parse_bg_statuses(backlog_text)
    bg_complete = all(
        bg_statuses.get(item, "").startswith("2026-")
        for item in ("BG-01", "BG-02", "BG-03", "BG-04", "BG-05")
    )

    tg01 = commands[0].returncode == 0
    tg02 = commands[4].returncode == 0
    tg03 = all(cmd.returncode == 0 for cmd in commands[1:4])
    tg04 = bg_complete
    tg05 = all(path.exists() for path in (GATE_FILE, MATRIX_FILE, AUDIT_FILE))
    tg06 = all(path.exists() for path in (REGISTRY_FILE, DCP_FILE))
    overall_ready = all((tg01, tg02, tg03, tg04, tg05, tg06))

    now = dt.datetime.now(dt.timezone.utc)
    ts = now.strftime("%Y-%m-%d %H:%M:%SZ")
    snapshot_date = now.strftime("%Y-%m-%d")

    lines = [
        "# T81Lang Promotion Gate Snapshot",
        "",
        f"Generated (UTC): {ts}",
        f"Generator: `scripts/governance/t81lang_promotion_gate_snapshot.py`",
        "",
        "## Gate Criteria Status",
        "",
        "| Criterion | Status | Basis |",
        "| :--- | :--- | :--- |",
        f"| TG-01 | {bool_to_status(tg01)} | Governance hygiene check command status |",
        f"| TG-02 | {bool_to_status(tg02)} | Repro gate command status |",
        f"| TG-03 | {bool_to_status(tg03)} | Conformance/semantic/determinism/Axion ctest slices |",
        f"| TG-04 | {bool_to_status(tg04)} | BG-01..BG-05 completion status in backlog |",
        f"| TG-05 | {bool_to_status(tg05)} | Gate/matrix/audit artifacts present |",
        f"| TG-06 | {bool_to_status(tg06)} | Registry + DCP references present |",
        "",
        "## Promotion Readiness",
        "",
        f"- Snapshot Date: {snapshot_date}",
        (
            "- Result: Ready for Beta-candidate review"
            if overall_ready
            else "- Result: Not Ready"
        ),
        "",
        "## Backlog Statuses",
        "",
    ]

    for item in ("BG-01", "BG-02", "BG-03", "BG-04", "BG-05"):
        lines.append(f"- {item}: {bg_statuses.get(item, 'Missing')}")

    lines.extend(
        [
            "",
            "## Command Runs",
            "",
        ]
    )

    for cmd in commands:
        lines.append(f"### {cmd.name}")
        lines.append("")
        lines.append(f"- Status: {cmd.status}")
        lines.append(f"- Command: `{' '.join(cmd.command)}`")
        lines.append("")
        lines.append("```text")
        lines.append(cmd.output.strip() or "(no output)")
        lines.append("```")
        lines.append("")

    output_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")

    print(f"Wrote snapshot: {output_path.relative_to(REPO_ROOT)}")
    print(f"Overall result: {'READY' if overall_ready else 'NOT_READY'}")
    return 0 if overall_ready else 1


if __name__ == "__main__":
    raise SystemExit(main())
