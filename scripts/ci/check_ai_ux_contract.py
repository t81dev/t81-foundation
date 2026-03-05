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
                "backend",
                "inference",
                "quantization",
                "benchmark",
                "policy",
                "observability",
                "workflow",
            ],
            "minimum_actions": {
                "model": ["inspect"],
                "verify": ["determinism"],
                "backend": ["capabilities"],
                "inference": ["run"],
                "quantization": ["inspect"],
                "benchmark": ["run"],
                "policy": ["test"],
                "workflow": ["run", "replay", "report"],
                "observability": ["trace"],
            },
        },
        "backend_capabilities_contract": {
            "schema": "t81.ai.backend-capabilities.v1",
            "required_fields": [
                "schema",
                "default_backend",
                "selection_policy",
                "backends",
            ],
            "required_backend_fields": [
                "backend_name",
                "supported_formats",
                "determinism_modes",
                "max_context_tokens",
                "supports_streaming",
                "supports_logit_bias",
            ],
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
    if "backend" not in cats:
        errs.append("required category missing: backend")
    if "inference" not in cats:
        errs.append("required category missing: inference")
    if "quantization" not in cats:
        errs.append("required category missing: quantization")
    if "benchmark" not in cats:
        errs.append("required category missing: benchmark")
    if "policy" not in cats:
        errs.append("required category missing: policy")

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

    bc = contract["backend_capabilities_contract"]
    required_backend_fields = set(bc["required_backend_fields"])
    for field in ("backend_name", "supported_formats", "determinism_modes", "max_context_tokens"):
        if field not in required_backend_fields:
            errs.append(f"backend_capabilities_contract missing backend field: {field}")

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
        "backend capabilities",
        "inference run",
        "quantization inspect",
        "benchmark run",
        "policy test",
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

    backend_result = run_cmd([str(ai_bin), "backend", "capabilities"])
    runtime["backend_capabilities"] = {"rc": backend_result["rc"], "stdout_sha256": sha256_text(backend_result["stdout"])}
    if backend_result["rc"] != 0:
        errs.append("backend capabilities failed")
    else:
        try:
            backend_caps = json.loads(backend_result["stdout"])
        except json.JSONDecodeError:
            backend_caps = None
            errs.append("backend capabilities output is not valid JSON")
        if isinstance(backend_caps, dict):
            cap_out_path = out_dir / "ai_backend_capabilities.json"
            cap_out_path.write_text(json.dumps(backend_caps, indent=2, sort_keys=True), encoding="utf-8")
            runtime["backend_capabilities_artifact_sha256"] = sha256_text(canonical_json(backend_caps))
            if backend_caps.get("schema") != "t81.ai.backend-capabilities.v1":
                errs.append("backend capabilities schema mismatch")
            required_fields = {"schema", "default_backend", "selection_policy", "backends"}
            missing_fields = sorted(required_fields - set(backend_caps.keys()))
            if missing_fields:
                errs.append(f"backend capabilities missing fields: {', '.join(missing_fields)}")
            backends = backend_caps.get("backends")
            if not isinstance(backends, list) or not backends:
                errs.append("backend capabilities backends must be a non-empty list")
            else:
                required_backend_fields = {
                    "backend_name",
                    "supported_formats",
                    "determinism_modes",
                    "max_context_tokens",
                    "supports_streaming",
                    "supports_logit_bias",
                }
                for i, backend in enumerate(backends):
                    if not isinstance(backend, dict):
                        errs.append(f"backend capabilities entry {i} is not an object")
                        continue
                    missing = sorted(required_backend_fields - set(backend.keys()))
                    if missing:
                        errs.append(
                            f"backend capabilities entry {i} missing fields: {', '.join(missing)}"
                        )
                default_backend = backend_caps.get("default_backend")
                backend_names = {
                    b.get("backend_name", "")
                    for b in backends
                    if isinstance(b, dict)
                }
                if default_backend not in backend_names:
                    errs.append("backend capabilities default_backend not found in backends list")

    inference_path = out_dir / "ai_inference_run.json"
    inference_result = run_cmd(
        [
            str(ai_bin),
            "inference",
            "run",
            "--model",
            "ci-ux-model",
            "--prompt",
            "deterministic prompt",
            "--out",
            str(inference_path),
        ]
    )
    runtime["inference_run"] = {"rc": inference_result["rc"], "stdout_sha256": sha256_text(inference_result["stdout"])}
    if inference_result["rc"] != 0:
        errs.append("inference run failed")
    if not inference_path.exists():
        errs.append("inference run did not emit artifact")
    else:
        inference = parse_json(inference_path)
        req = {"schema", "model_id", "prompt_sha256", "output", "status"}
        missing = sorted(req - set(inference.keys()))
        if missing:
            errs.append(f"inference artifact missing fields: {', '.join(missing)}")
        if inference.get("schema") != "t81.ai.inference-run.v1":
            errs.append("inference artifact schema mismatch")
        runtime["inference_artifact_sha256"] = sha256_text(canonical_json(inference))

    quant_path = out_dir / "ai_quantization_inspect.json"
    quant_result = run_cmd(
        [
            str(ai_bin),
            "quantization",
            "inspect",
            "--model",
            "ci-ux-model",
            "--out",
            str(quant_path),
        ]
    )
    runtime["quantization_inspect"] = {"rc": quant_result["rc"], "stdout_sha256": sha256_text(quant_result["stdout"])}
    if quant_result["rc"] != 0:
        errs.append("quantization inspect failed")
    if not quant_path.exists():
        errs.append("quantization inspect did not emit artifact")
    else:
        quant = parse_json(quant_path)
        req = {"schema", "model_id", "codec", "bits_per_weight", "status"}
        missing = sorted(req - set(quant.keys()))
        if missing:
            errs.append(f"quantization artifact missing fields: {', '.join(missing)}")
        if quant.get("schema") != "t81.ai.quantization-inspect.v1":
            errs.append("quantization artifact schema mismatch")
        runtime["quantization_artifact_sha256"] = sha256_text(canonical_json(quant))

    benchmark_path = out_dir / "ai_benchmark_run.json"
    benchmark_result = run_cmd(
        [
            str(ai_bin),
            "benchmark",
            "run",
            "--model",
            "ci-ux-model",
            "--out",
            str(benchmark_path),
        ]
    )
    runtime["benchmark_run"] = {"rc": benchmark_result["rc"], "stdout_sha256": sha256_text(benchmark_result["stdout"])}
    if benchmark_result["rc"] != 0:
        errs.append("benchmark run failed")
    if not benchmark_path.exists():
        errs.append("benchmark run did not emit artifact")
    else:
        benchmark = parse_json(benchmark_path)
        req = {"schema", "model_id", "latency_ms", "throughput_tokens_per_sec", "status"}
        missing = sorted(req - set(benchmark.keys()))
        if missing:
            errs.append(f"benchmark artifact missing fields: {', '.join(missing)}")
        if benchmark.get("schema") != "t81.ai.benchmark-run.v1":
            errs.append("benchmark artifact schema mismatch")
        runtime["benchmark_artifact_sha256"] = sha256_text(canonical_json(benchmark))

    policy_path = out_dir / "ai_policy_test.json"
    policy_result = run_cmd(
        [
            str(ai_bin),
            "policy",
            "test",
            "--event-type",
            "model_load",
            "--out",
            str(policy_path),
        ]
    )
    runtime["policy_test"] = {"rc": policy_result["rc"], "stdout_sha256": sha256_text(policy_result["stdout"])}
    if policy_result["rc"] != 0:
        errs.append("policy test failed")
    if not policy_path.exists():
        errs.append("policy test did not emit artifact")
    else:
        policy = parse_json(policy_path)
        req = {"schema", "event_type", "decision", "reason_code", "status"}
        missing = sorted(req - set(policy.keys()))
        if missing:
            errs.append(f"policy artifact missing fields: {', '.join(missing)}")
        if policy.get("schema") != "t81.ai.policy-test.v1":
            errs.append("policy artifact schema mismatch")
        runtime["policy_artifact_sha256"] = sha256_text(canonical_json(policy))

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
