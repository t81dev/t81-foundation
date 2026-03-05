#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
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


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI backend adapter contract determinism baseline (RFC-00A5).")
    p.add_argument("--out-dir", required=True, help="Directory to write artifacts")
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
    status = "pass" if deterministic and contract_ok else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_registry_hash": deterministic,
        "registry_sha256": hash_a,
        "contract_ok": contract_ok,
        "errors": contract_errors,
        "registry": registry_a,
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
