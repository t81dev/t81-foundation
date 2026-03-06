#!/usr/bin/env python3
"""Build RFC-0026 runtime-readiness tracker from CI evidence artifacts."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.rfc0026-readiness.v1"


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Emit RFC-0026 runtime readiness tracker artifact.")
    p.add_argument("--opcode-report", required=True, help="Path to ai_opcode_runtime_report.json")
    p.add_argument("--benchmark-capability-matrix", required=True, help="Path to ai_benchmark_capability_matrix.json")
    p.add_argument("--out-dir", required=True, help="Output directory")
    return p.parse_args()


def find_matrix_state(matrix: list[dict[str, Any]], fmt: str, mode: str) -> str:
    for item in matrix:
        if str(item.get("format", "")).strip() == fmt and str(item.get("mode", "")).strip() == mode:
            return str(item.get("support_state", "unknown")).strip() or "unknown"
    return "missing"


def main() -> int:
    args = parse_args()
    opcode_path = Path(args.opcode_report).resolve()
    matrix_path = Path(args.benchmark_capability_matrix).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    errors: list[str] = []

    if not opcode_path.exists():
        errors.append(f"missing opcode report: {opcode_path}")
    if not matrix_path.exists():
        errors.append(f"missing benchmark capability matrix: {matrix_path}")
    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    opcode = parse_json(opcode_path)
    matrix_payload = parse_json(matrix_path)

    phase_status = str(opcode.get("phase_status", "unknown")).strip() or "unknown"
    opcodes = opcode.get("opcodes")
    if not isinstance(opcodes, list):
        errors.append("opcode report missing opcodes[]")
        opcodes = []

    qmatmul_runtime_ready = False
    for item in opcodes:
        if not isinstance(item, dict):
            continue
        if str(item.get("opcode", "")).strip() == "QMATMUL":
            qmatmul_runtime_ready = bool(item.get("runtime_ready", False))
            break

    wload_policy_gate_ready = False
    wload_policy_gate_reason = (
        "WLOAD runtime/policy gate is not part of phase-1 opcode conformance evidence yet."
    )

    matrix = matrix_payload.get("matrix")
    if not isinstance(matrix, list):
        errors.append("benchmark capability matrix missing matrix[]")
        matrix = []
    gguf_state = find_matrix_state(matrix, "gguf", "strict_deterministic")
    t3k_state = find_matrix_state(matrix, "t3k", "strict_deterministic")
    t3k_benchmark_supported = t3k_state == "supported"

    if gguf_state != "supported":
        errors.append(f"required benchmark lane gguf:strict_deterministic unsupported (state={gguf_state})")
    if t3k_benchmark_supported and not qmatmul_runtime_ready:
        errors.append(
            "inconsistent readiness: t3k benchmark lane is supported but QMATMUL runtime evidence is not ready"
        )

    overall_ready = qmatmul_runtime_ready and wload_policy_gate_ready and t3k_benchmark_supported
    readiness_state = "ready" if overall_ready else "blocked"
    blockers: list[str] = []
    if not qmatmul_runtime_ready:
        blockers.append("QMATMUL runtime evidence not ready")
    if not wload_policy_gate_ready:
        blockers.append("WLOAD policy-gate runtime evidence missing")
    if not t3k_benchmark_supported:
        blockers.append("t3k benchmark lane unsupported")

    gate_status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "gate_status": gate_status,
        "readiness_state": readiness_state,
        "source_rfc": "RFC-0026",
        "evidence": {
            "opcode_report": str(opcode_path),
            "benchmark_capability_matrix": str(matrix_path),
        },
        "signals": {
            "phase_status": phase_status,
            "qmatmul_runtime_ready": qmatmul_runtime_ready,
            "wload_policy_gate_ready": wload_policy_gate_ready,
            "wload_policy_gate_reason": wload_policy_gate_reason,
            "benchmark_lanes": {
                "gguf:strict_deterministic": gguf_state,
                "t3k:strict_deterministic": t3k_state,
            },
            "t3k_benchmark_supported": t3k_benchmark_supported,
        },
        "blockers": blockers,
        "errors": errors,
    }

    json_path = out_dir / "ai_rfc0026_readiness.json"
    md_path = out_dir / "ai_rfc0026_readiness.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# RFC-0026 Runtime Readiness",
        "",
        f"- schema: `{SCHEMA_VERSION}`",
        f"- gate_status: `{gate_status}`",
        f"- readiness_state: `{readiness_state}`",
        "",
        "| Signal | Value |",
        "| :--- | :--- |",
        f"| `phase_status` | `{phase_status}` |",
        f"| `qmatmul_runtime_ready` | `{str(qmatmul_runtime_ready).lower()}` |",
        f"| `wload_policy_gate_ready` | `{str(wload_policy_gate_ready).lower()}` |",
        f"| `gguf:strict_deterministic` | `{gguf_state}` |",
        f"| `t3k:strict_deterministic` | `{t3k_state}` |",
        "",
    ]
    if blockers:
        lines.append("Blockers:")
        for item in blockers:
            lines.append(f"- {item}")
        lines.append("")
    if errors:
        lines.append("Errors:")
        for item in errors:
            lines.append(f"- {item}")
        lines.append("")
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai rfc0026 readiness gate status: {gate_status}")
    print(f"rfc0026 readiness state: {readiness_state}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if gate_status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
