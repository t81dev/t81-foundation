#!/usr/bin/env python3
"""RFC-00D4 Accountable Decision Bundle (ADB) emitter and offline verifier.

Experimental prototype binding T81's bounded AI task outputs, CanonFS refs,
and Axion policy hashes into a signed RFC-00D4 bundle with detached Ed25519
verification and optional deterministic replay.
"""

from __future__ import annotations

import argparse
import base64
import datetime as dt
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


REQUIRED_TOP_LEVEL = (
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


def sign_ed25519(private_key_pem: Path, data: bytes) -> str:
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf.write(data)
        msg_path = Path(tf.name)
    try:
        proc = subprocess.run(
            [
                "openssl",
                "pkeyutl",
                "-sign",
                "-inkey",
                str(private_key_pem),
                "-rawin",
                "-in",
                str(msg_path),
            ],
            capture_output=True,
            check=True,
        )
        return base64.b64encode(proc.stdout).decode("ascii")
    finally:
        msg_path.unlink(missing_ok=True)


def verify_ed25519(public_key_pem: Path, data: bytes, signature_b64: str) -> bool:
    sig_bytes = base64.b64decode(signature_b64.encode("ascii"), validate=True)
    with tempfile.NamedTemporaryFile(delete=False) as mf, tempfile.NamedTemporaryFile(
        delete=False
    ) as sf:
        mf.write(data)
        sf.write(sig_bytes)
        msg_path = Path(mf.name)
        sig_path = Path(sf.name)
    try:
        proc = subprocess.run(
            [
                "openssl",
                "pkeyutl",
                "-verify",
                "-pubin",
                "-inkey",
                str(public_key_pem),
                "-rawin",
                "-in",
                str(msg_path),
                "-sigfile",
                str(sig_path),
            ],
            capture_output=True,
        )
        return proc.returncode == 0
    finally:
        msg_path.unlink(missing_ok=True)
        sig_path.unlink(missing_ok=True)


def extract_allowed_model_hashes(policy_text: str) -> list[str]:
    match = re.search(r"\(allowed-ternary-model-hashes\s+\[(.*?)\]\)", policy_text, re.S)
    if not match:
        return []
    return re.findall(r'"([^"]+)"', match.group(1))


def cmd_emit(args: argparse.Namespace) -> int:
    task_data = json.loads(Path(args.task_json).read_text(encoding="utf-8"))
    if task_data.get("status") != "pass" or task_data.get("policy_result") != "allowed":
        print("error: cannot emit ADB from non-passing or denied task output", file=sys.stderr)
        return 1

    now_utc = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    policy_path = Path(args.policy)
    policy_sha3_512 = "sha3-512:" + hashlib.sha3_512(policy_path.read_bytes()).hexdigest()

    bundle: dict[str, Any] = {
        "schema_version": "1.0.0-draft",
        "bundle_id": task_data["result_ref"],
        "timestamp_utc": now_utc,
        "meaning": {
            "action_type": task_data["task_kind"],
            "parameters": {
                "model_id": task_data["model_id"],
                "input_hash": task_data["input_hash"],
                "decision": task_data["decision"],
                "reason_code": task_data["reason_code"],
            },
            "proposed_side_effects": [
                {
                    "target": "host_action_gate",
                    "mutation": f"{task_data['decision']}:{task_data['reason_code']}",
                }
            ],
        },
        "evidence": {
            "input_digest": task_data["input_hash"],
            "output_digest": task_data["result_ref"],
            "execution_trace_hash": task_data["provenance_ref"],
            "intermediate_state_hashes": [
                task_data["result_ref"],
                task_data["provenance_ref"],
            ],
        },
        "method": {
            "runtime_environment": task_data["selected_backend"],
            "transformation_id": task_data["task_kind"],
            "model_weights_digest": task_data["model_hash"],
            "hyperparameters": {
                "mode": task_data["requested_mode"],
            },
        },
        "authority": {
            "policy_id": policy_path.name,
            "policy_hash": policy_sha3_512,
            "canonfs_policy_hash": task_data["policy_hash"],
            "verdict": "ALLOW",
            "evaluation_timestamp": now_utc,
            "matched_rules": ["allowed-ternary-model-hashes"],
        },
        "challengeability": {
            "replayable": True,
            "verification_procedure": "t81 ai task assess-fixed --mode strict_deterministic",
            "audit_package_uri": task_data["result_ref"],
        },
    }

    sig_b64 = sign_ed25519(Path(args.signing_key), canonical_signing_bytes(bundle))
    bundle["accountability"] = {
        "principal_id": args.principal_id,
        "public_key_uri": args.public_key_uri,
        "signing_scheme": "ed25519-canonical-json-v1",
        "signature": sig_b64,
    }

    Path(args.out).write_text(json.dumps(bundle, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"ok": True, "bundle_id": bundle["bundle_id"], "out": args.out}))
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    bundle = json.loads(Path(args.bundle).read_text(encoding="utf-8"))

    # 1. Schema Validation (RFC-00D4 §5.1)
    missing = [k for k in REQUIRED_TOP_LEVEL if k not in bundle]
    if missing:
        print(json.dumps({"ok": False, "stage": "schema", "error": f"missing fields: {missing}"}))
        return 1
    if bundle.get("schema_version") != "1.0.0-draft":
        print(json.dumps({"ok": False, "stage": "schema", "error": "unsupported schema_version"}))
        return 1

    # 2. Authority Check (RFC-00D4 §5.2)
    authority = bundle["authority"]
    if authority.get("verdict") != "ALLOW":
        print(json.dumps({"ok": False, "stage": "authority", "error": "verdict is not ALLOW"}))
        return 1

    if args.trusted_policy:
        policy_path = Path(args.trusted_policy)
        policy_bytes = policy_path.read_bytes()
        trusted_sha3_512 = "sha3-512:" + hashlib.sha3_512(policy_bytes).hexdigest()
        if authority.get("policy_hash") != trusted_sha3_512:
            print(
                json.dumps(
                    {
                        "ok": False,
                        "stage": "authority",
                        "error": "policy_hash mismatch against trusted policy (possible mid-flight policy mutation/revocation)",
                    }
                )
            )
            return 1
        allowed_models = extract_allowed_model_hashes(policy_bytes.decode("utf-8"))
        if bundle["method"].get("model_weights_digest") not in allowed_models:
            print(
                json.dumps(
                    {
                        "ok": False,
                        "stage": "authority",
                        "error": "model_weights_digest not admitted by trusted policy",
                    }
                )
            )
            return 1

    # 3. Cryptographic Integrity (RFC-00D4 §5.3)
    sig = bundle["accountability"].get("signature", "")
    try:
        sig_ok = verify_ed25519(Path(args.public_key), canonical_signing_bytes(bundle), sig)
    except Exception as exc:
        print(json.dumps({"ok": False, "stage": "signature", "error": str(exc)}))
        return 1
    if not sig_ok:
        print(json.dumps({"ok": False, "stage": "signature", "error": "Ed25519 signature invalid"}))
        return 1

    # 4. Evidence Replay (RFC-00D4 §5.4)
    if args.replay:
        if not (args.t81_bin and args.model_file and args.trusted_policy and args.input_text and args.canonfs_root):
            print(
                json.dumps(
                    {
                        "ok": False,
                        "stage": "replay",
                        "error": "--replay requires --t81-bin, --model-file, --trusted-policy, --input-text, --canonfs-root",
                    }
                )
            )
            return 1
        proc = subprocess.run(
            [
                args.t81_bin,
                "ai",
                "task",
                "assess-fixed",
                "--model",
                bundle["meaning"]["parameters"]["model_id"],
                "--model-file",
                args.model_file,
                "--policy",
                args.trusted_policy,
                "--canonfs-root",
                args.canonfs_root,
                "--mode",
                "strict_deterministic",
                "--input",
                args.input_text,
            ],
            capture_output=True,
            text=True,
        )
        if proc.returncode != 0:
            print(
                json.dumps(
                    {
                        "ok": False,
                        "stage": "replay",
                        "error": f"replay execution trapped/failed: {proc.stderr.strip() or proc.stdout.strip()}",
                    }
                )
            )
            return 1
        replay_data = json.loads(proc.stdout)
        if replay_data.get("result_ref") != bundle["evidence"]["output_digest"]:
            print(
                json.dumps(
                    {
                        "ok": False,
                        "stage": "replay",
                        "error": "replayed result_ref does not match bundle output_digest",
                    }
                )
            )
            return 1

    print(
        json.dumps(
            {
                "ok": True,
                "bundle_id": bundle["bundle_id"],
                "principal_id": bundle["accountability"]["principal_id"],
                "signature_verified": True,
                "policy_verified": bool(args.trusted_policy),
                "replay_verified": bool(args.replay),
            }
        )
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="RFC-00D4 Accountable Decision Bundle tool")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_emit = sub.add_parser("emit")
    p_emit.add_argument("--task-json", required=True)
    p_emit.add_argument("--policy", required=True)
    p_emit.add_argument("--signing-key", required=True)
    p_emit.add_argument("--public-key-uri", required=True)
    p_emit.add_argument("--principal-id", required=True)
    p_emit.add_argument("--out", required=True)
    p_emit.set_defaults(func=cmd_emit)

    p_ver = sub.add_parser("verify")
    p_ver.add_argument("--bundle", required=True)
    p_ver.add_argument("--public-key", required=True)
    p_ver.add_argument("--trusted-policy")
    p_ver.add_argument("--replay", action="store_true")
    p_ver.add_argument("--t81-bin")
    p_ver.add_argument("--model-file")
    p_ver.add_argument("--input-text")
    p_ver.add_argument("--canonfs-root")
    p_ver.set_defaults(func=cmd_verify)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
