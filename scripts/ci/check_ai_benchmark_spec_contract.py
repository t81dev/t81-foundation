#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from datetime import UTC, date, datetime
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.benchmark-spec.v1"


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


def parse_iso_date(raw: str, label: str) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"invalid {label} date: {raw} (expected YYYY-MM-DD)") from exc


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

    fallback = out_dir / "benchmark_test_model.gguf"
    fallback.write_text("t81-ai-benchmark-fixture\n", encoding="utf-8")
    return fallback, "generated"


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate runtime AI benchmark execution + regression thresholds (RFC-00A2).")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--ai-bin", required=True)
    p.add_argument("--runtime-model", default="")
    p.add_argument(
        "--thresholds-file",
        default="",
        help="Path to benchmark threshold baseline JSON (defaults to scripts/ci/ai_benchmark_thresholds.json).",
    )
    p.add_argument(
        "--thresholds-history-file",
        default="",
        help="Optional benchmark threshold history file for active window selection.",
    )
    p.add_argument(
        "--as-of-date",
        default="",
        help="Optional YYYY-MM-DD date to select active threshold window (defaults to today UTC).",
    )
    return p.parse_args()


def select_threshold_window(
    thresholds_path: Path,
    history_path: Path | None,
    as_of: date,
) -> tuple[dict[str, Any], dict[str, Any]]:
    default_thresholds = parse_json(thresholds_path)
    metadata = {
        "source": str(thresholds_path),
        "history_file": str(history_path) if history_path else "",
        "as_of_date": as_of.isoformat(),
        "window_id": "",
        "window_start": "",
        "window_end": "",
    }
    if history_path is None or not history_path.exists():
        return default_thresholds, metadata

    history = parse_json(history_path)
    if history.get("schema") != "t81.ai.benchmark-thresholds-history.v1":
        raise ValueError(f"benchmark thresholds history schema mismatch: {history.get('schema')}")
    windows = history.get("windows")
    if not isinstance(windows, list) or not windows:
        raise ValueError("benchmark thresholds history windows must be non-empty list")

    selected: dict[str, Any] | None = None
    for win in windows:
        if not isinstance(win, dict):
            continue
        start_raw = str(win.get("window_start", "")).strip()
        end_raw = str(win.get("window_end", "")).strip()
        if not start_raw or not end_raw:
            continue
        start = parse_iso_date(start_raw, "window_start")
        end = parse_iso_date(end_raw, "window_end")
        if start <= as_of <= end:
            selected = win
            break
    if selected is None:
        candidates: list[tuple[date, dict[str, Any]]] = []
        for win in windows:
            if not isinstance(win, dict):
                continue
            start_raw = str(win.get("window_start", "")).strip()
            if not start_raw:
                continue
            try:
                start = parse_iso_date(start_raw, "window_start")
            except ValueError:
                continue
            if start <= as_of:
                candidates.append((start, win))
        if candidates:
            candidates.sort(key=lambda item: item[0], reverse=True)
            selected = candidates[0][1]
    if selected is None:
        raise ValueError("no valid threshold window found in history")

    thresholds = selected.get("thresholds")
    if not isinstance(thresholds, dict):
        raise ValueError("selected threshold window missing thresholds object")
    metadata.update(
        {
            "source": str(history_path),
            "window_id": str(selected.get("window_id", "")),
            "window_start": str(selected.get("window_start", "")),
            "window_end": str(selected.get("window_end", "")),
        }
    )
    return thresholds, metadata


