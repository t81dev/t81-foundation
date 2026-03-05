#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import hmac
import json
import os
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.model-provenance.v2"
KEYRING_SCHEMA_VERSION = "t81.ai.model-provenance-keyring.v1"


@dataclass
class GateResult:
    ok: bool
    reason: str


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


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


def validate_keyring(path: Path) -> tuple[dict[str, Any], dict[str, Any], list[str]]:
    payload = parse_json(path)
    errors: list[str] = []

    if payload.get("schema") != KEYRING_SCHEMA_VERSION:
        errors.append(
            f"keyring schema mismatch: {payload.get('schema')} (expected {KEYRING_SCHEMA_VERSION})"
        )

    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        errors.append("keyring keys must be a non-empty list")
        return payload, {}, errors

    key_by_id: dict[str, dict[str, Any]] = {}
    active_keys: list[dict[str, Any]] = []
    for key in keys:
        if not isinstance(key, dict):
            errors.append("keyring key entry must be an object")
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
            errors.append(f"key {key_id}: empty decoded key material")

    active_key_id = str(payload.get("active_key_id", "")).strip()
    if not active_key_id:
        errors.append("keyring missing active_key_id")
    if len(active_keys) != 1:
        errors.append(f"keyring must contain exactly one active key (found {len(active_keys)})")

    active = key_by_id.get(active_key_id, {})
    if not active:
        errors.append(f"active_key_id not found in key list: {active_key_id}")
    elif str(active.get("status", "")).strip() != "active":
        errors.append(f"active_key_id {active_key_id} does not point to status=active key")

    rotation = payload.get("rotation_policy", {})
    if not isinstance(rotation, dict):
        errors.append("rotation_policy must be an object")
    elif bool(rotation.get("require_next_key", False)):
        has_next = any(str(k.get("status", "")).strip() == "next" for k in key_by_id.values())
        if not has_next:
            errors.append("rotation_policy.require_next_key=true but no next key exists")

    return payload, active, errors


def sign_payload_hex(payload: dict[str, Any], key_entry: dict[str, Any]) -> str:
    material = resolve_key_material(key_entry, [])
    return hmac.new(material, canonical_json(payload).encode("utf-8"), hashlib.sha256).hexdigest()


def verify_signature(payload: dict[str, Any], signature_hex: str, keyring: dict[str, Any]) -> tuple[bool, str]:
    keys = keyring.get("keys")
    if not isinstance(keys, list):
        return False, ""
    payload_bytes = canonical_json(payload).encode("utf-8")
    for key in keys:
        if not isinstance(key, dict):
            continue
        key_id = str(key.get("key_id", "")).strip()
        material = resolve_key_material(key, [])
        if not material:
            continue
        candidate = hmac.new(material, payload_bytes, hashlib.sha256).hexdigest()
        if hmac.compare_digest(candidate, signature_hex):
            return True, key_id
    return False, ""


def expected_canonfs_object_id(model_hash_hex: str) -> str:
    return f"canonfs://model/sha256/{model_hash_hex}"


def build_chain_entry(
    seq: int,
    event: str,
    prev_entry_sha256: str,
    model_hash_hex: str,
    canonfs_object_id: str,
    key_entry: dict[str, Any],
) -> dict[str, Any]:
    entry_core = {
        "seq": seq,
        "event": event,
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "model_hash": f"sha256:{model_hash_hex}",
        "canonfs_object_id": canonfs_object_id,
        "prev_entry_sha256": prev_entry_sha256,
    }
    entry_sha256 = sha256_text(canonical_json(entry_core))
    sig_payload = {
        "entry_sha256": entry_sha256,
        "seq": entry_core["seq"],
    }
    signature_hex = sign_payload_hex(sig_payload, key_entry)
    return {
        **entry_core,
        "entry_sha256": entry_sha256,
        "signature": {
            "algorithm": "hmac-sha256",
            "key_id": str(key_entry.get("key_id", "")).strip(),
            "signature_hex": signature_hex,
        },
    }


