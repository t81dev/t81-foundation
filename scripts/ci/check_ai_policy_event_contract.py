#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.policy-events.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def evaluate_event(policy: dict[str, Any], event: dict[str, Any]) -> dict[str, str]:
    event_type = event["event_type"]

    if event_type == "model_load":
        model_hash = event.get("model_hash", "")
        if model_hash in policy["allowed_model_hashes"]:
            return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_MODEL_HASH_MATCH"}
        return {"decision": "deny", "reason_code": "AI_POLICY_DENY_MODEL_HASH_NOT_ALLOWED"}

    if event_type == "inference_start":
        tokens = int(event.get("requested_tokens", 0))
        if tokens <= int(policy["max_tokens"]):
            return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_INFERENCE_BUDGET_OK"}
        return {"decision": "deny", "reason_code": "AI_POLICY_DENY_INFERENCE_BUDGET_EXCEEDED"}

    if event_type == "tool_use":
        tool = event.get("tool_name", "")
        if tool in policy["allowed_tools"]:
            return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_TOOL_WHITELISTED"}
        return {"decision": "deny", "reason_code": "AI_POLICY_DENY_TOOL_NOT_WHITELISTED"}

    if event_type == "resource_allocate":
        mb = int(event.get("memory_mb", 0))
        if mb <= int(policy["max_memory_mb"]):
            return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_RESOURCE_WITHIN_LIMIT"}
        return {"decision": "deny", "reason_code": "AI_POLICY_DENY_RESOURCE_LIMIT_EXCEEDED"}

    if event_type == "content_emit":
        severity = str(event.get("safety_severity", "low"))
        blocked = set(policy["blocked_severities"])
        if severity in blocked:
            return {"decision": "deny", "reason_code": "AI_POLICY_DENY_CONTENT_SAFETY_BLOCKED"}
        return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_CONTENT_SAFETY_OK"}

    return {"decision": "deny", "reason_code": "AI_POLICY_DENY_UNKNOWN_EVENT_TYPE"}


