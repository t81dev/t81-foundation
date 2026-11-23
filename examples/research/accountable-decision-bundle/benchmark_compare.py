#!/usr/bin/env python3
"""Comparative benchmark for Hypothesis A (Computation as Evidence).

Compares two implementations on identical RFC-00D4 decision-bundle semantics
and adversarial checks:
  (A) T81 Substrate: t81 ai task assess-fixed + CanonFS + Axion .apl + adb_tool.py
  (B) Lightweight Python Control: pure-Python deterministic evaluator + SHA3-512
      file store + JSON policy + Ed25519 (openssl pkeyutl)

Reports measured wall-clock latencies, unauthorized action rate, tamper
detection rate, determinism agreement, and integration LOC.
"""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[3]
T81_BIN = ROOT_DIR / "build" / "t81"
DEMO_MAKER = ROOT_DIR / "build" / "t81_make_assess_fixed_demo"
ADB_TOOL = Path(__file__).resolve().parent / "adb_tool.py"

sys.path.insert(0, str(Path(__file__).resolve().parent))
import adb_tool  # noqa: E402


def python_control_eval(model_bytes: bytes, policy_obj: dict, input_text: str) -> dict:
    model_hash = "sha3-512:" + hashlib.sha3_512(model_bytes).hexdigest()
    if model_hash not in policy_obj.get("allowed_model_hashes", []):
        raise PermissionError(f"SecurityFault: model_hash {model_hash} denied by policy")
    decision = "ALLOW" if "hello" in input_text or "greet" in input_text else "DENY"
    reason = "GREETING_PAIR" if decision == "ALLOW" else "UNMATCHED"
    input_hash = "sha3-256:" + hashlib.sha3_256(input_text.encode("utf-8")).hexdigest()
    result_obj = {
        "schema": "control.task.assess-fixed.v1",
        "input_hash": input_hash,
        "model_hash": model_hash,
        "decision": decision,
        "reason_code": reason,
    }
    raw = json.dumps(result_obj, sort_keys=True, separators=(",", ":")).encode("utf-8")
    result_ref = "sha3-256:" + hashlib.sha3_256(raw).hexdigest()
    return {"result_obj": result_obj, "raw": raw, "result_ref": result_ref, "model_hash": model_hash, "input_hash": input_hash}


def python_control_verify(bundle: dict, pub_pem: Path, trusted_policy: dict, model_bytes: bytes, input_text: str, store_dir: Path) -> bool:
    if bundle["authority"]["verdict"] != "ALLOW":
        return False
    policy_bytes = json.dumps(trusted_policy, sort_keys=True, separators=(",", ":")).encode("utf-8")
    if bundle["authority"]["policy_hash"] != "sha3-512:" + hashlib.sha3_512(policy_bytes).hexdigest():
        return False
    if not adb_tool.verify_ed25519(pub_pem, adb_tool.canonical_signing_bytes(bundle), bundle["accountability"]["signature"]):
        return False
    # Verify stored artifact integrity
    stored_path = store_dir / (bundle["evidence"]["output_digest"].replace(":", "_") + ".json")
    if not stored_path.exists():
        return False
    actual_ref = "sha3-256:" + hashlib.sha3_256(stored_path.read_bytes()).hexdigest()
    if actual_ref != bundle["evidence"]["output_digest"]:
        return False
    # Deterministic replay
    replayed = python_control_eval(model_bytes, trusted_policy, input_text)
    return replayed["result_ref"] == bundle["evidence"]["output_digest"]


