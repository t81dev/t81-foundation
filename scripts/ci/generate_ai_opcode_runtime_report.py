#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import UTC, date, datetime
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.opcode-runtime-report.v1"
PHASE1 = ("ATTN", "QMATMUL", "EMBED")
PHASE1_TESTS = {
    "ATTN": "t81_vm_ai_phase1_attention_conformance_test",
    "QMATMUL": "t81_vm_ai_phase1_qmatmul_conformance_test",
    "EMBED": "t81_vm_ai_phase1_embed_conformance_test",
}


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_enum_members(text: str, enum_name: str) -> set[str]:
    m = re.search(
        rf"enum\s+class\s+{re.escape(enum_name)}(?:\s*:\s*[^{{]+)?\s*\{{(?P<body>.*?)\}};",
        text,
        re.S,
    )
    if not m:
        return set()
    body = m.group("body")
    body = re.sub(r"//.*?$", "", body, flags=re.M)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    members: set[str] = set()
    for raw in body.split(","):
        token = raw.strip()
        if not token:
            continue
        token = token.split("=")[0].strip()
        if token.startswith("//"):
            continue
        token = re.sub(r"/\*.*?\*/", "", token).strip()
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", token):
            members.add(token)
    return members


def parse_vm_dispatch_cases(vm_text: str) -> set[str]:
    return set(
        re.findall(r"case\s+(?:t81::tisc::)?Opcode::([A-Za-z_][A-Za-z0-9_]*)\s*:", vm_text)
    )


def canonical_json(obj: Any) -> str:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def parse_iso_date(raw: str, label: str) -> date:
    try:
        return date.fromisoformat(raw)
    except ValueError as exc:
        raise ValueError(f"invalid {label} date: {raw} (expected YYYY-MM-DD)") from exc


def parse_phase1_conformance_log(ctest_log: Path) -> dict[str, Any]:
    if not ctest_log.exists():
        raise FileNotFoundError(f"ctest log not found: {ctest_log}")
    text = read_text(ctest_log)
    tests_present = {name: (name in text) for name in PHASE1_TESTS.values()}
    hash_rows = re.findall(r"AI_PHASE1_HASH\s+(ATTN|QMATMUL|EMBED)\s+([0-9a-fA-F]{16,64})", text)
    output_hashes = {opcode: value.lower() for opcode, value in hash_rows}
    hashes_present = {opcode: (opcode in output_hashes) for opcode in PHASE1}
    return {
        "ctest_log": str(ctest_log),
        "ctest_log_sha256": sha256_text(text),
        "tests_present": tests_present,
        "all_required_tests_present": all(tests_present.values()),
        "ctest_success_marker_present": "100% tests passed" in text,
        "output_hashes": output_hashes,
        "all_required_output_hashes_present": all(hashes_present.values()),
    }


def parse_phase1_baseline_hashes_payload(payload: dict[str, Any], label: str) -> dict[str, str]:
    if payload.get("schema") != "t81.ai.phase1-hash-baseline.v1":
        raise ValueError(f"invalid baseline schema in {label}")
    hashes = payload.get("output_hashes", {})
    result: dict[str, str] = {}
    for opcode in PHASE1:
        value = str(hashes.get(opcode, "")).strip().lower()
        if not re.fullmatch(r"[0-9a-f]{16,64}", value):
            raise ValueError(f"invalid or missing baseline hash for {opcode} in {label}")
        result[opcode] = value
    return result


def parse_phase1_baseline_hashes(path: Path) -> dict[str, str]:
    payload = json.loads(read_text(path))
    return parse_phase1_baseline_hashes_payload(payload, str(path))


