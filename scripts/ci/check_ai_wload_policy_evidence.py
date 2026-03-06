#!/usr/bin/env python3
"""Track RFC-0026 WLOAD runtime/policy evidence readiness from policy artifacts."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.wload-policy-evidence.v1"
EXPECTATION_SCHEMA_VERSION = "t81.ai.wload-policy-evidence-expectations.v1"
DEFAULT_MISSING_REASON = "WLOAD runtime/policy gate is not part of policy trace evidence yet."


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Track RFC-0026 WLOAD policy-evidence readiness.")
    p.add_argument("--policy-contract", required=True, help="Path to ai_policy_event_contract.json")
    p.add_argument("--runtime-trace", required=True, help="Path to ai_runtime_trace.json")
    p.add_argument(
        "--expectations-file",
        default="",
        help="Optional WLOAD policy-evidence expectation contract JSON.",
    )
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def collect_reason_codes(policy_contract: dict[str, Any], runtime_trace: dict[str, Any]) -> list[str]:
    reason_codes: list[str] = []
    trace_rows = policy_contract.get("trace")
    if isinstance(trace_rows, list):
        for row in trace_rows:
            if not isinstance(row, dict):
                continue
            reason = str(row.get("reason_code", "")).strip()
            if reason:
                reason_codes.append(reason)
    runtime_reason = str(runtime_trace.get("reason_code", "")).strip()
    if runtime_reason:
        reason_codes.append(runtime_reason)
    return sorted(set(reason_codes))


def main() -> int:
    args = parse_args()
    policy_contract_path = Path(args.policy_contract).resolve()
    runtime_trace_path = Path(args.runtime_trace).resolve()
    expectations_path = (
        Path(args.expectations_file).resolve()
        if args.expectations_file
        else Path(__file__).resolve().with_name("ai_wload_policy_evidence_expectations.json")
    )
    out_json = Path(args.out_json).resolve()
    out_md = out_json.with_suffix(".md")

    errors: list[str] = []
    if not policy_contract_path.exists():
        errors.append(f"missing policy contract: {policy_contract_path}")
    if not runtime_trace_path.exists():
        errors.append(f"missing runtime trace: {runtime_trace_path}")
    if not expectations_path.exists():
        errors.append(f"missing expectations file: {expectations_path}")
    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    policy_contract = parse_json(policy_contract_path)
    runtime_trace = parse_json(runtime_trace_path)
    expectations = parse_json(expectations_path)

    reason_codes = collect_reason_codes(policy_contract, runtime_trace)
    observed_wload_codes = sorted([code for code in reason_codes if "WLOAD" in code.upper()])
    wload_policy_gate_ready = bool(observed_wload_codes)
    wload_policy_gate_reason = "" if wload_policy_gate_ready else DEFAULT_MISSING_REASON

    expectation_results: dict[str, Any] = {}
    if expectations.get("schema") != EXPECTATION_SCHEMA_VERSION:
        errors.append(
            "wload policy evidence expectations schema mismatch: "
            f"{expectations.get('schema')} != {EXPECTATION_SCHEMA_VERSION}"
        )
    else:
        expected_ready = bool(expectations.get("expected_wload_policy_gate_ready", False))
        required_codes_raw = expectations.get("required_observed_wload_reason_codes", [])
        required_codes = (
            [str(item).strip() for item in required_codes_raw if str(item).strip()]
            if isinstance(required_codes_raw, list)
            else []
        )
        missing_codes = [code for code in required_codes if code not in observed_wload_codes]
        if wload_policy_gate_ready != expected_ready:
            errors.append(
                "wload policy gate readiness mismatch: "
                f"expected={str(expected_ready).lower()}, observed={str(wload_policy_gate_ready).lower()}"
            )
        if missing_codes:
            errors.append("missing required WLOAD reason codes: " + ", ".join(missing_codes))
        expectation_results = {
            "expected_wload_policy_gate_ready": expected_ready,
            "required_observed_wload_reason_codes": required_codes,
            "missing_required_observed_wload_reason_codes": missing_codes,
            "match": (wload_policy_gate_ready == expected_ready) and not missing_codes,
        }

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "policy_contract": str(policy_contract_path),
        "runtime_trace": str(runtime_trace_path),
        "expectations_file": str(expectations_path),
        "wload_policy_gate_ready": wload_policy_gate_ready,
        "wload_policy_gate_reason": wload_policy_gate_reason,
        "observed_reason_codes": reason_codes,
        "observed_wload_reason_codes": observed_wload_codes,
        "expectation_results": expectation_results,
        "errors": errors,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# WLOAD Policy Evidence",
        "",
        f"- schema: `{SCHEMA_VERSION}`",
        f"- status: `{status}`",
        f"- wload_policy_gate_ready: `{str(wload_policy_gate_ready).lower()}`",
    ]
    if wload_policy_gate_reason:
        lines.append(f"- wload_policy_gate_reason: `{wload_policy_gate_reason}`")
    if expectation_results:
        lines.extend(
            [
                "- expectation_match: "
                f"`{str(bool(expectation_results.get('match', False))).lower()}`",
                "- expected_wload_policy_gate_ready: "
                f"`{str(bool(expectation_results.get('expected_wload_policy_gate_ready', False))).lower()}`",
            ]
        )
    if observed_wload_codes:
        lines.append(f"- observed_wload_reason_codes: `{', '.join(observed_wload_codes)}`")
    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai wload policy evidence status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
