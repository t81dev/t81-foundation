#!/usr/bin/env python3
"""Enforce KMS metadata contract for AI signing keyrings."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


def parse_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValueError(f"missing keyring file: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"invalid json in keyring {path}: {exc}") from exc


def validate_keyring(path: Path, max_active_days_limit: int) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    payload = parse_json(path)

    rotation = payload.get("rotation_policy")
    if not isinstance(rotation, dict):
        errors.append(f"{path}: rotation_policy must be object")
    else:
        max_active_days = rotation.get("max_active_days")
        if not isinstance(max_active_days, int) or max_active_days <= 0:
            errors.append(f"{path}: rotation_policy.max_active_days must be positive integer")
        elif max_active_days > max_active_days_limit:
            errors.append(
                f"{path}: rotation_policy.max_active_days={max_active_days} exceeds limit {max_active_days_limit}"
            )
        if bool(rotation.get("require_next_key", False)) is not True:
            errors.append(f"{path}: rotation_policy.require_next_key must be true")

    keys = payload.get("keys")
    if not isinstance(keys, list) or not keys:
        errors.append(f"{path}: keys must be a non-empty list")
        return errors, warnings

    has_active = False
    has_next = False
    for idx, entry in enumerate(keys, start=1):
        if not isinstance(entry, dict):
            errors.append(f"{path}: key entry {idx} is not an object")
            continue
        key_id = str(entry.get("key_id", f"entry-{idx}")).strip()
        status = str(entry.get("status", "")).strip()
        if status == "active":
            has_active = True
        if status == "next":
            has_next = True

        env_name = str(entry.get("material_env", "")).strip()
        if not env_name:
            errors.append(f"{path}: key {key_id} missing material_env")

        kms_key_ref = str(entry.get("kms_key_ref", "")).strip()
        if not kms_key_ref:
            errors.append(f"{path}: key {key_id} missing kms_key_ref")
        elif not kms_key_ref.startswith("kms://"):
            errors.append(f"{path}: key {key_id} kms_key_ref must start with kms://")

        if "material_b64" in entry:
            errors.append(f"{path}: key {key_id} must not contain material_b64 plaintext fallback")

    if not has_active:
        errors.append(f"{path}: no active key present")
    if not has_next:
        errors.append(f"{path}: no next key present")
    return errors, warnings


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate KMS metadata contract for AI keyrings.")
    p.add_argument(
        "--keyring",
        action="append",
        required=True,
        help="Path to keyring JSON. Pass multiple times.",
    )
    p.add_argument(
        "--max-active-days-limit",
        type=int,
        default=120,
        help="Maximum allowed rotation_policy.max_active_days (default: 120).",
    )
    p.add_argument(
        "--out-json",
        default="",
        help="Optional output report path.",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    if args.max_active_days_limit <= 0:
        print("error: --max-active-days-limit must be > 0", file=sys.stderr)
        return 2

    all_errors: list[str] = []
    all_warnings: list[str] = []
    for raw in args.keyring:
        path = Path(raw).resolve()
        try:
            errors, warnings = validate_keyring(path, args.max_active_days_limit)
        except ValueError as exc:
            all_errors.append(str(exc))
            continue
        all_errors.extend(errors)
        all_warnings.extend(warnings)

    for w in all_warnings:
        print(f"warning: {w}", file=sys.stderr)
    for e in all_errors:
        print(f"error: {e}", file=sys.stderr)

    status = "pass" if not all_errors else "fail"
    report = {
        "schema": "t81.ai.keyring-kms-contract.v1",
        "status": status,
        "max_active_days_limit": args.max_active_days_limit,
        "errors": all_errors,
        "warnings": all_warnings,
        "keyring_count": len(args.keyring),
    }
    if args.out_json:
        out_path = Path(args.out_json).resolve()
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"ai keyring kms contract status: {status}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
