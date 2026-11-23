#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

VALID_STATUSES = {"draft", "proposed", "accepted", "integrated", "superseded", "rejected"}
NO_STATUS_HEADER = "no-status-header"

RFC_ID_RE = re.compile(r"^(RFC-(?:DPE-)?[0-9A-Z]+)")
ADR_NUM_RE = re.compile(r"^ADR-(\d+)")


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
        re.compile(r"^\|\s*(?:\*\*)?Status(?:\*\*)?\s*\|\s*(?:\*\*)?([A-Za-z-]+)", re.IGNORECASE | re.MULTILINE),
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


def resolve_root(argv: list[str]) -> Path | None:
    if len(argv) == 1:
        return REPO_ROOT
    if len(argv) == 3 and argv[1] == "--root":
        return Path(argv[2]).resolve()
    print("usage: check_rfc_lifecycle_hygiene.py [--root DIR]", file=sys.stderr)
    return None


def collect_rfc_files(rfc_root: Path) -> dict[str, list[Path]]:
    found: dict[str, list[Path]] = {}
    if not rfc_root.is_dir():
        return found
    for path in sorted(rfc_root.iterdir()):
        if not path.is_file():
            continue
        match = RFC_ID_RE.match(path.name)
        if not match:
            continue
        found.setdefault(match.group(1), []).append(path)
    return found


def markdown_files(paths: list[Path]) -> list[Path]:
    return [path for path in paths if path.suffix == ".md"]


def unindexed_rfc_issues(file_map: dict[str, list[Path]], indexed_ids: set[str]) -> list[str]:
    issues: list[str] = []
    for rfc_id in sorted(file_map):
        if rfc_id in indexed_ids:
            continue
        names = ", ".join(path.name for path in file_map[rfc_id])
        issues.append(f"{rfc_id} exists as a file but is not in spec/rfcs/index.md: {names}")
    return issues


def multiple_markdown_warnings(file_map: dict[str, list[Path]]) -> list[str]:
    warnings: list[str] = []
    for rfc_id in sorted(file_map):
        documents = markdown_files(file_map[rfc_id])
        if len(documents) < 2:
            continue
        names = ", ".join(path.name for path in documents)
        warnings.append(f"WARNING: {rfc_id} has {len(documents)} markdown files: {names}")
    return warnings


def duplicate_adr_issues(adr_root: Path) -> tuple[list[str], int]:
    if not adr_root.is_dir():
        return [f"missing ADR directory: {adr_root}"], 0
    grouped: dict[int, list[Path]] = {}
    for path in sorted(adr_root.iterdir()):
        if not path.is_file():
            continue
        match = ADR_NUM_RE.match(path.name)
        if not match:
            continue
        grouped.setdefault(int(match.group(1)), []).append(path)
    issues: list[str] = []
    for number in sorted(grouped):
        paths = grouped[number]
        if len(paths) < 2:
            continue
        names = ", ".join(path.name for path in paths)
        issues.append(f"duplicate ADR number {number:04d}: {names}")
    return issues, sum(len(paths) for paths in grouped.values())


def status_coherence_issues(
    rows: list[tuple[str, str, str]],
    file_map: dict[str, list[Path]],
) -> list[str]:
    issues: list[str] = []
    for rfc_id, index_status, notes in rows:
        files = file_map.get(rfc_id, [])
        documents = markdown_files(files)
        if not documents:
            if not files:
                issues.append(f"missing RFC file for index entry: {rfc_id}")
            elif index_status != NO_STATUS_HEADER:
                names = ", ".join(path.name for path in files)
                issues.append(
                    f"{rfc_id}: no Status header in {names}; "
                    f"index status must be {NO_STATUS_HEADER} (found '{index_status}')"
                )
            continue

        primary = [
            path for path in documents
            if not path.name.endswith("-template.md") and not path.name.endswith("-hardware-profile.md")
        ]
        path = primary[0] if primary else documents[0]
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
        match = re.search(r"Superseded by (RFC-[0-9A-Za-z-]+)", notes, flags=re.IGNORECASE)
        if match:
            expected_superseder = match.group(1).upper()

        actual_superseder = read_superseded_by(text)
        if index_status == "superseded":
            if actual_superseder is None:
                issues.append(f"{path.name}: superseded RFC missing Superseded-By metadata")
            elif expected_superseder and actual_superseder != expected_superseder:
                issues.append(
                    f"{path.name}: superseder mismatch index={expected_superseder} file={actual_superseder}"
                )
    return issues


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv if argv is None else argv)
    root = resolve_root(argv)
    if root is None:
        return 2

    rfc_root = root / "spec/rfcs"
    index_path = rfc_root / "index.md"
    adr_root = root / "docs/architecture/adr"
    issues: list[str] = []

    if not index_path.exists():
        print(f"rfc lifecycle hygiene check FAILED\n- missing RFC index: {index_path}")
        return 1

    rows = parse_index_rows(index_path.read_text(encoding="utf-8"))
    if not rows:
        print("rfc lifecycle hygiene check FAILED\n- no RFC rows found in spec/rfcs/index.md")
        return 1

    file_map = collect_rfc_files(rfc_root)
    indexed_ids = {rfc_id for rfc_id, _status, _notes in rows}
    issues.extend(unindexed_rfc_issues(file_map, indexed_ids))
    warnings = multiple_markdown_warnings(file_map)
    adr_issues, adr_count = duplicate_adr_issues(adr_root)
    issues.extend(adr_issues)
    issues.extend(status_coherence_issues(rows, file_map))

    for warning in warnings:
        print(warning)
        print(f"::warning::{warning}")

    if issues:
        print("rfc lifecycle hygiene check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("rfc lifecycle hygiene check PASSED")
    print(f"- validated RFC index/file lifecycle coherence for {len(rows)} RFC entries")
    print(f"- checked {len(file_map)} RFC ids present as files are indexed")
    print(f"- checked {adr_count} numbered ADR files for duplicate numbers")
    if warnings:
        print(f"- reported {len(warnings)} RFC ids with more than one markdown file")
    return 0


if __name__ == "__main__":
    sys.exit(main())
