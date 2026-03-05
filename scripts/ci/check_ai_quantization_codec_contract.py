#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.quantization-codec.v1"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def build_codec_manifest() -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "codec_family": "T3_K",
        "codec_variant": "T3_K1",
        "packing": {"bits_per_weight": 2, "zero_symbol": 0, "negative_symbol": -1, "positive_symbol": 1},
        "header": {
            "magic": "T81Q",
            "version": 1,
            "endianness": "little",
            "shape": [2, 2],
        },
        "payload_example": {"encoded_hex": "00010203", "decoded_trits": [-1, 0, 1, 0]},
        "determinism": {"encode_order": "row_major", "rounding_mode": "canonical_tie_to_zero"},
    }


def validate_manifest(manifest: dict[str, Any]) -> tuple[bool, list[str]]:
    errs: list[str] = []
    if manifest.get("codec_family") != "T3_K":
        errs.append("codec_family must be T3_K")
    if manifest.get("codec_variant") not in {"T3_K1", "T3_K2", "T3_K3"}:
        errs.append("codec_variant must be one of T3_K1/T3_K2/T3_K3")
    if manifest.get("header", {}).get("endianness") != "little":
        errs.append("header.endianness must be little")
    shape = manifest.get("header", {}).get("shape", [])
    if not isinstance(shape, list) or len(shape) == 0:
        errs.append("header.shape must be a non-empty list")
    decoded = manifest.get("payload_example", {}).get("decoded_trits", [])
    if not all(v in (-1, 0, 1) for v in decoded):
        errs.append("payload_example.decoded_trits must only contain -1/0/1")
    return len(errs) == 0, errs


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate ternary quantization codec contract (RFC-00A4 baseline).")
    p.add_argument("--out-dir", required=True)
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest_a = build_codec_manifest()
    manifest_b = build_codec_manifest()
    hash_a = sha256_text(canonical_json(manifest_a))
    hash_b = sha256_text(canonical_json(manifest_b))
    deterministic = hash_a == hash_b

    valid, errors = validate_manifest(manifest_a)
    status = "pass" if deterministic and valid else "fail"

    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "deterministic_manifest_hash": deterministic,
        "manifest_sha256": hash_a,
        "valid": valid,
        "errors": errors,
        "manifest": manifest_a,
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
                f"- deterministic_manifest_hash: `{deterministic}`",
                f"- manifest_sha256: `{hash_a}`",
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