def validate_benchmark_payload(payload: dict[str, Any], model_path: Path) -> tuple[bool, list[str]]:
    errs: list[str] = []
    required = {
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
    missing = sorted(required - set(payload.keys()))
    if missing:
        errs.append("benchmark payload missing fields: " + ", ".join(missing))

    if payload.get("schema") != "t81.ai.benchmark-run.v1":
        errs.append("benchmark payload schema mismatch")
    if Path(str(payload.get("model_file", ""))).resolve() != model_path.resolve():
        errs.append("benchmark payload model_file mismatch")

    try:
        latency = float(payload.get("latency_ms", -1))
        if latency <= 0:
            errs.append("benchmark payload latency_ms must be > 0")
    except Exception:
        errs.append("benchmark payload latency_ms must be numeric")

    try:
        throughput = float(payload.get("throughput_tokens_per_sec", -1))
        if throughput <= 0:
            errs.append("benchmark payload throughput_tokens_per_sec must be > 0")
    except Exception:
        errs.append("benchmark payload throughput_tokens_per_sec must be numeric")

    try:
        score = float(payload.get("determinism_score", -1))
        if score < 0 or score > 1:
            errs.append("benchmark payload determinism_score must be in [0,1]")
    except Exception:
        errs.append("benchmark payload determinism_score must be numeric")

    if payload.get("status") != "pass":
        errs.append("benchmark payload status must be pass")

    return len(errs) == 0, errs


def evaluate_thresholds(
    observed: dict[str, float],
    thresholds: dict[str, Any],
) -> tuple[bool, dict[str, Any], list[str]]:
    errs: list[str] = []
    evaluation: dict[str, Any] = {}

    baseline = thresholds.get("baseline", {})
    gates = thresholds.get("gates", {})

    baseline_latency = float(baseline.get("latency_ms", 0))
    baseline_throughput = float(baseline.get("throughput_tokens_per_sec", 0))
    min_det_score = float(gates.get("min_determinism_score", 1.0))
    max_latency_regression_pct = float(gates.get("max_latency_regression_pct", 0))
    max_throughput_regression_pct = float(gates.get("max_throughput_regression_pct", 0))
    stable_band_pct = float(gates.get("stable_band_pct", 0))

    latency = observed["latency_ms"]
    throughput = observed["throughput_tokens_per_sec"]
    det_score = observed["determinism_score"]

    latency_limit = baseline_latency * (1.0 + max_latency_regression_pct / 100.0)
    throughput_floor = baseline_throughput * (1.0 - max_throughput_regression_pct / 100.0)

    latency_regressed = latency > latency_limit
    throughput_regressed = throughput < throughput_floor
    determinism_regressed = det_score < min_det_score

    def trend(value: float, baseline_value: float, lower_is_better: bool) -> str:
        if baseline_value <= 0:
            return "unknown"
        delta_pct = ((value - baseline_value) / baseline_value) * 100.0
        if abs(delta_pct) <= stable_band_pct:
            return "stable"
        if lower_is_better:
            return "improved" if delta_pct < 0 else "regressed"
        return "improved" if delta_pct > 0 else "regressed"

    evaluation = {
        "observed": observed,
        "baseline": {
            "latency_ms": baseline_latency,
            "throughput_tokens_per_sec": baseline_throughput,
        },
        "limits": {
            "latency_ms_max": latency_limit,
            "throughput_tokens_per_sec_min": throughput_floor,
            "determinism_score_min": min_det_score,
        },
        "regressions": {
            "latency": latency_regressed,
            "throughput": throughput_regressed,
            "determinism": determinism_regressed,
        },
        "trend": {
            "latency": trend(latency, baseline_latency, lower_is_better=True),
            "throughput": trend(throughput, baseline_throughput, lower_is_better=False),
            "determinism": "stable" if det_score >= min_det_score else "regressed",
        },
    }

    if latency_regressed:
        errs.append(f"latency regression: {latency} > {latency_limit}")
    if throughput_regressed:
        errs.append(f"throughput regression: {throughput} < {throughput_floor}")
    if determinism_regressed:
        errs.append(f"determinism regression: {det_score} < {min_det_score}")

    return len(errs) == 0, evaluation, errs


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    ai_bin = Path(args.ai_bin).resolve()
    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")

    repo_root = Path.cwd().resolve()
    model_path, model_origin = resolve_runtime_model(repo_root, out_dir, args.runtime_model)

    thresholds_path = (
        Path(args.thresholds_file).resolve()
        if args.thresholds_file
        else Path(__file__).resolve().with_name("ai_benchmark_thresholds.json")
    )
    if not thresholds_path.exists():
        raise SystemExit(f"benchmark thresholds file not found: {thresholds_path}")
    history_path = (
        Path(args.thresholds_history_file).resolve()
        if args.thresholds_history_file
        else Path(__file__).resolve().with_name("ai_benchmark_thresholds_history.json")
    )
    as_of = parse_iso_date(args.as_of_date, "as_of_date") if args.as_of_date else datetime.now(UTC).date()
    try:
        thresholds, threshold_source = select_threshold_window(
            thresholds_path=thresholds_path,
            history_path=history_path if history_path.exists() else None,
            as_of=as_of,
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

    run_artifacts: list[dict[str, Any]] = []
    payloads: list[dict[str, Any]] = []
    errors: list[str] = []

    for i in (1, 2):
        out_json = out_dir / f"ai_benchmark_run_{i}.json"
        res = run_cmd(
            [
                str(ai_bin),
                "benchmark",
                "run",
                "--model",
                "ci-benchmark-model",
                "--model-file",
                str(model_path),
                "--out",
                str(out_json),
            ]
        )
        run_artifacts.append(
            {
                "run": i,
                "rc": res["rc"],
                "stdout_sha256": res["stdout_sha256"],
                "stderr_sha256": res["stderr_sha256"],
                "artifact": str(out_json),
            }
        )
        if res["rc"] != 0:
            errors.append(f"benchmark run {i} failed")
            continue
        if not out_json.exists():
            errors.append(f"benchmark run {i} did not emit artifact")
            continue

        payload = parse_json(out_json)
        ok, val_errs = validate_benchmark_payload(payload, model_path)
        if not ok:
            errors.extend([f"run {i}: {e}" for e in val_errs])
        payloads.append(payload)

    deterministic_replay = False
    if len(payloads) == 2:
        deterministic_replay = canonical_json(payloads[0]) == canonical_json(payloads[1])
        if not deterministic_replay:
            errors.append("benchmark deterministic replay mismatch: run1 != run2")

    threshold_ok = False
    threshold_eval: dict[str, Any] = {}
    threshold_errors: list[str] = []
    observed = {
        "latency_ms": 0.0,
        "throughput_tokens_per_sec": 0.0,
        "determinism_score": 0.0,
    }
    if payloads:
        observed = {
            "latency_ms": float(payloads[0].get("latency_ms", 0.0)),
            "throughput_tokens_per_sec": float(payloads[0].get("throughput_tokens_per_sec", 0.0)),
            "determinism_score": float(payloads[0].get("determinism_score", 0.0)),
        }
        threshold_ok, threshold_eval, threshold_errors = evaluate_thresholds(observed, thresholds)
        errors.extend(threshold_errors)

    status = "pass" if deterministic_replay and threshold_ok and not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "model": str(model_path),
        "model_origin": model_origin,
        "thresholds_file": str(thresholds_path),
        "thresholds_sha256": sha256_file(thresholds_path),
        "threshold_source": threshold_source,
        "as_of_date": as_of.isoformat(),
        "deterministic_replay": deterministic_replay,
        "runtime_runs": run_artifacts,
        "observed": observed,
        "threshold_evaluation": threshold_eval,
        "errors": errors,
    }

    json_path = out_dir / "ai_benchmark_spec_contract.json"
    md_path = out_dir / "ai_benchmark_spec_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Benchmark Runtime Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- deterministic_replay: `{str(deterministic_replay).lower()}`",
                f"- latency_ms: `{observed['latency_ms']}`",
                f"- throughput_tokens_per_sec: `{observed['throughput_tokens_per_sec']}`",
                f"- determinism_score: `{observed['determinism_score']}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai benchmark spec status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
