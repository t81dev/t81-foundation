#!/usr/bin/env python3
"""T81 Agent-Native Receipt & Policy Sidecar (MCP / JSON-RPC 2.0 over stdio).

Provides a warm, resident verification and policy-gating boundary for AI agents:
  - t81.gate_and_seal: Evaluates Axion .apl policy pre-side-effect, seals
    artifact into CanonFS, and emits a signed RFC-00D4 decision bundle.
  - t81.verify_receipt: Offline verifies RFC-00D4 schema, policy hash +
    admission against trusted .apl, Ed25519 signature, and optional CanonFS fsck.
  - t81.canonfs_fsck: Runs `t81 canonfs fsck --verify-hashes --json`.
  - t81.model_diff: Runs `t81 model diff <lhs> <rhs> --json`.
"""

from __future__ import annotations

import base64
import datetime as dt
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT_DIR = Path(__file__).resolve().parents[3]
DEFAULT_T81_BIN = os.environ.get("T81_BIN", str(ROOT_DIR / "build" / "t81"))

REQUIRED_ADB_FIELDS = (
    "schema_version",
    "bundle_id",
    "timestamp_utc",
    "meaning",
    "evidence",
    "method",
    "authority",
    "accountability",
    "challengeability",
)


def canonical_signing_bytes(bundle: dict[str, Any]) -> bytes:
    payload = {
        "meaning": bundle["meaning"],
        "evidence": bundle["evidence"],
        "authority": bundle["authority"],
    }
    return json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")


def sign_ed25519(private_key_pem: str, data: bytes) -> str:
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf.write(data)
        msg_path = Path(tf.name)
    try:
        proc = subprocess.run(
            ["openssl", "pkeyutl", "-sign", "-inkey", private_key_pem, "-rawin", "-in", str(msg_path)],
            capture_output=True,
            check=True,
        )
        return base64.b64encode(proc.stdout).decode("ascii")
    finally:
        msg_path.unlink(missing_ok=True)


def verify_ed25519(public_key_pem: str, data: bytes, signature_b64: str) -> bool:
    try:
        sig_bytes = base64.b64decode(signature_b64.encode("ascii"), validate=True)
    except Exception:
        return False
    with tempfile.NamedTemporaryFile(delete=False) as mf, tempfile.NamedTemporaryFile(delete=False) as sf:
        mf.write(data)
        sf.write(sig_bytes)
        msg_path = Path(mf.name)
        sig_path = Path(sf.name)
    try:
        proc = subprocess.run(
            [
                "openssl", "pkeyutl", "-verify", "-pubin",
                "-inkey", public_key_pem, "-rawin",
                "-in", str(msg_path), "-sigfile", str(sig_path),
            ],
            capture_output=True,
        )
        return proc.returncode == 0
    finally:
        msg_path.unlink(missing_ok=True)
        sig_path.unlink(missing_ok=True)


def extract_allowed_hashes(policy_text: str) -> list[str]:
    match = re.search(r"\(allowed-ternary-model-hashes\s+\[(.*?)\]\)", policy_text, re.S)
    if not match:
        return []
    return re.findall(r'"([^"]+)"', match.group(1))


MODE_RANK = {"observe-only": 0, "warn": 1, "fail-closed": 2}


def extract_policy_mode(policy_text: str, action_type: str | None = None) -> str:
    """Resolve enforcement floor from .apl: action-policy overrides top-level, then env/fail-closed."""
    if action_type:
        ap_pat = (
            r'\(action-policy\s+"'
            + re.escape(action_type)
            + r'"[^)]*\(enforcement-mode\s+"(observe-only|warn|fail-closed)"\)'
        )
        ap_match = re.search(ap_pat, policy_text, re.S)
        if ap_match:
            return ap_match.group(1)
    # Strip action-policy blocks before checking top-level default enforcement-mode
    stripped = re.sub(r'\(action-policy\s+"[^"]+"[^)]*\([^)]*\)[^)]*\)', "", policy_text, flags=re.S)
    match = re.search(r'\(enforcement-mode\s+"(observe-only|warn|fail-closed)"\)', stripped)
    if match:
        return match.group(1)
    env_mode = os.environ.get("T81_ENFORCEMENT_MODE", "fail-closed")
    return env_mode if env_mode in MODE_RANK else "fail-closed"


