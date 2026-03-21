#!/usr/bin/env python3
"""Regression tests for scripts/ci/check_legacy_v1_numeric_includes.py."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def run_checker(repo_root: Path, scan_root: Path) -> subprocess.CompletedProcess[str]:
    checker = repo_root / "scripts/ci/check_legacy_v1_numeric_includes.py"
    return subprocess.run(
        ["python3", str(checker), "--root", str(scan_root)],
        text=True,
        capture_output=True,
        check=False,
    )


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]

    with tempfile.TemporaryDirectory(prefix="t81-legacy-v1-include-policy-") as td:
        scan_root = Path(td)

        # Allowlisted migration include locations should pass.
        write(
            scan_root / "include/t81/lang/numeric_format.hpp",
            '#include "t81/core/T81BigInt.hpp"\n#include "t81/core/T81Fraction.hpp"\n',
        )
        write(
            scan_root / "lang/python/t81_python.cpp",
            '#include "t81/core/T81BigInt.hpp"\n',
        )
        # Canonical include path should pass.
        write(
            scan_root / "src/ok.cpp",
            '#include "t81/bigint.hpp"\n',
        )
        # Excluded dependency/tooling directories should be ignored.
        write(
            scan_root / "node_modules/pkg/ignored.cpp",
            '#include "t81/core/T81BigInt.hpp"\n',
        )
        write(
            scan_root / ".venv/lib/ignored.cpp",
            '#include "t81/core/T81Fraction.hpp"\n',
        )
        pass_run = run_checker(repo_root, scan_root)
        if pass_run.returncode != 0:
            print("expected pass but checker failed")
            print(pass_run.stdout)
            print(pass_run.stderr)
            return 1

        # A non-allowlisted legacy include should fail.
        write(
            scan_root / "src/violation.cpp",
            '#include "t81/core/T81BigInt.hpp"\n',
        )
        fail_run = run_checker(repo_root, scan_root)
        if fail_run.returncode == 0:
            print("expected failure but checker passed")
            print(fail_run.stdout)
            print(fail_run.stderr)
            return 1
        if "src/violation.cpp:1" not in fail_run.stdout:
            print("checker failure output missing violating file path/line")
            print(fail_run.stdout)
            return 1

    print("check_legacy_v1_numeric_includes_test: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
