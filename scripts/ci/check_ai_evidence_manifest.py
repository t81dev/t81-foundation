#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import hmac
import json
import os
import sys
from datetime import date, datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.evidence-manifest.v1"
KEYRING_SCHEMA_VERSION = "t81.ai.evidence-manifest-keyring.v1"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def lane_entry(name: str, path: Path) -> dict[str, Any]:
    payload = parse_json(path)
    status = str(payload.get("status", "")).strip()
    if not status and isinstance(payload.get("checks"), dict):
        status = str(payload["checks"].get("status", "")).strip()
    return {
        "lane": name,
        "path": str(path),
        "status": status,
        "sha256": sha256_file(path),
    }


def _parse_iso_date(value: str, field_name: str, errors: list[str]) -> date | None:
    try:
        return date.fromisoformat(value)
    except ValueError:
        errors.append(f"invalid {field_name}: {value} (expected YYYY-MM-DD)")
        return None


def _parse_key_material_b64(raw: str, key_id: str, field_name: str, errors: list[str]) -> bytes:
    try:
        decoded = base64.b64decode(raw, validate=True)
    except Exception:
        errors.append(f"key {key_id}: {field_name} is not valid base64")
        return b""
    if not decoded:
        errors.append(f"key {key_id}: {field_name} decoded to empty bytes")
    return decoded


def _resolve_key_material(entry: dict[str, Any], key_id: str, errors: list[str]) -> bytes:
    env_name = str(entry.get("material_env", "")).strip()
    if env_name:
        env_value = os.environ.get(env_name, "")
        if not env_value:
            errors.append(f"key {key_id}: material_env '{env_name}' is unset/empty")
            return b""
        return _parse_key_material_b64(env_value, key_id, f"material_env '{env_name}'", errors)
    return _parse_key_material_b64(str(entry.get("material_b64", "")), key_id, "material_b64", errors)


def validate_and_select_keyring(keyring_path: Path) -> tuple[dict[str, Any], dict[str, Any], list[str]]:
    payload = parse_json(keyring_path)
    errors: list[str] = []

    if payload.get("schema") != KEYRING_SCHEMA_VERSION:
        errors.append(
            f"keyring schema mismatch: {payload.get('schema')} (expected {KEYRING_SCHEMA_VERSION})"
        )

    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        errors.append("keyring must include a non-empty keys list")
        return payload, {}, errors

    key_by_id: dict[str, dict[str, Any]] = {}
    active_keys: list[dict[str, Any]] = []
    for entry in keys:
        if not isinstance(entry, dict):
            errors.append("keyring keys entries must be objects")
            continue
        key_id = str(entry.get("key_id", "")).strip()
        if not key_id:
            errors.append("key entry missing key_id")
            continue
        if key_id in key_by_id:
            errors.append(f"duplicate key_id in keyring: {key_id}")
            continue
        key_by_id[key_id] = entry
        if str(entry.get("status", "")).strip() == "active":
            active_keys.append(entry)
        if str(entry.get("algorithm", "")).strip() != "hmac-sha256":
            errors.append(f"key {key_id}: unsupported algorithm (expected hmac-sha256)")
        _resolve_key_material(entry, key_id, errors)

    active_key_id = str(payload.get("active_key_id", "")).strip()
    if not active_key_id:
        errors.append("keyring missing active_key_id")
    if len(active_keys) != 1:
        errors.append(f"keyring must declare exactly one active key (found {len(active_keys)})")

    selected = key_by_id.get(active_key_id, {})
    if not selected:
        errors.append(f"active_key_id not found in keys: {active_key_id}")
        return payload, {}, errors
    if str(selected.get("status", "")).strip() != "active":
        errors.append(f"active_key_id {active_key_id} does not have status=active")

    today_utc = datetime.now(timezone.utc).date()
    not_before_raw = str(selected.get("not_before", "")).strip()
    not_after_raw = str(selected.get("not_after", "")).strip()
    not_before = _parse_iso_date(not_before_raw, f"{active_key_id}.not_before", errors) if not_before_raw else None
    not_after = _parse_iso_date(not_after_raw, f"{active_key_id}.not_after", errors) if not_after_raw else None
    if not_before and not_after and not_before > not_after:
        errors.append(f"active key validity window is invalid: {not_before} > {not_after}")
    if not_before and today_utc < not_before:
        errors.append(f"active key is not yet valid: today={today_utc} not_before={not_before}")
    if not_after and today_utc > not_after:
        errors.append(f"active key expired: today={today_utc} not_after={not_after}")

    rotation = payload.get("rotation_policy", {})
    if not isinstance(rotation, dict):
        errors.append("rotation_policy must be an object")
        rotation = {}

    max_active_days = rotation.get("max_active_days")
    if max_active_days is not None:
        try:
            max_active_days_int = int(max_active_days)
        except Exception:
            errors.append(f"rotation_policy.max_active_days must be an integer: {max_active_days}")
            max_active_days_int = None
        if max_active_days_int is not None and not_before and not_after:
            active_span = (not_after - not_before).days + 1
            if active_span > max_active_days_int:
                errors.append(
                    f"active key window exceeds rotation policy: {active_span} > {max_active_days_int} days"
                )

    require_next = bool(rotation.get("require_next_key", False))
    if require_next:
        next_keys = [k for k in key_by_id.values() if str(k.get("status", "")).strip() == "next"]
        if not next_keys:
            errors.append("rotation_policy.require_next_key=true but no status=next key is present")

    return payload, selected, errors