def select_phase1_baseline_window(history_path: Path, as_of: date) -> tuple[dict[str, str], dict[str, Any]]:
    history = json.loads(read_text(history_path))
    if history.get("schema") != "t81.ai.phase1-hash-baseline-history.v1":
        raise ValueError(f"invalid baseline history schema in {history_path}")
    windows = history.get("windows")
    if not isinstance(windows, list) or not windows:
        raise ValueError(f"baseline history windows must be non-empty in {history_path}")

    selected: dict[str, Any] | None = None
    for win in windows:
        if not isinstance(win, dict):
            continue
        start_raw = str(win.get("window_start", "")).strip()
        end_raw = str(win.get("window_end", "")).strip()
        if not start_raw or not end_raw:
            continue
        start = parse_iso_date(start_raw, "window_start")
        end = parse_iso_date(end_raw, "window_end")
        if start <= as_of <= end:
            selected = win
            break
    if selected is None:
        candidates: list[tuple[date, dict[str, Any]]] = []
        for win in windows:
            if not isinstance(win, dict):
                continue
            start_raw = str(win.get("window_start", "")).strip()
            if not start_raw:
                continue
            try:
                start = parse_iso_date(start_raw, "window_start")
            except ValueError:
                continue
            if start <= as_of:
                candidates.append((start, win))
        if candidates:
            candidates.sort(key=lambda item: item[0], reverse=True)
            selected = candidates[0][1]
    if selected is None:
        raise ValueError(f"no valid baseline history window found in {history_path}")

    baseline_payload = selected.get("baseline")
    if not isinstance(baseline_payload, dict):
        raise ValueError(f"selected baseline window missing baseline object in {history_path}")
    hashes = parse_phase1_baseline_hashes_payload(
        baseline_payload, f"{history_path} window {selected.get('window_id', '')}"
    )
    metadata = {
        "history_path": str(history_path),
        "window_id": str(selected.get("window_id", "")),
        "window_start": str(selected.get("window_start", "")),
        "window_end": str(selected.get("window_end", "")),
        "as_of_date": as_of.isoformat(),
        "provenance": selected.get("provenance", {}) if isinstance(selected.get("provenance"), dict) else {},
    }
    return hashes, metadata


