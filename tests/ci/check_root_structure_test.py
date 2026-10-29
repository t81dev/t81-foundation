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

    # Test 3: Synthetic negative test - mock missing and unexpected top-level entries.
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

    # Test 4: Historical regression test - detect PR #617 (commit 7afd324) stale allowlist failure.
    historical_617_allowlist = {
        ".clang-format", ".clang-tidy", ".cursorrules", ".devcontainer", ".editorconfig",
        ".gitattributes", ".github", ".gitignore", ".gitmodules", ".pre-commit-config.yaml",
        ".t81", "BUILD.bazel", "CMakeDoxyfile.in", "CMakeLists.txt", "CMakePresets.json",
        "CODE_OF_CONDUCT.md", "CONTRIBUTING.md", "LICENSE", "Makefile", "README.es.md",
        "README.md", "README.pt-BR.md", "README.ru.md", "README.zh-CN.md", "SECURITY.md",
        "SUPPORT_TAXONOMY.md", "T81_CAPABILITIES_REPORT.md", "artifacts", "benchmarks",
        "cmake", "codec", "contracts", "crypto", "docs", "drivers", "examples", "experimental",
        "ffi", "fs", "include", "inference", "kernel", "lang", "logs", "package-lock.json",
        "package.json", "pyproject.toml", "scripts", "spec", "ternaryos", "tests",
        "third_party", "tools", "vm", "install.ps1", "install.sh", "isa", "_config.yml",
        "AGENTS.md", "Dockerfile",
    }

    with tempfile.TemporaryDirectory(prefix="t81-root-structure-hist-") as td:
        temp_dir = Path(td)
        hist_script = temp_dir / "check_root_structure_hist_617.py"
        script_content = validator_script.read_text(encoding="utf-8")

        # Replace EXPECTED_TOP_LEVEL with historical 60-item allowlist
        start_idx = script_content.find("EXPECTED_TOP_LEVEL = {")
        end_idx = script_content.find("}", start_idx) + 1
        hist_content = (
            script_content[:start_idx]
            + f"EXPECTED_TOP_LEVEL = {repr(historical_617_allowlist)}"
            + script_content[end_idx:]
        )
        hist_content = hist_content.replace(
            "root = Path(__file__).resolve().parents[2]",
            f"root = Path({repr(str(repo_root))})"
        )
        hist_script.write_text(hist_content, encoding="utf-8")

        res_hist = run_validator(cwd=repo_root, script_path=hist_script)
        if res_hist.returncode == 0:
            print("Test 4 FAILED: Expected historical PR #617 allowlist to fail against repository root tree.")
            return 1
        if "root structure check FAILED" not in res_hist.stdout:
            print("Test 4 FAILED: Output missing expected 'FAILED' header.")
            print("stdout:", res_hist.stdout)
            return 1

        # Check for historical missing entries
        for missing_item in ["package-lock.json", "package.json"]:
            if missing_item not in res_hist.stdout:
                print(f"Test 4 FAILED: Expected missing item {missing_item} not found in output.")
                print("stdout:", res_hist.stdout)
                return 1

        # Check for key historical unexpected entries (omitted in 617 allowlist)
        for unexpected_item in ["src", "experiments", "skills", "strategic_docs", "PROJECT_STRUCTURE.md"]:
            if unexpected_item not in res_hist.stdout:
                print(f"Test 4 FAILED: Expected unexpected item {unexpected_item} not found in output.")
                print("stdout:", res_hist.stdout)
                return 1

    print("check_root_structure_test: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
