#!/usr/bin/env python3
"""Validate approval metadata for benchmark threshold history windows."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from datetime import date
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.benchmark-threshold-approval.v1"


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
        raise ValueError(f"invalid {label}: {raw} (expected YYYY-MM-DD)") from exc


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate benchmark threshold history promotion approvals.")
    p.add_argument("--history-file", required=True, help="Path to ai_benchmark_thresholds_history.json")
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    history_path = Path(args.history_file).resolve()
    out_json = Path(args.out_json).resolve()

    errors: list[str] = []
    warnings: list[str] = []
    windows_report: list[dict[str, Any]] = []

    if not history_path.exists():
        print(f"error: missing history file: {history_path}", file=sys.stderr)
        return 1
    history = parse_json(history_path)
    if history.get("schema") != "t81.ai.benchmark-thresholds-history.v1":
        print(f"error: history schema mismatch: {history.get('schema')}", file=sys.stderr)
        return 1

    windows = history.get("windows")
    if not isinstance(windows, list) or not windows:
        print("error: history windows must be non-empty list", file=sys.stderr)
        return 1

    prev_start: date | None = None
    for idx, win in enumerate(windows, start=1):
        if not isinstance(win, dict):
            errors.append(f"window {idx}: entry is not an object")
            continue

        window_id = str(win.get("window_id", f"window-{idx}")).strip()
        start_raw = str(win.get("window_start", "")).strip()
        end_raw = str(win.get("window_end", "")).strip()
        thresholds = win.get("thresholds")
        approval = win.get("promotion_approval")
        approval_hash = str(win.get("promotion_approval_attestation_sha256", "")).strip()

        try:
            start = parse_iso_date(start_raw, "window_start")
            end = parse_iso_date(end_raw, "window_end")
            if start > end:
                errors.append(f"{window_id}: window_start > window_end")
            if prev_start is not None and start < prev_start:
                warnings.append(f"{window_id}: window ordering is not monotonic by start date")
            prev_start = start
        except ValueError as exc:
            errors.append(f"{window_id}: {exc}")

        if not isinstance(thresholds, dict):
            errors.append(f"{window_id}: thresholds must be object")
            thresholds = {}

        if not isinstance(approval, dict):
            errors.append(f"{window_id}: promotion_approval must be object")
            approval = {}
        approved_by = str(approval.get("approved_by", "")).strip()
        approved_at = str(approval.get("approved_at", "")).strip()
        change_ref = str(approval.get("change_ref", "")).strip()
        if not approved_by:
            errors.append(f"{window_id}: promotion_approval.approved_by required")
        if not approved_at:
            errors.append(f"{window_id}: promotion_approval.approved_at required")
        else:
            try:
                parse_iso_date(approved_at, "promotion_approval.approved_at")
            except ValueError as exc:
                errors.append(f"{window_id}: {exc}")
        if not change_ref:
            errors.append(f"{window_id}: promotion_approval.change_ref required")

        attestation_core = {
            "window_id": window_id,
            "window_start": start_raw,
            "window_end": end_raw,
            "thresholds": thresholds,
            "promotion_approval": {
                "approved_by": approved_by,
                "approved_at": approved_at,
                "change_ref": change_ref,
            },
        }
        expected_hash = sha256_text(canonical_json(attestation_core))
        if not approval_hash:
            errors.append(f"{window_id}: promotion_approval_attestation_sha256 required")
        elif approval_hash != expected_hash:
            errors.append(f"{window_id}: promotion_approval_attestation_sha256 mismatch")

        windows_report.append(
            {
                "window_id": window_id,
                "window_start": start_raw,
                "window_end": end_raw,
                "approval_attestation_valid": approval_hash == expected_hash and bool(approval_hash),
            }
        )

    status = "pass" if not errors else "fail"
    report = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "history_file": str(history_path),
        "window_count": len(windows),
        "windows": windows_report,
        "errors": errors,
        "warnings": warnings,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    print(f"ai benchmark threshold approvals status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
