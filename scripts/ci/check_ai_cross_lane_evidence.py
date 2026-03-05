#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.cross-lane-evidence.v1"


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    p = argparse.ArgumentParser(description="Validate RFC-00A1 cross-lane evidence hash continuity.")
    p.add_argument("--out-dir", required=True, help="Directory to write cross-lane artifacts")
    p.add_argument("--evidence-bundle", required=True, help="Path to ai_evidence_bundle.json")
    p.add_argument("--ux-contract", required=True, help="Path to ai_ux_contract.json")
    p.add_argument("--ux-inference", required=True, help="Path to ai_inference_run.json")
    p.add_argument("--ux-quantization", required=True, help="Path to ai_quantization_inspect.json")
    p.add_argument("--ux-benchmark", required=True, help="Path to ai_benchmark_run.json")
    p.add_argument("--tloadhash-toolchain", required=True, help="Path to ai_tloadhash_toolchain.json")
    p.add_argument(
        "--governed-flow",
        default="",
        help="Optional governed llama flow artifact path",
    )
    args = p.parse_args()

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    evidence = parse_json(Path(args.evidence_bundle).resolve())
    ux_contract = parse_json(Path(args.ux_contract).resolve())
    ux_inference = parse_json(Path(args.ux_inference).resolve())
    ux_quant = parse_json(Path(args.ux_quantization).resolve())
    ux_bench = parse_json(Path(args.ux_benchmark).resolve())
    tloadhash = parse_json(Path(args.tloadhash_toolchain).resolve())
    governed = parse_json(Path(args.governed_flow).resolve()) if args.governed_flow else None

    errors: list[str] = []
    checks: dict[str, Any] = {}

    evidence_model_sha = str(evidence.get("inputs", {}).get("model_fixture_sha256", "")).strip()
    evidence_model_path = str(evidence.get("inputs", {}).get("model_fixture", "")).strip()
    if not evidence_model_sha:
        errors.append("evidence bundle missing inputs.model_fixture_sha256")
    if not evidence_model_path:
        errors.append("evidence bundle missing inputs.model_fixture")

    checks["evidence_model_fixture_sha256"] = evidence_model_sha
    checks["evidence_model_fixture"] = evidence_model_path
    replay_vector = evidence.get("fixture_locked_replay_vector", {})
    checks["fixture_locked_replay_vector_present"] = bool(replay_vector)
    if not replay_vector:
        errors.append("evidence bundle missing fixture_locked_replay_vector")

    tloadhash_fixture_sha = str(tloadhash.get("fixture_sha256", "")).strip()
    checks["tloadhash_fixture_sha256"] = tloadhash_fixture_sha
    if evidence_model_sha and tloadhash_fixture_sha and evidence_model_sha != tloadhash_fixture_sha:
        errors.append("cross-lane mismatch: evidence model_fixture_sha256 != tloadhash fixture_sha256")

    ux_model_paths: list[str] = []
    ux_model_hashes: list[str] = []
    for lane_name, payload in (
        ("ux_inference", ux_inference),
        ("ux_quantization", ux_quant),
        ("ux_benchmark", ux_bench),
    ):
        lane_sha = str(payload.get("model_file_sha256", "")).strip()
        lane_model_path = str(payload.get("model_file", "")).strip()
        checks[f"{lane_name}_model_file_sha256"] = lane_sha
        checks[f"{lane_name}_model_file"] = lane_model_path
        ux_model_paths.append(lane_model_path)
        ux_model_hashes.append(lane_sha)
        if not lane_sha.startswith("sha256:"):
            errors.append(f"{lane_name}: model_file_sha256 missing sha256: prefix")
        if not lane_model_path:
            errors.append(f"{lane_name}: model_file missing")

    if ux_model_paths and any(p != ux_model_paths[0] for p in ux_model_paths):
        errors.append("cross-lane mismatch: UX model_file paths differ across lanes")
    if ux_model_hashes and any(h != ux_model_hashes[0] for h in ux_model_hashes):
        errors.append("cross-lane mismatch: UX model_file_sha256 values differ across lanes")
    if evidence_model_path and ux_model_paths and Path(ux_model_paths[0]).resolve() != Path(evidence_model_path).resolve():
        errors.append("cross-lane mismatch: evidence model_fixture path != UX model_file path")

    replay = parse_json(Path(ux_contract["runtime_binding"].get("workflow_replay_artifact", out_dir / "_none")).resolve()) \
        if isinstance(ux_contract.get("runtime_binding", {}), dict) and ux_contract["runtime_binding"].get("workflow_replay_artifact") \
        else None
    if replay is None:
        # Fallback: use explicit uploaded UX replay artifact path.
        replay_path = Path(args.ux_contract).resolve().parent / "ai_workflow_replay.json"
        if replay_path.exists():
            replay = parse_json(replay_path)
    if replay is not None:
        checks["workflow_replay_policy_reason_code"] = replay.get("policy_reason_code")
        checks["workflow_replay_policy_decision"] = replay.get("policy_decision")
        if replay.get("policy_reason_code") != "AI_POLICY_ALLOW_MODEL_HASH_MATCH":
            errors.append("workflow replay policy_reason_code mismatch")
        if replay.get("policy_decision") != "allow":
            errors.append("workflow replay policy_decision mismatch")
    else:
        errors.append("missing workflow replay artifact for policy linkage check")

    if governed is not None:
        checks["governed_flow_model"] = governed.get("model", "")
        checks["governed_flow_status"] = governed.get("status", "")
        if governed.get("status") != "pass":
            errors.append("governed flow status must be pass when provided")

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "errors": errors,
        "checks": checks,
        "cross_lane_sha256": sha256_text(canonical_json(checks)),
    }

    json_path = out_dir / "ai_cross_lane_evidence.json"
    md_path = out_dir / "ai_cross_lane_evidence.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Cross-Lane Evidence",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- cross_lane_sha256: `{payload['cross_lane_sha256']}`",
                f"- errors: `{len(errors)}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"ai cross-lane evidence status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
