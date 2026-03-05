#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.ux-contract.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def run_cmd(argv: list[str]) -> dict[str, Any]:
    proc = subprocess.run(argv, capture_output=True, text=True, check=False)
    return {
        "argv": argv,
        "rc": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def build_contract() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "cli_surface": {
            "root": "t81_ai",
            "required_categories": [
                "model",
                "verify",
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


def validate_static_contract(contract: dict[str, Any]) -> tuple[bool, list[str]]:
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


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_runtime(ai_bin: Path, out_dir: Path) -> tuple[bool, list[str], dict[str, Any]]:
    errs: list[str] = []
    runtime: dict[str, Any] = {}

    help_result = run_cmd([str(ai_bin), "--help"])
    runtime["help"] = {
        "rc": help_result["rc"],
        "stdout_sha256": sha256_text(help_result["stdout"]),
        "stderr_sha256": sha256_text(help_result["stderr"]),
    }
    if help_result["rc"] != 0:
        errs.append("t81_ai --help failed")
    for marker in (
        "model inspect",
        "verify determinism",
        "workflow run",
        "workflow replay",
        "workflow report",
        "observability trace",
    ):
        if marker not in help_result["stdout"]:
            errs.append(f"missing help marker: {marker}")

    replay_path = out_dir / "ai_workflow_replay.json"
    trace_path = out_dir / "ai_observability_trace.json"
    model_path = out_dir / "ux_test_model.gguf"
    model_path.write_text("t81-ai-ux-fixture\n", encoding="utf-8")

    run_result = run_cmd(
        [
            str(ai_bin),
            "workflow",
            "run",
            "ci-ux-smoke",
            "--seed",
            "0",
            "--out",
            str(replay_path),
        ]
    )
    runtime["workflow_run"] = {"rc": run_result["rc"], "stdout_sha256": sha256_text(run_result["stdout"])}
    if run_result["rc"] != 0:
        errs.append("workflow run failed")
    if not replay_path.exists():
        errs.append("workflow run did not emit replay artifact")

    replay_result = run_cmd([str(ai_bin), "workflow", "replay", str(replay_path)])
    runtime["workflow_replay"] = {"rc": replay_result["rc"], "stdout_sha256": sha256_text(replay_result["stdout"])}
    if replay_result["rc"] != 0:
        errs.append("workflow replay failed")
    if "pass" not in replay_result["stdout"].lower():
        errs.append("workflow replay did not report pass")

    report_result = run_cmd([str(ai_bin), "workflow", "report", str(replay_path)])
    runtime["workflow_report"] = {"rc": report_result["rc"], "stdout_sha256": sha256_text(report_result["stdout"])}
    if report_result["rc"] != 0:
        errs.append("workflow report failed")

    trace_result = run_cmd([str(ai_bin), "observability", "trace", str(trace_path)])
    runtime["observability_trace"] = {"rc": trace_result["rc"], "stdout_sha256": sha256_text(trace_result["stdout"])}
    if trace_result["rc"] != 0:
        errs.append("observability trace failed")
    if not trace_path.exists():
        errs.append("observability trace did not emit artifact")

    inspect_result = run_cmd([str(ai_bin), "model", "inspect", str(model_path)])
    runtime["model_inspect"] = {"rc": inspect_result["rc"], "stdout_sha256": sha256_text(inspect_result["stdout"])}
    if inspect_result["rc"] != 0:
        errs.append("model inspect failed")

    verify_result = run_cmd([str(ai_bin), "verify", "determinism", str(model_path)])
    runtime["verify_determinism"] = {"rc": verify_result["rc"], "stdout_sha256": sha256_text(verify_result["stdout"])}
    if verify_result["rc"] != 0:
        errs.append("verify determinism failed")

    if replay_path.exists():
        replay = parse_json(replay_path)
        required_replay_fields = {"workflow_id", "session_id", "seed", "steps", "replay_hash", "status"}
        missing = sorted(required_replay_fields - set(replay.keys()))
        if missing:
            errs.append(f"workflow replay artifact missing fields: {', '.join(missing)}")
        if replay.get("schema") != "t81.ai.workflow-replay.v1":
            errs.append("workflow replay schema mismatch")
        if replay.get("status") not in {"pass", "fail"}:
            errs.append("workflow replay status must be pass/fail")
        runtime["workflow_artifact_sha256"] = sha256_text(canonical_json(replay))

    if trace_path.exists():
        trace = parse_json(trace_path)
        required_trace_fields = {"reason_code", "event_type", "decision", "timestamp_utc", "trace_sha256"}
        missing = sorted(required_trace_fields - set(trace.keys()))
        if missing:
            errs.append(f"trace artifact missing fields: {', '.join(missing)}")
        runtime["trace_artifact_sha256"] = sha256_text(canonical_json(trace))

    return len(errs) == 0, errs, runtime


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate RFC-00A7 UX runtime contract.")
    p.add_argument("--ai-bin", required=True)
    p.add_argument("--out-dir", required=True)
    return p.parse_args()


def main() -> int:
    args = parse_args()
    ai_bin = Path(args.ai_bin).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")

    c1 = build_contract()
    c2 = build_contract()
    h1 = sha256_text(canonical_json(c1))
    h2 = sha256_text(canonical_json(c2))
    deterministic = h1 == h2
    static_valid, static_errors = validate_static_contract(c1)
    runtime_valid, runtime_errors, runtime_details = validate_runtime(ai_bin, out_dir)
    errors = static_errors + runtime_errors
    valid = static_valid and runtime_valid
    status = "pass" if deterministic and valid else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_contract_hash": deterministic,
        "contract_sha256": h1,
        "valid": valid,
        "errors": errors,
        "contract": c1,
        "runtime_binding": runtime_details,
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
                f"- runtime_binding: `{runtime_valid}`",
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
