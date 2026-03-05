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


SCHEMA_VERSION = "t81.ai.backend-adapter.v1"
SELECTION_MANIFEST_SCHEMA = "t81.ai.backend-selection-manifest.v1"
KEYRING_SCHEMA_VERSION = "t81.ai.backend-selection-keyring.v1"


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
        if not env_value:
            if errors is not None:
                errors.append(f"key {key_id}: material_env '{env_name}' is unset/empty")
            return b""
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

    key_by_id: dict[str, dict[str, Any]] = {}
    active_keys: list[dict[str, Any]] = []
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


def build_registry() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "adapter_contract": {
            "required_methods": [
                "initialize",
                "load_model",
                "inference",
                "get_capabilities",
                "cleanup",
            ],
            "required_capability_fields": [
                "backend_name",
                "supported_formats",
                "determinism_modes",
                "max_context_tokens",
                "supports_streaming",
                "supports_logit_bias",
            ],
        },
        "backends": [
            {
                "backend_name": "llama.cpp",
                "supported_formats": ["gguf", "t81_canonical"],
                "determinism_modes": ["strict_deterministic", "reproducible_nondeterministic"],
                "max_context_tokens": 4096,
                "supports_streaming": True,
                "supports_logit_bias": True,
            },
            {
                "backend_name": "onnx_runtime",
                "supported_formats": ["onnx", "t81_canonical"],
                "determinism_modes": ["strict_deterministic", "statistical_deterministic"],
                "max_context_tokens": 8192,
                "supports_streaming": False,
                "supports_logit_bias": False,
            },
        ],
        "negotiation": {
            "preferred_order": ["llama.cpp", "onnx_runtime"],
            "selection_policy": "first_backend_supporting_requested_format_and_mode",
        },
    }


def check_contract(registry: dict[str, Any]) -> tuple[bool, list[str]]:
    errors: list[str] = []
    req_fields = registry["adapter_contract"]["required_capability_fields"]
    backend_names: set[str] = set()

    for backend in registry["backends"]:
        name = backend.get("backend_name", "")
        if not name:
            errors.append("backend missing backend_name")
            continue
        if name in backend_names:
            errors.append(f"duplicate backend_name: {name}")
        backend_names.add(name)

        for field in req_fields:
            if field not in backend:
                errors.append(f"{name}: missing capability field {field}")

        if "t81_canonical" not in backend.get("supported_formats", []):
            errors.append(f"{name}: must support t81_canonical format")

        if "strict_deterministic" not in backend.get("determinism_modes", []):
            errors.append(f"{name}: must include strict_deterministic mode")

        if int(backend.get("max_context_tokens", 0)) <= 0:
            errors.append(f"{name}: max_context_tokens must be > 0")

    preferred = registry["negotiation"]["preferred_order"]
    for name in preferred:
        if name not in backend_names:
            errors.append(f"preferred_order backend missing from registry: {name}")

    return len(errors) == 0, errors


def run_cmd(argv: list[str]) -> dict[str, Any]:
    proc = subprocess.run(argv, capture_output=True, text=True, check=False)
    return {
        "argv": argv,
        "rc": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "stdout_sha256": sha256_text(proc.stdout),
        "stderr_sha256": sha256_text(proc.stderr),
    }


def parse_backend_select_output(text: str) -> dict[str, Any] | None:
    try:
        payload = json.loads(text)
    except json.JSONDecodeError:
        return None
    if not isinstance(payload, dict):
        return None
    return payload


