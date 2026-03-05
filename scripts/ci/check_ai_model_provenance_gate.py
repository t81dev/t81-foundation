#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from dataclasses import dataclass
from pathlib import Path


SCHEMA_VERSION = "t81.ai.model-provenance.v1"


@dataclass
class GateResult:
    ok: bool
    reason: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def write_manifest(path: Path, model_path: Path, model_hash: str) -> None:
    payload = {
        "schema": SCHEMA_VERSION,
        "model_path": str(model_path),
        "model_hash": f"sha256:{model_hash}",
    }
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def gate_model_load(model_path: Path, manifest_path: Path, expected_hash: str | None = None) -> GateResult:
    if not model_path.exists():
        return GateResult(False, f"missing model file: {model_path}")
    if not manifest_path.exists():
        return GateResult(False, f"missing manifest: {manifest_path}")

    manifest = load_manifest(manifest_path)
    manifest_hash = str(manifest.get("model_hash", "")).strip()
    if not manifest_hash.startswith("sha256:"):
        return GateResult(False, "manifest model_hash missing sha256: prefix")

    actual_hash = sha256_file(model_path)
    expected = expected_hash.strip() if expected_hash else manifest_hash
    if not expected.startswith("sha256:"):
        expected = f"sha256:{expected}"

    if manifest_hash != f"sha256:{actual_hash}":
        return GateResult(False, "manifest hash does not match model file")
    if expected != f"sha256:{actual_hash}":
        return GateResult(False, "provided expected hash does not match model file")
    return GateResult(True, "provenance gate passed")


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Check AI model provenance hash gate behavior.")
    p.add_argument("--model", required=True, help="Model file path")
    p.add_argument("--manifest", required=True, help="Manifest path")
    p.add_argument(
        "--expected-hash",
        default="",
        help="Optional expected hash (sha256:<hex> or <hex>). If omitted, manifest hash is authoritative.",
    )
    p.add_argument(
        "--self-test-deny",
        action="store_true",
        help="Also run a negative test with a bad hash and require deterministic denial.",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    model_path = Path(args.model).resolve()
    manifest_path = Path(args.manifest).resolve()

    if not model_path.exists():
        model_path.parent.mkdir(parents=True, exist_ok=True)
        model_path.write_text("t81-model-provenance-fixture\n", encoding="utf-8")

    computed = sha256_file(model_path)
    if not manifest_path.exists():
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        write_manifest(manifest_path, model_path, computed)

    positive = gate_model_load(model_path, manifest_path, args.expected_hash if args.expected_hash else None)
    if not positive.ok:
        print(f"[FAIL] positive gate check: {positive.reason}")
        return 1
    print(f"[PASS] positive gate check: {positive.reason}")

    if args.self_test_deny:
        negative = gate_model_load(model_path, manifest_path, "sha256:" + ("0" * 64))
        if negative.ok:
            print("[FAIL] negative gate check: expected denial but gate passed")
            return 1
        print(f"[PASS] negative gate check: denied as expected ({negative.reason})")

    print("model provenance gate: pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
