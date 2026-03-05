#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.ux-contract.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def build_contract() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "cli_surface": {
            "root": "t81 ai",
            "required_categories": [
                "model",
                "inference",
                "quantization",
                "benchmark",
                "verify",
                "policy",
                "observability",
                "workflow",
            ],
            "minimum_actions": {
                "model": ["inspect"],
                "verify": ["determinism"],
                "workflow": ["run", "replay", "report"],
                "observability": ["trace"],
            },
        },
        "workflow_replay_contract": {
            "schema": "t81.ai.workflow-replay.v1",
            "required_fields": [
                "workflow_id",
                "session_id",
                "seed",
                "steps",
                "replay_hash",
                "status",
            ],
            "status_values": ["pass", "fail"],
        },
        "observability_contract": {
            "trace_reason_fields": ["reason_code", "event_type", "decision", "timestamp_utc"],
            "determinism_field": "trace_sha256",
        },
    }


def validate_contract(contract: dict[str, Any]) -> tuple[bool, list[str]]:
    errs: list[str] = []
    cats = set(contract["cli_surface"]["required_categories"])
    if "workflow" not in cats:
        errs.append("required category missing: workflow")
    if "observability" not in cats:
        errs.append("required category missing: observability")

    min_actions = contract["cli_surface"]["minimum_actions"]
    for category, required in min_actions.items():
        actions = set(required)
        if len(actions) == 0:
            errs.append(f"{category}: minimum_actions cannot be empty")

    wr = contract["workflow_replay_contract"]
    required_fields = set(wr["required_fields"])
    for field in ("workflow_id", "session_id", "seed", "steps", "replay_hash", "status"):
        if field not in required_fields:
            errs.append(f"workflow_replay_contract missing field: {field}")

    if set(wr["status_values"]) != {"pass", "fail"}:
        errs.append("workflow_replay_contract.status_values must be exactly pass/fail")

    obs = contract["observability_contract"]
    needed = {"reason_code", "event_type", "decision", "timestamp_utc"}
    if set(obs["trace_reason_fields"]) != needed:
        errs.append("observability_contract.trace_reason_fields mismatch")
    if obs.get("determinism_field") != "trace_sha256":
        errs.append("observability_contract.determinism_field must be trace_sha256")

    return len(errs) == 0, errs


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate RFC-00A7 UX contract baseline.")
    p.add_argument("--out-dir", required=True)
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    c1 = build_contract()
    c2 = build_contract()
    h1 = sha256_text(canonical_json(c1))
    h2 = sha256_text(canonical_json(c2))
    deterministic = h1 == h2
    valid, errors = validate_contract(c1)
    status = "pass" if deterministic and valid else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_contract_hash": deterministic,
        "contract_sha256": h1,
        "valid": valid,
        "errors": errors,
        "contract": c1,
    }
    json_path = out_dir / "ai_ux_contract.json"
    md_path = out_dir / "ai_ux_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI UX Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_contract_hash: `{deterministic}`",
                f"- contract_sha256: `{h1}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"ai ux contract status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
