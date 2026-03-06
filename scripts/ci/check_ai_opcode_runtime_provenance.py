#!/usr/bin/env python3
"""Validate opcode runtime report provenance bindings for baseline-history windows."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.opcode-runtime-provenance-check.v1"
EXPECTATION_SCHEMA_VERSION = "t81.ai.opcode-baseline-provenance-expectations.v1"


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_safe_relative_path(value: str) -> bool:
    path = Path(value)
    if path.is_absolute():
        return False
    return ".." not in path.parts


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate opcode runtime report provenance bindings.")
    p.add_argument("--runtime-report", required=True, help="Path to ai_opcode_runtime_report.json")
    p.add_argument(
        "--provenance-expectations",
        default="",
        help="Path to opcode baseline provenance expectation contract (defaults to scripts/ci/ai_opcode_baseline_provenance_expectations.json).",
    )
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    runtime_report_path = Path(args.runtime_report).resolve()
    expectations_path = (
        Path(args.provenance_expectations).resolve()
        if args.provenance_expectations
        else Path(__file__).resolve().with_name("ai_opcode_baseline_provenance_expectations.json")
    )
    out_json = Path(args.out_json).resolve()
    repo_root = Path(__file__).resolve().parents[2]

    errors: list[str] = []
    warnings: list[str] = []
    required_fields: list[str] = []
    path_safety_fields: list[str] = []
    must_exist_in_repo_fields: list[str] = []
    allowed_prefix_by_field: dict[str, list[str]] = {}

    if not runtime_report_path.exists():
        errors.append(f"missing runtime report: {runtime_report_path}")
    if not expectations_path.exists():
        errors.append(f"missing provenance expectations file: {expectations_path}")
    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    runtime_report = parse_json(runtime_report_path)
    expectations = parse_json(expectations_path)

    if expectations.get("schema") != EXPECTATION_SCHEMA_VERSION:
        errors.append(
            "provenance expectations schema mismatch: "
            f"{expectations.get('schema')} != {EXPECTATION_SCHEMA_VERSION}"
        )
    else:
        raw_required = expectations.get("required_provenance_fields")
        if isinstance(raw_required, list):
            required_fields = [str(item).strip() for item in raw_required if str(item).strip()]
        raw_path = expectations.get("path_safety_fields")
        if isinstance(raw_path, list):
            path_safety_fields = [str(item).strip() for item in raw_path if str(item).strip()]
        raw_exist = expectations.get("must_exist_in_repo_fields")
        if isinstance(raw_exist, list):
            must_exist_in_repo_fields = [str(item).strip() for item in raw_exist if str(item).strip()]
        raw_prefix = expectations.get("allowed_prefix_by_field")
        if isinstance(raw_prefix, dict):
            for key, value in raw_prefix.items():
                k = str(key).strip()
                if not k or not isinstance(value, list):
                    continue
                clean = [str(item).strip() for item in value if str(item).strip()]
                if clean:
                    allowed_prefix_by_field[k] = clean

    evidence = runtime_report.get("evidence")
    if not isinstance(evidence, dict):
        errors.append("runtime report missing evidence object")
        evidence = {}
    selection = evidence.get("phase1_baseline_selection")
    if not isinstance(selection, dict):
        errors.append("runtime report missing evidence.phase1_baseline_selection object")
        selection = {}
    selection_provenance = selection.get("provenance")
    if not isinstance(selection_provenance, dict):
        errors.append("runtime report missing evidence.phase1_baseline_selection.provenance object")
        selection_provenance = {}
    vector_provenance = evidence.get("phase1_vector_provenance")
    if not isinstance(vector_provenance, dict):
        errors.append("runtime report missing evidence.phase1_vector_provenance object")
        vector_provenance = {}

    if selection_provenance != vector_provenance:
        errors.append(
            "runtime report provenance mismatch: "
            "phase1_baseline_selection.provenance != phase1_vector_provenance"
        )

    missing_fields: list[str] = []
    invalid_fields: list[str] = []
    for field in required_fields:
        value = str(selection_provenance.get(field, "")).strip()
        if not value:
            missing_fields.append(field)
            errors.append(f"missing required provenance field: {field}")
            continue
        if field in path_safety_fields and not validate_safe_relative_path(value):
            invalid_fields.append(field)
            errors.append(f"invalid provenance path for {field}: must be safe relative path")
        prefixes = allowed_prefix_by_field.get(field, [])
        if prefixes and not any(value.startswith(prefix) for prefix in prefixes):
            invalid_fields.append(field)
            errors.append(
                f"invalid provenance prefix for {field}: expected one of {', '.join(prefixes)}"
            )
        if field in must_exist_in_repo_fields and not (repo_root / value).exists():
            invalid_fields.append(field)
            errors.append(f"provenance path does not exist for {field}: {(repo_root / value)}")

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "runtime_report": str(runtime_report_path),
        "provenance_expectations_file": str(expectations_path),
        "required_provenance_fields": required_fields,
        "path_safety_fields": path_safety_fields,
        "must_exist_in_repo_fields": must_exist_in_repo_fields,
        "allowed_prefix_by_field": allowed_prefix_by_field,
        "baseline_selection_window": {
            "window_id": str(selection.get("window_id", "")).strip(),
            "window_start": str(selection.get("window_start", "")).strip(),
            "window_end": str(selection.get("window_end", "")).strip(),
            "as_of_date": str(selection.get("as_of_date", "")).strip(),
            "history_path": str(selection.get("history_path", "")).strip(),
        },
        "provenance": selection_provenance,
        "selection_vector_provenance_match": selection_provenance == vector_provenance,
        "missing_provenance_fields": missing_fields,
        "invalid_provenance_fields": sorted(set(invalid_fields)),
        "errors": errors,
        "warnings": warnings,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    print(f"ai opcode runtime provenance status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