def validate_runtime_binding(ai_bin: Path, model_path: Path) -> tuple[bool, list[str], dict[str, Any]]:
    errors: list[str] = []
    binding: dict[str, Any] = {"ai_bin": str(ai_bin), "model_path": str(model_path)}
    if not ai_bin.exists():
        errors.append(f"ai binary not found: {ai_bin}")
        return False, errors, binding

    model_path.parent.mkdir(parents=True, exist_ok=True)
    if not model_path.exists():
        model_path.write_text("t81-ai-backend-probe\n", encoding="utf-8")
    binding["model_sha256"] = sha256_text(model_path.read_text(encoding="utf-8"))

    help_res = run_cmd([str(ai_bin), "--help"])
    caps_res = run_cmd([str(ai_bin), "backend", "capabilities"])
    selection_trace_path = model_path.parent / "runtime_backend_selection_trace.json"
    select_gguf_res = run_cmd(
        [
            str(ai_bin),
            "backend",
            "select",
            "--format",
            "gguf",
            "--mode",
            "strict_deterministic",
            "--out",
            str(selection_trace_path),
        ]
    )
    select_onnx_res = run_cmd(
        [
            str(ai_bin),
            "backend",
            "select",
            "--format",
            "onnx",
            "--mode",
            "statistical_deterministic",
        ]
    )
    inspect_res = run_cmd([str(ai_bin), "model", "inspect", str(model_path)])
    verify_res = run_cmd([str(ai_bin), "verify", "determinism", str(model_path)])

    binding["help"] = {"rc": help_res["rc"], "stdout_sha256": help_res["stdout_sha256"]}
    binding["backend_capabilities"] = {
        "rc": caps_res["rc"],
        "stdout_sha256": caps_res["stdout_sha256"],
    }
    binding["backend_select_gguf_strict"] = {
        "rc": select_gguf_res["rc"],
        "stdout_sha256": select_gguf_res["stdout_sha256"],
        "artifact": str(selection_trace_path),
    }
    binding["backend_select_onnx_statistical"] = {
        "rc": select_onnx_res["rc"],
        "stdout_sha256": select_onnx_res["stdout_sha256"],
    }
    binding["model_inspect"] = {"rc": inspect_res["rc"], "stdout_sha256": inspect_res["stdout_sha256"]}
    binding["verify_determinism"] = {
        "rc": verify_res["rc"],
        "stdout_sha256": verify_res["stdout_sha256"],
    }

    if help_res["rc"] != 0:
        errors.append("runtime binding: t81_ai --help failed")
    if caps_res["rc"] != 0:
        errors.append("runtime binding: t81_ai backend capabilities failed")
    if select_gguf_res["rc"] != 0:
        errors.append("runtime binding: t81_ai backend select gguf/strict_deterministic failed")
    if select_onnx_res["rc"] != 0:
        errors.append("runtime binding: t81_ai backend select onnx/statistical_deterministic failed")
    if inspect_res["rc"] != 0:
        errors.append("runtime binding: t81_ai model inspect failed")
    if verify_res["rc"] != 0:
        errors.append("runtime binding: t81_ai verify determinism failed")

    required_help_markers = ("model inspect", "verify determinism", "backend capabilities", "backend select")
    for marker in required_help_markers:
        if marker not in help_res["stdout"]:
            errors.append(f"runtime binding: help output missing marker '{marker}'")

    if caps_res["rc"] == 0:
        try:
            caps = json.loads(caps_res["stdout"])
        except json.JSONDecodeError:
            caps = None
            errors.append("runtime binding: backend capabilities output is not valid JSON")
        if isinstance(caps, dict):
            binding["backend_capabilities_artifact_sha256"] = sha256_text(canonical_json(caps))
            required_top = {"schema", "default_backend", "selection_policy", "backends"}
            missing_top = sorted(required_top - set(caps.keys()))
            if missing_top:
                errors.append("runtime binding: backend capabilities missing fields " + ", ".join(missing_top))
            if caps.get("schema") != "t81.ai.backend-capabilities.v1":
                errors.append("runtime binding: backend capabilities schema mismatch")
            backends = caps.get("backends")
            if not isinstance(backends, list) or not backends:
                errors.append("runtime binding: backend capabilities backends must be non-empty list")
            else:
                required_backend_fields = {
                    "backend_name",
                    "supported_formats",
                    "determinism_modes",
                    "max_context_tokens",
                    "supports_streaming",
                    "supports_logit_bias",
                }
                backend_names: set[str] = set()
                for idx, backend in enumerate(backends):
                    if not isinstance(backend, dict):
                        errors.append(f"runtime binding: backend capabilities entry {idx} is not object")
                        continue
                    backend_names.add(str(backend.get("backend_name", "")))
                    missing = sorted(required_backend_fields - set(backend.keys()))
                    if missing:
                        errors.append(
                            f"runtime binding: backend capabilities entry {idx} missing " + ", ".join(missing)
                        )
                if caps.get("default_backend") not in backend_names:
                    errors.append("runtime binding: default_backend missing from backends list")

    if select_gguf_res["rc"] == 0:
        try:
            select_gguf = json.loads(select_gguf_res["stdout"])
        except json.JSONDecodeError:
            select_gguf = None
            errors.append("runtime binding: backend select gguf output is not valid JSON")
        if isinstance(select_gguf, dict):
            required_select_fields = {
                "schema",
                "requested_format",
                "requested_mode",
                "selection_policy",
                "preferred_order",
                "candidates",
                "selected_backend",
                "decision_reason",
                "trace_sha256",
                "status",
            }
            missing = sorted(required_select_fields - set(select_gguf.keys()))
            if missing:
                errors.append("runtime binding: backend select gguf missing fields " + ", ".join(missing))
            if select_gguf.get("schema") != "t81.ai.backend-selection-trace.v1":
                errors.append("runtime binding: backend select gguf schema mismatch")
            if select_gguf.get("selected_backend") != "llama.cpp":
                errors.append("runtime binding: backend select gguf selected_backend mismatch")
            if select_gguf.get("status") != "pass":
                errors.append("runtime binding: backend select gguf status must be pass")
            if not selection_trace_path.exists():
                errors.append("runtime binding: backend selection trace artifact was not emitted")
            else:
                binding["backend_selection_trace_artifact_sha256"] = sha256_text(
                    canonical_json(parse_json(selection_trace_path))
                )

    if select_onnx_res["rc"] == 0:
        try:
            select_onnx = json.loads(select_onnx_res["stdout"])
        except json.JSONDecodeError:
            select_onnx = None
            errors.append("runtime binding: backend select onnx output is not valid JSON")
        if isinstance(select_onnx, dict):
            if select_onnx.get("schema") != "t81.ai.backend-selection-trace.v1":
                errors.append("runtime binding: backend select onnx schema mismatch")
            if select_onnx.get("selected_backend") != "onnx_runtime":
                errors.append("runtime binding: backend select onnx selected_backend mismatch")
            if select_onnx.get("status") != "pass":
                errors.append("runtime binding: backend select onnx status must be pass")
            binding["backend_selection_onnx_trace_sha256"] = sha256_text(canonical_json(select_onnx))

    matrix_specs = [
        ("gguf", "strict_deterministic"),
        ("gguf", "reproducible_nondeterministic"),
        ("onnx", "strict_deterministic"),
        ("onnx", "statistical_deterministic"),
        ("t81_canonical", "strict_deterministic"),
    ]
    replay_runs: list[list[dict[str, Any]]] = []
    for _ in range(2):
        run_payload: list[dict[str, Any]] = []
        for fmt, mode in matrix_specs:
            res = run_cmd([str(ai_bin), "backend", "select", "--format", fmt, "--mode", mode])
            entry: dict[str, Any] = {
                "format": fmt,
                "mode": mode,
                "rc": res["rc"],
                "stdout_sha256": res["stdout_sha256"],
            }
            parsed = parse_backend_select_output(res["stdout"])
            if parsed is not None:
                entry["parsed"] = {
                    "schema": parsed.get("schema"),
                    "selected_backend": parsed.get("selected_backend"),
                    "decision_reason": parsed.get("decision_reason"),
                    "trace_sha256": parsed.get("trace_sha256"),
                    "status": parsed.get("status"),
                }
            run_payload.append(entry)
        replay_runs.append(run_payload)

    expected_backend = {
        ("gguf", "strict_deterministic"): "llama.cpp",
        ("gguf", "reproducible_nondeterministic"): "llama.cpp",
        ("onnx", "strict_deterministic"): "onnx_runtime",
        ("onnx", "statistical_deterministic"): "onnx_runtime",
        ("t81_canonical", "strict_deterministic"): "llama.cpp",
    }
    replay_errors: list[str] = []
    for entry in replay_runs[0]:
        key = (entry["format"], entry["mode"])
        if entry["rc"] != 0:
            replay_errors.append(f"backend selection replay: rc!=0 for {key[0]}/{key[1]}")
            continue
        parsed = entry.get("parsed", {})
        if parsed.get("schema") != "t81.ai.backend-selection-trace.v1":
            replay_errors.append(f"backend selection replay: schema mismatch for {key[0]}/{key[1]}")
        if parsed.get("selected_backend") != expected_backend.get(key):
            replay_errors.append(f"backend selection replay: selected_backend mismatch for {key[0]}/{key[1]}")
        if parsed.get("status") != "pass":
            replay_errors.append(f"backend selection replay: status!=pass for {key[0]}/{key[1]}")

    deterministic_replay = replay_runs[0] == replay_runs[1]
    if not deterministic_replay:
        replay_errors.append("backend selection replay: run#1 != run#2")

    replay_payload = {
        "schema": "t81.ai.backend-selection-replay.v1",
        "status": "pass" if not replay_errors else "fail",
        "deterministic_replay": deterministic_replay,
        "runs": replay_runs,
    }
    replay_path = model_path.parent / "runtime_backend_selection_replay.json"
    replay_path.write_text(json.dumps(replay_payload, indent=2, sort_keys=True), encoding="utf-8")
    binding["backend_selection_replay"] = {
        "artifact": str(replay_path),
        "sha256": sha256_text(canonical_json(replay_payload)),
        "deterministic_replay": deterministic_replay,
    }
    if replay_errors:
        errors.extend(replay_errors)

    if "Status: Inspection completed" not in inspect_res["stdout"]:
        errors.append("runtime binding: model inspect output missing completion marker")
    if "Determinism mode: strict" not in verify_res["stdout"]:
        errors.append("runtime binding: verify determinism output missing strict mode marker")

    return len(errors) == 0, errors, binding


