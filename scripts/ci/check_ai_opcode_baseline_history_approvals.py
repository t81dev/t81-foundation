#!/usr/bin/env python3
"""Validate approval metadata for opcode baseline history windows."""

from __future__ import annotations

import argparse
import base64
import hashlib
import hmac
import json
import os
import sys
from datetime import date
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.opcode-baseline-approval.v1"
HISTORY_SCHEMA_VERSION = "t81.ai.phase1-hash-baseline-history.v1"
KEYRING_SCHEMA_VERSION = "t81.ai.opcode-baseline-approval-keyring.v1"
PROVENANCE_EXPECTATION_SCHEMA_VERSION = "t81.ai.opcode-baseline-provenance-expectations.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


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


def validate_keyring(path: Path) -> tuple[dict[str, Any], list[str]]:
    payload = parse_json(path)
    errors: list[str] = []
    if payload.get("schema") != KEYRING_SCHEMA_VERSION:
        errors.append(f"keyring schema mismatch: {payload.get('schema')} != {KEYRING_SCHEMA_VERSION}")
    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        errors.append("keyring keys must be a non-empty list")
        return payload, errors
    for key in keys:
        if not isinstance(key, dict):
            errors.append("key entry must be object")
            continue
        key_id = str(key.get("key_id", "")).strip()
        if not key_id:
            errors.append("key entry missing key_id")
            continue
        if str(key.get("algorithm", "")).strip() != "hmac-sha256":
            errors.append(f"key {key_id}: unsupported algorithm")
        decoded = resolve_key_material(key, errors)
        if not decoded:
            errors.append(f"key {key_id}: empty key material")
    return payload, errors


def verify_payload_signature(
    payload: dict[str, Any], signature_hex: str, keyring: dict[str, Any], expected_key_id: str
) -> tuple[bool, str]:
    keys = keyring.get("keys")
    if not isinstance(keys, list):
        return False, ""
    encoded = canonical_json(payload).encode("utf-8")
    for key in keys:
        if not isinstance(key, dict):
            continue
        key_id = str(key.get("key_id", "")).strip()
        if expected_key_id and key_id != expected_key_id:
            continue
        material = resolve_key_material(key, [])
        if not material:
            continue
        candidate = hmac.new(material, encoded, hashlib.sha256).hexdigest()
        if hmac.compare_digest(candidate, signature_hex):
            return True, key_id
    return False, ""


def parse_iso_date(raw: str, label: str) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"invalid {label}: {raw} (expected YYYY-MM-DD)") from exc


