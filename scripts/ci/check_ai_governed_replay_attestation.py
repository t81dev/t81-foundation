#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.governed-llama-replay.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


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

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
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
    }

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