def write_manifest(path: Path, model_path: Path, model_hash_hex: str, keyring_path: Path) -> None:
    keyring_payload, active_key, errors = validate_keyring(keyring_path)
    if errors:
        raise RuntimeError("invalid provenance keyring: " + "; ".join(errors))

    canonfs_object_id = expected_canonfs_object_id(model_hash_hex)
    chain_events = [
        "artifact_ingest",
        "artifact_promotion_candidate",
    ]
    entries: list[dict[str, Any]] = []
    prev_sha = ""
    for seq, event in enumerate(chain_events, start=1):
        entry = build_chain_entry(
            seq=seq,
            event=event,
            prev_entry_sha256=prev_sha,
            model_hash_hex=model_hash_hex,
            canonfs_object_id=canonfs_object_id,
            key_entry=active_key,
        )
        entries.append(entry)
        prev_sha = str(entry.get("entry_sha256", "")).strip()

    payload = {
        "schema": SCHEMA_VERSION,
        "model_path": str(model_path),
        "model_hash": f"sha256:{model_hash_hex}",
        "canonfs_identity": {
            "scheme": "canonfs-sha256",
            "object_id": canonfs_object_id,
        },
        "provenance_chain": {
            "schema": "t81.ai.model-provenance-chain.v1",
            "keyring_path": str(keyring_path),
            "keyring_sha256": sha256_file(keyring_path),
            "entries": entries,
        },
    }
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def gate_model_load(
    model_path: Path,
    manifest_path: Path,
    keyring_path: Path,
    expected_hash: str | None = None,
    min_lineage_entries: int = 1,
) -> GateResult:
    if not model_path.exists():
        return GateResult(False, f"missing model file: {model_path}")
    if not manifest_path.exists():
        return GateResult(False, f"missing manifest: {manifest_path}")
    if not keyring_path.exists():
        return GateResult(False, f"missing signing keyring: {keyring_path}")

    manifest = parse_json(manifest_path)
    if manifest.get("schema") != SCHEMA_VERSION:
        return GateResult(False, f"manifest schema mismatch: {manifest.get('schema')} != {SCHEMA_VERSION}")

    manifest_hash = str(manifest.get("model_hash", "")).strip()
    if not manifest_hash.startswith("sha256:"):
        return GateResult(False, "manifest model_hash missing sha256: prefix")

    actual_hash = sha256_file(model_path)
    expected = expected_hash.strip() if expected_hash else manifest_hash
    if not expected.startswith("sha256:"):
        expected = f"sha256:{expected}"

    if manifest_hash != f"sha256:{actual_hash}":
        return GateResult(False, "manifest hash does not match model file")
    if expected != f"sha256:{actual_hash}":
        return GateResult(False, "provided expected hash does not match model file")

    canonfs_identity = manifest.get("canonfs_identity")
    if not isinstance(canonfs_identity, dict):
        return GateResult(False, "manifest missing canonfs_identity object")
    object_id = str(canonfs_identity.get("object_id", "")).strip()
    expected_object_id = expected_canonfs_object_id(actual_hash)
    if object_id != expected_object_id:
        return GateResult(False, f"canonfs object_id mismatch: {object_id} != {expected_object_id}")

    keyring_payload, _, keyring_errors = validate_keyring(keyring_path)
    if keyring_errors:
        return GateResult(False, "invalid signing keyring: " + "; ".join(keyring_errors))

    chain = manifest.get("provenance_chain")
    if not isinstance(chain, dict):
        return GateResult(False, "manifest missing provenance_chain object")
    if chain.get("schema") != "t81.ai.model-provenance-chain.v1":
        return GateResult(False, "provenance_chain schema mismatch")
    if str(chain.get("keyring_sha256", "")).strip() != sha256_file(keyring_path):
        return GateResult(False, "provenance_chain keyring_sha256 mismatch")

    entries = chain.get("entries")
    if not isinstance(entries, list) or not entries:
        return GateResult(False, "provenance_chain.entries must be a non-empty list")
    if len(entries) < min_lineage_entries:
        return GateResult(
            False,
            f"provenance_chain.entries must contain at least {min_lineage_entries} entries (found {len(entries)})",
        )

    prev_sha = ""
    for idx, entry in enumerate(entries, start=1):
        if not isinstance(entry, dict):
            return GateResult(False, f"chain entry {idx} is not an object")
        if entry.get("seq") != idx:
            return GateResult(False, f"chain entry seq mismatch at index {idx}")

        core = {
            "seq": entry.get("seq"),
            "event": entry.get("event"),
            "timestamp_utc": entry.get("timestamp_utc"),
            "model_hash": entry.get("model_hash"),
            "canonfs_object_id": entry.get("canonfs_object_id"),
            "prev_entry_sha256": entry.get("prev_entry_sha256"),
        }
        if core["model_hash"] != f"sha256:{actual_hash}":
            return GateResult(False, f"chain entry {idx} model_hash mismatch")
        if core["canonfs_object_id"] != expected_object_id:
            return GateResult(False, f"chain entry {idx} canonfs_object_id mismatch")
        if core["prev_entry_sha256"] != prev_sha:
            return GateResult(False, f"chain entry {idx} prev_entry_sha256 mismatch")

        expected_entry_sha = sha256_text(canonical_json(core))
        entry_sha = str(entry.get("entry_sha256", "")).strip()
        if entry_sha != expected_entry_sha:
            return GateResult(False, f"chain entry {idx} entry_sha256 mismatch")

        signature = entry.get("signature")
        if not isinstance(signature, dict):
            return GateResult(False, f"chain entry {idx} missing signature object")
        if str(signature.get("algorithm", "")).strip() != "hmac-sha256":
            return GateResult(False, f"chain entry {idx} signature algorithm mismatch")
        signature_hex = str(signature.get("signature_hex", "")).strip()
        if not signature_hex:
            return GateResult(False, f"chain entry {idx} signature_hex missing")

        sig_payload = {
            "entry_sha256": entry_sha,
            "seq": idx,
        }
        verified, _ = verify_signature(sig_payload, signature_hex, keyring_payload)
        if not verified:
            return GateResult(False, f"chain entry {idx} signature verification failed")

        prev_sha = entry_sha

    return GateResult(True, "provenance gate passed")


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Check AI model provenance hash gate behavior.")
    p.add_argument("--model", required=True, help="Model file path")
    p.add_argument("--manifest", required=True, help="Manifest path")
    p.add_argument(
        "--expected-hash",
        default="",
        help="Optional expected hash (sha256:<hex> or <hex>). If omitted, manifest hash is authoritative.",
    )
    p.add_argument(
        "--signing-keyring",
        default="",
        help="Path to model provenance keyring JSON (defaults to scripts/ci/ai_model_provenance_keyring.json).",
    )
    p.add_argument(
        "--self-test-deny",
        action="store_true",
        help="Also run a negative test with a bad hash and require deterministic denial.",
    )
    p.add_argument(
        "--min-lineage-entries",
        type=int,
        default=2,
        help="Require at least this many provenance_chain entries (default: 2).",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    model_path = Path(args.model).resolve()
    manifest_path = Path(args.manifest).resolve()
    default_keyring = Path(__file__).resolve().with_name("ai_model_provenance_keyring.json")
    keyring_path = Path(args.signing_keyring).resolve() if args.signing_keyring else default_keyring

    if not model_path.exists():
        model_path.parent.mkdir(parents=True, exist_ok=True)
        model_path.write_text("t81-model-provenance-fixture\n", encoding="utf-8")

    computed = sha256_file(model_path)
    if not manifest_path.exists():
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        write_manifest(manifest_path, model_path, computed, keyring_path)

    positive = gate_model_load(
        model_path,
        manifest_path,
        keyring_path,
        args.expected_hash if args.expected_hash else None,
        max(1, int(args.min_lineage_entries)),
    )
    if not positive.ok:
        print(f"[FAIL] positive gate check: {positive.reason}")
        return 1
    print(f"[PASS] positive gate check: {positive.reason}")

    if args.self_test_deny:
        negative = gate_model_load(
            model_path,
            manifest_path,
            keyring_path,
            "sha256:" + ("0" * 64),
            max(1, int(args.min_lineage_entries)),
        )
        if negative.ok:
            print("[FAIL] negative gate check: expected denial but gate passed")
            return 1
        print(f"[PASS] negative gate check: denied as expected ({negative.reason})")

    print("model provenance gate: pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
