#!/usr/bin/env python3
"""Build RFC-0026 runtime-readiness tracker from CI evidence artifacts."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.rfc0026-readiness.v1"
EXPECTATION_SCHEMA_VERSION = "t81.ai.rfc0026-readiness-expectations.v1"


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Emit RFC-0026 runtime readiness tracker artifact.")
    p.add_argument("--opcode-report", required=True, help="Path to ai_opcode_runtime_report.json")
    p.add_argument("--benchmark-capability-matrix", required=True, help="Path to ai_benchmark_capability_matrix.json")
    p.add_argument("--inference-capability-matrix", required=True, help="Path to ai_inference_capability_matrix.json")
    p.add_argument("--runtime-capability-alignment", required=True, help="Path to ai_runtime_capability_alignment.json")
    p.add_argument("--wload-policy-evidence-report", required=True, help="Path to ai_wload_policy_evidence.json")
    p.add_argument(
        "--opcode-baseline-approval-report",
        required=True,
        help="Path to ai_opcode_baseline_approval_report.json",
    )
    p.add_argument(
        "--expectations-file",
        default="",
        help="Optional readiness expectation contract JSON.",
    )
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
    benchmark_matrix_path = Path(args.benchmark_capability_matrix).resolve()
    inference_matrix_path = Path(args.inference_capability_matrix).resolve()
    alignment_path = Path(args.runtime_capability_alignment).resolve()
    wload_policy_evidence_path = Path(args.wload_policy_evidence_report).resolve()
    opcode_baseline_approval_path = Path(args.opcode_baseline_approval_report).resolve()
    expectations_path = (
        Path(args.expectations_file).resolve()
        if args.expectations_file
        else Path(__file__).resolve().with_name("ai_rfc0026_readiness_expectations.json")
    )
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    errors: list[str] = []

    if not opcode_path.exists():
        errors.append(f"missing opcode report: {opcode_path}")
    if not benchmark_matrix_path.exists():
        errors.append(f"missing benchmark capability matrix: {benchmark_matrix_path}")
    if not inference_matrix_path.exists():
        errors.append(f"missing inference capability matrix: {inference_matrix_path}")
    if not alignment_path.exists():
        errors.append(f"missing runtime capability alignment report: {alignment_path}")
    if not wload_policy_evidence_path.exists():
        errors.append(f"missing wload policy evidence report: {wload_policy_evidence_path}")
    if not opcode_baseline_approval_path.exists():
        errors.append(f"missing opcode baseline approval report: {opcode_baseline_approval_path}")
    if not expectations_path.exists():
        errors.append(f"missing readiness expectations file: {expectations_path}")
    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    opcode = parse_json(opcode_path)
    benchmark_matrix_payload = parse_json(benchmark_matrix_path)
    inference_matrix_payload = parse_json(inference_matrix_path)
    alignment_payload = parse_json(alignment_path)
    wload_policy_evidence_payload = parse_json(wload_policy_evidence_path)
    opcode_baseline_approval_payload = parse_json(opcode_baseline_approval_path)
    expectations_payload = parse_json(expectations_path)

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

    wload_policy_evidence_status = (
        str(wload_policy_evidence_payload.get("status", "unknown")).strip() or "unknown"
    )
    if wload_policy_evidence_status != "pass":
        errors.append(f"wload policy evidence gate failed (status={wload_policy_evidence_status})")
    wload_policy_gate_ready = bool(wload_policy_evidence_payload.get("wload_policy_gate_ready", False))
    wload_policy_gate_reason = (
        str(wload_policy_evidence_payload.get("wload_policy_gate_reason", "")).strip()
        or "wload policy evidence report did not include reason"
    )

    benchmark_matrix = benchmark_matrix_payload.get("matrix")
    if not isinstance(benchmark_matrix, list):
        errors.append("benchmark capability matrix missing matrix[]")
        benchmark_matrix = []
    benchmark_gguf_state = find_matrix_state(benchmark_matrix, "gguf", "strict_deterministic")
    benchmark_t3k_state = find_matrix_state(benchmark_matrix, "t3k", "strict_deterministic")
    t3k_benchmark_supported = benchmark_t3k_state == "supported"

    inference_matrix = inference_matrix_payload.get("matrix")
    if not isinstance(inference_matrix, list):
        errors.append("inference capability matrix missing matrix[]")
        inference_matrix = []
    inference_gguf_state = find_matrix_state(inference_matrix, "gguf", "strict_deterministic")
    inference_t3k_state = find_matrix_state(inference_matrix, "t3k", "strict_deterministic")
    t3k_inference_supported = inference_t3k_state == "supported"

    if benchmark_gguf_state != "supported":
        errors.append(
            "required benchmark lane gguf:strict_deterministic unsupported "
            f"(state={benchmark_gguf_state})"
        )
    if inference_gguf_state != "supported":
        errors.append(
            "required inference lane gguf:strict_deterministic unsupported "
            f"(state={inference_gguf_state})"
        )
    if t3k_benchmark_supported and not qmatmul_runtime_ready:
        errors.append(
            "inconsistent readiness: t3k benchmark lane is supported but QMATMUL runtime evidence is not ready"
        )
    if t3k_inference_supported and not qmatmul_runtime_ready:
        errors.append(
            "inconsistent readiness: t3k inference lane is supported but QMATMUL runtime evidence is not ready"
        )
    if t3k_benchmark_supported != t3k_inference_supported:
        errors.append(
            "inconsistent readiness: t3k benchmark and inference support states diverged "
            f"(benchmark={benchmark_t3k_state}, inference={inference_t3k_state})"
        )
    alignment_status = str(alignment_payload.get("status", "unknown")).strip() or "unknown"
    if alignment_status != "pass":
        errors.append(f"runtime capability alignment gate failed (status={alignment_status})")
    opcode_baseline_approval_status = (
        str(opcode_baseline_approval_payload.get("status", "unknown")).strip() or "unknown"
    )
    if opcode_baseline_approval_status != "pass":
        errors.append(
            "opcode baseline approval policy gate failed "
            f"(status={opcode_baseline_approval_status})"
        )

    overall_ready = (
        qmatmul_runtime_ready
        and wload_policy_gate_ready
        and t3k_benchmark_supported
        and t3k_inference_supported
    )
    readiness_state = "ready" if overall_ready else "blocked"
    blockers: list[str] = []
    if not qmatmul_runtime_ready:
        blockers.append("QMATMUL runtime evidence not ready")
    if not wload_policy_gate_ready:
        blockers.append("WLOAD policy-gate runtime evidence missing")
    if not t3k_benchmark_supported:
        blockers.append("t3k benchmark lane unsupported")
    if not t3k_inference_supported:
        blockers.append("t3k inference lane unsupported")

    gate_status = "pass" if not errors else "fail"
    expectation_results: dict[str, Any] = {}
    expected_readiness_state = ""
    expected_wload_policy_evidence_status = ""
    expected_wload_policy_gate_ready: bool | None = None
    expected_blockers: list[str] = []
    if expectations_payload.get("schema") != EXPECTATION_SCHEMA_VERSION:
        errors.append(
            f"readiness expectations schema mismatch: "
            f"{expectations_payload.get('schema')} != {EXPECTATION_SCHEMA_VERSION}"
        )
    else:
        expected_readiness_state = str(expectations_payload.get("expected_readiness_state", "")).strip()
        expected_wload_policy_evidence_status = str(
            expectations_payload.get("expected_wload_policy_evidence_status", "")
        ).strip()
        if "expected_wload_policy_gate_ready" in expectations_payload:
            expected_wload_policy_gate_ready = bool(
                expectations_payload.get("expected_wload_policy_gate_ready")
            )
        blockers_raw = expectations_payload.get("required_blockers", [])
        if isinstance(blockers_raw, list):
            expected_blockers = [str(item).strip() for item in blockers_raw if str(item).strip()]
        if expected_readiness_state and readiness_state != expected_readiness_state:
            errors.append(
                f"readiness state mismatch: expected={expected_readiness_state}, observed={readiness_state}"
            )
        if (
            expected_wload_policy_evidence_status
            and wload_policy_evidence_status != expected_wload_policy_evidence_status
        ):
            errors.append(
                "wload policy evidence status mismatch: "
                f"expected={expected_wload_policy_evidence_status}, observed={wload_policy_evidence_status}"
            )
        if (
            expected_wload_policy_gate_ready is not None
            and wload_policy_gate_ready != expected_wload_policy_gate_ready
        ):
            errors.append(
                "wload policy gate readiness mismatch: "
                f"expected={str(expected_wload_policy_gate_ready).lower()}, "
                f"observed={str(wload_policy_gate_ready).lower()}"
            )
        missing_required_blockers = [item for item in expected_blockers if item not in blockers]
        unexpected_blockers = [item for item in blockers if item not in expected_blockers]
        if missing_required_blockers:
            errors.append(
                "missing required readiness blockers: " + ", ".join(missing_required_blockers)
            )
        if unexpected_blockers:
            errors.append(
                "unexpected readiness blockers detected: " + ", ".join(unexpected_blockers)
            )
        expectation_results = {
            "expected_readiness_state": expected_readiness_state,
            "expected_wload_policy_evidence_status": expected_wload_policy_evidence_status,
            "expected_wload_policy_gate_ready": expected_wload_policy_gate_ready,
            "required_blockers": expected_blockers,
            "missing_required_blockers": missing_required_blockers,
            "unexpected_blockers": unexpected_blockers,
            "match": not missing_required_blockers and not unexpected_blockers
            and (not expected_readiness_state or readiness_state == expected_readiness_state)
            and (
                not expected_wload_policy_evidence_status
                or wload_policy_evidence_status == expected_wload_policy_evidence_status
            )
            and (
                expected_wload_policy_gate_ready is None
                or wload_policy_gate_ready == expected_wload_policy_gate_ready
            ),
        }

    gate_status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "gate_status": gate_status,
        "readiness_state": readiness_state,
        "source_rfc": "RFC-0026",
        "evidence": {
            "opcode_report": str(opcode_path),
            "benchmark_capability_matrix": str(benchmark_matrix_path),
            "inference_capability_matrix": str(inference_matrix_path),
            "runtime_capability_alignment": str(alignment_path),
            "wload_policy_evidence_report": str(wload_policy_evidence_path),
            "opcode_baseline_approval_report": str(opcode_baseline_approval_path),
            "expectations_file": str(expectations_path),
        },
        "signals": {
            "phase_status": phase_status,
            "qmatmul_runtime_ready": qmatmul_runtime_ready,
            "wload_policy_gate_ready": wload_policy_gate_ready,
            "wload_policy_gate_reason": wload_policy_gate_reason,
            "benchmark_lanes": {
                "gguf:strict_deterministic": benchmark_gguf_state,
                "t3k:strict_deterministic": benchmark_t3k_state,
            },
            "inference_lanes": {
                "gguf:strict_deterministic": inference_gguf_state,
                "t3k:strict_deterministic": inference_t3k_state,
            },
            "t3k_benchmark_supported": t3k_benchmark_supported,
            "t3k_inference_supported": t3k_inference_supported,
            "runtime_capability_alignment_status": alignment_status,
            "wload_policy_evidence_status": wload_policy_evidence_status,
            "opcode_baseline_approval_status": opcode_baseline_approval_status,
        },
        "expectation_results": expectation_results,
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
        f"| `benchmark gguf:strict_deterministic` | `{benchmark_gguf_state}` |",
        f"| `benchmark t3k:strict_deterministic` | `{benchmark_t3k_state}` |",
        f"| `inference gguf:strict_deterministic` | `{inference_gguf_state}` |",
        f"| `inference t3k:strict_deterministic` | `{inference_t3k_state}` |",
        f"| `runtime_capability_alignment_status` | `{alignment_status}` |",
        f"| `wload_policy_evidence_status` | `{wload_policy_evidence_status}` |",
        f"| `opcode_baseline_approval_status` | `{opcode_baseline_approval_status}` |",
        "",
    ]
    if expectation_results:
        lines.extend(
            [
                "Expectation Contract:",
                f"- expected_readiness_state: `{expectation_results.get('expected_readiness_state', '')}`",
                "- expected_wload_policy_evidence_status: "
                f"`{expectation_results.get('expected_wload_policy_evidence_status', '')}`",
                "- expected_wload_policy_gate_ready: "
                f"`{str(expectation_results.get('expected_wload_policy_gate_ready', False)).lower()}`",
                f"- required_blockers: `{', '.join(expectation_results.get('required_blockers', []))}`",
                f"- match: `{str(expectation_results.get('match', False)).lower()}`",
                "",
            ]
        )
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