def build_payload(
    repo_root: Path,
    ctest_log: Path | None,
    baseline_hashes_path: Path | None,
    baseline_history_path: Path | None,
    as_of_date: date,
) -> dict[str, Any]:
    ai_header = repo_root / "include/t81/isa/ai_native_opcodes.hpp"
    tisc_header = repo_root / "include/t81/isa/opcodes.hpp"
    vm_cpp = repo_root / "core/vm/vm.cpp"

    ai_text = read_text(ai_header)
    tisc_text = read_text(tisc_header)
    vm_text = read_text(vm_cpp)

    ai_enum = parse_enum_members(ai_text, "AIOpcode")
    tisc_enum = parse_enum_members(tisc_text, "Opcode")
    vm_cases = parse_vm_dispatch_cases(vm_text)
    conformance = parse_phase1_conformance_log(ctest_log) if ctest_log is not None else None
    baseline_hashes: dict[str, str] | None = None
    baseline_source_path = ""
    baseline_selection: dict[str, Any] | None = None
    if baseline_history_path is not None and baseline_history_path.exists():
        baseline_hashes, baseline_selection = select_phase1_baseline_window(baseline_history_path, as_of_date)
        baseline_source_path = str(baseline_history_path)
    elif baseline_hashes_path is not None:
        baseline_hashes = parse_phase1_baseline_hashes(baseline_hashes_path)
        baseline_source_path = str(baseline_hashes_path)
        baseline_selection = {
            "history_path": "",
            "window_id": "",
            "window_start": "",
            "window_end": "",
            "as_of_date": as_of_date.isoformat(),
        }

    phase_rows: list[dict[str, Any]] = []
    for opcode in PHASE1:
        in_ai_header = opcode in ai_enum
        in_tisc_enum = opcode in tisc_enum
        in_vm_dispatch = opcode in vm_cases
        test_name = PHASE1_TESTS[opcode]
        conformance_test_present = (
            True if conformance is None else bool(conformance["tests_present"].get(test_name, False))
        )
        output_hash = None if conformance is None else conformance["output_hashes"].get(opcode)
        output_hash_present = True if conformance is None else output_hash is not None
        runtime_ready = in_tisc_enum and in_vm_dispatch and conformance_test_present and output_hash_present
        phase_rows.append(
            {
                "opcode": opcode,
                "declared_ai_opcode_header": in_ai_header,
                "present_tisc_opcode_enum": in_tisc_enum,
                "present_vm_dispatch_case": in_vm_dispatch,
                "phase1_conformance_test": test_name,
                "phase1_conformance_test_present": conformance_test_present,
                "phase1_output_hash": output_hash,
                "phase1_output_hash_present": output_hash_present,
                "runtime_ready": runtime_ready,
                "status": "runtime_bound" if runtime_ready else "contract_only",
            }
        )

    runtime_ready_count = sum(1 for r in phase_rows if r["runtime_ready"] and r["status"] == "runtime_bound")
    baseline_comparison: dict[str, Any] | None = None
    baseline_hashes_match = True
    if baseline_hashes is not None and conformance is not None:
        observed = conformance["output_hashes"]
        per_opcode: dict[str, dict[str, Any]] = {}
        for opcode in PHASE1:
            expected = baseline_hashes[opcode]
            actual = observed.get(opcode)
            match = actual == expected
            baseline_hashes_match = baseline_hashes_match and match
            per_opcode[opcode] = {
                "expected": expected,
                "actual": actual,
                "match": match,
            }
        baseline_comparison = {
            "baseline_path": baseline_source_path,
            "all_match": baseline_hashes_match,
            "per_opcode": per_opcode,
            "selection": baseline_selection,
        }
    conformance_valid = (
        True
        if conformance is None
        else bool(conformance["all_required_tests_present"])
        and bool(conformance["all_required_output_hashes_present"])
        and bool(conformance["ctest_success_marker_present"])
    )
    phase_status = (
        "runtime_bound"
        if runtime_ready_count == len(PHASE1) and conformance_valid
        else "baseline_contract_only"
    )

    payload: dict[str, Any] = {
        "schema": SCHEMA_VERSION,
        "source_rfc": "RFC-0026",
        "exploration_rfc": "RFC-00A8",
        "phase": "phase1",
        "phase_status": phase_status,
        "opcodes": phase_rows,
        "evidence": {
            "ai_opcode_header": str(ai_header.relative_to(repo_root)),
            "tisc_opcode_header": str(tisc_header.relative_to(repo_root)),
            "vm_dispatch_source": str(vm_cpp.relative_to(repo_root)),
            "ai_opcode_header_sha256": sha256_text(ai_text),
            "tisc_opcode_header_sha256": sha256_text(tisc_text),
            "vm_dispatch_source_sha256": sha256_text(vm_text),
            "phase1_conformance": conformance,
            "phase1_baseline_comparison": baseline_comparison,
            "phase1_baseline_selection": baseline_selection,
            "phase1_vector_provenance": (
                baseline_selection.get("provenance", {}) if isinstance(baseline_selection, dict) else {}
            ),
        },
        "summary": {
            "phase1_opcode_count": len(PHASE1),
            "runtime_ready_count": runtime_ready_count,
            "contract_only_count": len(PHASE1) - runtime_ready_count,
            "phase1_conformance_evidence_present": conformance is not None,
            "phase1_conformance_valid": conformance_valid,
            "phase1_baseline_hashes_provided": baseline_hashes is not None,
            "phase1_baseline_hashes_match": baseline_hashes_match if baseline_hashes is not None else None,
        },
    }
    payload["report_sha256"] = sha256_text(canonical_json(payload))
    return payload