def resolve_enforcement_mode(policy_mode: str, requested_mode: str | None) -> tuple[str, bool]:
    """Resolve effective mode. Callers may tighten mode, NEVER downgrade policy floor."""
    if not requested_mode or requested_mode not in MODE_RANK:
        return policy_mode, False
    if MODE_RANK[requested_mode] < MODE_RANK[policy_mode]:
        return policy_mode, True
    return requested_mode, False


def tool_canonfs_fsck(args: dict[str, Any]) -> dict[str, Any]:
    t81_bin = args.get("t81_bin", DEFAULT_T81_BIN)
    canonfs_root = args["canonfs_root"]
    proc = subprocess.run(
        [t81_bin, "canonfs", "fsck", "--verify-hashes", "--canonfs-root", canonfs_root, "--json"],
        capture_output=True,
        text=True,
    )
    try:
        data = json.loads(proc.stdout)
    except Exception:
        data = {"ok": False, "error": proc.stderr.strip() or proc.stdout.strip()}
    return data


def tool_model_diff(args: dict[str, Any]) -> dict[str, Any]:
    t81_bin = args.get("t81_bin", DEFAULT_T81_BIN)
    proc = subprocess.run(
        [t81_bin, "model", "diff", args["lhs"], args["rhs"], "--json"],
        capture_output=True,
        text=True,
    )
    try:
        return json.loads(proc.stdout)
    except Exception:
        return {"ok": False, "error": proc.stderr.strip() or proc.stdout.strip()}


