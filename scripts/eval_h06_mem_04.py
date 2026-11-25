#!/usr/bin/env python3
"""Evaluate H06-MEM-04 multi-batch benchmark results against schema, determinism, and regression gates."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SCHEMA = REPO_ROOT / "benchmarks/h06_mem_04/h06_mem_04_bench.schema.json"


def _check_type(val: Any, expected: str | list[str], path: str) -> None:
    allowed = expected if isinstance(expected, list) else [expected]
    for typ in allowed:
        if typ == "null" and val is None:
            return
        if typ == "boolean" and isinstance(val, bool):
            return
        if typ == "integer" and isinstance(val, int) and not isinstance(val, bool):
            return
        if typ == "number" and isinstance(val, (int, float)) and not isinstance(val, bool):
            return
        if typ == "string" and isinstance(val, str):
            return
        if typ == "array" and isinstance(val, list):
            return
        if typ == "object" and isinstance(val, dict):
            return
    raise ValueError(f"{path}: expected type {allowed}, got {type(val).__name__}")


def validate_schema(instance: Any, schema: dict[str, Any], path: str = "$") -> None:
    if "enum" in schema and instance not in schema["enum"]:
        raise ValueError(f"{path}: {instance!r} not in enum {schema['enum']!r}")
    if "type" in schema:
        _check_type(instance, schema["type"], path)
    if isinstance(instance, str) and "minLength" in schema and len(instance) < schema["minLength"]:
        raise ValueError(f"{path}: string shorter than {schema['minLength']}")
    if isinstance(instance, (int, float)) and not isinstance(instance, bool):
        if "minimum" in schema and instance < schema["minimum"]:
            raise ValueError(f"{path}: {instance} < minimum {schema['minimum']}")
        if "maximum" in schema and instance > schema["maximum"]:
            raise ValueError(f"{path}: {instance} > maximum {schema['maximum']}")
    if isinstance(instance, list):
        if "minItems" in schema and len(instance) < schema["minItems"]:
            raise ValueError(f"{path}: array length {len(instance)} < minItems {schema['minItems']}")
        if "items" in schema:
            for i, item in enumerate(instance):
                validate_schema(item, schema["items"], f"{path}[{i}]")
    if isinstance(instance, dict):
        props = schema.get("properties", {})
        for req in schema.get("required", []):
            if req not in instance:
                raise ValueError(f"{path}: missing required field {req!r}")
        if schema.get("additionalProperties") is False:
            extra = set(instance.keys()) - set(props.keys())
            if extra:
                raise ValueError(f"{path}: unexpected additionalProperties {sorted(extra)}")
        for k, v in instance.items():
            if k in props:
                validate_schema(v, props[k], f"{path}.{k}")


def check_determinism(results: list[dict[str, Any]]) -> list[str]:
    by_cell: dict[tuple[str, int, str], dict[str, str]] = {}
    for row in results:
        key = (row["tier"], int(row["batch_size"]), row["kernel"])
        by_cell.setdefault(key, {})[row["backend"]] = row["determinism_hash"]

    failures: list[str] = []
    for key, backend_hashes in sorted(by_cell.items()):
        ref = backend_hashes.get("cpu_simd_1t")
        if ref is None:
            failures.append(f"{key}: missing cpu_simd_1t reference hash")
            continue
        for backend, digest in sorted(backend_hashes.items()):
            if digest != ref:
                failures.append(
                    f"{key}: backend {backend} hash {digest} != cpu_simd_1t {ref}"
                )
    return failures


def check_regressions(
    current: list[dict[str, Any]],
    baseline: list[dict[str, Any]],
    max_p95_reg_pct: float,
    min_tput_ratio: float,
) -> list[str]:
    base_map = {
        (r["tier"], int(r["batch_size"]), r["kernel"], r["backend"]): r for r in baseline
    }
    failures: list[str] = []
    for row in current:
        key = (row["tier"], int(row["batch_size"]), row["kernel"], row["backend"])
        b = base_map.get(key)
        if not b:
            continue
        b_p95 = float(b["p95_us"])
        c_p95 = float(row["p95_us"])
        if b_p95 > 0.0:
            reg_pct = ((c_p95 - b_p95) / b_p95) * 100.0
            if reg_pct > max_p95_reg_pct:
                failures.append(
                    f"{key}: p95_us regressed {reg_pct:.2f}% ({b_p95:.2f} -> {c_p95:.2f} us > {max_p95_reg_pct:.2f}%)"
                )
        b_gops = float(b["gops_per_sec"])
        c_gops = float(row["gops_per_sec"])
        if b_gops > 0.0:
            ratio = c_gops / b_gops
            if ratio < min_tput_ratio:
                failures.append(
                    f"{key}: throughput ratio {ratio:.3f} < {min_tput_ratio:.3f}"
                )
    return failures


def render_markdown(
    data: dict[str, Any], det_ok: bool, baseline_checked: bool, reg_failures: list[str]
) -> str:
    if not baseline_checked:
        reg_status = "NOT RUN (no --baseline provided)"
    elif not reg_failures:
        reg_status = "PASS"
    else:
        reg_status = f"FAIL ({len(reg_failures)} violations)"

    lines = [
        f"## H06-MEM-04 Benchmark Summary — `{data['benchmark_id']}`",
        "",
        f"- **Hardware:** {data['hardware_profile']['name']} (`{data['hardware_profile']['arch']}`, {data['hardware_profile']['physical_cores']} cores)",
        f"- **Determinism Gate:** {'PASS (bit-exact across all backends)' if det_ok else 'FAIL'}",
        f"- **Regression Gate:** {reg_status}",
        "",
        "| Tier | Batch | Kernel | Backend | Regime | p50 (µs) | p95 (µs) | GOPS/s | GB/s | Hash |",
        "| :--- | :---: | :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- |",
    ]
    for r in data.get("results", []):
        if r["backend"] in ("auto_dispatch", "cpu_simd_1t"):
            lines.append(
                f"| {r['tier']} | {r['batch_size']} | {r['kernel']} | {r['backend']} | "
                f"{r['regime_selected']} | {r['p50_us']:.2f} | {r['p95_us']:.2f} | "
                f"{r['gops_per_sec']:.3f} | {r['effective_gb_per_sec']:.3f} | `{r['determinism_hash']}` |"
            )
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Evaluate H06-MEM-04 benchmark JSON output.")
    ap.add_argument("--results", required=True, help="Path to benchmark results JSON")
    ap.add_argument("--schema", default=str(DEFAULT_SCHEMA), help="Path to JSON schema")
    ap.add_argument("--baseline", default="", help="Optional baseline JSON for regression checks")
    ap.add_argument("--markdown-out", default="", help="Optional path to write Markdown summary")
    args = ap.parse_args()

    schema = json.loads(Path(args.schema).read_text(encoding="utf-8"))
    data = json.loads(Path(args.results).read_text(encoding="utf-8"))
    validate_schema(data, schema)

    results = data.get("results", [])
    if not results:
        print("ERROR: results array is empty", file=sys.stderr)
        return 1

    gates = data["regression_gates"]
    det_failures: list[str] = []
    if gates.get("require_exact_determinism", True):
        det_failures = check_determinism(results)

    baseline_checked = bool(args.baseline)
    reg_failures: list[str] = []
    if baseline_checked:
        base_data = json.loads(Path(args.baseline).read_text(encoding="utf-8"))
        validate_schema(base_data, schema)
        reg_failures = check_regressions(
            results,
            base_data.get("results", []),
            float(gates["max_p95_regression_pct"]),
            float(gates["min_throughput_ratio"]),
        )

    md = render_markdown(data, len(det_failures) == 0, baseline_checked, reg_failures)
    if args.markdown_out:
        Path(args.markdown_out).write_text(md, encoding="utf-8")
    else:
        print(md)

    if det_failures:
        for f in det_failures:
            print(f"DETERMINISM FAIL: {f}", file=sys.stderr)
        return 1
    if reg_failures:
        for f in reg_failures:
            print(f"REGRESSION FAIL: {f}", file=sys.stderr)
        return 1

    print(f"H06-MEM-04 EVAL PASSED: {len(results)} cells verified bit-exact and schema-valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
