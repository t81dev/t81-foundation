#!/usr/bin/env python3
"""Regression tests for scripts/governance/check_root_structure.py."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def run_validator(cwd: Path, script_path: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["python3", str(script_path)],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    validator_script = repo_root / "scripts/governance/check_root_structure.py"

    if not validator_script.exists():
        print(f"Error: Validator script not found at {validator_script}")
        return 1

    # Test 1: Execution from repository root.
    res_root = run_validator(cwd=repo_root, script_path=validator_script)
    if res_root.returncode != 0:
        print("Test 1 FAILED: Expected check_root_structure.py to pass from repo root.")
        print("stdout:", res_root.stdout)
        print("stderr:", res_root.stderr)
        return 1
    if "root structure check PASSED" not in res_root.stdout:
        print("Test 1 FAILED: Output missing expected 'PASSED' message.")
        return 1

    # Test 2: Execution from non-root working directory (e.g., docs/).
    docs_dir = repo_root / "docs"
    res_docs = run_validator(cwd=docs_dir, script_path=validator_script)
    if res_docs.returncode != 0:
        print("Test 2 FAILED: Expected check_root_structure.py to pass from docs/ directory.")
        print("stdout:", res_docs.stdout)
        print("stderr:", res_docs.stderr)
        return 1
    if "root structure check PASSED" not in res_docs.stdout:
        print("Test 2 FAILED: Output missing expected 'PASSED' message from non-root CWD.")
        return 1

    # Test 3: Negative test - mock missing and unexpected top-level entries.
    with tempfile.TemporaryDirectory(prefix="t81-root-structure-test-") as td:
        temp_dir = Path(td)
        mod_script = temp_dir / "check_root_structure_mock.py"
        script_content = validator_script.read_text(encoding="utf-8")

        # Pin root resolution to actual repo_root and inject a missing entry & unexpected entry
        mod_content = script_content.replace(
            "root = Path(__file__).resolve().parents[2]",
            f"root = Path({repr(str(repo_root))})"
        ).replace(
            '"README.md",',
            '"__NON_EXISTENT_FILE_MOCK__",',
        )
        mod_script.write_text(mod_content, encoding="utf-8")

        res_neg = run_validator(cwd=repo_root, script_path=mod_script)
        if res_neg.returncode == 0:
            print("Test 3 FAILED: Expected mock validator to fail on mismatched root structure.")
            return 1
        if "root structure check FAILED" not in res_neg.stdout:
            print("Test 3 FAILED: Failure output missing expected 'FAILED' header.")
            print("stdout:", res_neg.stdout)
            print("stderr:", res_neg.stderr)
            return 1
        if "- missing:" not in res_neg.stdout or "__NON_EXISTENT_FILE_MOCK__" not in res_neg.stdout:
            print("Test 3 FAILED: Failure output missing expected missing entry detail.")
            print("stdout:", res_neg.stdout)
            return 1
        if "- unexpected:" not in res_neg.stdout or "README.md" not in res_neg.stdout:
            print("Test 3 FAILED: Failure output missing expected unexpected entry detail.")
            print("stdout:", res_neg.stdout)
            return 1

    print("check_root_structure_test: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