def run_trace(policy: dict[str, Any], events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for idx, event in enumerate(events):
        verdict = evaluate_event(policy, event)
        out.append(
            {
                "index": idx,
                "event_type": event["event_type"],
                "decision": verdict["decision"],
                "reason_code": verdict["reason_code"],
            }
        )
    return out


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_runtime_trace_binding(
    runtime_trace_path: Path, ai_bin: Path | None
) -> tuple[bool, list[str], dict[str, Any]]:
    errs: list[str] = []
    binding: dict[str, Any] = {"runtime_trace_path": str(runtime_trace_path)}

    if not runtime_trace_path.exists() and ai_bin is not None:
        proc = subprocess.run(
            [str(ai_bin), "observability", "trace", str(runtime_trace_path)],
            capture_output=True,
            text=True,
            check=False,
        )
        binding["runtime_trace_emission"] = {
            "rc": proc.returncode,
            "stdout_sha256": sha256_text(proc.stdout),
            "stderr_sha256": sha256_text(proc.stderr),
        }
        if proc.returncode != 0:
            errs.append("failed to emit runtime trace via ai_bin observability trace")

    if not runtime_trace_path.exists():
        errs.append(f"runtime trace artifact missing: {runtime_trace_path}")
        return False, errs, binding

    trace = parse_json(runtime_trace_path)
    required = {"reason_code", "event_type", "decision", "timestamp_utc", "trace_sha256"}
    missing = sorted(required - set(trace.keys()))
    if missing:
        errs.append(f"runtime trace missing fields: {', '.join(missing)}")
    binding["runtime_trace_sha256"] = sha256_text(canonical_json(trace))
    binding["runtime_trace"] = trace

    event_type = str(trace.get("event_type", ""))
    decision = str(trace.get("decision", ""))
    reason_code = str(trace.get("reason_code", ""))
    if event_type not in {
        "model_load",
        "inference_start",
        "tool_use",
        "resource_allocate",
        "content_emit",
    }:
        errs.append(f"runtime trace event_type not recognized by policy contract: {event_type}")
    if decision not in {"allow", "deny"}:
        errs.append(f"runtime trace decision must be allow/deny (got {decision})")
    if reason_code == "":
        errs.append("runtime trace reason_code must be non-empty")
    alias_map = {
        "ALLOW_MODEL_LOAD": "AI_POLICY_ALLOW_MODEL_HASH_MATCH",
    }
    mapped_reason_code = alias_map.get(reason_code, reason_code)
    binding["mapped_reason_code"] = mapped_reason_code
    if mapped_reason_code not in {
        "AI_POLICY_ALLOW_MODEL_HASH_MATCH",
        "AI_POLICY_DENY_MODEL_HASH_NOT_ALLOWED",
        "AI_POLICY_ALLOW_INFERENCE_BUDGET_OK",
        "AI_POLICY_DENY_INFERENCE_BUDGET_EXCEEDED",
        "AI_POLICY_ALLOW_TOOL_WHITELISTED",
        "AI_POLICY_DENY_TOOL_NOT_WHITELISTED",
        "AI_POLICY_ALLOW_RESOURCE_WITHIN_LIMIT",
        "AI_POLICY_DENY_RESOURCE_LIMIT_EXCEEDED",
        "AI_POLICY_ALLOW_CONTENT_SAFETY_OK",
        "AI_POLICY_DENY_CONTENT_SAFETY_BLOCKED",
    }:
        errs.append(f"runtime trace reason_code not recognized by policy contract: {reason_code}")

    return len(errs) == 0, errs, binding


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate deterministic AI policy event reason-code contract.")
    p.add_argument("--out-dir", required=True, help="Directory to write policy contract artifacts")
    p.add_argument("--runtime-trace", default="", help="Optional path to runtime trace artifact json")
    p.add_argument("--ai-bin", default="", help="Optional AI binary path used to emit runtime trace if missing")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    policy = {
        "allowed_model_hashes": [
            "sha256:1111111111111111111111111111111111111111111111111111111111111111",
            "sha256:2222222222222222222222222222222222222222222222222222222222222222",
        ],
        "max_tokens": 1024,
        "allowed_tools": ["search", "calculator", "summarize"],
        "max_memory_mb": 4096,
        "blocked_severities": ["critical", "high"],
    }

    events = [
        {"event_type": "model_load", "model_hash": policy["allowed_model_hashes"][0]},
        {"event_type": "model_load", "model_hash": "sha256:deadbeef"},
        {"event_type": "inference_start", "requested_tokens": 512},
        {"event_type": "inference_start", "requested_tokens": 4097},
        {"event_type": "tool_use", "tool_name": "search"},
        {"event_type": "tool_use", "tool_name": "shell_exec"},
        {"event_type": "resource_allocate", "memory_mb": 1024},
        {"event_type": "resource_allocate", "memory_mb": 8192},
        {"event_type": "content_emit", "safety_severity": "medium"},
        {"event_type": "content_emit", "safety_severity": "high"},
    ]

    trace_a = run_trace(policy, events)
    trace_b = run_trace(policy, events)

    canonical_a = canonical_json(trace_a)
    canonical_b = canonical_json(trace_b)
    hash_a = sha256_text(canonical_a)
    hash_b = sha256_text(canonical_b)
    deterministic = hash_a == hash_b

    required_reason_codes = {
        "AI_POLICY_ALLOW_MODEL_HASH_MATCH",
        "AI_POLICY_DENY_MODEL_HASH_NOT_ALLOWED",
        "AI_POLICY_ALLOW_INFERENCE_BUDGET_OK",
        "AI_POLICY_DENY_INFERENCE_BUDGET_EXCEEDED",
        "AI_POLICY_ALLOW_TOOL_WHITELISTED",
        "AI_POLICY_DENY_TOOL_NOT_WHITELISTED",
        "AI_POLICY_ALLOW_RESOURCE_WITHIN_LIMIT",
        "AI_POLICY_DENY_RESOURCE_LIMIT_EXCEEDED",
        "AI_POLICY_ALLOW_CONTENT_SAFETY_OK",
        "AI_POLICY_DENY_CONTENT_SAFETY_BLOCKED",
    }
    observed = {entry["reason_code"] for entry in trace_a}
    reason_code_coverage_ok = required_reason_codes.issubset(observed)
    runtime_binding_valid = True
    runtime_binding: dict[str, Any] = {}
    if args.runtime_trace:
        runtime_trace_path = Path(args.runtime_trace).resolve()
        ai_bin = Path(args.ai_bin).resolve() if args.ai_bin else None
        runtime_binding_valid, runtime_errors, runtime_binding = validate_runtime_trace_binding(
            runtime_trace_path, ai_bin
        )
        reason_code_coverage_ok = reason_code_coverage_ok and runtime_binding_valid
        if runtime_errors:
            payload_errors = runtime_errors
        else:
            payload_errors = []
    else:
        payload_errors = []

    status = "pass" if deterministic and reason_code_coverage_ok else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic": deterministic,
        "reason_code_coverage_ok": reason_code_coverage_ok,
        "trace_sha256": hash_a,
        "trace": trace_a,
        "runtime_binding_valid": runtime_binding_valid,
        "runtime_binding": runtime_binding,
        "errors": payload_errors,
    }

    json_path = out_dir / "ai_policy_event_contract.json"
    sum_path = out_dir / "ai_policy_event_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    sum_path.write_text(
        "\n".join(
            [
                "# AI Policy Event Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic: `{deterministic}`",
                f"- reason_code_coverage_ok: `{reason_code_coverage_ok}`",
                f"- runtime_binding_valid: `{runtime_binding_valid}`",
                f"- trace_sha256: `{hash_a}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai policy contract status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {sum_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