def write_markdown(path: Path, payload: dict[str, Any]) -> None:
    lines = [
        "# AI Opcode Runtime Report",
        "",
        f"- schema: `{payload['schema']}`",
        f"- phase_status: `{payload['phase_status']}`",
        f"- report_sha256: `{payload['report_sha256']}`",
        "",
        "| Opcode | AI Header | TISC Enum | VM Dispatch | CTest Evidence | Output Hash | Status |",
        "| :--- | :---: | :---: | :---: | :---: | :---: | :--- |",
    ]
    for row in payload["opcodes"]:
        lines.append(
            "| {opcode} | {aih} | {tisc} | {vm} | {ctest} | `{hashv}` | {status} |".format(
                opcode=row["opcode"],
                aih="yes" if row["declared_ai_opcode_header"] else "no",
                tisc="yes" if row["present_tisc_opcode_enum"] else "no",
                vm="yes" if row["present_vm_dispatch_case"] else "no",
                ctest="yes" if row["phase1_conformance_test_present"] else "no",
                hashv=row["phase1_output_hash"] if row["phase1_output_hash"] is not None else "missing",
                status=row["status"],
            )
        )
    conformance = payload["evidence"].get("phase1_conformance")
    baseline = payload["evidence"].get("phase1_baseline_comparison")
    lines.extend(
        [
            "",
            "Evidence Files:",
            f"- `{payload['evidence']['ai_opcode_header']}`",
            f"- `{payload['evidence']['tisc_opcode_header']}`",
            f"- `{payload['evidence']['vm_dispatch_source']}`",
        ]
    )
    if conformance is not None:
        lines.extend(
            [
                f"- `{conformance['ctest_log']}`",
                "",
                "Phase-1 Conformance Evidence:",
                f"- all_required_tests_present: `{conformance['all_required_tests_present']}`",
                f"- all_required_output_hashes_present: `{conformance['all_required_output_hashes_present']}`",
                f"- ctest_success_marker_present: `{conformance['ctest_success_marker_present']}`",
                "",
            ]
        )
    if baseline is not None:
        lines.extend(
            [
                "Phase-1 Baseline Hash Comparison:",
                f"- baseline_path: `{baseline['baseline_path']}`",
                f"- all_match: `{baseline['all_match']}`",
            ]
        )
        selection = baseline.get("selection")
        if isinstance(selection, dict):
            lines.extend(
                [
                    f"- baseline_window_id: `{selection.get('window_id', '')}`",
                    f"- baseline_window_start: `{selection.get('window_start', '')}`",
                    f"- baseline_window_end: `{selection.get('window_end', '')}`",
                    f"- baseline_as_of_date: `{selection.get('as_of_date', '')}`",
                ]
            )
            provenance = selection.get("provenance")
            if isinstance(provenance, dict) and provenance:
                lines.append("- baseline_window_provenance:")
                for key in sorted(provenance.keys()):
                    lines.append(f"  - {key}: `{provenance.get(key)}`")
        for opcode in PHASE1:
            row = baseline["per_opcode"][opcode]
            lines.append(
                f"- {opcode}: expected=`{row['expected']}` actual=`{row['actual']}` match=`{row['match']}`"
            )
        lines.append("")
    else:
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Generate RFC-0026/RFC-00A8 runtime evidence report.")
    p.add_argument("--repo-root", default=".")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--ctest-log", default="", help="Optional path to AI phase1 opcode ctest log.")
    p.add_argument(
        "--baseline-hashes",
        default="",
        help="Optional path to phase1 baseline output hashes json.",
    )
    p.add_argument(
        "--baseline-hashes-history",
        default="",
        help="Optional path to phase1 baseline history windows json.",
    )
    p.add_argument(
        "--as-of-date",
        default="",
        help="Optional YYYY-MM-DD date for baseline history window selection (default: today UTC).",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    ctest_log = Path(args.ctest_log) if args.ctest_log else None
    baseline_hashes = Path(args.baseline_hashes).resolve() if args.baseline_hashes else None
    baseline_history = Path(args.baseline_hashes_history).resolve() if args.baseline_hashes_history else None
    as_of = parse_iso_date(args.as_of_date, "as_of_date") if args.as_of_date else datetime.now(UTC).date()
    payload = build_payload(repo_root, ctest_log, baseline_hashes, baseline_history, as_of)
    json_path = out_dir / "ai_opcode_runtime_report.json"
    md_path = out_dir / "ai_opcode_runtime_report.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(md_path, payload)

    print(f"ai opcode runtime phase status: {payload['phase_status']}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
