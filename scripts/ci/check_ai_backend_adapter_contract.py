#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.backend-adapter.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


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
    inspect_res = run_cmd([str(ai_bin), "model", "inspect", str(model_path)])
    verify_res = run_cmd([str(ai_bin), "verify", "determinism", str(model_path)])

    binding["help"] = {"rc": help_res["rc"], "stdout_sha256": help_res["stdout_sha256"]}
    binding["backend_capabilities"] = {
        "rc": caps_res["rc"],
        "stdout_sha256": caps_res["stdout_sha256"],
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
    if inspect_res["rc"] != 0:
        errors.append("runtime binding: t81_ai model inspect failed")
    if verify_res["rc"] != 0:
        errors.append("runtime binding: t81_ai verify determinism failed")

    required_help_markers = ("model inspect", "verify determinism", "backend capabilities")
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
                errors.append(
                    "runtime binding: backend capabilities missing fields "
                    + ", ".join(missing_top)
                )
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
                            f"runtime binding: backend capabilities entry {idx} missing "
                            + ", ".join(missing)
                        )
                if caps.get("default_backend") not in backend_names:
                    errors.append("runtime binding: default_backend missing from backends list")

    if "Status: Inspection completed" not in inspect_res["stdout"]:
        errors.append("runtime binding: model inspect output missing completion marker")
    if "Determinism mode: strict" not in verify_res["stdout"]:
        errors.append("runtime binding: verify determinism output missing strict mode marker")

    return len(errors) == 0, errors, binding


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI backend adapter contract determinism baseline (RFC-00A5).")
    p.add_argument("--out-dir", required=True, help="Directory to write artifacts")
    p.add_argument("--ai-bin", default="", help="Optional path to t81_ai binary for runtime binding")
    p.add_argument(
        "--runtime-model",
        default="",
        help="Optional model path used during runtime binding probe (created if absent)",
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
    if args.ai_bin:
        ai_bin = Path(args.ai_bin).resolve()
        runtime_model = (
            Path(args.runtime_model).resolve()
            if args.runtime_model
            else (out_dir / "runtime_backend_probe_model.gguf")
        )
        runtime_binding_ok, runtime_errors, runtime_binding = validate_runtime_binding(ai_bin, runtime_model)

    status = "pass" if deterministic and contract_ok and runtime_binding_ok else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_registry_hash": deterministic,
        "registry_sha256": hash_a,
        "contract_ok": contract_ok,
        "runtime_binding_ok": runtime_binding_ok,
        "errors": contract_errors + runtime_errors,
        "registry": registry_a,
        "runtime_binding": runtime_binding,
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
                f"- registry_sha256: `{hash_a}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai backend adapter contract status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