def run_benchmark(iterations: int = 5) -> dict:
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        keys = tmp / "keys"
        keys.mkdir()
        priv_pem = keys / "node_a.priv.pem"
        pub_pem = keys / "node_a.pub.pem"
        subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(priv_pem)], check=True, capture_output=True)
        subprocess.run(["openssl", "pkey", "-in", str(priv_pem), "-pubout", "-out", str(pub_pem)], check=True, capture_output=True)

        model_dir = tmp / "model"
        model_dir.mkdir()
        subprocess.run([str(DEMO_MAKER), str(model_dir)], check=True, capture_output=True)
        model_path = model_dir / "assess-fixed-demo.t81w"
        model_bytes = model_path.read_bytes()
        model_hash_hex = subprocess.check_output([str(T81_BIN), "determinism", "hash", str(model_path)], text=True).split()[0]

        apl_policy = tmp / "policy.apl"
        apl_policy.write_text(
            f'(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["sha3-512:{model_hash_hex}"])\n  (require-axion-event (reason "task:assess_fixed.v1")))\n',
            encoding="utf-8",
        )
        revoked_apl = tmp / "revoked.apl"
        revoked_apl.write_text(
            '(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["sha3-512:' + ("0" * 128) + '"])\n  (require-axion-event (reason "task:assess_fixed.v1")))\n',
            encoding="utf-8",
        )

        # --- (A) T81 Substrate ---
        t81_emit_ms = []
        t81_verify_ms = []
        t81_refs = []
        for i in range(iterations):
            croot_a = tmp / f"t81_a_{i}"
            croot_b = tmp / f"t81_b_{i}"
            croot_a.mkdir()
            croot_b.mkdir()
            task_json = tmp / f"task_{i}.json"
            bundle_json = tmp / f"t81_bundle_{i}.json"

            t0 = time.perf_counter()
            out = subprocess.check_output(
                [
                    str(T81_BIN), "ai", "task", "assess-fixed",
                    "--model", "assess-fixed-demo",
                    "--model-file", str(model_path),
                    "--policy", str(apl_policy),
                    "--canonfs-root", str(croot_a),
                    "--mode", "strict_deterministic",
                    "--input", "greet hello",
                ],
                text=True,
            )
            task_json.write_text(out, encoding="utf-8")
            subprocess.run(
                [
                    sys.executable, str(ADB_TOOL), "emit",
                    "--task-json", str(task_json),
                    "--policy", str(apl_policy),
                    "--signing-key", str(priv_pem),
                    "--public-key-uri", f"file://{pub_pem}",
                    "--principal-id", "bench-a",
                    "--out", str(bundle_json),
                ],
                check=True,
                capture_output=True,
            )
            subprocess.run([str(T81_BIN), "canonfs", "fsck", "--verify-hashes", "--canonfs-root", str(croot_a), "--json"], check=True, capture_output=True)
            t81_emit_ms.append((time.perf_counter() - t0) * 1000.0)
            t81_refs.append(json.loads(out)["result_ref"])

            t1 = time.perf_counter()
            subprocess.run(
                [
                    sys.executable, str(ADB_TOOL), "verify",
                    "--bundle", str(bundle_json),
                    "--public-key", str(pub_pem),
                    "--trusted-policy", str(apl_policy),
                    "--replay",
                    "--t81-bin", str(T81_BIN),
                    "--model-file", str(model_path),
                    "--input-text", "greet hello",
                    "--canonfs-root", str(croot_b),
                ],
                check=True,
                capture_output=True,
            )
            subprocess.run([str(T81_BIN), "canonfs", "fsck", "--verify-hashes", "--canonfs-root", str(croot_b), "--json"], check=True, capture_output=True)
            t81_verify_ms.append((time.perf_counter() - t1) * 1000.0)

        # T81 Adversarial checks
        t81_revoked_blocked = subprocess.run(
            [sys.executable, str(ADB_TOOL), "verify", "--bundle", str(tmp / "t81_bundle_0.json"), "--public-key", str(pub_pem), "--trusted-policy", str(revoked_apl)],
            capture_output=True,
        ).returncode != 0
        tampered_t81 = json.loads((tmp / "t81_bundle_0.json").read_text())
        tampered_t81["meaning"]["parameters"]["decision"] = "DENY"
        (tmp / "t81_tampered.json").write_text(json.dumps(tampered_t81))
        t81_bundle_tamper_caught = subprocess.run(
            [sys.executable, str(ADB_TOOL), "verify", "--bundle", str(tmp / "t81_tampered.json"), "--public-key", str(pub_pem)],
            capture_output=True,
        ).returncode != 0
        blk = next((tmp / "t81_b_0" / "objects").rglob("*.blk"))
        blk.write_bytes(blk.read_bytes() + b"X")
        t81_store_tamper_caught = subprocess.run(
            [str(T81_BIN), "canonfs", "fsck", "--verify-hashes", "--canonfs-root", str(tmp / "t81_b_0"), "--json"],
            capture_output=True,
        ).returncode != 0

        # --- (B) Lightweight Python Control ---
        py_policy = {"allowed_model_hashes": ["sha3-512:" + hashlib.sha3_512(model_bytes).hexdigest()]}
        py_revoked_policy = {"allowed_model_hashes": ["sha3-512:" + ("0" * 128)]}
        py_emit_ms = []
        py_verify_ms = []
        py_refs = []
        for i in range(iterations):
            store_dir = tmp / f"py_store_{i}"
            store_dir.mkdir()
            t0 = time.perf_counter()
            res = python_control_eval(model_bytes, py_policy, "greet hello")
            (store_dir / (res["result_ref"].replace(":", "_") + ".json")).write_bytes(res["raw"])
            pbytes = json.dumps(py_policy, sort_keys=True, separators=(",", ":")).encode("utf-8")
            bundle = {
                "schema_version": "1.0.0-draft",
                "bundle_id": res["result_ref"],
                "timestamp_utc": "2026-11-18T00:00:00Z",
                "meaning": {"action_type": "assess_fixed.v1", "parameters": res["result_obj"], "proposed_side_effects": [{"target": "host_action_gate", "mutation": "ALLOW:GREETING_PAIR"}]},
                "evidence": {"input_digest": res["input_hash"], "output_digest": res["result_ref"], "execution_trace_hash": res["result_ref"]},
                "method": {"runtime_environment": "python3-control", "transformation_id": "assess_fixed.v1", "model_weights_digest": res["model_hash"]},
                "authority": {"policy_id": "policy.json", "policy_hash": "sha3-512:" + hashlib.sha3_512(pbytes).hexdigest(), "verdict": "ALLOW", "evaluation_timestamp": "2026-11-18T00:00:00Z"},
                "challengeability": {"replayable": True, "verification_procedure": "python_control_eval"},
            }
            bundle["accountability"] = {
                "principal_id": "bench-py",
                "public_key_uri": f"file://{pub_pem}",
                "signature": adb_tool.sign_ed25519(priv_pem, adb_tool.canonical_signing_bytes(bundle)),
            }
            py_emit_ms.append((time.perf_counter() - t0) * 1000.0)
            py_refs.append(res["result_ref"])
            if i == 0:
                py_bundle_0 = bundle
                py_store_0 = store_dir

            t1 = time.perf_counter()
            ok = python_control_verify(bundle, pub_pem, py_policy, model_bytes, "greet hello", store_dir)
            assert ok
            py_verify_ms.append((time.perf_counter() - t1) * 1000.0)

        py_revoked_blocked = not python_control_verify(py_bundle_0, pub_pem, py_revoked_policy, model_bytes, "greet hello", py_store_0)
        py_tampered = json.loads(json.dumps(py_bundle_0))
        py_tampered["meaning"]["parameters"]["decision"] = "DENY"
        py_bundle_tamper_caught = not python_control_verify(py_tampered, pub_pem, py_policy, model_bytes, "greet hello", py_store_0)
        stored_0 = next(py_store_0.iterdir())
        stored_0.write_bytes(stored_0.read_bytes() + b"X")
        py_store_tamper_caught = not python_control_verify(py_bundle_0, pub_pem, py_policy, model_bytes, "greet hello", py_store_0)

        return {
            "iterations": iterations,
            "t81_substrate": {
                "emit_seal_mean_ms": round(sum(t81_emit_ms) / len(t81_emit_ms), 2),
                "verify_replay_mean_ms": round(sum(t81_verify_ms) / len(t81_verify_ms), 2),
                "reproducibility_match_pct": 100.0 if len(set(t81_refs)) == 1 else 0.0,
                "unauthorized_action_rate_pct": 0.0 if t81_revoked_blocked else 100.0,
                "tamper_detection_rate_pct": 100.0 if (t81_bundle_tamper_caught and t81_store_tamper_caught) else 0.0,
            },
            "python_control": {
                "emit_seal_mean_ms": round(sum(py_emit_ms) / len(py_emit_ms), 2),
                "verify_replay_mean_ms": round(sum(py_verify_ms) / len(py_verify_ms), 2),
                "reproducibility_match_pct": 100.0 if len(set(py_refs)) == 1 else 0.0,
                "unauthorized_action_rate_pct": 0.0 if py_revoked_blocked else 100.0,
                "tamper_detection_rate_pct": 100.0 if (py_bundle_tamper_caught and py_store_tamper_caught) else 0.0,
            },
        }


if __name__ == "__main__":
    print(json.dumps(run_benchmark(5), indent=2))
