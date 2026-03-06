#!/usr/bin/env python3
"""Enforce alignment between benchmark and inference capability matrices."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.runtime-capability-alignment.v1"


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate benchmark/inference capability alignment.")
    p.add_argument("--benchmark-matrix", required=True, help="Path to ai_benchmark_capability_matrix.json")
    p.add_argument("--inference-matrix", required=True, help="Path to ai_inference_capability_matrix.json")
    p.add_argument(
        "--required-pairs",
        default="gguf:strict_deterministic,t3k:strict_deterministic",
        help="Comma-separated format:mode pairs that must align across runtime lanes.",
    )
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def find_state(matrix: list[dict[str, Any]], fmt: str, mode: str) -> str:
    for item in matrix:
        if str(item.get("format", "")).strip() == fmt and str(item.get("mode", "")).strip() == mode:
            return str(item.get("support_state", "unknown")).strip() or "unknown"
    return "missing"


def main() -> int:
    args = parse_args()
    bench_path = Path(args.benchmark_matrix).resolve()
    infer_path = Path(args.inference_matrix).resolve()
    out_json = Path(args.out_json).resolve()

    errors: list[str] = []
    warnings: list[str] = []

    if not bench_path.exists():
        errors.append(f"missing benchmark matrix: {bench_path}")
    if not infer_path.exists():
        errors.append(f"missing inference matrix: {infer_path}")
    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    bench = parse_json(bench_path)
    infer = parse_json(infer_path)
    bench_rows = bench.get("matrix") if isinstance(bench.get("matrix"), list) else []
    infer_rows = infer.get("matrix") if isinstance(infer.get("matrix"), list) else []

    required_pairs: list[tuple[str, str]] = []
    for raw in str(args.required_pairs).split(","):
        pair = raw.strip()
        if not pair:
            continue
        if ":" not in pair:
            warnings.append(f"ignored malformed required pair: {pair}")
            continue
        fmt, mode = pair.split(":", 1)
        required_pairs.append((fmt.strip(), mode.strip()))

    alignment_rows: list[dict[str, Any]] = []
    for fmt, mode in required_pairs:
        bench_state = find_state(bench_rows, fmt, mode)
        infer_state = find_state(infer_rows, fmt, mode)
        aligned = bench_state == infer_state
        alignment_rows.append(
            {
                "format": fmt,
                "mode": mode,
                "benchmark_state": bench_state,
                "inference_state": infer_state,
                "aligned": aligned,
            }
        )
        if not aligned:
            errors.append(
                f"runtime capability mismatch for {fmt}:{mode} "
                f"(benchmark={bench_state}, inference={infer_state})"
            )

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "benchmark_matrix": str(bench_path),
        "inference_matrix": str(infer_path),
        "required_pairs": [f"{fmt}:{mode}" for fmt, mode in required_pairs],
        "alignment": alignment_rows,
        "errors": errors,
        "warnings": warnings,
    }

    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai runtime capability alignment status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
