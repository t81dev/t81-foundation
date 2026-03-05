#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.vm-trace-evidence.v1"
REQUIRED_TESTS = (
    "t81_vm_trace_test",
    "canonfs_axion_trace_test",
)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate and materialize AI VM trace-level evidence artifacts.")
    p.add_argument("--out-dir", required=True, help="Output directory for trace evidence artifacts")
    p.add_argument("--ctest-log", required=True, help="Path to trace-focused ctest log")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    ctest_log = Path(args.ctest_log).resolve()

    errors: list[str] = []
    runtime: dict[str, Any] = {"ctest_log": str(ctest_log)}
    if not ctest_log.exists():
        errors.append(f"ctest log missing: {ctest_log}")
        log_text = ""
    else:
        log_text = ctest_log.read_text(encoding="utf-8", errors="replace")
    runtime["ctest_log_sha256"] = sha256_text(log_text)

    tests_present: dict[str, bool] = {}
    for name in REQUIRED_TESTS:
        present = name in log_text
        tests_present[name] = present
        if not present:
            errors.append(f"ctest log missing trace test name: {name}")
    runtime["tests_present"] = tests_present
    runtime["all_required_tests_present"] = all(tests_present.values())

    success_marker = "100% tests passed" in log_text
    runtime["ctest_success_marker_present"] = success_marker
    if not success_marker:
        errors.append("ctest log missing success marker: 100% tests passed")

    payload = {
        "schema": SCHEMA_VERSION,
        "status": "pass" if not errors else "fail",
        "valid": len(errors) == 0,
        "errors": errors,
        "runtime_evidence": runtime,
    }
    payload["evidence_sha256"] = sha256_text(canonical_json(payload))

    json_path = out_dir / "ai_vm_trace_evidence.json"
    md_path = out_dir / "ai_vm_trace_evidence.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI VM Trace Evidence",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{payload['status']}`",
                f"- evidence_sha256: `{payload['evidence_sha256']}`",
                f"- all_required_tests_present: `{runtime['all_required_tests_present']}`",
                f"- ctest_success_marker_present: `{runtime['ctest_success_marker_present']}`",
                "",
                "Required Trace Tests:",
                f"- `{REQUIRED_TESTS[0]}`",
                f"- `{REQUIRED_TESTS[1]}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai vm trace evidence status: {payload['status']}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if payload["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
