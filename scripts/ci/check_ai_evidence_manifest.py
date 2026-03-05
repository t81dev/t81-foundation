#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from datetime import date, datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.evidence-manifest.v1"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def lane_entry(name: str, path: Path) -> dict[str, Any]:
    payload = parse_json(path)
    status = str(payload.get("status", "")).strip()
    if not status and isinstance(payload.get("checks"), dict):
        status = str(payload["checks"].get("status", "")).strip()
    return {
        "lane": name,
        "path": str(path),
        "status": status,
        "sha256": sha256_file(path),
    }


def main() -> int:
    p = argparse.ArgumentParser(description="Build RFC-00A1 signed multi-lane evidence manifest.")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--evidence-bundle", required=True)
    p.add_argument("--vm-trace", required=True)
    p.add_argument("--cross-lane", required=True)
    p.add_argument("--backend-contract", required=True)
    p.add_argument("--ux-contract", required=True)
    p.add_argument("--tloadhash-toolchain", required=True)
    p.add_argument("--governed-flow", default="")
    p.add_argument("--promotion-window-start", required=True, help="YYYY-MM-DD")
    p.add_argument("--promotion-window-end", required=True, help="YYYY-MM-DD")
    args = p.parse_args()

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    start = date.fromisoformat(args.promotion_window_start)
    end = date.fromisoformat(args.promotion_window_end)
    window_valid = start <= end

    lanes: list[dict[str, Any]] = []
    lanes.append(lane_entry("evidence_bundle", Path(args.evidence_bundle).resolve()))
    lanes.append(lane_entry("vm_trace", Path(args.vm_trace).resolve()))
    lanes.append(lane_entry("cross_lane", Path(args.cross_lane).resolve()))
    lanes.append(lane_entry("backend_contract", Path(args.backend_contract).resolve()))
    lanes.append(lane_entry("ux_contract", Path(args.ux_contract).resolve()))
    lanes.append(lane_entry("tloadhash_toolchain", Path(args.tloadhash_toolchain).resolve()))
    if args.governed_flow:
        governed_path = Path(args.governed_flow).resolve()
        if governed_path.exists():
            lanes.append(lane_entry("governed_flow", governed_path))

    errors: list[str] = []
    for lane in lanes:
        if lane["status"] != "pass":
            errors.append(f"{lane['lane']}: status={lane['status']} (expected pass)")
    if not window_valid:
        errors.append("promotion window is invalid: start must be <= end")

    manifest_core = {
        "schema": SCHEMA_VERSION,
        "promotion_window": {
            "start": args.promotion_window_start,
            "end": args.promotion_window_end,
            "valid": window_valid,
        },
        "lanes": lanes,
        "attestor": "ai-experiments-ci",
        "attestation_method": "deterministic-sha256-attestation",
    }
    attestation_sha256 = sha256_bytes(canonical_json(manifest_core).encode("utf-8"))
    status = "pass" if not errors else "fail"
    manifest = {
        **manifest_core,
        "status": status,
        "errors": errors,
        "attestation_sha256": attestation_sha256,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    }

    json_path = out_dir / "ai_evidence_manifest.json"
    md_path = out_dir / "ai_evidence_manifest.md"
    json_path.write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")
    md_path.write_text(
        "\n".join(
            [
                "# AI Evidence Manifest",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                f"- status: `{status}`",
                f"- promotion_window: `{args.promotion_window_start}..{args.promotion_window_end}`",
                f"- attestation_sha256: `{attestation_sha256}`",
                f"- lane_count: `{len(lanes)}`",
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"ai evidence manifest status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
