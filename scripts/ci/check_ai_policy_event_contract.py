#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import hmac
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.policy-events.v1"
LEDGER_SCHEMA_VERSION = "t81.ai.axion-policy-ledger.v1"
LEDGER_REPLAY_SCHEMA_VERSION = "t81.ai.axion-policy-ledger-replay.v1"
KEYRING_SCHEMA_VERSION = "t81.ai.policy-ledger-keyring.v1"
EXPECTATIONS_SCHEMA_VERSION = "t81.ai.policy-event-expectations.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_reason_code_expectations(path: Path) -> tuple[dict[str, Any], list[str], set[str], set[str]]:
    errors: list[str] = []
    payload = parse_json(path)
    if payload.get("schema") != EXPECTATIONS_SCHEMA_VERSION:
        errors.append(
            f"expectations schema mismatch: {payload.get('schema')} != {EXPECTATIONS_SCHEMA_VERSION}"
        )

    required_raw = payload.get("required_reason_codes", [])
    if not isinstance(required_raw, list):
        errors.append("required_reason_codes must be an array")
        required_raw = []
    required_reason_codes = {str(code).strip() for code in required_raw if str(code).strip()}
    if not required_reason_codes:
        errors.append("required_reason_codes must contain at least one value")

    recognized_raw = payload.get("recognized_reason_codes", [])
    if not isinstance(recognized_raw, list):
        errors.append("recognized_reason_codes must be an array when provided")
        recognized_raw = []
    recognized_reason_codes = {str(code).strip() for code in recognized_raw if str(code).strip()}
    if not recognized_reason_codes:
        recognized_reason_codes = set(required_reason_codes)
    if not required_reason_codes.issubset(recognized_reason_codes):
        errors.append("recognized_reason_codes must include all required_reason_codes")

    return payload, errors, required_reason_codes, recognized_reason_codes


def parse_key_material_b64(raw: str) -> bytes:
    return base64.b64decode(raw, validate=True)


def resolve_key_material(key: dict[str, Any], errors: list[str] | None = None) -> bytes:
    key_id = str(key.get("key_id", "")).strip()
    env_name = str(key.get("material_env", "")).strip()
    if env_name:
        env_value = os.environ.get(env_name, "")
        if env_value:
            try:
                return parse_key_material_b64(env_value)
            except Exception:
                if errors is not None:
                    errors.append(f"key {key_id}: material_env '{env_name}' is not valid base64")
                return b""
    try:
        return parse_key_material_b64(str(key.get("material_b64", "")))
    except Exception:
        if errors is not None:
            errors.append(f"key {key_id}: invalid material_b64")
        return b""


def validate_keyring(path: Path) -> tuple[dict[str, Any], dict[str, Any], list[str]]:
    payload = parse_json(path)
    errors: list[str] = []

    if payload.get("schema") != KEYRING_SCHEMA_VERSION:
        errors.append(f"keyring schema mismatch: {payload.get('schema')} != {KEYRING_SCHEMA_VERSION}")

    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        errors.append("keyring keys must be a non-empty list")
        return payload, {}, errors

    active_keys: list[dict[str, Any]] = []
    key_by_id: dict[str, dict[str, Any]] = {}
    for key in keys:
        if not isinstance(key, dict):
            errors.append("key entry must be object")
            continue
        key_id = str(key.get("key_id", "")).strip()
        if not key_id:
            errors.append("key entry missing key_id")
            continue
        if key_id in key_by_id:
            errors.append(f"duplicate key_id in keyring: {key_id}")
            continue
        key_by_id[key_id] = key
        if str(key.get("status", "")).strip() == "active":
            active_keys.append(key)
        if str(key.get("algorithm", "")).strip() != "hmac-sha256":
            errors.append(f"key {key_id}: unsupported algorithm")
        decoded = resolve_key_material(key, errors)
        if not decoded:
            errors.append(f"key {key_id}: empty key material")

    active_key_id = str(payload.get("active_key_id", "")).strip()
    if not active_key_id:
        errors.append("keyring missing active_key_id")
    if len(active_keys) != 1:
        errors.append(f"keyring must have exactly one active key (found {len(active_keys)})")

    active = key_by_id.get(active_key_id, {})
    if not active:
        errors.append(f"active_key_id not found: {active_key_id}")
    elif str(active.get("status", "")).strip() != "active":
        errors.append(f"active key {active_key_id} does not have status=active")

    rotation = payload.get("rotation_policy", {})
    if isinstance(rotation, dict) and bool(rotation.get("require_next_key", False)):
        has_next = any(str(k.get("status", "")).strip() == "next" for k in key_by_id.values())
        if not has_next:
            errors.append("rotation_policy.require_next_key=true but no next key exists")

    return payload, active, errors