def validate_safe_relative_path(value: str) -> bool:
    path = Path(value)
    if path.is_absolute():
        return False
    return ".." not in path.parts


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate opcode baseline history promotion approvals.")
    p.add_argument("--history-file", required=True, help="Path to PHASE1_BASELINE_HASHES_HISTORY.json")
    p.add_argument(
        "--signing-keyring",
        default="",
        help="Path to opcode baseline approval keyring (defaults to scripts/ci/ai_opcode_baseline_approval_keyring.json).",
    )
    p.add_argument(
        "--provenance-expectations",
        default="",
        help="Path to opcode baseline provenance expectation contract (defaults to scripts/ci/ai_opcode_baseline_provenance_expectations.json).",
    )
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    history_path = Path(args.history_file).resolve()
    default_keyring = Path(__file__).resolve().with_name("ai_opcode_baseline_approval_keyring.json")
    keyring_path = Path(args.signing_keyring).resolve() if args.signing_keyring else default_keyring
    default_provenance_expectations = (
        Path(__file__).resolve().with_name("ai_opcode_baseline_provenance_expectations.json")
    )
    provenance_expectations_path = (
        Path(args.provenance_expectations).resolve()
        if args.provenance_expectations
        else default_provenance_expectations
    )
    out_json = Path(args.out_json).resolve()

    errors: list[str] = []
    warnings: list[str] = []
    windows_report: list[dict[str, Any]] = []
    keyring_payload: dict[str, Any] = {}
    if keyring_path.exists():
        keyring_payload, keyring_errors = validate_keyring(keyring_path)
        errors.extend(keyring_errors)
    else:
        errors.append(f"missing signing keyring: {keyring_path}")
    required_provenance_fields: list[str] = []
    path_safety_fields: list[str] = []
    must_exist_in_repo_fields: list[str] = []
    allowed_prefix_by_field: dict[str, list[str]] = {}
    repo_root = Path(__file__).resolve().parents[2]
    if provenance_expectations_path.exists():
        provenance_expectations = parse_json(provenance_expectations_path)
        if provenance_expectations.get("schema") != PROVENANCE_EXPECTATION_SCHEMA_VERSION:
            errors.append(
                "provenance expectations schema mismatch: "
                f"{provenance_expectations.get('schema')} != {PROVENANCE_EXPECTATION_SCHEMA_VERSION}"
            )
        else:
            raw_fields = provenance_expectations.get("required_provenance_fields")
            if isinstance(raw_fields, list):
                required_provenance_fields = [str(item).strip() for item in raw_fields if str(item).strip()]
            if not required_provenance_fields:
                errors.append("provenance expectations required_provenance_fields must be non-empty list")
            raw_path_fields = provenance_expectations.get("path_safety_fields")
            if isinstance(raw_path_fields, list):
                path_safety_fields = [str(item).strip() for item in raw_path_fields if str(item).strip()]
            raw_exist_fields = provenance_expectations.get("must_exist_in_repo_fields")
            if isinstance(raw_exist_fields, list):
                must_exist_in_repo_fields = [str(item).strip() for item in raw_exist_fields if str(item).strip()]
            raw_prefix_map = provenance_expectations.get("allowed_prefix_by_field")
            if isinstance(raw_prefix_map, dict):
                for field_name, prefixes in raw_prefix_map.items():
                    fname = str(field_name).strip()
                    if not fname:
                        continue
                    if isinstance(prefixes, list):
                        clean = [str(item).strip() for item in prefixes if str(item).strip()]
                        if clean:
                            allowed_prefix_by_field[fname] = clean
    else:
        errors.append(f"missing provenance expectations file: {provenance_expectations_path}")

    if not history_path.exists():
        print(f"error: missing history file: {history_path}", file=sys.stderr)
        return 1
    history = parse_json(history_path)
    if history.get("schema") != HISTORY_SCHEMA_VERSION:
        print(f"error: history schema mismatch: {history.get('schema')}", file=sys.stderr)
        return 1

    windows = history.get("windows")
    if not isinstance(windows, list) or not windows:
        print("error: history windows must be non-empty list", file=sys.stderr)
        return 1

    prev_start: date | None = None
    for idx, win in enumerate(windows, start=1):
        if not isinstance(win, dict):
            errors.append(f"window {idx}: entry is not an object")
            continue

        window_id = str(win.get("window_id", f"window-{idx}")).strip()
        start_raw = str(win.get("window_start", "")).strip()
        end_raw = str(win.get("window_end", "")).strip()
        baseline = win.get("baseline")
        approval = win.get("promotion_approval")
        provenance = win.get("provenance")
        approval_hash = str(win.get("promotion_approval_attestation_sha256", "")).strip()

        try:
            start = parse_iso_date(start_raw, "window_start")
            end = parse_iso_date(end_raw, "window_end")
            if start > end:
                errors.append(f"{window_id}: window_start > window_end")
            if prev_start is not None and start < prev_start:
                warnings.append(f"{window_id}: window ordering is not monotonic by start date")
            prev_start = start
        except ValueError as exc:
            errors.append(f"{window_id}: {exc}")

        if not isinstance(baseline, dict):
            errors.append(f"{window_id}: baseline must be object")
            baseline = {}

        if not isinstance(approval, dict):
            errors.append(f"{window_id}: promotion_approval must be object")
            approval = {}
        if not isinstance(provenance, dict):
            errors.append(f"{window_id}: provenance must be object")
            provenance = {}
        approved_by = str(approval.get("approved_by", "")).strip()
        approved_at = str(approval.get("approved_at", "")).strip()
        change_ref = str(approval.get("change_ref", "")).strip()
        if not approved_by:
            errors.append(f"{window_id}: promotion_approval.approved_by required")
        if not approved_at:
            errors.append(f"{window_id}: promotion_approval.approved_at required")
        else:
            try:
                parse_iso_date(approved_at, "promotion_approval.approved_at")
            except ValueError as exc:
                errors.append(f"{window_id}: {exc}")
        if not change_ref:
            errors.append(f"{window_id}: promotion_approval.change_ref required")
        missing_provenance_fields: list[str] = []
        invalid_provenance_fields: list[str] = []
        for field in required_provenance_fields:
            value = str(provenance.get(field, "")).strip()
            if not value:
                missing_provenance_fields.append(field)
                errors.append(f"{window_id}: provenance.{field} required by expectations contract")
                continue
            if field in path_safety_fields and not validate_safe_relative_path(value):
                invalid_provenance_fields.append(field)
                errors.append(
                    f"{window_id}: provenance.{field} must be a safe relative path (no absolute/.. segments)"
                )
            prefixes = allowed_prefix_by_field.get(field, [])
            if prefixes and not any(value.startswith(prefix) for prefix in prefixes):
                invalid_provenance_fields.append(field)
                errors.append(
                    f"{window_id}: provenance.{field} must start with one of: {', '.join(prefixes)}"
                )
            if field in must_exist_in_repo_fields:
                if not (repo_root / value).exists():
                    invalid_provenance_fields.append(field)
                    errors.append(
                        f"{window_id}: provenance.{field} path does not exist in repo: {(repo_root / value)}"
                    )

        attestation_core = {
            "window_id": window_id,
            "window_start": start_raw,
            "window_end": end_raw,
            "baseline": baseline,
            "provenance": provenance,
            "promotion_approval": {
                "approved_by": approved_by,
                "approved_at": approved_at,
                "change_ref": change_ref,
            },
        }
        expected_hash = sha256_text(canonical_json(attestation_core))
        if not approval_hash:
            errors.append(f"{window_id}: promotion_approval_attestation_sha256 required")
        elif approval_hash != expected_hash:
            errors.append(f"{window_id}: promotion_approval_attestation_sha256 mismatch")

        signature = win.get("promotion_approval_signature")
        signature_valid = False
        signature_verified_by = ""
        if not isinstance(signature, dict):
            errors.append(f"{window_id}: promotion_approval_signature required")
        else:
            if str(signature.get("algorithm", "")).strip() != "hmac-sha256":
                errors.append(f"{window_id}: promotion_approval_signature algorithm mismatch")
            signature_key_id = str(signature.get("key_id", "")).strip()
            if not signature_key_id:
                errors.append(f"{window_id}: promotion_approval_signature.key_id required")
            sig_hex = str(signature.get("signature_hex", "")).strip()
            if not sig_hex:
                errors.append(f"{window_id}: promotion_approval_signature.signature_hex required")
            elif keyring_payload:
                signature_valid, signature_verified_by = verify_payload_signature(
                    {
                        "window_id": window_id,
                        "promotion_approval_attestation_sha256": approval_hash,
                    },
                    sig_hex,
                    keyring_payload,
                    signature_key_id,
                )
                if not signature_valid:
                    errors.append(f"{window_id}: promotion_approval_signature verification failed")

        windows_report.append(
            {
                "window_id": window_id,
                "window_start": start_raw,
                "window_end": end_raw,
                "approval_attestation_valid": approval_hash == expected_hash and bool(approval_hash),
                "approval_signature_valid": signature_valid,
                "approval_signature_verified_by": signature_verified_by,
                "provenance_fields_present": sorted(
                    [field for field in required_provenance_fields if str(provenance.get(field, "")).strip()]
                ),
                "missing_provenance_fields": missing_provenance_fields,
                "invalid_provenance_fields": sorted(set(invalid_provenance_fields)),
            }
        )

    status = "pass" if not errors else "fail"
    report = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "history_file": str(history_path),
        "signing_keyring": str(keyring_path),
        "provenance_expectations_file": str(provenance_expectations_path),
        "required_provenance_fields": required_provenance_fields,
        "path_safety_fields": path_safety_fields,
        "must_exist_in_repo_fields": must_exist_in_repo_fields,
        "allowed_prefix_by_field": allowed_prefix_by_field,
        "window_count": len(windows),
        "windows": windows_report,
        "errors": errors,
        "warnings": warnings,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    print(f"ai opcode baseline approvals status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