def detect_status(payload: dict[str, Any]) -> str:
    status = str(payload.get("status", "")).strip()
    if not status and isinstance(payload.get("checks"), dict):
        status = str(payload["checks"].get("status", "")).strip()
    return status


def build_backend_selection_manifest(
    out_dir: Path,
    runtime_binding: dict[str, Any],
    policy_contract_path: Path,
    runtime_trace_path: Path,
    policy_ledger_snapshot_path: Path,
    signing_keyring_path: Path,
) -> tuple[bool, list[str], dict[str, Any], Path, Path]:
    errors: list[str] = []

    replay_artifact = Path(str(runtime_binding.get("backend_selection_replay", {}).get("artifact", ""))).resolve()
    selection_trace_artifact = Path(
        str(runtime_binding.get("backend_select_gguf_strict", {}).get("artifact", ""))
    ).resolve()
    capabilities_sha = str(runtime_binding.get("backend_capabilities_artifact_sha256", "")).strip()
    replay_sha = str(runtime_binding.get("backend_selection_replay", {}).get("sha256", "")).strip()

    for path, label in [
        (policy_contract_path, "policy_contract"),
        (runtime_trace_path, "runtime_trace"),
        (policy_ledger_snapshot_path, "policy_ledger_snapshot"),
        (signing_keyring_path, "signing_keyring"),
        (replay_artifact, "backend_selection_replay"),
        (selection_trace_artifact, "backend_selection_trace"),
    ]:
        if not path.exists():
            errors.append(f"backend selection manifest: missing {label} artifact: {path}")

    policy_status = ""
    runtime_trace_status = ""
    ledger_status = ""
    if policy_contract_path.exists():
        policy_status = detect_status(parse_json(policy_contract_path))
        if policy_status != "pass":
            errors.append(f"backend selection manifest: policy contract status={policy_status} (expected pass)")
    if runtime_trace_path.exists():
        runtime_trace_status = "pass"
    if policy_ledger_snapshot_path.exists():
        ledger_payload = parse_json(policy_ledger_snapshot_path)
        ledger_status = detect_status(ledger_payload)
        if ledger_status != "pass":
            errors.append(f"backend selection manifest: policy ledger status={ledger_status} (expected pass)")
        sig = ledger_payload.get("signature", {})
        if not isinstance(sig, dict) or not bool(sig.get("verified", False)):
            errors.append("backend selection manifest: policy ledger signature is not verified")

    keyring_payload: dict[str, Any] = {}
    active_key: dict[str, Any] = {}
    if signing_keyring_path.exists():
        keyring_payload, active_key, key_errors = validate_keyring(signing_keyring_path)
        errors.extend(key_errors)

    manifest_core = {
        "schema": SELECTION_MANIFEST_SCHEMA,
        "runtime_binding_snapshot": {
            "backend_capabilities_sha256": capabilities_sha,
            "backend_selection_trace_sha256": sha256_file(selection_trace_artifact)
            if selection_trace_artifact.exists()
            else "",
            "backend_selection_replay_sha256": replay_sha
            if replay_sha
            else (sha256_file(replay_artifact) if replay_artifact.exists() else ""),
        },
        "bound_evidence": {
            "policy_contract": {
                "path": str(policy_contract_path),
                "sha256": sha256_file(policy_contract_path) if policy_contract_path.exists() else "",
                "status": policy_status,
            },
            "runtime_trace": {
                "path": str(runtime_trace_path),
                "sha256": sha256_file(runtime_trace_path) if runtime_trace_path.exists() else "",
                "status": runtime_trace_status,
            },
            "policy_ledger_snapshot": {
                "path": str(policy_ledger_snapshot_path),
                "sha256": sha256_file(policy_ledger_snapshot_path) if policy_ledger_snapshot_path.exists() else "",
                "status": ledger_status,
            },
        },
        "attestation_method": "hmac-sha256-keyring-signature.v1",
    }

    signature_hex = ""
    signature_verified = False
    verified_by = ""
    key_id = ""
    if active_key:
        key_id = str(active_key.get("key_id", "")).strip()
        signature_hex = sign_payload_hex(manifest_core, active_key)
        signature_verified, verified_by = verify_payload_signature(manifest_core, signature_hex, keyring_payload)
        if not signature_verified:
            errors.append("backend selection manifest: signature verification failed")
    else:
        errors.append("backend selection manifest: no active signing key available")

    status = "pass" if not errors else "fail"
    manifest_payload = {
        **manifest_core,
        "status": status,
        "errors": errors,
        "signature": {
            "algorithm": "hmac-sha256",
            "key_id": key_id,
            "signature_hex": signature_hex,
            "verified": signature_verified,
            "verified_by_key_id": verified_by,
            "keyring_path": str(signing_keyring_path),
            "keyring_sha256": sha256_file(signing_keyring_path) if signing_keyring_path.exists() else "",
            "rotation_policy": keyring_payload.get("rotation_policy", {}),
        },
    }

    json_path = out_dir / "runtime_backend_selection_manifest.json"
    md_path = out_dir / "runtime_backend_selection_manifest.md"
    json_path.write_text(json.dumps(manifest_payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# Backend Selection Manifest",
                "",
                f"- schema: `{SELECTION_MANIFEST_SCHEMA}`",
                f"- status: `{status}`",
                f"- key_id: `{key_id or 'n/a'}`",
                f"- signature_verified: `{str(signature_verified).lower()}`",
                f"- replay_sha256: `{manifest_core['runtime_binding_snapshot']['backend_selection_replay_sha256']}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    return status == "pass", errors, manifest_payload, json_path, md_path


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI backend adapter contract determinism baseline (RFC-00A5).")
    p.add_argument("--out-dir", required=True, help="Directory to write artifacts")
    p.add_argument("--ai-bin", default="", help="Optional path to t81_ai binary for runtime binding")
    p.add_argument(
        "--runtime-model",
        default="",
        help="Optional model path used during runtime binding probe (created if absent)",
    )
    p.add_argument("--policy-contract", default="", help="Path to ai_policy_event_contract.json for manifest binding")
    p.add_argument("--runtime-trace", default="", help="Path to ai_runtime_trace.json for manifest binding")
    p.add_argument(
        "--policy-ledger-snapshot",
        default="",
        help="Path to ai_axion_policy_ledger_snapshot.json for manifest binding",
    )
    p.add_argument(
        "--selection-signing-keyring",
        default="",
        help="Path to backend selection keyring (defaults to scripts/ci/ai_backend_selection_keyring.json)",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    registry_a = build_registry()
    registry_b = build_registry()
    hash_a = sha256_text(canonical_json(registry_a))
    hash_b = sha256_text(canonical_json(registry_b))
    deterministic = hash_a == hash_b

    contract_ok, contract_errors = check_contract(registry_a)
    runtime_binding_ok = True
    runtime_binding: dict[str, Any] = {}
    runtime_errors: list[str] = []
    selection_manifest_ok = True
    selection_manifest: dict[str, Any] = {}
    selection_manifest_errors: list[str] = []
    selection_manifest_artifact = ""
    selection_manifest_summary = ""

    if args.ai_bin:
        ai_bin = Path(args.ai_bin).resolve()
        runtime_model = Path(args.runtime_model).resolve() if args.runtime_model else (out_dir / "runtime_backend_probe_model.gguf")
        runtime_binding_ok, runtime_errors, runtime_binding = validate_runtime_binding(ai_bin, runtime_model)
        if runtime_binding_ok:
            policy_contract = Path(args.policy_contract).resolve() if args.policy_contract else Path()
            runtime_trace = Path(args.runtime_trace).resolve() if args.runtime_trace else Path()
            policy_ledger = Path(args.policy_ledger_snapshot).resolve() if args.policy_ledger_snapshot else Path()
            default_keyring = Path(__file__).resolve().with_name("ai_backend_selection_keyring.json")
            signing_keyring = Path(args.selection_signing_keyring).resolve() if args.selection_signing_keyring else default_keyring

            if not args.policy_contract or not args.runtime_trace or not args.policy_ledger_snapshot:
                selection_manifest_ok = False
                selection_manifest_errors = [
                    "backend selection manifest requires --policy-contract, --runtime-trace, and --policy-ledger-snapshot"
                ]
            else:
                selection_manifest_ok, selection_manifest_errors, selection_manifest, selection_json, selection_md = (
                    build_backend_selection_manifest(
                        out_dir=out_dir,
                        runtime_binding=runtime_binding,
                        policy_contract_path=policy_contract,
                        runtime_trace_path=runtime_trace,
                        policy_ledger_snapshot_path=policy_ledger,
                        signing_keyring_path=signing_keyring,
                    )
                )
                selection_manifest_artifact = str(selection_json)
                selection_manifest_summary = str(selection_md)

    status = "pass" if deterministic and contract_ok and runtime_binding_ok and selection_manifest_ok else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_registry_hash": deterministic,
        "registry_sha256": hash_a,
        "contract_ok": contract_ok,
        "runtime_binding_ok": runtime_binding_ok,
        "selection_manifest_ok": selection_manifest_ok,
        "errors": contract_errors + runtime_errors + selection_manifest_errors,
        "registry": registry_a,
        "runtime_binding": runtime_binding,
        "selection_manifest": selection_manifest,
        "selection_manifest_artifact": selection_manifest_artifact,
        "selection_manifest_summary": selection_manifest_summary,
    }

    json_path = out_dir / "ai_backend_adapter_contract.json"
    md_path = out_dir / "ai_backend_adapter_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Backend Adapter Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_registry_hash: `{deterministic}`",
                f"- contract_ok: `{contract_ok}`",
                f"- runtime_binding_ok: `{runtime_binding_ok}`",
                f"- selection_manifest_ok: `{selection_manifest_ok}`",
                f"- registry_sha256: `{hash_a}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai backend adapter contract status: {status}")
    print(f"artifact: {json_path}")
    if selection_manifest_artifact:
        print(f"selection manifest: {selection_manifest_artifact}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
