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
from collections import Counter
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.governed-llama-replay.v1"
KEYRING_SCHEMA_VERSION = "t81.ai.governed-replay-keyring.v1"


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
    payload = json.loads(path.read_text(encoding="utf-8"))
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


def build_replay_escalation(errors: list[str]) -> dict[str, Any]:
    rules: list[tuple[str, str, str, str]] = [
        (
            "keyring",
            "AI_GOVERNED_ESCALATE_KEYRING_INVALID",
            "platform-oncall",
            "fail_closed_and_review_governed_replay_keyring",
        ),
        (
            "signature verification failed",
            "AI_GOVERNED_ESCALATE_SIGNATURE_INVALID",
            "security-oncall",
            "block_promotion_and_rotate_governed_replay_keys",
        ),
        (
            "nondeterministic_",
            "AI_GOVERNED_ESCALATE_DETERMINISM_REGRESSION",
            "ai-runtime-oncall",
            "block_promotion_and_debug_replay_nondeterminism",
        ),
    ]
    actions: list[dict[str, str]] = []
    seen: set[str] = set()
    joined = "\n".join(errors).lower()
    for needle, reason_code, owner, action in rules:
        if needle in joined and reason_code not in seen:
            actions.append({"reason_code": reason_code, "owner": owner, "action": action})
            seen.add(reason_code)
    return {"status": "triggered" if actions else "none", "actions": actions}


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd, text=True, capture_output=True, check=False)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Run RFC-0025 governed llama deterministic multi-seed replay attestations."
    )
    p.add_argument("--t81-bin", required=True, help="Path to t81 binary (llama-enabled build)")
    p.add_argument("--model", required=True, help="Path to GGUF model")
    p.add_argument("--out-dir", required=True, help="Output directory for artifacts")
    p.add_argument("--prompt", default="Deterministic governed inference replay check.")
    p.add_argument("--max-tokens", type=int, default=8)
    p.add_argument("--threads", type=int, default=1)
    p.add_argument("--temperature", type=float, default=0.0)
    p.add_argument("--top-k", type=int, default=1)
    p.add_argument("--top-p", type=float, default=1.0)
    p.add_argument("--seeds", default="0,1,2", help="Comma-separated seeds to attest")
    p.add_argument("--replays-per-seed", type=int, default=2)
    p.add_argument(
        "--llama-hash-probe",
        default="scripts/ci/llama_model_hash.py",
        help="Path to canonical llama model-hash probe script",
    )
    p.add_argument(
        "--baseline-governed-flow",
        default="",
        help="Optional path to governed_llama_flow.json for consistency checks",
    )
    p.add_argument(
        "--signing-keyring",
        default="",
        help="Path to governed replay attestation keyring (defaults to scripts/ci/ai_governed_replay_keyring.json).",
    )
    return p.parse_args()


def parse_seeds(raw: str) -> list[int]:
    out: list[int] = []
    for part in raw.split(","):
        s = part.strip()
        if not s:
            continue
        out.append(int(s))
    if not out:
        raise ValueError("at least one seed is required")
    return out


def classify_failure(text: str) -> str:
    t = text.lower()
    if "policy_violation" in t or "allowed-tensor-hashes" in t:
        return "policy_denial"
    if "expected-model-hash" in t or "model hash" in t:
        return "model_hash_mismatch"
    if "no such file" in t or "not found" in t:
        return "missing_input"
    return "command_failure"


