#!/usr/bin/env python3
"""Run the admitted-family bundle consumers and verify their conformance output."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


CONSUMER_CASES = (
    {
        "label": "assess-fixed",
        "script": "examples/ai-and-inference/model-load-canonfs/run_assess_fixed_bundle_consumer.sh",
        "required": (
            "bundle_schema=t81.ai.task.assess-fixed.bundle.v1",
            "record_ref=sha3-256:",
            "action_ref=sha3-256:",
            "selected_action=write_allow_marker",
            "selected_path=actions/allow.marker",
        ),
    },
    {
        "label": "route-fixed",
        "script": "examples/ai-and-inference/model-load-canonfs/run_route_fixed_bundle_consumer.sh",
        "required": (
            "bundle_schema=t81.ai.task.route-fixed.bundle.v1",
            "record_ref=sha3-256:",
            "action_ref=sha3-256:",
            "selected_action=write_route_a_target",
            "selected_path=routes/a.target",
        ),
    },
    {
        "label": "classify-fixed",
        "script": "examples/ai-and-inference/model-load-canonfs/run_classify_fixed_bundle_consumer.sh",
        "required": (
            "bundle_schema=t81.ai.task.classify-fixed.bundle.v1",
            "record_ref=sha3-256:",
            "action_ref=sha3-256:",
            "selected_rule_set=positive-default",
            "rule_set_ref=sha3-256:",
        ),
    },
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the admitted-family bundle consumer scripts and verify their output."
    )
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Path to the repository root (default: current directory).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()

    for case in CONSUMER_CASES:
        script = repo_root / case["script"]
        if not script.is_file():
            print(
                f"Bundle consumer conformance failed: missing script for {case['label']}: {script}",
                file=sys.stderr,
            )
            return 1

        result = subprocess.run(
            ["bash", str(script)],
            cwd=repo_root,
            check=False,
            text=True,
            capture_output=True,
        )
        if result.returncode != 0:
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            print(
                f"Bundle consumer conformance failed: {case['label']} consumer exited non-zero.",
                file=sys.stderr,
            )
            return result.returncode

        missing = [pattern for pattern in case["required"] if pattern not in result.stdout]
        if missing:
            sys.stderr.write(result.stdout)
            print(
                f"Bundle consumer conformance failed: {case['label']} output is missing required markers.",
                file=sys.stderr,
            )
            for pattern in missing:
                print(f"  - {pattern}", file=sys.stderr)
            return 1

        print(f"Bundle consumer conformance passed: {case['label']}")
        for pattern in case["required"]:
            print(f"  - {pattern}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