def sign_payload_hex(payload: dict[str, Any], key_entry: dict[str, Any]) -> str:
    material = resolve_key_material(key_entry, [])
    return hmac.new(material, canonical_json(payload).encode("utf-8"), hashlib.sha256).hexdigest()


def verify_payload_signature(payload: dict[str, Any], signature_hex: str, keyring: dict[str, Any]) -> tuple[bool, str]:
    keys = keyring.get("keys")
    if not isinstance(keys, list):
        return False, ""
    encoded = canonical_json(payload).encode("utf-8")
    for key in keys:
        if not isinstance(key, dict):
            continue
        key_id = str(key.get("key_id", "")).strip()
        material = resolve_key_material(key, [])
        if not material:
            continue
        candidate = hmac.new(material, encoded, hashlib.sha256).hexdigest()
        if hmac.compare_digest(candidate, signature_hex):
            return True, key_id
    return False, ""


def build_ledger_escalation(errors: list[str]) -> dict[str, Any]:
    rules: list[tuple[str, str, str, str]] = [
        (
            "keyring",
            "AI_POLICY_ESCALATE_KEYRING_INVALID",
            "platform-oncall",
            "fail_closed_and_review_policy_ledger_keyring",
        ),
        (
            "signature verification failed",
            "AI_POLICY_ESCALATE_LEDGER_SIGNATURE_INVALID",
            "security-oncall",
            "block_promotion_and_rotate_policy_ledger_keys",
        ),
        (
            "runtime trace",
            "AI_POLICY_ESCALATE_RUNTIME_TRACE_BINDING_INVALID",
            "policy-oncall",
            "block_promotion_and_repair_runtime_trace_binding",
        ),
    ]
    actions: list[dict[str, str]] = []
    seen: set[str] = set()
    joined = "\n".join(errors).lower()
    for needle, reason_code, owner, action in rules:
        if needle in joined and reason_code not in seen:
            actions.append(
                {
                    "reason_code": reason_code,
                    "owner": owner,
                    "action": action,
                }
            )
            seen.add(reason_code)
    return {
        "status": "triggered" if actions else "none",
        "actions": actions,
    }


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

    if event_type == "wload_request":
        model_hash = str(event.get("model_hash", "")).strip()
        if model_hash and model_hash in set(policy.get("allowed_wload_hashes", [])):
            return {"decision": "allow", "reason_code": "AI_POLICY_ALLOW_WLOAD_POLICY_GATE"}
        return {"decision": "deny", "reason_code": "AI_POLICY_DENY_WLOAD_UNSUPPORTED"}

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


