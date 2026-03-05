#!/usr/bin/env python3
"""Lightweight docs/governance hygiene checks (non-CI mandatory by default)."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
QUEUE_FILE = REPO_ROOT / "docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md"

REQUIRED_READMES = [
    "core/README.md",
    "kernel/README.md",
    "runtime/README.md",
    "runtime/jit/README.md",
    "lang/README.md",
    "experimental/README.md",
    "experimental/distributed/README.md",
    "internal/README.md",
    "internal/axion/README.md",
    "internal/tooling/README.md",
    "tooling/README.md",
    "scripts/architecture/README.md",
    "scripts/governance/README.md",
    "scripts/restructure/README.md",
    "spec/tisc/README.md",
    "examples/consumer_cmake/README.md",
    "examples/system-integration/README.md",
    "docs/architecture/README.md",
    "docs/benchmarks/README.md",
    "docs/explanation/README.md",
    "docs/governance/README.md",
    "docs/how-to/README.md",
    "docs/migration/README.md",
    "docs/policies/README.md",
    "docs/product/README.md",
    "docs/records/README.md",
    "docs/records/inventories/README.md",
    "docs/reference/README.md",
    "docs/releases/README.md",
    "docs/research/README.md",
    "docs/rfcs/README.md",
    "spec/supplemental/README.md",
    "docs/standards/README.md",
    "docs/status/README.md",
    "docs/tutorials/README.md",
]


def parse_queue_statuses(queue_text: str) -> dict[str, set[str]]:
    statuses: dict[str, set[str]] = {}
    for line in queue_text.splitlines():
        if not line.startswith("|"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) < 9:
            continue
        task_id = parts[1]
        status = parts[7]
        if not re.match(r"^[A-Z0-9-]+$", task_id):
            continue
        statuses.setdefault(task_id, set()).add(status)
    return statuses


def main() -> int:
    issues: list[str] = []

    # 1) Required README presence check.
    for rel in REQUIRED_READMES:
        path = REPO_ROOT / rel
        if not path.exists():
            issues.append(f"missing required README: {rel}")

    # 2) Queue consistency check.
    if not QUEUE_FILE.exists():
        issues.append(f"missing queue file: {QUEUE_FILE.relative_to(REPO_ROOT)}")
        queue_statuses: dict[str, set[str]] = {}
    else:
        queue_text = QUEUE_FILE.read_text(encoding="utf-8")
        queue_statuses = parse_queue_statuses(queue_text)
        for task_id, vals in sorted(queue_statuses.items()):
            has_planned = any("Planned" in v for v in vals)
            has_completed = any("Completed" in v for v in vals)
            if has_planned and has_completed:
                issues.append(
                    f"task has both Planned and Completed statuses in queue: {task_id}"
                )

    # 3) Stale planned markers for completed tasks in status/audit artifacts.
    scan_dirs = [REPO_ROOT / "docs/status", REPO_ROOT / "docs/records/audits"]
    for task_id, vals in sorted(queue_statuses.items()):
        if not any("Completed" in v for v in vals):
            continue
        pattern = re.compile(rf"\b{re.escape(task_id)}\b.*\bPlanned\b")
        for scan_dir in scan_dirs:
            for md_file in scan_dir.rglob("*.md"):
                if not md_file.is_file():
                    continue
                rel = md_file.relative_to(REPO_ROOT)
                for idx, line in enumerate(md_file.read_text(encoding="utf-8").splitlines(), 1):
                    if pattern.search(line):
                        issues.append(
                            f"stale planned marker for completed task {task_id}: {rel}:{idx}"
                        )

    # 4) Cross-document status label coherence check.
    coherence_check = REPO_ROOT / "scripts/governance/check_status_label_coherence.py"
    if not coherence_check.exists():
      issues.append("missing status coherence check script: scripts/governance/check_status_label_coherence.py")
    else:
      result = subprocess.run(
          [sys.executable, str(coherence_check)],
          cwd=REPO_ROOT,
          capture_output=True,
          text=True,
          check=False,
      )
      if result.returncode != 0:
          issues.append("status label coherence check failed")
          output = (result.stdout + "\n" + result.stderr).strip()
          if output:
              for line in output.splitlines():
                  issues.append(f"coherence: {line}")

    # 5) Supplemental governance policy checks promoted from warning-only rows.
    supplemental_checks = [
        ("root structure", "scripts/governance/check_root_structure.py"),
        ("README naming", "scripts/governance/check_readme_naming.py"),
        ("translation metadata", "scripts/governance/check_translation_metadata.py"),
        ("translation staleness", "scripts/governance/check_translation_staleness.py"),
        ("translation semantic alignment", "scripts/governance/check_translation_semantic_alignment.py"),
        ("docs structure", "scripts/governance/check_docs_structure.py"),
        ("license policy", "scripts/governance/check_license_policy.py"),
        ("artifact hygiene", "scripts/governance/check_repo_artifact_hygiene.py"),
        ("public api semver lock", "scripts/governance/check_public_api_semver.py"),
        ("spec-code alignment baseline", "scripts/governance/check_spec_code_alignment_baseline.py"),
        ("stdlib surface baseline", "scripts/governance/check_stdlib_surface_baseline.py"),
        ("stdlib promotion snapshot", "scripts/governance/check_stdlib_promotion_snapshot.py"),
        ("cognitive-tier boundary", "scripts/governance/check_cognitive_tier_boundary.py"),
        ("overclaim guardrails", "scripts/governance/check_overclaim_guardrails.py"),
        ("rfc lifecycle hygiene", "scripts/governance/check_rfc_lifecycle_hygiene.py"),
        ("target name drift", "scripts/governance/check_target_name_drift.py"),
    ]
    for label, rel_script in supplemental_checks:
        script_path = REPO_ROOT / rel_script
        if not script_path.exists():
            issues.append(f"missing {label} check script: {rel_script}")
            continue
        result = subprocess.run(
            [sys.executable, str(script_path)],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            issues.append(f"{label} check failed")
            output = (result.stdout + "\n" + result.stderr).strip()
            if output:
                for line in output.splitlines():
                    issues.append(f"{label}: {line}")

    if issues:
        print("governance hygiene check FAILED:")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("governance hygiene check PASSED")
    print(f"- required README coverage checked: {len(REQUIRED_READMES)} paths")
    print("- task-queue status consistency checked")
    print("- stale planned markers checked for completed tasks")
    print("- status label coherence checked")
    print(
        "- supplemental governance checks "
        "(structure/readme/translation/staleness/semantic/license/artifact/api/spec-boundary/stdlib/snapshot/overclaim/rfc-lifecycle) checked"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