def sign_manifest_payload(core_payload: dict[str, Any], key_entry: dict[str, Any]) -> str:
    key_id = str(key_entry.get("key_id", ""))
    key_material = _resolve_key_material(key_entry, key_id, [])
    return hmac.new(key_material, canonical_json(core_payload).encode("utf-8"), hashlib.sha256).hexdigest()


def verify_manifest_signature(
    core_payload: dict[str, Any], signature_hex: str, keyring_payload: dict[str, Any]
) -> tuple[bool, str]:
    keys = keyring_payload.get("keys")
    if not isinstance(keys, list):
        return False, ""
    payload_bytes = canonical_json(core_payload).encode("utf-8")
    for entry in keys:
        if not isinstance(entry, dict):
            continue
        key_id = str(entry.get("key_id", "")).strip()
        key_material = _resolve_key_material(entry, key_id, [])
        if not key_material:
            continue
        candidate = hmac.new(key_material, payload_bytes, hashlib.sha256).hexdigest()
        if hmac.compare_digest(candidate, signature_hex):
            return True, key_id
    return False, ""


def main() -> int:
    p = argparse.ArgumentParser(description="Build RFC-00A1 signed multi-lane evidence manifest.")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--evidence-bundle", required=True)
    p.add_argument("--vm-trace", required=True)
    p.add_argument("--cross-lane", required=True)
    p.add_argument("--backend-contract", required=True)
    p.add_argument("--ux-contract", required=True)
    p.add_argument("--tloadhash-toolchain", required=True)
    p.add_argument("--signing-keyring", required=True)
    p.add_argument("--governed-flow", default="")
    p.add_argument("--promotion-window-start", required=True, help="YYYY-MM-DD")
    p.add_argument("--promotion-window-end", required=True, help="YYYY-MM-DD")
    args = p.parse_args()

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    keyring_path = Path(args.signing_keyring).resolve()

    start = date.fromisoformat(args.promotion_window_start)
    end = date.fromisoformat(args.promotion_window_end)
    window_valid = start <= end

    lanes: list[dict[str, Any]] = []
    lanes.append(lane_entry("evidence_bundle", Path(args.evidence_bundle).resolve()))
    lanes.append(lane_entry("vm_trace", Path(args.vm_trace).resolve()))
    lanes.append(lane_entry("cross_lane", Path(args.cross_lane).resolve()))
    lanes.append(lane_entry("backend_contract", Path(args.backend_contract).resolve()))
    lanes.append(lane_entry("ux_contract", Path(args.ux_contract).resolve()))
    lanes.append(lane_entry("tloadhash_toolchain", Path(args.tloadhash_toolchain).resolve()))
    if args.governed_flow:
        governed_path = Path(args.governed_flow).resolve()
        if governed_path.exists():
            lanes.append(lane_entry("governed_flow", governed_path))

    errors: list[str] = []
    for lane in lanes:
        if lane["status"] != "pass":
            errors.append(f"{lane['lane']}: status={lane['status']} (expected pass)")
    if not window_valid:
        errors.append("promotion window is invalid: start must be <= end")
    keyring_payload, signing_key, keyring_errors = validate_and_select_keyring(keyring_path)
    errors.extend(keyring_errors)

    manifest_core = {
        "schema": SCHEMA_VERSION,
        "promotion_window": {
            "start": args.promotion_window_start,
            "end": args.promotion_window_end,
            "valid": window_valid,
        },
        "lanes": lanes,
        "attestor": "ai-experiments-ci",
        "attestation_method": "deterministic-sha256-attestation+hmac-sha256-keyring-signature.v1",
    }
    attestation_sha256 = sha256_bytes(canonical_json(manifest_core).encode("utf-8"))
    signature_hex = ""
    signature_verified = False
    verified_key_id = ""
    signing_key_id = ""
    if signing_key:
        signing_key_id = str(signing_key.get("key_id", "")).strip()
        signature_hex = sign_manifest_payload(manifest_core, signing_key)
        signature_verified, verified_key_id = verify_manifest_signature(
            manifest_core, signature_hex, keyring_payload
        )
        if not signature_verified:
            errors.append("manifest signature verification failed against provided keyring")

    status = "pass" if not errors else "fail"
    manifest = {
        **manifest_core,
        "status": status,
        "errors": errors,
        "attestation_sha256": attestation_sha256,
        "signature": {
            "algorithm": "hmac-sha256",
            "key_id": signing_key_id,
            "signature_hex": signature_hex,
            "verified": signature_verified,
            "verified_by_key_id": verified_key_id,
            "keyring_path": str(keyring_path),
            "keyring_sha256": sha256_file(keyring_path),
            "rotation_policy": keyring_payload.get("rotation_policy", {}),
        },
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    }

    json_path = out_dir / "ai_evidence_manifest.json"
    md_path = out_dir / "ai_evidence_manifest.md"
    json_path.write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Evidence Manifest",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- promotion_window: `{args.promotion_window_start}..{args.promotion_window_end}`",
                f"- attestation_sha256: `{attestation_sha256}`",
                f"- signature_key_id: `{signing_key_id or 'n/a'}`",
                f"- signature_verified: `{str(signature_verified).lower()}`",
                f"- lane_count: `{len(lanes)}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"ai evidence manifest status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
