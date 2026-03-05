#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
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


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate deterministic AI policy event reason-code contract.")
    p.add_argument("--out-dir", required=True, help="Directory to write policy contract artifacts")
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

    status = "pass" if deterministic and reason_code_coverage_ok else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic": deterministic,
        "reason_code_coverage_ok": reason_code_coverage_ok,
        "trace_sha256": hash_a,
        "trace": trace_a,
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