def tool_gate_and_seal(args: dict[str, Any]) -> dict[str, Any]:
    t81_bin = args.get("t81_bin", DEFAULT_T81_BIN)
    artifact_path = Path(args["artifact_path"])
    policy_path = Path(args["policy_path"])
    canonfs_root = args["canonfs_root"]
    signing_key = args["signing_key"]
    public_key_uri = args["public_key_uri"]
    principal_id = args["principal_id"]
    action_type = args.get("action_type", "agent.artifact.promote.v1")
    target = args.get("target", "host_action_gate")

    if not artifact_path.is_file() or not policy_path.is_file():
        return {"ok": False, "verdict": "DENY", "reason": "missing artifact or policy file"}

    artifact_bytes = artifact_path.read_bytes()
    artifact_sha3_512 = "sha3-512:" + hashlib.sha3_512(artifact_bytes).hexdigest()
    policy_bytes = policy_path.read_bytes()
    policy_text = policy_bytes.decode("utf-8")
    policy_sha3_512 = "sha3-512:" + hashlib.sha3_512(policy_bytes).hexdigest()
    allowed = extract_allowed_hashes(policy_text)

    policy_mode = extract_policy_mode(policy_text, action_type)
    effective_mode, downgrade_attempt = resolve_enforcement_mode(policy_mode, args.get("mode"))
    admitted = artifact_sha3_512 in allowed

    # Pre-side-effect policy check (fail closed when effective_mode == "fail-closed")
    if not admitted and effective_mode == "fail-closed":
        return {
            "ok": False,
            "decision": "DENY",
            "verdict": "DENY",
            "policy_floor": policy_mode,
            "mode_applied": effective_mode,
            "downgrade_attempt_blocked": downgrade_attempt,
            "blocked": True,
            "receipt": None,
            "reason": f"SecurityFault: artifact {artifact_sha3_512} not admitted by {policy_path.name}",
            "side_effect_executed": False,
        }

    # Seal into CanonFS via t81 canonfs import
    imp = subprocess.run(
        [t81_bin, "canonfs", "import", str(artifact_path), "--canonfs-root", canonfs_root, "--json"],
        capture_output=True,
        text=True,
    )
    if imp.returncode != 0:
        return {"ok": False, "decision": "DENY", "verdict": "DENY", "blocked": True, "reason": f"canonfs import failed: {imp.stderr.strip()}"}
    imp_data = json.loads(imp.stdout)
    canon_ref = imp_data["imported_objects"][0]
    prov_ref = imp_data["provenance_ref"]

    # Truthful verdict: never emit ALLOW in observe-only/warn when policy denied
    if admitted:
        true_verdict = "ALLOW"
        reason = "admitted by policy"
    elif effective_mode == "warn":
        true_verdict = "WARN"
        reason = f"WARNING: artifact {artifact_sha3_512} not admitted by {policy_path.name} (allowed only due to warn mode)"
    else:
        true_verdict = "DENY"
        reason = f"OBSERVE-ONLY: artifact {artifact_sha3_512} would be denied by {policy_path.name}"

    now_utc = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    bundle: dict[str, Any] = {
        "schema_version": "1.0.0-draft",
        "bundle_id": canon_ref,
        "timestamp_utc": now_utc,
        "meaning": {
            "action_type": action_type,
            "parameters": {
                "artifact_name": artifact_path.name,
                "artifact_sha3_512": artifact_sha3_512,
                "canon_ref": canon_ref,
            },
            "proposed_side_effects": [{"target": target, "mutation": true_verdict}],
        },
        "evidence": {
            "input_digest": artifact_sha3_512,
            "output_digest": canon_ref,
            "execution_trace_hash": prov_ref,
            "intermediate_state_hashes": [canon_ref, prov_ref],
        },
        "method": {
            "runtime_environment": "t81-receipt-sidecar-v1",
            "transformation_id": action_type,
            "model_weights_digest": artifact_sha3_512,
        },
        "authority": {
            "policy_id": policy_path.name,
            "policy_hash": policy_sha3_512,
            "verdict": true_verdict,
            "enforcement_mode": effective_mode,
            "evaluation_timestamp": now_utc,
            "matched_rules": ["allowed-ternary-model-hashes"] if admitted else [],
        },
        "challengeability": {
            "replayable": True,
            "verification_procedure": "t81.verify_receipt + t81 canonfs fsck --verify-hashes",
            "audit_package_uri": canon_ref,
        },
    }
    bundle["accountability"] = {
        "principal_id": principal_id,
        "public_key_uri": public_key_uri,
        "signing_scheme": "ed25519-canonical-json-v1",
        "signature": sign_ed25519(signing_key, canonical_signing_bytes(bundle)),
    }
    return {
        "ok": True,
        "decision": true_verdict,
        "verdict": true_verdict,
        "policy_floor": policy_mode,
        "mode_applied": effective_mode,
        "downgrade_attempt_blocked": downgrade_attempt,
        "blocked": False,
        "reason": reason,
        "bundle": bundle,
        "receipt": bundle,
    }


def tool_get_effective_policy(args: dict[str, Any]) -> dict[str, Any]:
    policy_path = Path(args["policy_path"])
    if not policy_path.is_file():
        return {"ok": False, "error": f"missing policy file: {policy_path}"}
    policy_bytes = policy_path.read_bytes()
    policy_text = policy_bytes.decode("utf-8")
    action_type = args.get("action_type")
    policy_floor = extract_policy_mode(policy_text, action_type)
    return {
        "ok": True,
        "policy_id": policy_path.name,
        "policy_hash": "sha3-512:" + hashlib.sha3_512(policy_bytes).hexdigest(),
        "action_type": action_type,
        "policy_floor": policy_floor,
        "allowed_hash_count": len(extract_allowed_hashes(policy_text)),
    }


