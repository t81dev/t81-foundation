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
DIRECT_BACKEND_SCHEMA = "t81.ai.ux-direct-backend-attestation.v1"


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


def resolve_runtime_model(repo_root: Path, out_dir: Path, user_model: str) -> tuple[Path, str]:
    if user_model:
        p = Path(user_model).resolve()
        return p, "user"

    preferred = [
        repo_root / "tests/fixtures/llama_cpp_repro/model.gguf",
        repo_root / "artifacts/archive/dummy.gguf",
    ]
    for p in preferred:
        if p.exists():
            return p, "fixture"

    model_candidates = sorted((repo_root / "models").glob("*.gguf"))
    if model_candidates:
        return model_candidates[0], "models"

    fallback = out_dir / "ux_test_model.gguf"
    fallback.write_text("t81-ai-ux-fixture\n", encoding="utf-8")
    return fallback, "generated"


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
                "policy_decision",
                "policy_reason_code",
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
    for field in (
        "workflow_id",
        "session_id",
        "seed",
        "steps",
        "policy_decision",
        "policy_reason_code",
        "replay_hash",
        "status",
    ):
        if field not in required_fields:
            errs.append(f"workflow_replay_contract missing field: {field}")

    if set(wr["status_values"]) != {"pass", "fail"}:
        errs.append("workflow_replay_contract.status_values must be exactly pass/fail")

    bc = contract["backend_capabilities_contract"]
    required_backend_fields = set(bc["required_backend_fields"])
    for f in (
        "backend_name",
        "supported_formats",
        "determinism_modes",
        "max_context_tokens",
        "supports_streaming",
        "supports_logit_bias",
    ):
        if f not in required_backend_fields:
            errs.append(f"backend_capabilities_contract missing backend field: {f}")

    obs = contract["observability_contract"]
    rf = set(obs["trace_reason_fields"])
    for f in ("reason_code", "event_type", "decision", "timestamp_utc"):
        if f not in rf:
            errs.append(f"observability_contract missing trace reason field: {f}")
    if obs.get("determinism_field") != "trace_sha256":
        errs.append("observability_contract.determinism_field must be trace_sha256")

    return len(errs) == 0, errs


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def run_direct_backend_attestation(
    out_dir: Path,
    model_path: Path,
    t81_bin: Path,
    hash_probe: Path,
) -> tuple[bool, list[str], dict[str, Any]]:
    errs: list[str] = []
    details: dict[str, Any] = {
        "schema": DIRECT_BACKEND_SCHEMA,
        "model_path": str(model_path),
        "t81_bin": str(t81_bin),
        "hash_probe": str(hash_probe),
    }

    if not t81_bin.exists():
        errs.append(f"direct backend attestation: t81 binary not found: {t81_bin}")
    if not model_path.exists():
        errs.append(f"direct backend attestation: model not found: {model_path}")
    if not hash_probe.exists():
        errs.append(f"direct backend attestation: hash probe script not found: {hash_probe}")

    model_hash = ""
    if not errs:
        probe = run_cmd([sys.executable, str(hash_probe), "--t81-bin", str(t81_bin), str(model_path)])
        details["hash_probe_rc"] = probe["rc"]
        details["hash_probe_stdout_sha256"] = sha256_text(probe["stdout"])
        details["hash_probe_stderr_sha256"] = sha256_text(probe["stderr"])
        if probe["rc"] != 0:
            errs.append("direct backend attestation: llama_model_hash probe failed")
        else:
            model_hash = probe["stdout"].strip()
            if not model_hash.startswith("sha3-512:"):
                errs.append(f"direct backend attestation: invalid model hash output: {model_hash}")

    policy_path = out_dir / "ai_direct_backend_policy.apl"
    prompt = "RFC-00A7 direct backend execution attestation."
    if not errs:
        policy_path.write_text(
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

    runs: list[dict[str, Any]] = []
    if not errs:
        for i in (1, 2):
            cmd = [
                str(t81_bin),
                "llama-run",
                str(model_path),
                prompt,
                "--policy",
                str(policy_path),
                "--max-tokens",
                "8",
                "--seed",
                "0",
                "--threads",
                "1",
                "--temperature",
                "0",
                "--top-k",
                "1",
                "--top-p",
                "1.0",
                "--expected-model-hash",
                model_hash,
            ]
            res = run_cmd(cmd)
            stdout_path = out_dir / f"ai_direct_backend_run{i}.stdout.log"
            stderr_path = out_dir / f"ai_direct_backend_run{i}.stderr.log"
            stdout_path.write_text(res["stdout"], encoding="utf-8")
            stderr_path.write_text(res["stderr"], encoding="utf-8")
            runs.append(
                {
                    "run": i,
                    "rc": res["rc"],
                    "stdout_sha256": sha256_text(res["stdout"]),
                    "stderr_sha256": sha256_text(res["stderr"]),
                    "stdout_log": str(stdout_path),
                    "stderr_log": str(stderr_path),
                }
            )
            if res["rc"] != 0:
                errs.append(f"direct backend attestation: llama-run replay {i} failed")

    deterministic_replay = False
    if len(runs) == 2:
        deterministic_replay = (
            runs[0]["rc"] == runs[1]["rc"]
            and runs[0]["stdout_sha256"] == runs[1]["stdout_sha256"]
            and runs[0]["stderr_sha256"] == runs[1]["stderr_sha256"]
        )
        if not deterministic_replay:
            errs.append("direct backend attestation: replay output hashes mismatch")

    status = "pass" if not errs and deterministic_replay else "fail"
    payload = {
        **details,
        "status": status,
        "model_hash": model_hash,
        "policy_file": str(policy_path),
        "deterministic_replay": deterministic_replay,
        "runs": runs,
        "errors": errs,
    }

    json_path = out_dir / "ai_direct_backend_execution_attestation.json"
    md_path = out_dir / "ai_direct_backend_execution_attestation.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# Direct Backend Execution Attestation",
                "",
                f"- schema: `{DIRECT_BACKEND_SCHEMA}`",
                f"- status: `{status}`",
                f"- deterministic_replay: `{str(deterministic_replay).lower()}`",
                f"- model_hash: `{model_hash}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    payload["artifact"] = str(json_path)
    payload["summary"] = str(md_path)
    return status == "pass", errs, payload


def validate_runtime(
    ai_bin: Path,
    out_dir: Path,
    user_model: str,
    t81_bin: str,
    llama_hash_probe: str,
) -> tuple[bool, list[str], dict[str, Any]]:
    errs: list[str] = []
    runtime: dict[str, Any] = {"ai_bin": str(ai_bin)}
    if not ai_bin.exists():
        return False, [f"ai binary not found: {ai_bin}"], runtime

    repo_root = Path.cwd().resolve()
    model_path, model_origin = resolve_runtime_model(repo_root, out_dir, user_model)
    runtime["runtime_model"] = str(model_path)
    runtime["runtime_model_origin"] = model_origin

    run_path = out_dir / "ai_workflow_run.json"
    replay_path = out_dir / "ai_workflow_replay.json"
    report_path = out_dir / "ai_workflow_report.json"
    trace_path = out_dir / "ai_observability_trace.json"

    run_result = run_cmd([str(ai_bin), "workflow", "run", "--out", str(run_path)])
    runtime["workflow_run"] = {"rc": run_result["rc"], "stdout_sha256": sha256_text(run_result["stdout"])}
    if run_result["rc"] != 0:
        errs.append("workflow run failed")

    replay_result = run_cmd([str(ai_bin), "workflow", "replay", "--out", str(replay_path)])
    runtime["workflow_replay"] = {"rc": replay_result["rc"], "stdout_sha256": sha256_text(replay_result["stdout"])}
    if replay_result["rc"] != 0:
        errs.append("workflow replay failed")

    report_result = run_cmd([str(ai_bin), "workflow", "report", "--out", str(report_path)])
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
                        errs.append(f"backend capabilities entry {i} missing fields: {', '.join(missing)}")
                default_backend = backend_caps.get("default_backend")
                backend_names = {b.get("backend_name", "") for b in backends if isinstance(b, dict)}
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
            "--model-file",
            str(model_path),
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
        req = {
            "schema",
            "model_id",
            "model_file",
            "model_file_sha256",
            "requested_format",
            "requested_mode",
            "selected_backend",
            "backend_selection_trace_sha256",
            "prompt_sha256",
            "output",
            "generated_tokens",
            "status",
        }
        missing = sorted(req - set(inference.keys()))
        if missing:
            errs.append(f"inference artifact missing fields: {', '.join(missing)}")
        if inference.get("schema") != "t81.ai.inference-run.v1":
            errs.append("inference artifact schema mismatch")
        if Path(inference.get("model_file", "")).resolve() != model_path.resolve():
            errs.append("inference artifact model_file does not match runtime model path")
        runtime["inference_artifact_sha256"] = sha256_text(canonical_json(inference))

    quant_path = out_dir / "ai_quantization_inspect.json"
    quant_result = run_cmd(
        [
            str(ai_bin),
            "quantization",
            "inspect",
            "--model",
            "ci-ux-model",
            "--model-file",
            str(model_path),
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
        req = {
            "schema",
            "model_id",
            "model_file",
            "model_file_sha256",
            "requested_format",
            "requested_mode",
            "selected_backend",
            "backend_selection_trace_sha256",
            "codec",
            "bits_per_weight",
            "quantization_profile",
            "status",
        }
        missing = sorted(req - set(quant.keys()))
        if missing:
            errs.append(f"quantization artifact missing fields: {', '.join(missing)}")
        if quant.get("schema") != "t81.ai.quantization-inspect.v1":
            errs.append("quantization artifact schema mismatch")
        if Path(quant.get("model_file", "")).resolve() != model_path.resolve():
            errs.append("quantization artifact model_file does not match runtime model path")
        runtime["quantization_artifact_sha256"] = sha256_text(canonical_json(quant))

    benchmark_path = out_dir / "ai_benchmark_run.json"
    benchmark_result = run_cmd(
        [
            str(ai_bin),
            "benchmark",
            "run",
            "--model",
            "ci-ux-model",
            "--model-file",
            str(model_path),
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
        req = {
            "schema",
            "model_id",
            "model_file",
            "model_file_sha256",
            "requested_format",
            "requested_mode",
            "selected_backend",
            "backend_selection_trace_sha256",
            "latency_ms",
            "throughput_tokens_per_sec",
            "determinism_score",
            "status",
        }
        missing = sorted(req - set(benchmark.keys()))
        if missing:
            errs.append(f"benchmark artifact missing fields: {', '.join(missing)}")
        if benchmark.get("schema") != "t81.ai.benchmark-run.v1":
            errs.append("benchmark artifact schema mismatch")
        if Path(benchmark.get("model_file", "")).resolve() != model_path.resolve():
            errs.append("benchmark artifact model_file does not match runtime model path")
        runtime["benchmark_artifact_sha256"] = sha256_text(canonical_json(benchmark))

    policy_path = out_dir / "ai_policy_test.json"
    policy_result = run_cmd(
        [
            str(ai_bin),
            "policy",
            "test",
            "--event-type",
            "model_load",
            "--model-file",
            str(model_path),
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
        req = {"schema", "event_type", "model_file", "model_file_sha256", "decision", "reason_code", "status"}
        missing = sorted(req - set(policy.keys()))
        if missing:
            errs.append(f"policy artifact missing fields: {', '.join(missing)}")
        if policy.get("schema") != "t81.ai.policy-test.v1":
            errs.append("policy artifact schema mismatch")
        if Path(policy.get("model_file", "")).resolve() != model_path.resolve():
            errs.append("policy artifact model_file does not match runtime model path")
        if policy.get("decision") != "allow":
            errs.append("policy artifact decision must be allow for model_load fixture path")
        runtime["policy_artifact_sha256"] = sha256_text(canonical_json(policy))

    if inference_path.exists() and quant_path.exists() and benchmark_path.exists():
        inference = parse_json(inference_path)
        quant = parse_json(quant_path)
        benchmark = parse_json(benchmark_path)
        selected = {
            inference.get("selected_backend"),
            quant.get("selected_backend"),
            benchmark.get("selected_backend"),
        }
        if len(selected) != 1:
            errs.append("runtime semantics: selected_backend mismatch across inference/quantization/benchmark")
        traces = {
            inference.get("backend_selection_trace_sha256"),
            quant.get("backend_selection_trace_sha256"),
            benchmark.get("backend_selection_trace_sha256"),
        }
        if len(traces) != 1:
            errs.append("runtime semantics: backend_selection_trace_sha256 mismatch across commands")

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
        required_replay_fields = {
            "workflow_id",
            "session_id",
            "seed",
            "steps",
            "policy_decision",
            "policy_reason_code",
            "replay_hash",
            "status",
        }
        missing = sorted(required_replay_fields - set(replay.keys()))
        if missing:
            errs.append(f"workflow replay artifact missing fields: {', '.join(missing)}")
        if replay.get("schema") != "t81.ai.workflow-replay.v1":
            errs.append("workflow replay schema mismatch")
        if replay.get("status") not in {"pass", "fail"}:
            errs.append("workflow replay status must be pass/fail")
        steps = replay.get("steps", [])
        if not any(isinstance(s, dict) and s.get("action") == "policy.test" for s in steps):
            errs.append("workflow replay missing policy.test step")
        runtime["workflow_artifact_sha256"] = sha256_text(canonical_json(replay))
        if policy_path.exists():
            policy = parse_json(policy_path)
            if replay.get("policy_reason_code") != policy.get("reason_code"):
                errs.append("workflow replay policy_reason_code does not match policy test reason_code")
            if replay.get("policy_decision") != policy.get("decision"):
                errs.append("workflow replay policy_decision does not match policy test decision")

    if trace_path.exists():
        trace = parse_json(trace_path)
        required_trace_fields = {"reason_code", "event_type", "decision", "timestamp_utc", "trace_sha256"}
        missing = sorted(required_trace_fields - set(trace.keys()))
        if missing:
            errs.append(f"trace artifact missing fields: {', '.join(missing)}")
        runtime["trace_artifact_sha256"] = sha256_text(canonical_json(trace))

    t81_path = Path(t81_bin).resolve() if t81_bin else Path()
    hash_probe_path = Path(llama_hash_probe).resolve() if llama_hash_probe else Path()
    if not t81_bin:
        errs.append("direct backend attestation requires --t81-bin")
    else:
        direct_ok, direct_errs, direct_payload = run_direct_backend_attestation(
            out_dir=out_dir,
            model_path=model_path,
            t81_bin=t81_path,
            hash_probe=hash_probe_path,
        )
        runtime["direct_backend_execution"] = {
            "ok": direct_ok,
            "artifact": direct_payload.get("artifact", ""),
            "summary": direct_payload.get("summary", ""),
            "deterministic_replay": direct_payload.get("deterministic_replay", False),
            "model_hash": direct_payload.get("model_hash", ""),
        }
        errs.extend(direct_errs)

    return len(errs) == 0, errs, runtime


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate RFC-00A7 UX runtime contract.")
    p.add_argument("--ai-bin", required=True)
    p.add_argument("--out-dir", required=True)
    p.add_argument(
        "--runtime-model",
        default="",
        help="Optional GGUF model path for fixture-backed runtime UX commands",
    )
    p.add_argument(
        "--t81-bin",
        default="",
        help="Path to llama-enabled t81 binary for direct backend execution attestation",
    )
    p.add_argument(
        "--llama-hash-probe",
        default="scripts/ci/llama_model_hash.py",
        help="Path to llama hash probe helper script",
    )
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
    runtime_valid, runtime_errors, runtime_details = validate_runtime(
        ai_bin,
        out_dir,
        args.runtime_model,
        args.t81_bin,
        args.llama_hash_probe,
    )
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
