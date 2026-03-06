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


SCHEMA_VERSION = "t81.ai.quantization-codec.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_iso_date(raw: str, label: str) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"invalid {label} date: {raw} (expected YYYY-MM-DD)") from exc


def select_codec_profile_window(
    profile_path: Path,
    history_path: Path | None,
    as_of: date,
) -> tuple[dict[str, Any], dict[str, Any]]:
    profile = parse_json(profile_path)
    metadata = {
        "source": str(profile_path),
        "history_file": str(history_path) if history_path else "",
        "as_of_date": as_of.isoformat(),
        "window_id": "",
        "window_start": "",
        "window_end": "",
    }
    if history_path is None or not history_path.exists():
        return profile, metadata

    history = parse_json(history_path)
    if history.get("schema") != "t81.ai.quantization-codec-profile-history.v1":
        raise ValueError(f"codec profile history schema mismatch: {history.get('schema')}")
    windows = history.get("windows")
    if not isinstance(windows, list) or not windows:
        raise ValueError("codec profile history windows must be non-empty list")

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
        raise ValueError("no valid codec profile window found in history")

    selected_profile = selected.get("profile")
    if not isinstance(selected_profile, dict):
        raise ValueError("selected codec profile window missing profile object")
    metadata.update(
        {
            "source": str(history_path),
            "window_id": str(selected.get("window_id", "")),
            "window_start": str(selected.get("window_start", "")),
            "window_end": str(selected.get("window_end", "")),
        }
    )
    return selected_profile, metadata


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

    fallback = out_dir / "quantization_test_model.gguf"
    fallback.write_text("t81-ai-quantization-fixture\n", encoding="utf-8")
    return fallback, "generated"


def encode_trits(trits: list[int]) -> str:
    # Deterministic 2-bit packing: -1->0, 0->1, 1->2 (3 reserved)
    mapping = {-1: 0, 0: 1, 1: 2}
    out = bytearray()
    for i in range(0, len(trits), 4):
        chunk = trits[i : i + 4]
        byte = 0
        for j, t in enumerate(chunk):
            byte |= (mapping[t] & 0x3) << (j * 2)
        out.append(byte)
    return out.hex()


def decode_trits(encoded_hex: str, expected_len: int) -> list[int]:
    rev = {0: -1, 1: 0, 2: 1}
    data = bytes.fromhex(encoded_hex)
    out: list[int] = []
    for b in data:
        for j in range(4):
            v = (b >> (j * 2)) & 0x3
            out.append(rev.get(v, 0))
            if len(out) == expected_len:
                return out
    return out[:expected_len]


def build_fixture_corpus() -> list[dict[str, Any]]:
    vectors = [
        {"id": "fx01", "trits": [-1, 0, 1, 0, -1, 1, 0, 0]},
        {"id": "fx02", "trits": [1, 1, 0, -1, -1, 0, 1, 1, 0, 0, -1, 1]},
        {"id": "fx03", "trits": [0, 0, 0, 0, 1, -1, 1, -1, 1, 0, -1, 0, 1]},
    ]
    corpus: list[dict[str, Any]] = []
    for vec in vectors:
        trits = vec["trits"]
        encoded = encode_trits(trits)
        decoded = decode_trits(encoded, len(trits))
        corpus.append(
            {
                "id": vec["id"],
                "trits": trits,
                "encoded_hex": encoded,
                "decoded_trits": decoded,
                "roundtrip_ok": decoded == trits,
                "encoded_sha256": sha256_text(encoded),
            }
        )
    return corpus


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate quantization codec fixture corpus + runtime pipeline (RFC-00A4).")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--ai-bin", required=True)
    p.add_argument("--runtime-model", default="")
    p.add_argument(
        "--codec-profile-file",
        default="",
        help="Path to quantization codec profile JSON (defaults to scripts/ci/ai_quantization_codec_profile.json).",
    )
    p.add_argument(
        "--codec-profile-history-file",
        default="",
        help="Optional codec profile history file for active window selection.",
    )
    p.add_argument(
        "--as-of-date",
        default="",
        help="Optional YYYY-MM-DD date to select active codec profile window (defaults to today UTC).",
    )
    p.add_argument(
        "--trend-window-count",
        type=int,
        default=3,
        help="Number of governed history windows to include in rolling profile trend analysis (default: 3).",
    )
    return p.parse_args()


