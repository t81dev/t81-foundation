#!/usr/bin/env python3
"""Verify the current stable baseline contract has its expected enforcement surfaces."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


REQUIRED_DOCS = [
    "docs/reference/STABLE_BASELINE_CONTRACT.md",
    "docs/reference/AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md",
    "docs/reference/AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md",
    "docs/reference/AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md",
    "docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md",
]


REQUIRED_CTEST_RUNNERS = [
    "t81_ai_task_assess_fixed_composition_test_runner",
    "t81_ai_task_route_fixed_composition_test_runner",
    "t81_ai_task_classify_fixed_composition_test_runner",
    "artifact_validate_record_test_runner",
    "artifact_family_test_runner",
]

BASELINE_CTEST_REGEX = (
    "t81_ai_task_(assess|route|classify)_fixed_composition_test_runner|"
    "artifact_validate_record_test_runner|"
    "artifact_family_test_runner"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check that the stable baseline contract has its expected docs and test runners."
    )
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Path to the repository root (default: current directory).",
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="Path to the CMake build directory (default: build).",
    )
    parser.add_argument(
        "--run-tests",
        action="store_true",
        help="Also run the stable baseline CTest slice after presence checks pass.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    build_dir = (repo_root / args.build_dir).resolve()

    missing_docs = [doc for doc in REQUIRED_DOCS if not (repo_root / doc).is_file()]
    if missing_docs:
        print("Stable baseline compliance failed: missing required docs:", file=sys.stderr)
        for doc in missing_docs:
            print(f"  - {doc}", file=sys.stderr)
        return 1

    if not build_dir.exists():
        print(f"Stable baseline compliance failed: build directory not found: {build_dir}", file=sys.stderr)
        return 1

    result = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "-N"],
        check=False,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or result.stdout)
        print("Stable baseline compliance failed: unable to enumerate CTest tests.", file=sys.stderr)
        return 1

    ctest_listing = result.stdout
    missing_runners = [name for name in REQUIRED_CTEST_RUNNERS if name not in ctest_listing]
    if missing_runners:
        print("Stable baseline compliance failed: missing required CTest runners:", file=sys.stderr)
        for name in missing_runners:
            print(f"  - {name}", file=sys.stderr)
        return 1

    print("Stable baseline compliance check passed.")
    print("Required docs present:")
    for doc in REQUIRED_DOCS:
        print(f"  - {doc}")
    print("Required CTest runners present:")
    for name in REQUIRED_CTEST_RUNNERS:
        print(f"  - {name}")

    if args.run_tests:
        print("Running stable baseline CTest slice...")
        run_result = subprocess.run(
            [
                "ctest",
                "--test-dir",
                str(build_dir),
                "--output-on-failure",
                "-R",
                BASELINE_CTEST_REGEX,
            ],
            check=False,
            text=True,
        )
        if run_result.returncode != 0:
            print("Stable baseline compliance failed: baseline test slice did not pass.", file=sys.stderr)
            return run_result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
