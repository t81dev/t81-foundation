#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
RFC_ROOT = REPO_ROOT / "spec/rfcs"
INDEX_PATH = RFC_ROOT / "index.md"

VALID_STATUSES = {"draft", "proposed", "accepted", "integrated", "superseded", "rejected"}


def parse_index_rows(index_text: str) -> list[tuple[str, str, str]]:
    rows: list[tuple[str, str, str]] = []
    for line in index_text.splitlines():
        if not line.startswith("| RFC-"):
            continue
        parts = [p.strip() for p in line.strip("|").split("|")]
        if len(parts) < 4:
            continue
        rfc_id, _title, status, notes = parts[:4]
        rows.append((rfc_id, status.lower(), notes))
    return rows


def read_status_from_rfc(text: str) -> str | None:
    top = "\n".join(text.splitlines()[:120])
    patterns = [
        re.compile(r"^\s*(?:[-*]\s+)?\*\*Status:\*\*\s*([A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?\*\*Status\*\*:\s*([A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?Status:\s*([A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?status:\s*([A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
    ]
    for pat in patterns:
        m = pat.search(top)
        if m:
            return m.group(1).strip().rstrip("\\").lower()
    return None


def read_superseded_by(text: str) -> str | None:
    top = "\n".join(text.splitlines()[:150])
    patterns = [
        re.compile(r"^\s*(?:[-*]\s+)?\*\*Superseded-By:\*\*\s*(RFC-[0-9A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?\*\*Superseded-By\*\*:\s*(RFC-[0-9A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?Superseded-By:\s*(RFC-[0-9A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
        re.compile(r"^\s*(?:[-*]\s+)?superseded_by:\s*(RFC-[0-9A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
    ]
    for pat in patterns:
        m = pat.search(top)
        if m:
            return m.group(1).strip().upper()
    return None


def main() -> int:
    issues: list[str] = []

    if not INDEX_PATH.exists():
        print(f"rfc lifecycle hygiene check FAILED\n- missing RFC index: {INDEX_PATH.relative_to(REPO_ROOT)}")
        return 1

    rows = parse_index_rows(INDEX_PATH.read_text(encoding="utf-8"))
    if not rows:
        print("rfc lifecycle hygiene check FAILED\n- no RFC rows found in spec/rfcs/index.md")
        return 1

    for rfc_id, index_status, notes in rows:
        matches = sorted(RFC_ROOT.glob(f"{rfc_id}-*.md"))
        if not matches:
            issues.append(f"missing RFC file for index entry: {rfc_id}")
            continue

        path = matches[0]
        text = path.read_text(encoding="utf-8", errors="ignore")

        file_status = read_status_from_rfc(text)
        if not file_status:
            issues.append(f"{path.name}: unable to parse status/version metadata")
            continue

        if file_status not in VALID_STATUSES:
            issues.append(f"{path.name}: invalid parsed status '{file_status}'")

        if index_status != file_status:
            issues.append(
                f"{path.name}: status mismatch index={index_status} file={file_status}"
            )

        expected_superseder = None
        m = re.search(r"Superseded by (RFC-[0-9A-Za-z-]+)", notes, flags=re.IGNORECASE)
        if m:
            expected_superseder = m.group(1).upper()

        actual_superseder = read_superseded_by(text)
        if index_status == "superseded":
            if actual_superseder is None:
                issues.append(f"{path.name}: superseded RFC missing Superseded-By metadata")
            elif expected_superseder and actual_superseder != expected_superseder:
                issues.append(
                    f"{path.name}: superseder mismatch index={expected_superseder} file={actual_superseder}"
                )

    if issues:
        print("rfc lifecycle hygiene check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("rfc lifecycle hygiene check PASSED")
    print(f"- validated RFC index/file lifecycle coherence for {len(rows)} RFC entries")
    return 0


if __name__ == "__main__":
    sys.exit(main())