def compute_rolling_profile_trend_analysis(
    observed: dict[str, Any],
    history_path: Path | None,
    as_of: date,
    window_count: int,
) -> dict[str, Any]:
    analysis: dict[str, Any] = {
        "enabled": False,
        "window_count": max(1, window_count),
        "windows": [],
        "summary": {
            "codec_prefix_match": "unknown",
            "profile_prefix_match": "unknown",
            "bits_range_match": "unknown",
            "profile_policy_drift": "unknown",
        },
    }
    if history_path is None or not history_path.exists():
        return analysis

    history = parse_json(history_path)
    if history.get("schema") != "t81.ai.quantization-codec-profile-history.v1":
        return analysis
    windows = history.get("windows")
    if not isinstance(windows, list):
        return analysis

    eligible: list[tuple[date, dict[str, Any]]] = []
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
            eligible.append((start, win))

    if not eligible:
        return analysis
    eligible.sort(key=lambda item: item[0], reverse=True)
    selected = [win for _, win in eligible[: max(1, window_count)]]

    codec_prefix_all = True
    profile_prefix_all = True
    bits_range_all = True
    profile_hashes: list[str] = []
    observed_codec = str(observed.get("codec", ""))
    observed_profile = str(observed.get("quantization_profile", ""))
    observed_bits = int(observed.get("bits_per_weight", 0))

    for win in selected:
        profile = win.get("profile")
        if not isinstance(profile, dict):
            continue
        codec_prefix = str(profile.get("codec_prefix", ""))
        profile_prefix = str(profile.get("quantization_profile_prefix", ""))
        min_bits = int(profile.get("min_bits_per_weight", 0))
        max_bits = int(profile.get("max_bits_per_weight", 0))

        codec_match = observed_codec.startswith(codec_prefix) if codec_prefix else False
        profile_match = observed_profile.startswith(profile_prefix) if profile_prefix else False
        bits_match = min_bits <= observed_bits <= max_bits if max_bits >= min_bits else False

        codec_prefix_all = codec_prefix_all and codec_match
        profile_prefix_all = profile_prefix_all and profile_match
        bits_range_all = bits_range_all and bits_match
        profile_hashes.append(sha256_text(canonical_json(profile)))

        analysis["windows"].append(
            {
                "window_id": str(win.get("window_id", "")),
                "window_start": str(win.get("window_start", "")),
                "window_end": str(win.get("window_end", "")),
                "expected": {
                    "codec_prefix": codec_prefix,
                    "quantization_profile_prefix": profile_prefix,
                    "min_bits_per_weight": min_bits,
                    "max_bits_per_weight": max_bits,
                },
                "observed": {
                    "codec": observed_codec,
                    "quantization_profile": observed_profile,
                    "bits_per_weight": observed_bits,
                },
                "match": {
                    "codec_prefix": codec_match,
                    "profile_prefix": profile_match,
                    "bits_range": bits_match,
                },
            }
        )

    drift = "unknown"
    if profile_hashes:
        drift = "stable" if len(set(profile_hashes)) == 1 else "drifted"

    analysis["enabled"] = True
    analysis["summary"] = {
        "codec_prefix_match": "pass" if codec_prefix_all else "mismatch",
        "profile_prefix_match": "pass" if profile_prefix_all else "mismatch",
        "bits_range_match": "pass" if bits_range_all else "mismatch",
        "profile_policy_drift": drift,
    }
    return analysis


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    ai_bin = Path(args.ai_bin).resolve()
    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")

    repo_root = Path.cwd().resolve()
    model_path, model_origin = resolve_runtime_model(repo_root, out_dir, args.runtime_model)
    profile_path = (
        Path(args.codec_profile_file).resolve()
        if args.codec_profile_file
        else Path(__file__).resolve().with_name("ai_quantization_codec_profile.json")
    )
    if not profile_path.exists():
        raise SystemExit(f"quantization codec profile file not found: {profile_path}")
    history_path = (
        Path(args.codec_profile_history_file).resolve()
        if args.codec_profile_history_file
        else Path(__file__).resolve().with_name("ai_quantization_codec_profile_history.json")
    )
    as_of = parse_iso_date(args.as_of_date, "as_of_date") if args.as_of_date else datetime.now(UTC).date()
    try:
        codec_profile, codec_profile_source = select_codec_profile_window(
            profile_path=profile_path,
            history_path=history_path if history_path.exists() else None,
            as_of=as_of,
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

    errors: list[str] = []

    quant1 = out_dir / "ai_quantization_inspect_run1.json"
    quant2 = out_dir / "ai_quantization_inspect_run2.json"

    run1 = run_cmd(
        [
            str(ai_bin),
            "quantization",
            "inspect",
            "--model",
            "ci-quant-model",
            "--model-file",
            str(model_path),
            "--out",
            str(quant1),
        ]
    )
    run2 = run_cmd(
        [
            str(ai_bin),
            "quantization",
            "inspect",
            "--model",
            "ci-quant-model",
            "--model-file",
            str(model_path),
            "--out",
            str(quant2),
        ]
    )

    if run1["rc"] != 0:
        errors.append("runtime quantization inspect run1 failed")
    if run2["rc"] != 0:
        errors.append("runtime quantization inspect run2 failed")
    if not quant1.exists():
        errors.append("runtime quantization inspect run1 did not emit artifact")
    if not quant2.exists():
        errors.append("runtime quantization inspect run2 did not emit artifact")

    runtime_payload_1: dict[str, Any] = {}
    runtime_payload_2: dict[str, Any] = {}
    if quant1.exists():
        runtime_payload_1 = parse_json(quant1)
    if quant2.exists():
        runtime_payload_2 = parse_json(quant2)

    required_fields = {
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
    if runtime_payload_1:
        missing = sorted(required_fields - set(runtime_payload_1.keys()))
        if missing:
            errors.append("runtime quantization payload missing fields: " + ", ".join(missing))
        if runtime_payload_1.get("schema") != "t81.ai.quantization-inspect.v1":
            errors.append("runtime quantization schema mismatch")
        if Path(str(runtime_payload_1.get("model_file", ""))).resolve() != model_path.resolve():
            errors.append("runtime quantization model_file mismatch")
        expected_codec_prefix = str(codec_profile.get("codec_prefix", "T3_K"))
        expected_profile_prefix = str(codec_profile.get("quantization_profile_prefix", "runtime-"))
        min_bits = int(codec_profile.get("min_bits_per_weight", 1))
        max_bits = int(codec_profile.get("max_bits_per_weight", 16))
        if not str(runtime_payload_1.get("codec", "")).startswith(expected_codec_prefix):
            errors.append(f"runtime quantization codec must start with {expected_codec_prefix}")
        bits = int(runtime_payload_1.get("bits_per_weight", 0))
        if bits < min_bits or bits > max_bits:
            errors.append(f"runtime quantization bits_per_weight must be in [{min_bits},{max_bits}]")
        if not str(runtime_payload_1.get("quantization_profile", "")).startswith(expected_profile_prefix):
            errors.append(f"runtime quantization profile must start with {expected_profile_prefix}")
        if runtime_payload_1.get("status") != "pass":
            errors.append("runtime quantization status must be pass")

    rolling_trend_analysis: dict[str, Any] = {}
    if runtime_payload_1:
        rolling_trend_analysis = compute_rolling_profile_trend_analysis(
            observed={
                "codec": str(runtime_payload_1.get("codec", "")),
                "quantization_profile": str(runtime_payload_1.get("quantization_profile", "")),
                "bits_per_weight": int(runtime_payload_1.get("bits_per_weight", 0)),
            },
            history_path=history_path if history_path.exists() else None,
            as_of=as_of,
            window_count=max(1, int(args.trend_window_count)),
        )

    deterministic_runtime = False
    if runtime_payload_1 and runtime_payload_2:
        deterministic_runtime = canonical_json(runtime_payload_1) == canonical_json(runtime_payload_2)
        if not deterministic_runtime:
            errors.append("runtime quantization inspect replay mismatch between run1/run2")

    corpus = build_fixture_corpus()
    corpus_ok = all(item["roundtrip_ok"] for item in corpus)
    if not corpus_ok:
        errors.append("codec fixture corpus roundtrip failed")

    status = "pass" if not errors and deterministic_runtime and corpus_ok else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "runtime_model": str(model_path),
        "runtime_model_origin": model_origin,
        "codec_profile_file": str(profile_path),
        "codec_profile_sha256": sha256_text(canonical_json(codec_profile)),
        "codec_profile_source": codec_profile_source,
        "as_of_date": as_of.isoformat(),
        "codec_profile": codec_profile,
        "runtime_quantization_deterministic_replay": deterministic_runtime,
        "runtime_runs": [
            {
                "run": 1,
                "rc": run1["rc"],
                "stdout_sha256": run1["stdout_sha256"],
                "stderr_sha256": run1["stderr_sha256"],
                "artifact": str(quant1),
            },
            {
                "run": 2,
                "rc": run2["rc"],
                "stdout_sha256": run2["stdout_sha256"],
                "stderr_sha256": run2["stderr_sha256"],
                "artifact": str(quant2),
            },
        ],
        "runtime_quantization_payload_sha256": sha256_text(canonical_json(runtime_payload_1))
        if runtime_payload_1
        else "",
        "rolling_trend_analysis": rolling_trend_analysis,
        "fixture_corpus_roundtrip_ok": corpus_ok,
        "fixture_corpus": corpus,
        "errors": errors,
    }

    json_path = out_dir / "ai_quantization_codec_contract.json"
    md_path = out_dir / "ai_quantization_codec_contract.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Quantization Codec Contract",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- runtime_quantization_deterministic_replay: `{str(deterministic_runtime).lower()}`",
                f"- rolling_profile_policy_drift: `{rolling_trend_analysis.get('summary', {}).get('profile_policy_drift', 'unknown')}`",
                f"- fixture_corpus_roundtrip_ok: `{str(corpus_ok).lower()}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai quantization codec status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