def validate_runtime_trace_binding(
    runtime_trace_path: Path, ai_bin: Path | None, recognized_reason_codes: set[str]
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
        "wload_request",
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
    if mapped_reason_code not in recognized_reason_codes:
        errs.append(f"runtime trace reason_code not recognized by policy contract: {reason_code}")

    return len(errs) == 0, errs, binding


def build_signed_ledger_snapshot(
    trace: list[dict[str, Any]], keyring_payload: dict[str, Any], key_entry: dict[str, Any], keyring_path: Path
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    prev_sha = ""

    for seq, event in enumerate(trace, start=1):
        core = {
            "seq": seq,
            "event_type": event["event_type"],
            "decision": event["decision"],
            "reason_code": event["reason_code"],
            "prev_entry_sha256": prev_sha,
        }
        entry_sha = sha256_text(canonical_json(core))
        sig_payload = {"entry_sha256": entry_sha, "seq": seq}
        signature_hex = sign_payload_hex(sig_payload, key_entry)

        entry = {
            **core,
            "entry_sha256": entry_sha,
            "signature": {
                "algorithm": "hmac-sha256",
                "key_id": str(key_entry.get("key_id", "")).strip(),
                "signature_hex": signature_hex,
            },
        }
        entries.append(entry)
        prev_sha = entry_sha

    snapshot_core = {
        "schema": LEDGER_SCHEMA_VERSION,
        "entry_count": len(entries),
        "root_entry_sha256": prev_sha,
        "entries": entries,
    }
    snapshot_sig_payload = {
        "entry_count": snapshot_core["entry_count"],
        "root_entry_sha256": snapshot_core["root_entry_sha256"],
    }
    snapshot_sig = sign_payload_hex(snapshot_sig_payload, key_entry)

    return {
        **snapshot_core,
        "snapshot_sha256": sha256_text(canonical_json(snapshot_core)),
        "signature": {
            "algorithm": "hmac-sha256",
            "key_id": str(key_entry.get("key_id", "")).strip(),
            "signature_hex": snapshot_sig,
            "verified": False,
            "verified_by_key_id": "",
        },
        "keyring": {
            "path": str(keyring_path),
            "sha256": sha256_text(keyring_path.read_text(encoding="utf-8")),
            "rotation_policy": keyring_payload.get("rotation_policy", {}),
        },
    }


def verify_ledger_snapshot(snapshot: dict[str, Any], keyring_payload: dict[str, Any]) -> tuple[bool, list[str], dict[str, Any]]:
    errs: list[str] = []
    details: dict[str, Any] = {}

    entries = snapshot.get("entries")
    if not isinstance(entries, list) or not entries:
        return False, ["ledger snapshot entries must be non-empty list"], details

    prev_sha = ""
    for idx, entry in enumerate(entries, start=1):
        if not isinstance(entry, dict):
            errs.append(f"entry {idx} is not object")
            continue
        core = {
            "seq": entry.get("seq"),
            "event_type": entry.get("event_type"),
            "decision": entry.get("decision"),
            "reason_code": entry.get("reason_code"),
            "prev_entry_sha256": entry.get("prev_entry_sha256"),
        }
        if core["seq"] != idx:
            errs.append(f"entry {idx} seq mismatch")
        if core["prev_entry_sha256"] != prev_sha:
            errs.append(f"entry {idx} prev_entry_sha256 mismatch")

        entry_sha = str(entry.get("entry_sha256", "")).strip()
        expected_entry_sha = sha256_text(canonical_json(core))
        if entry_sha != expected_entry_sha:
            errs.append(f"entry {idx} entry_sha256 mismatch")

        sig = entry.get("signature")
        if not isinstance(sig, dict):
            errs.append(f"entry {idx} missing signature")
        else:
            payload = {"entry_sha256": entry_sha, "seq": idx}
            ok, key_id = verify_payload_signature(payload, str(sig.get("signature_hex", "")), keyring_payload)
            if not ok:
                errs.append(f"entry {idx} signature verification failed")
            else:
                details[f"entry_{idx}_verified_by"] = key_id

        prev_sha = entry_sha

    snapshot_core = {
        "schema": snapshot.get("schema"),
        "entry_count": snapshot.get("entry_count"),
        "root_entry_sha256": snapshot.get("root_entry_sha256"),
        "entries": entries,
    }
    expected_snapshot_sha = sha256_text(canonical_json(snapshot_core))
    if str(snapshot.get("snapshot_sha256", "")).strip() != expected_snapshot_sha:
        errs.append("snapshot_sha256 mismatch")

    if snapshot.get("root_entry_sha256") != prev_sha:
        errs.append("root_entry_sha256 mismatch")
    if snapshot.get("entry_count") != len(entries):
        errs.append("entry_count mismatch")

    sig = snapshot.get("signature")
    if not isinstance(sig, dict):
        errs.append("snapshot signature missing")
    else:
        payload = {
            "entry_count": snapshot.get("entry_count"),
            "root_entry_sha256": snapshot.get("root_entry_sha256"),
        }
        ok, key_id = verify_payload_signature(payload, str(sig.get("signature_hex", "")), keyring_payload)
        details["snapshot_signature_verified"] = ok
        details["snapshot_signature_verified_by"] = key_id
        if not ok:
            errs.append("snapshot signature verification failed")

    return len(errs) == 0, errs, details


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate deterministic AI policy event reason-code contract.")
    p.add_argument("--out-dir", required=True, help="Directory to write policy contract artifacts")
    p.add_argument("--runtime-trace", default="", help="Optional path to runtime trace artifact json")
    p.add_argument("--ai-bin", default="", help="Optional AI binary path used to emit runtime trace if missing")
    p.add_argument(
        "--ledger-keyring",
        default="",
        help="Optional keyring path for signed Axion ledger snapshots (defaults to scripts/ci/ai_policy_ledger_keyring.json)",
    )
    p.add_argument(
        "--expectations-file",
        default="",
        help="Optional policy reason-code expectations contract "
        "(defaults to scripts/ci/ai_policy_event_expectations.json)",
    )
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
        "allowed_wload_hashes": [],
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
        {"event_type": "wload_request", "model_hash": policy["allowed_model_hashes"][0]},
    ]

    trace_a = run_trace(policy, events)
    trace_b = run_trace(policy, events)

    canonical_a = canonical_json(trace_a)
    canonical_b = canonical_json(trace_b)
    hash_a = sha256_text(canonical_a)
    hash_b = sha256_text(canonical_b)
    deterministic = hash_a == hash_b

    default_expectations = Path(__file__).resolve().with_name("ai_policy_event_expectations.json")
    expectations_path = Path(args.expectations_file).resolve() if args.expectations_file else default_expectations
    expectations_payload: dict[str, Any] = {}
    required_reason_codes: set[str] = set()
    recognized_reason_codes: set[str] = set()
    errors: list[str] = []
    if not expectations_path.exists():
        errors.append(f"expectations file missing: {expectations_path}")
    else:
        expectations_payload, expectation_errors, required_reason_codes, recognized_reason_codes = (
            load_reason_code_expectations(expectations_path)
        )
        errors.extend(expectation_errors)

    observed = {entry["reason_code"] for entry in trace_a}
    reason_code_coverage_ok = required_reason_codes.issubset(observed)
    runtime_binding_valid = True
    runtime_binding: dict[str, Any] = {}
    if args.runtime_trace:
        runtime_trace_path = Path(args.runtime_trace).resolve()
        ai_bin = Path(args.ai_bin).resolve() if args.ai_bin else None
        runtime_binding_valid, runtime_errors, runtime_binding = validate_runtime_trace_binding(
            runtime_trace_path, ai_bin, recognized_reason_codes
        )
        if runtime_errors:
            errors.extend(runtime_errors)

    default_keyring = Path(__file__).resolve().with_name("ai_policy_ledger_keyring.json")
    keyring_path = Path(args.ledger_keyring).resolve() if args.ledger_keyring else default_keyring
    keyring_payload: dict[str, Any] = {}
    active_key: dict[str, Any] = {}
    keyring_errors: list[str] = []
    if keyring_path.exists():
        keyring_payload, active_key, keyring_errors = validate_keyring(keyring_path)
    else:
        keyring_errors = [f"ledger keyring file missing: {keyring_path}"]
    errors.extend(keyring_errors)

    ledger_snapshot: dict[str, Any] = {
        "schema": LEDGER_SCHEMA_VERSION,
        "status": "fail",
        "errors": ["ledger generation skipped due to keyring validation errors"],
    }
    ledger_snapshot_valid = False
    ledger_verification: dict[str, Any] = {
        "schema": LEDGER_REPLAY_SCHEMA_VERSION,
        "status": "fail",
        "deterministic_replay": False,
        "errors": ["ledger replay verification skipped"],
    }

    if not keyring_errors:
        ledger_snapshot = build_signed_ledger_snapshot(trace_a, keyring_payload, active_key, keyring_path)
        ledger_ok, ledger_errs, ledger_details = verify_ledger_snapshot(ledger_snapshot, keyring_payload)
        ledger_snapshot["signature"]["verified"] = bool(ledger_details.get("snapshot_signature_verified", False))
        ledger_snapshot["signature"]["verified_by_key_id"] = str(
            ledger_details.get("snapshot_signature_verified_by", "")
        )
        ledger_snapshot["status"] = "pass" if ledger_ok else "fail"
        ledger_snapshot["errors"] = ledger_errs
        ledger_snapshot["verification"] = ledger_details
        ledger_snapshot_valid = ledger_ok

        replay_one = build_signed_ledger_snapshot(trace_a, keyring_payload, active_key, keyring_path)
        replay_two = build_signed_ledger_snapshot(trace_a, keyring_payload, active_key, keyring_path)
        replay_one_ok, replay_one_errs, _ = verify_ledger_snapshot(replay_one, keyring_payload)
        replay_two_ok, replay_two_errs, _ = verify_ledger_snapshot(replay_two, keyring_payload)
        deterministic_replay = (
            replay_one_ok
            and replay_two_ok
            and replay_one.get("root_entry_sha256") == replay_two.get("root_entry_sha256")
            and replay_one.get("snapshot_sha256") == replay_two.get("snapshot_sha256")
            and replay_one.get("signature", {}).get("signature_hex")
            == replay_two.get("signature", {}).get("signature_hex")
        )
        replay_errors = replay_one_errs + replay_two_errs
        if not deterministic_replay:
            replay_errors.append("ledger replay mismatch across deterministic replays")

        ledger_verification = {
            "schema": LEDGER_REPLAY_SCHEMA_VERSION,
            "status": "pass" if deterministic_replay else "fail",
            "deterministic_replay": deterministic_replay,
            "root_entry_sha256": replay_one.get("root_entry_sha256", ""),
            "snapshot_sha256": replay_one.get("snapshot_sha256", ""),
            "signature_hex": replay_one.get("signature", {}).get("signature_hex", ""),
            "errors": replay_errors,
        }

    reason_code_coverage_ok = reason_code_coverage_ok and runtime_binding_valid and ledger_snapshot_valid
    status = "pass" if deterministic and reason_code_coverage_ok and ledger_verification.get("status") == "pass" else "fail"

    combined_errors = errors + ledger_snapshot.get("errors", []) + ledger_verification.get("errors", [])
    escalation_policy = build_ledger_escalation(combined_errors)
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic": deterministic,
        "reason_code_coverage_ok": reason_code_coverage_ok,
        "expectations_path": str(expectations_path),
        "expectations_schema": expectations_payload.get("schema", ""),
        "required_reason_codes": sorted(required_reason_codes),
        "recognized_reason_codes": sorted(recognized_reason_codes),
        "trace_sha256": hash_a,
        "trace": trace_a,
        "runtime_binding_valid": runtime_binding_valid,
        "runtime_binding": runtime_binding,
        "axion_ledger_snapshot_status": ledger_snapshot.get("status", "fail"),
        "axion_ledger_replay_status": ledger_verification.get("status", "fail"),
        "errors": combined_errors,
        "escalation_policy": escalation_policy,
    }

    json_path = out_dir / "ai_policy_event_contract.json"
    sum_path = out_dir / "ai_policy_event_contract.md"
    ledger_json_path = out_dir / "ai_axion_policy_ledger_snapshot.json"
    ledger_md_path = out_dir / "ai_axion_policy_ledger_snapshot.md"
    replay_json_path = out_dir / "ai_axion_policy_ledger_replay_verification.json"
    replay_md_path = out_dir / "ai_axion_policy_ledger_replay_verification.md"

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
                f"- axion_ledger_snapshot_status: `{ledger_snapshot.get('status', 'fail')}`",
                f"- axion_ledger_replay_status: `{ledger_verification.get('status', 'fail')}`",
                f"- escalation_status: `{escalation_policy['status']}`",
                f"- trace_sha256: `{hash_a}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    ledger_json_path.write_text(json.dumps(ledger_snapshot, indent=2, sort_keys=True), encoding="utf-8")
    ledger_md_path.write_text(
        "\n".join(
            [
                "# Axion Policy Ledger Snapshot",
                "",
                f"- schema: `{LEDGER_SCHEMA_VERSION}`",
                f"- status: `{ledger_snapshot.get('status', 'fail')}`",
                f"- entry_count: `{ledger_snapshot.get('entry_count', 0)}`",
                f"- root_entry_sha256: `{ledger_snapshot.get('root_entry_sha256', '')}`",
                f"- snapshot_sha256: `{ledger_snapshot.get('snapshot_sha256', '')}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    replay_json_path.write_text(json.dumps(ledger_verification, indent=2, sort_keys=True), encoding="utf-8")
    replay_md_path.write_text(
        "\n".join(
            [
                "# Axion Policy Ledger Replay Verification",
                "",
                f"- schema: `{LEDGER_REPLAY_SCHEMA_VERSION}`",
                f"- status: `{ledger_verification.get('status', 'fail')}`",
                f"- deterministic_replay: `{str(ledger_verification.get('deterministic_replay', False)).lower()}`",
                f"- root_entry_sha256: `{ledger_verification.get('root_entry_sha256', '')}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai policy contract status: {status}")
    print(f"artifact: {json_path}")
    print(f"ledger:   {ledger_json_path}")
    print(f"replay:   {replay_json_path}")
    print(f"summary:  {sum_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
