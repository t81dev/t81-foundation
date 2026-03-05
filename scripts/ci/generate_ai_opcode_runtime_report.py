#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.opcode-runtime-report.v1"
PHASE1 = ("ATTN", "QMATMUL", "EMBED")


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


def build_payload(repo_root: Path) -> dict[str, Any]:
    ai_header = repo_root / "include/t81/isa/ai_native_opcodes.hpp"
    tisc_header = repo_root / "include/t81/isa/opcodes.hpp"
    vm_cpp = repo_root / "core/vm/vm.cpp"

    ai_text = read_text(ai_header)
    tisc_text = read_text(tisc_header)
    vm_text = read_text(vm_cpp)

    ai_enum = parse_enum_members(ai_text, "AIOpcode")
    tisc_enum = parse_enum_members(tisc_text, "Opcode")
    vm_cases = parse_vm_dispatch_cases(vm_text)

    phase_rows: list[dict[str, Any]] = []
    for opcode in PHASE1:
        in_ai_header = opcode in ai_enum
        in_tisc_enum = opcode in tisc_enum
        in_vm_dispatch = opcode in vm_cases
        runtime_ready = in_tisc_enum and in_vm_dispatch
        phase_rows.append(
            {
                "opcode": opcode,
                "declared_ai_opcode_header": in_ai_header,
                "present_tisc_opcode_enum": in_tisc_enum,
                "present_vm_dispatch_case": in_vm_dispatch,
                "runtime_ready": runtime_ready,
                "status": "runtime_bound" if runtime_ready else "contract_only",
            }
        )

    runtime_ready_count = sum(1 for r in phase_rows if r["runtime_ready"])
    phase_status = "runtime_bound" if runtime_ready_count == len(PHASE1) else "baseline_contract_only"

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
        },
        "summary": {
            "phase1_opcode_count": len(PHASE1),
            "runtime_ready_count": runtime_ready_count,
            "contract_only_count": len(PHASE1) - runtime_ready_count,
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
        "| Opcode | AI Header | TISC Enum | VM Dispatch | Status |",
        "| :--- | :---: | :---: | :---: | :--- |",
    ]
    for row in payload["opcodes"]:
        lines.append(
            "| {opcode} | {aih} | {tisc} | {vm} | {status} |".format(
                opcode=row["opcode"],
                aih="yes" if row["declared_ai_opcode_header"] else "no",
                tisc="yes" if row["present_tisc_opcode_enum"] else "no",
                vm="yes" if row["present_vm_dispatch_case"] else "no",
                status=row["status"],
            )
        )
    lines.extend(
        [
            "",
            "Evidence Files:",
            f"- `{payload['evidence']['ai_opcode_header']}`",
            f"- `{payload['evidence']['tisc_opcode_header']}`",
            f"- `{payload['evidence']['vm_dispatch_source']}`",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Generate RFC-0026/RFC-00A8 runtime evidence report.")
    p.add_argument("--repo-root", default=".")
    p.add_argument("--out-dir", required=True)
    return p.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    payload = build_payload(repo_root)
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
