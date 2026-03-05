#!/usr/bin/env python3
"""Validate AI signing keyring expiry windows for CI alerting."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from datetime import UTC, date, datetime
from pathlib import Path
from typing import Any


@dataclass
class KeyringExpiryResult:
    keyring: Path
    active_key_id: str
    active_not_after: date
    days_remaining: int
    has_next_key: bool
    warning: str | None = None
    error: str | None = None


def parse_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValueError(f"missing keyring file: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"invalid json in keyring {path}: {exc}") from exc


def parse_iso_date(raw: str, field_name: str, keyring: Path) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"{keyring}: invalid {field_name} date '{raw}' (expected YYYY-MM-DD)") from exc


def evaluate_keyring(keyring_path: Path, warn_days: int, fail_days: int, today: date) -> KeyringExpiryResult:
    payload = parse_json(keyring_path)
    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        raise ValueError(f"{keyring_path}: keyring must include a non-empty keys list")

    active_key_id = str(payload.get("active_key_id", "")).strip()
    if not active_key_id:
        raise ValueError(f"{keyring_path}: active_key_id must be set")

    active_entry = None
    next_keys = []
    for entry in keys:
        if not isinstance(entry, dict):
            raise ValueError(f"{keyring_path}: keys entries must be objects")
        key_id = str(entry.get("key_id", "")).strip()
        status = str(entry.get("status", "")).strip()
        if key_id == active_key_id:
            active_entry = entry
        if status == "next":
            next_keys.append(entry)

    if active_entry is None:
        raise ValueError(f"{keyring_path}: active_key_id '{active_key_id}' not present in keys[]")

    not_after_raw = str(active_entry.get("not_after", "")).strip()
    if not not_after_raw:
        raise ValueError(f"{keyring_path}: active key '{active_key_id}' missing not_after")
    active_not_after = parse_iso_date(not_after_raw, "not_after", keyring_path)
    days_remaining = (active_not_after - today).days

    result = KeyringExpiryResult(
        keyring=keyring_path,
        active_key_id=active_key_id,
        active_not_after=active_not_after,
        days_remaining=days_remaining,
        has_next_key=bool(next_keys),
    )

    if days_remaining <= fail_days:
        result.error = (
            f"{keyring_path}: active key '{active_key_id}' expires in {days_remaining} day(s) "
            f"on {active_not_after.isoformat()} (fail threshold={fail_days})"
        )
    elif days_remaining <= warn_days:
        result.warning = (
            f"{keyring_path}: active key '{active_key_id}' expires in {days_remaining} day(s) "
            f"on {active_not_after.isoformat()} (warn threshold={warn_days})"
        )
    return result


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate AI keyring expiry windows.")
    parser.add_argument(
        "--keyring",
        action="append",
        required=True,
        help="Path to a keyring JSON file. May be passed multiple times.",
    )
    parser.add_argument(
        "--warn-days",
        type=int,
        default=30,
        help="Emit warning when active key has <= this many days remaining (default: 30).",
    )
    parser.add_argument(
        "--fail-days",
        type=int,
        default=0,
        help="Fail when active key has <= this many days remaining (default: 0, already expired).",
    )
    parser.add_argument(
        "--out-json",
        type=Path,
        default=None,
        help="Optional JSON report output path.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.warn_days < 0 or args.fail_days < 0:
        print("error: --warn-days and --fail-days must be >= 0", file=sys.stderr)
        return 2
    if args.warn_days < args.fail_days:
        print("error: --warn-days must be >= --fail-days", file=sys.stderr)
        return 2

    today = datetime.now(UTC).date()
    results: list[KeyringExpiryResult] = []
    errors: list[str] = []
    warnings: list[str] = []

    for raw in args.keyring:
        path = Path(raw)
        try:
            result = evaluate_keyring(path, args.warn_days, args.fail_days, today)
            results.append(result)
        except ValueError as exc:
            errors.append(str(exc))
            continue
        if not result.has_next_key:
            warnings.append(f"{path}: no key with status='next' present")
        if result.warning:
            warnings.append(result.warning)
        if result.error:
            errors.append(result.error)

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)

    if args.out_json is not None:
        payload = {
            "generated_at": datetime.now(UTC).isoformat(),
            "today": today.isoformat(),
            "warn_days": args.warn_days,
            "fail_days": args.fail_days,
            "results": [
                {
                    "keyring": str(item.keyring),
                    "active_key_id": item.active_key_id,
                    "active_not_after": item.active_not_after.isoformat(),
                    "days_remaining": item.days_remaining,
                    "has_next_key": item.has_next_key,
                    "warning": item.warning,
                    "error": item.error,
                }
                for item in results
            ],
            "warnings": warnings,
            "errors": errors,
        }
        args.out_json.parent.mkdir(parents=True, exist_ok=True)
        args.out_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