def probe_model_hash(t81_bin: Path, model: Path, probe_script: Path) -> str:
    proc = run([sys.executable, str(probe_script), "--t81-bin", str(t81_bin), str(model)], cwd=Path.cwd())
    if proc.returncode != 0:
        raise RuntimeError(f"llama_model_hash probe failed rc={proc.returncode}: {(proc.stdout + proc.stderr).strip()}")
    out = (proc.stdout or "").strip()
    if not out.startswith("sha3-512:"):
        raise RuntimeError(f"llama_model_hash probe returned unexpected output: {out}")
    return out


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    t81_bin = Path(args.t81_bin).resolve()
    model = Path(args.model).resolve()
    probe_script = Path(args.llama_hash_probe).resolve()
    default_keyring = Path(__file__).resolve().with_name("ai_governed_replay_keyring.json")
    keyring_path = Path(args.signing_keyring).resolve() if args.signing_keyring else default_keyring

    errors: list[str] = []
    taxonomy_counts: Counter[str] = Counter()
    taxonomy_details: list[dict[str, Any]] = []

    if not t81_bin.exists():
        errors.append(f"missing t81 binary: {t81_bin}")
        taxonomy_counts["missing_input"] += 1
    if not model.exists():
        errors.append(f"missing model file: {model}")
        taxonomy_counts["missing_input"] += 1
    if not probe_script.exists():
        errors.append(f"missing llama hash probe script: {probe_script}")
        taxonomy_counts["missing_input"] += 1

    seeds: list[int] = []
    try:
        seeds = parse_seeds(args.seeds)
    except Exception as exc:
        errors.append(f"invalid --seeds: {exc}")
        taxonomy_counts["invalid_input"] += 1

    if args.replays_per_seed < 2:
        errors.append("--replays-per-seed must be >= 2")
        taxonomy_counts["invalid_input"] += 1

    model_hash = ""
    if not errors:
        try:
            model_hash = probe_model_hash(t81_bin, model, probe_script)
        except Exception as exc:
            errors.append(str(exc))
            taxonomy_counts["hash_probe_failure"] += 1

    policy_file = out_dir / "replay_policy.apl"
    if model_hash:
        policy_file.write_text(
            "\n".join(
                [
                    "(policy",
                    "  (tier 1)",
                    f"  (allowed-tensor-hashes [\"{model_hash}\"]))",
                ]
            )
            + "\n",
            encoding="utf-8",
        )

    baseline_stdout = ""
    baseline_flow_path = Path(args.baseline_governed_flow).resolve() if args.baseline_governed_flow else None
    if baseline_flow_path and baseline_flow_path.exists():
        try:
            baseline = json.loads(baseline_flow_path.read_text(encoding="utf-8"))
            baseline_stdout = str(baseline.get("llama_stdout_sha256", "")).strip()
        except Exception:
            errors.append(f"failed to parse baseline governed flow artifact: {baseline_flow_path}")
            taxonomy_counts["baseline_parse_failure"] += 1

    replay_records: list[dict[str, Any]] = []
    per_seed: dict[int, dict[str, Any]] = {}

    if not errors:
        for seed in seeds:
            seed_runs: list[dict[str, Any]] = []
            for replay_index in range(1, args.replays_per_seed + 1):
                cmd = [
                    str(t81_bin),
                    "llama-run",
                    str(model),
                    args.prompt,
                    "--policy",
                    str(policy_file),
                    "--max-tokens",
                    str(args.max_tokens),
                    "--seed",
                    str(seed),
                    "--threads",
                    str(args.threads),
                    "--temperature",
                    str(args.temperature),
                    "--top-k",
                    str(args.top_k),
                    "--top-p",
                    str(args.top_p),
                    "--expected-model-hash",
                    model_hash,
                ]
                proc = run(cmd, cwd=Path.cwd())

                stdout_path = out_dir / f"replay_seed{seed}_run{replay_index}.stdout.log"
                stderr_path = out_dir / f"replay_seed{seed}_run{replay_index}.stderr.log"
                stdout_path.write_text(proc.stdout or "", encoding="utf-8")
                stderr_path.write_text(proc.stderr or "", encoding="utf-8")

                record = {
                    "seed": seed,
                    "replay_index": replay_index,
                    "returncode": proc.returncode,
                    "stdout_sha256": sha256_text(proc.stdout or ""),
                    "stderr_sha256": sha256_text(proc.stderr or ""),
                    "stdout_log": str(stdout_path),
                    "stderr_log": str(stderr_path),
                }
                replay_records.append(record)
                seed_runs.append(record)

                if proc.returncode != 0:
                    category = classify_failure((proc.stdout or "") + "\n" + (proc.stderr or ""))
                    taxonomy_counts[category] += 1
                    taxonomy_details.append(
                        {
                            "seed": seed,
                            "replay_index": replay_index,
                            "category": category,
                            "returncode": proc.returncode,
                        }
                    )

            base = seed_runs[0]
            seed_errors: list[str] = []
            for rec in seed_runs[1:]:
                if rec["returncode"] != base["returncode"]:
                    seed_errors.append("nondeterministic_returncode")
                    taxonomy_counts["nondeterministic_returncode"] += 1
                if rec["stdout_sha256"] != base["stdout_sha256"]:
                    seed_errors.append("nondeterministic_stdout")
                    taxonomy_counts["nondeterministic_stdout"] += 1
                if rec["stderr_sha256"] != base["stderr_sha256"]:
                    seed_errors.append("nondeterministic_stderr")
                    taxonomy_counts["nondeterministic_stderr"] += 1

            baseline_consistent = True
            if baseline_stdout and seed == 0 and base["stdout_sha256"] != baseline_stdout:
                baseline_consistent = False
                seed_errors.append("baseline_stdout_mismatch")
                taxonomy_counts["baseline_stdout_mismatch"] += 1

            deterministic = not seed_errors
            per_seed[seed] = {
                "seed": seed,
                "deterministic_replay": deterministic,
                "baseline_consistent": baseline_consistent,
                "run_count": len(seed_runs),
                "reference": {
                    "returncode": base["returncode"],
                    "stdout_sha256": base["stdout_sha256"],
                    "stderr_sha256": base["stderr_sha256"],
                },
                "errors": seed_errors,
            }
            if seed_errors:
                errors.append(f"seed {seed}: " + ", ".join(seed_errors))

    status = "pass" if not errors else "fail"
    deterministic_multi_seed = bool(per_seed) and all(item["deterministic_replay"] for item in per_seed.values())
    keyring_payload: dict[str, Any] = {}
    active_key: dict[str, Any] = {}
    keyring_errors: list[str] = []
    signature_hex = ""
    signature_verified = False
    signature_verified_by = ""
    signing_key_id = ""
    if keyring_path.exists():
        keyring_payload, active_key, keyring_errors = validate_keyring(keyring_path)
    else:
        keyring_errors = [f"governed replay keyring missing: {keyring_path}"]
    errors.extend(keyring_errors)

    signature_payload = {
        "schema": SCHEMA_VERSION,
        "deterministic_multi_seed_replay": deterministic_multi_seed,
        "model_hash": model_hash,
        "seeds": seeds,
        "replays_per_seed": args.replays_per_seed,
        "failure_taxonomy_counts": dict(sorted(taxonomy_counts.items())),
        "errors": errors,
    }
    if active_key:
        signing_key_id = str(active_key.get("key_id", "")).strip()
        signature_hex = sign_payload_hex(signature_payload, active_key)
        signature_verified, signature_verified_by = verify_payload_signature(
            signature_payload, signature_hex, keyring_payload
        )
        if not signature_verified:
            errors.append("governed replay attestation signature verification failed")
    else:
        errors.append("governed replay attestation has no active signing key")

    payload = {
        "schema": SCHEMA_VERSION,
        "status": "pass" if not errors else "fail",
        "deterministic_multi_seed_replay": deterministic_multi_seed,
        "model": str(model),
        "model_hash": model_hash,
        "policy_file": str(policy_file),
        "seeds": seeds,
        "replays_per_seed": args.replays_per_seed,
        "baseline_governed_flow": str(baseline_flow_path) if baseline_flow_path else "",
        "per_seed_attestations": [per_seed[s] for s in sorted(per_seed.keys())],
        "runs": replay_records,
        "failure_taxonomy": {
            "counts": dict(sorted(taxonomy_counts.items())),
            "details": taxonomy_details,
        },
        "errors": errors,
        "escalation_policy": build_replay_escalation(errors),
        "signature": {
            "algorithm": "hmac-sha256",
            "key_id": signing_key_id,
            "signature_hex": signature_hex,
            "verified": signature_verified,
            "verified_by_key_id": signature_verified_by,
            "keyring_path": str(keyring_path),
            "keyring_sha256": sha256_file(keyring_path) if keyring_path.exists() else "",
            "rotation_policy": keyring_payload.get("rotation_policy", {}),
        },
    }
    status = str(payload["status"])

    json_path = out_dir / "governed_llama_replay_attestation.json"
    md_path = out_dir / "governed_llama_replay_attestation.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# Governed Llama Replay Attestation",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_multi_seed_replay: `{str(deterministic_multi_seed).lower()}`",
                f"- seeds: `{','.join(str(s) for s in seeds) if seeds else ''}`",
                f"- replays_per_seed: `{args.replays_per_seed}`",
                f"- taxonomy_categories: `{len(taxonomy_counts)}`",
                f"- signature_verified: `{str(signature_verified).lower()}`",
                f"- escalation_status: `{payload['escalation_policy']['status']}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"governed replay attestation status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
