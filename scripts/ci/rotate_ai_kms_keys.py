#!/usr/bin/env python3
"""Rotate AI KMS key windows automatically based on 120-day limit."""

import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path
import calendar

def add_months(d: date, months: int) -> date:
    month = d.month - 1 + months
    year = d.year + month // 12
    month = month % 12 + 1
    day = min(d.day, calendar.monthrange(year, month)[1])
    return date(year, month, day)

def bump_quarter_string(s: str) -> str:
    """Bump strings like 2026Q2 to 2026Q3 (or 2026q2 -> 2026q3), wrapping Q4 to Q1 of next year."""
    def repl(m: re.Match) -> str:
        year = int(m.group(1))
        q = int(m.group(3))
        q += 1
        if q > 4:
            q = 1
            year += 1
        q_char = m.group(2)
        return f"{year}{q_char}{q}"
    return re.sub(r'(\d{4})([qQ])([1-4])', repl, s)

def rotate_keyring(path: Path) -> None:
    try:
        data = json.loads(path.read_text(encoding='utf-8'))
    except Exception as e:
        print(f"error: failed to read {path}: {e}", file=sys.stderr)
        return

    keys = data.get("keys", [])
    active_key = None
    next_key = None

    for k in keys:
        if k.get('status') == 'active':
            active_key = k
        elif k.get('status') == 'next':
            next_key = k

    if not active_key or not next_key:
        print(f"warning: Skipping {path}: missing 'active' or 'next' key", file=sys.stderr)
        return
    
    # Rotate: active -> expired, next -> active
    active_key['status'] = 'expired'
    next_key['status'] = 'active'
    data['active_key_id'] = next_key['key_id']

    # Generate the new "next" key
    new_next = dict(next_key)
    new_next['status'] = 'next'
    new_next['key_id'] = bump_quarter_string(next_key['key_id'])

    try:
        nb = date.fromisoformat(next_key['not_before'])
        na = date.fromisoformat(next_key['not_after'])
        new_next['not_before'] = add_months(nb, 3).isoformat()
        new_next['not_after'] = add_months(na, 3).isoformat()
    except Exception as e:
        print(f"warning: failed to parse dates in {path}: {e}", file=sys.stderr)
        return

    new_next['material_env'] = bump_quarter_string(next_key['material_env'])
    new_next['kms_key_ref'] = bump_quarter_string(next_key['kms_key_ref'])

    data['keys'].append(new_next)

    try:
        path.write_text(json.dumps(data, indent=2) + "\n", encoding='utf-8')
        print(f"Rotated {path}: new active='{next_key['key_id']}', new next='{new_next['key_id']}'")
    except Exception as e:
        print(f"error: failed to write {path}: {e}", file=sys.stderr)

def main() -> int:
    p = argparse.ArgumentParser(description="Rotate AI KMS cryptographic signing keyrings.")
    p.add_argument("--keyring", action="append", required=True, help="Path to a keyring JSON file.")
    args = p.parse_args()

    for raw in args.keyring:
        rotate_keyring(Path(raw))

    return 0

if __name__ == "__main__":
    sys.exit(main())
