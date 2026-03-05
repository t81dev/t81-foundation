#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.benchmark-spec.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def build_report() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "metadata": {
            "suite": "standard",
            "benchmark_version": "1.0.0",
            "report_id": "sha256:placeholder",
        },
        "environment": {
            "platform": "linux-x86_64",
            "t81_version": "baseline",
            "determinism_mode": "strict",
            "thread_count": 1,
        },
        "execution": {
            "warmup_runs": 1,
            "measurement_runs": 5,
            "seed": 42,
        },
        "results": {
            "inference.text_generation": {
                "ttft_ms": {"mean": 10.0, "std": 0.0},
                "throughput_tokens_per_sec": {"mean": 100.0, "std": 0.0},
                "memory_peak_mb": 128,
            },
            "determinism.reproducibility": {
                "hash_consistency": 1.0,
                "statistical_variance": 0.0,
            },
        },
        "validation": {
            "determinism_check": "passed",
            "outlier_detection": "none",
        },
    }


def validate_report(report: dict[str, Any]) -> tuple[bool, list[str]]:
    errs: list[str] = []
    for key in ("metadata", "environment", "execution", "results", "validation"):
        if key not in report:
            errs.append(f"missing top-level key: {key}")

    exec_cfg = report.get("execution", {})
    if int(exec_cfg.get("measurement_runs", 0)) < 3:
        errs.append("measurement_runs must be >= 3")
    if int(exec_cfg.get("warmup_runs", 0)) < 1:
        errs.append("warmup_runs must be >= 1")

    results = report.get("results", {})
    if "inference.text_generation" not in results:
        errs.append("missing required benchmark result: inference.text_generation")
    if "determinism.reproducibility" not in results:
        errs.append("missing required benchmark result: determinism.reproducibility")

    return len(errs) == 0, errs


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI benchmark report contract (RFC-00A2 baseline).")
    p.add_argument("--out-dir", required=True)
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    report_a = build_report()
    report_b = build_report()
    hash_a = sha256_text(canonical_json(report_a))
    hash_b = sha256_text(canonical_json(report_b))
    deterministic = hash_a == hash_b

    valid, errors = validate_report(report_a)
    status = "pass" if deterministic and valid else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_report_hash": deterministic,
        "report_sha256": hash_a,
        "valid": valid,
        "errors": errors,
        "report": report_a,
    }
    json_path = out_dir / "ai_benchmark_spec_contract.json"
    md_path = out_dir / "ai_benchmark_spec_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Benchmark Spec Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_report_hash: `{deterministic}`",
                f"- report_sha256: `{hash_a}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai benchmark spec status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
