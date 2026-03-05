#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.opcode-subset.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def build_subset_contract() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "phase": "phase1",
        "source_rfc": "RFC-0026",
        "exploration_alignment_rfc": "RFC-00A8",
        "opcodes": [
            {
                "name": "ATTN",
                "category": "attention",
                "tier_min": 2,
                "requires_axion_guard": True,
                "determinism_class": "strict_deterministic",
            },
            {
                "name": "QMATMUL",
                "category": "linear_algebra",
                "tier_min": 2,
                "requires_axion_guard": True,
                "determinism_class": "strict_deterministic",
            },
            {
                "name": "EMBED",
                "category": "embedding",
                "tier_min": 2,
                "requires_axion_guard": True,
                "determinism_class": "strict_deterministic",
            },
        ],
        "explicitly_deferred": ["WLOAD", "GATHER", "SCATTER"],
        "required_reason_codes": [
            "AI_OPCODE_ATTN_GUARD",
            "AI_OPCODE_QMATMUL_GUARD",
            "AI_OPCODE_EMBED_GUARD",
        ],
    }


def validate_contract(contract: dict[str, Any]) -> tuple[bool, list[str]]:
    errs: list[str] = []
    expected = {"ATTN", "QMATMUL", "EMBED"}
    names = {op.get("name", "") for op in contract.get("opcodes", [])}
    if names != expected:
        errs.append(f"phase1 opcode set mismatch: expected {sorted(expected)} got {sorted(names)}")

    for op in contract.get("opcodes", []):
        if int(op.get("tier_min", 0)) < 2:
            errs.append(f"{op.get('name','?')}: tier_min must be >= 2")
        if not bool(op.get("requires_axion_guard", False)):
            errs.append(f"{op.get('name','?')}: requires_axion_guard must be true")
        if op.get("determinism_class") != "strict_deterministic":
            errs.append(f"{op.get('name','?')}: determinism_class must be strict_deterministic")

    deferred = set(contract.get("explicitly_deferred", []))
    for required in ("WLOAD", "GATHER", "SCATTER"):
        if required not in deferred:
            errs.append(f"missing deferred opcode marker: {required}")

    return len(errs) == 0, errs


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate RFC-0026/RFC-00A8 phase-1 opcode subset contract.")
    p.add_argument("--out-dir", required=True, help="Directory to write artifacts")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    contract_a = build_subset_contract()
    contract_b = build_subset_contract()
    hash_a = sha256_text(canonical_json(contract_a))
    hash_b = sha256_text(canonical_json(contract_b))
    deterministic = hash_a == hash_b

    valid, errors = validate_contract(contract_a)
    status = "pass" if deterministic and valid else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_contract_hash": deterministic,
        "contract_sha256": hash_a,
        "valid": valid,
        "errors": errors,
        "contract": contract_a,
    }

    json_path = out_dir / "ai_opcode_subset_contract.json"
    md_path = out_dir / "ai_opcode_subset_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Opcode Subset Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_contract_hash: `{deterministic}`",
                f"- contract_sha256: `{hash_a}`",
                f"- phase1 opcodes: `ATTN`, `QMATMUL`, `EMBED`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai opcode subset contract status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