def tool_verify_receipt(args: dict[str, Any]) -> dict[str, Any]:
    bundle = args["bundle"]
    public_key = args["public_key"]
    trusted_policy = args.get("trusted_policy")
    verify_canonfs_root = args.get("canonfs_root")

    # 1. Schema check
    missing = [k for k in REQUIRED_ADB_FIELDS if k not in bundle]
    if missing or bundle.get("schema_version") != "1.0.0-draft":
        return {"ok": False, "stage": "schema", "error": f"invalid or missing fields: {missing}"}

    # 2. Authority check
    authority = bundle["authority"]
    if authority.get("verdict") != "ALLOW":
        return {"ok": False, "stage": "authority", "error": "verdict is not ALLOW"}
    if trusted_policy:
        policy_bytes = Path(trusted_policy).read_bytes()
        trusted_hash = "sha3-512:" + hashlib.sha3_512(policy_bytes).hexdigest()
        if authority.get("policy_hash") != trusted_hash:
            return {"ok": False, "stage": "authority", "error": "policy_hash mismatch (mid-flight revocation/drift)"}
        allowed = extract_allowed_hashes(policy_bytes.decode("utf-8"))
        if bundle["method"].get("model_weights_digest") not in allowed:
            return {"ok": False, "stage": "authority", "error": "artifact digest not admitted by trusted policy"}

    # 3. Cryptographic signature check
    sig = bundle["accountability"].get("signature", "")
    if not verify_ed25519(public_key, canonical_signing_bytes(bundle), sig):
        return {"ok": False, "stage": "signature", "error": "Ed25519 signature invalid"}

    # 4. Optional CanonFS store integrity check
    if verify_canonfs_root:
        fsck = tool_canonfs_fsck({"canonfs_root": verify_canonfs_root, "t81_bin": args.get("t81_bin", DEFAULT_T81_BIN)})
        if not fsck.get("ok"):
            return {"ok": False, "stage": "canonfs", "error": "CanonFS fsck --verify-hashes failed", "fsck": fsck}

    return {
        "ok": True,
        "bundle_id": bundle["bundle_id"],
        "principal_id": bundle["accountability"]["principal_id"],
        "signature_verified": True,
        "policy_verified": bool(trusted_policy),
        "canonfs_verified": bool(verify_canonfs_root),
    }


TOOLS = {
    "t81.gate_and_seal": {
        "description": "Evaluate Axion .apl policy pre-side-effect, seal artifact into CanonFS, and emit signed RFC-00D4 bundle.",
        "handler": tool_gate_and_seal,
    },
    "t81.verify_receipt": {
        "description": "Verify an RFC-00D4 Accountable Decision Bundle offline (schema, policy, Ed25519 signature, optional CanonFS fsck).",
        "handler": tool_verify_receipt,
    },
    "t81.get_effective_policy": {
        "description": "Introspect the authoritative .apl policy floor and digest for a given action class.",
        "handler": tool_get_effective_policy,
    },
    "t81.canonfs_fsck": {
        "description": "Run `t81 canonfs fsck --verify-hashes --json` on a CanonFS root.",
        "handler": tool_canonfs_fsck,
    },
    "t81.model_diff": {
        "description": "Run `t81 model diff <lhs> <rhs> --json` to compare model artifacts.",
        "handler": tool_model_diff,
    },
}


def handle_request(req: dict[str, Any]) -> dict[str, Any]:
    req_id = req.get("id")
    method = req.get("method")
    params = req.get("params") or {}

    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": "2024-11-05",
                "serverInfo": {"name": "t81-receipt-sidecar", "version": "0.1.0"},
                "capabilities": {"tools": {}},
            },
        }
    if method == "tools/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "tools": [{"name": k, "description": v["description"]} for k, v in TOOLS.items()]
            },
        }
    if method == "tools/call":
        name = params.get("name")
        arguments = params.get("arguments") or {}
        if name not in TOOLS:
            return {"jsonrpc": "2.0", "id": req_id, "error": {"code": -32601, "message": f"unknown tool: {name}"}}
        result = TOOLS[name]["handler"](arguments)
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {"content": [{"type": "text", "text": json.dumps(result)}], "structured": result},
        }
    return {"jsonrpc": "2.0", "id": req_id, "error": {"code": -32601, "message": f"unknown method: {method}"}}


def main() -> int:
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
            resp = handle_request(req)
        except Exception as exc:
            resp = {"jsonrpc": "2.0", "id": None, "error": {"code": -32000, "message": str(exc)}}
        sys.stdout.write(json.dumps(resp) + "\n")
        sys.stdout.flush()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
