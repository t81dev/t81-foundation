#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.quantization-codec.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


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
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    ai_bin = Path(args.ai_bin).resolve()
    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")

    repo_root = Path.cwd().resolve()
    model_path, model_origin = resolve_runtime_model(repo_root, out_dir, args.runtime_model)

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
        if not str(runtime_payload_1.get("codec", "")).startswith("T3_K"):
            errors.append("runtime quantization codec must be T3_K*")
        if int(runtime_payload_1.get("bits_per_weight", 0)) <= 0:
            errors.append("runtime quantization bits_per_weight must be > 0")
        if not str(runtime_payload_1.get("quantization_profile", "")).startswith("runtime-"):
            errors.append("runtime quantization profile must be runtime-bound")
        if runtime_payload_1.get("status") != "pass":
            errors.append("runtime quantization status must be pass")

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
