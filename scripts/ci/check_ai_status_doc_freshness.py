#!/usr/bin/env python3
"""Ensure AI status docs are updated within governed freshness windows."""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import UTC, date, datetime
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.status-doc-freshness.v1"
EXPECTATION_SCHEMA_VERSION = "t81.ai.status-doc-freshness-expectations.v1"


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_iso_date(raw: str, label: str) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"invalid {label}: {raw} (expected YYYY-MM-DD)") from exc


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate freshness of AI status documentation.")
    p.add_argument(
        "--expectations-file",
        default="",
        help="Path to status-doc freshness expectation contract JSON.",
    )
    p.add_argument(
        "--as-of-date",
        default="",
        help="Optional YYYY-MM-DD date override (default: today UTC).",
    )
    p.add_argument("--out-json", required=True, help="Output report path")
    return p.parse_args()


def parse_last_updated(content: str, date_label: str) -> str:
    pattern = re.compile(
        rf"(?m)^\s*{re.escape(date_label)}\s*(\d{{4}}-\d{{2}}-\d{{2}})\s*$"
    )
    match = pattern.search(content)
    return match.group(1) if match else ""


def main() -> int:
    args = parse_args()
    expectations_path = (
        Path(args.expectations_file).resolve()
        if args.expectations_file
        else Path(__file__).resolve().with_name("ai_status_doc_freshness_expectations.json")
    )
    out_json = Path(args.out_json).resolve()
    out_md = out_json.with_suffix(".md")
    repo_root = Path(__file__).resolve().parents[2]

    errors: list[str] = []
    warnings: list[str] = []
    rows: list[dict[str, Any]] = []

    if not expectations_path.exists():
        print(f"error: missing expectations file: {expectations_path}", file=sys.stderr)
        return 1
    expectations = parse_json(expectations_path)
    if expectations.get("schema") != EXPECTATION_SCHEMA_VERSION:
        print(
            "error: expectations schema mismatch: "
            f"{expectations.get('schema')} != {EXPECTATION_SCHEMA_VERSION}",
            file=sys.stderr,
        )
        return 1

    as_of = parse_iso_date(args.as_of_date, "as_of_date") if args.as_of_date else datetime.now(UTC).date()
    docs = expectations.get("documents")
    if not isinstance(docs, list) or not docs:
        print("error: expectations.documents must be a non-empty list", file=sys.stderr)
        return 1

    for idx, item in enumerate(docs, start=1):
        if not isinstance(item, dict):
            errors.append(f"document[{idx}] must be an object")
            continue
        rel_path = str(item.get("path", "")).strip()
        if not rel_path:
            errors.append(f"document[{idx}] missing path")
            continue
        max_age_days_raw = item.get("max_age_days", 0)
        try:
            max_age_days = int(max_age_days_raw)
        except Exception:
            errors.append(f"{rel_path}: max_age_days must be integer")
            continue
        if max_age_days < 0:
            errors.append(f"{rel_path}: max_age_days must be >= 0")
            continue
        warn_age_days_raw = item.get("warn_age_days", max_age_days)
        try:
            warn_age_days = int(warn_age_days_raw)
        except Exception:
            errors.append(f"{rel_path}: warn_age_days must be integer")
            continue
        if warn_age_days < 0:
            errors.append(f"{rel_path}: warn_age_days must be >= 0")
            continue
        if warn_age_days > max_age_days:
            errors.append(f"{rel_path}: warn_age_days must be <= max_age_days")
            continue
        date_label = str(item.get("date_label", "Last Updated:")).strip() or "Last Updated:"
        required_substrings_raw = item.get("required_substrings", [])
        required_substrings = (
            [str(v) for v in required_substrings_raw if str(v)]
            if isinstance(required_substrings_raw, list)
            else []
        )

        doc_path = (repo_root / rel_path).resolve()
        row: dict[str, Any] = {
            "path": rel_path,
            "exists": doc_path.exists(),
            "max_age_days": max_age_days,
            "warn_age_days": warn_age_days,
            "as_of_date": as_of.isoformat(),
            "status": "pass",
            "last_updated": "",
            "age_days": None,
            "near_stale": False,
            "missing_required_substrings": [],
        }
        if not doc_path.exists():
            row["status"] = "fail"
            errors.append(f"{rel_path}: file missing")
            rows.append(row)
            continue

        content = doc_path.read_text(encoding="utf-8")
        last_updated_raw = parse_last_updated(content, date_label)
        if not last_updated_raw:
            row["status"] = "fail"
            errors.append(f"{rel_path}: missing '{date_label} YYYY-MM-DD' line")
            rows.append(row)
            continue
        row["last_updated"] = last_updated_raw
        try:
            updated = parse_iso_date(last_updated_raw, f"{rel_path} last_updated")
        except ValueError as exc:
            row["status"] = "fail"
            errors.append(str(exc))
            rows.append(row)
            continue

        age_days = (as_of - updated).days
        row["age_days"] = age_days
        if age_days < 0:
            row["status"] = "fail"
            errors.append(f"{rel_path}: last_updated {updated.isoformat()} is in the future of as_of_date {as_of}")
        elif age_days > max_age_days:
            row["status"] = "fail"
            errors.append(
                f"{rel_path}: stale status doc (age_days={age_days}, max_age_days={max_age_days})"
            )
        elif age_days >= warn_age_days:
            row["near_stale"] = True
            warnings.append(
                f"{rel_path}: approaching staleness (age_days={age_days}, "
                f"warn_age_days={warn_age_days}, max_age_days={max_age_days})"
            )

        missing = [s for s in required_substrings if s not in content]
        row["missing_required_substrings"] = missing
        if missing:
            row["status"] = "fail"
            errors.append(f"{rel_path}: missing required substrings: {', '.join(missing)}")

        rows.append(row)

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "expectations_file": str(expectations_path),
        "as_of_date": as_of.isoformat(),
        "documents": rows,
        "errors": errors,
        "warnings": warnings,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# AI Status Doc Freshness",
        "",
        f"- schema: `{SCHEMA_VERSION}`",
        f"- status: `{status}`",
        f"- as_of_date: `{as_of.isoformat()}`",
        "",
        "| Document | Last Updated | Age (days) | Max Age | Status |",
        "| :--- | :--- | ---: | ---: | :--- |",
    ]
    for row in rows:
        lines.append(
            "| {path} | {last_updated} | {age_days} | {max_age} | {status} |".format(
                path=row.get("path", ""),
                last_updated=row.get("last_updated", ""),
                age_days=row.get("age_days", ""),
                max_age=row.get("max_age_days", ""),
                status=row.get("status", ""),
            )
        )
    near_stale_rows = [row for row in rows if bool(row.get("near_stale", False))]
    if near_stale_rows:
        lines.extend(["", "Warnings:"])
        for row in near_stale_rows:
            lines.append(
                "- {path}: age_days={age_days}, warn_age_days={warn_age_days}, max_age_days={max_age_days}".format(
                    path=row.get("path", ""),
                    age_days=row.get("age_days", ""),
                    warn_age_days=row.get("warn_age_days", ""),
                    max_age_days=row.get("max_age_days", ""),
                )
            )
    if errors:
        lines.extend(["", "Errors:"])
        for err in errors:
            lines.append(f"- {err}")
    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai status-doc freshness status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
