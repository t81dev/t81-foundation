#!/usr/bin/env python3
"""Generate a deterministic reproducibility dashboard from CI artifacts."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def sha3_512(path: Path) -> str:
    h = hashlib.sha3_512()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def parse_ctest_junit(path: Path) -> dict[str, int]:
    root = ET.fromstring(path.read_text(encoding="utf-8"))
    attrs = root.attrib
    return {
        "tests": int(attrs.get("tests", "0")),
        "failures": int(attrs.get("failures", "0")),
        "errors": int(attrs.get("errors", "0")),
        "skipped": int(attrs.get("skipped", "0")),
    }


def parse_t3k_hash(path: Path) -> str:
    return path.read_text(encoding="utf-8").strip()

def parse_t81lang_hash(path: Path) -> str:
    return path.read_text(encoding="utf-8").strip()


def parse_bench_json(path: Path) -> dict[str, object]:
    data = json.loads(path.read_text(encoding="utf-8"))
    benches = data.get("benchmarks", [])
    summary: dict[str, object] = {
        "count": len(benches),
        "selected": [],
    }
    priority = [
        "BM_NegationSpeed",
        "BM_Llama_RMSNorm",
        "BM_Llama_SiLU",
        "BM_Llama_Softmax",
        "BM_Base81_Add",
        "BM_MemoryBandwidth_ReadWrite",
    ]

    chosen = []
    for name in priority:
        for b in benches:
            if b.get("name") == name:
                chosen.append(b)
                break

    if not chosen:
        chosen = benches[:5]

    selected = []
    for b in chosen:
        selected.append(
            {
                "name": b.get("name", "unknown"),
                "real_time": b.get("real_time"),
                "cpu_time": b.get("cpu_time"),
                "time_unit": b.get("time_unit"),
            }
        )
    summary["selected"] = selected
    return summary


def count_lines(path: Path) -> int:
    with path.open("r", encoding="utf-8", errors="replace") as f:
        return sum(1 for _ in f)


def artifact_row(path: Path) -> dict[str, object]:
    return {
        "path": str(path),
        "size_bytes": path.stat().st_size,
        "sha3_512": sha3_512(path),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate T81 reproducibility dashboard")
    ap.add_argument("--out-md", required=True)
    ap.add_argument("--out-json", required=True)
    ap.add_argument("--ctest-junit", required=True)
    ap.add_argument("--t3k-hash", required=True)
    ap.add_argument("--t81lang-hash", required=True)
    ap.add_argument("--t81lang-fixtures-dir", required=True)
    ap.add_argument("--bench-json", required=True)
    ap.add_argument(
        "--axion-log",
        action="append",
        default=[],
        help="Repeat for each Axion trace log to include",
    )
    args = ap.parse_args()

    ctest_junit = Path(args.ctest_junit)
    t3k_hash = Path(args.t3k_hash)
    t81lang_hash = Path(args.t81lang_hash)
    t81lang_fixtures_dir = Path(args.t81lang_fixtures_dir)
    bench_json = Path(args.bench_json)
    axion_logs = [Path(p) for p in args.axion_log]

    ctest = parse_ctest_junit(ctest_junit)
    t3k_sha256 = parse_t3k_hash(t3k_hash)
    t81lang_sha256 = parse_t81lang_hash(t81lang_hash)
    t81lang_fixture_count = len(list(t81lang_fixtures_dir.glob("*.t81")))
    bench = parse_bench_json(bench_json)
    axion = [
        {
            "path": str(p),
            "lines": count_lines(p),
            "sha3_512": sha3_512(p),
            "size_bytes": p.stat().st_size,
        }
        for p in axion_logs
        if p.exists()
    ]

    artifacts = [
        artifact_row(ctest_junit),
        artifact_row(t3k_hash),
        artifact_row(t81lang_hash),
        artifact_row(bench_json),
    ]
    artifacts.extend(
        artifact_row(p)
        for p in axion_logs
        if p.exists()
    )

    generated_at = dt.datetime.now(tz=dt.timezone.utc).isoformat()
    overall_ok = (
        ctest["failures"] == 0
        and ctest["errors"] == 0
        and len(t3k_sha256) == 64
        and len(t81lang_sha256) == 64
        and t81lang_fixture_count >= 1
    )

    payload = {
        "generated_at_utc": generated_at,
        "overall_status": "pass" if overall_ok else "fail",
        "ctest": ctest,
        "t3k_repro_gate": {
            "sha256": t3k_sha256,
        },
        "t81lang_repro_gate": {
            "sha256": t81lang_sha256,
            "fixture_count": t81lang_fixture_count,
        },
        "benchmarks": bench,
        "axion_traces": axion,
        "artifacts": artifacts,
    }

    out_json = Path(args.out_json)
    out_md = Path(args.out_md)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_md.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    status = "PASS" if overall_ok else "FAIL"
    lines = [
        "# T81 Reproducibility Ledger Dashboard",
        "",
        f"- Generated (UTC): `{generated_at}`",
        f"- Overall status: **{status}**",
        "",
        "## Build/Test",
        "",
        f"- Tests: `{ctest['tests']}`",
        f"- Failures: `{ctest['failures']}`",
        f"- Errors: `{ctest['errors']}`",
        f"- Skipped: `{ctest['skipped']}`",
        "",
        "## T3_K Repro Gate",
        "",
        f"- Cross-run SHA-256 (fixture quantization): `{t3k_sha256}`",
        "",
        "## T81Lang Repro Gate",
        "",
        f"- Cross-run SHA-256 (fixture compile set): `{t81lang_sha256}`",
        f"- Fixture count: `{t81lang_fixture_count}`",
        "",
        "## Benchmark Snapshot",
        "",
        f"- Benchmark rows: `{bench['count']}`",
    ]

    selected = bench.get("selected", [])
    if selected:
        lines.extend(
            [
                "",
                "| Benchmark | real_time | cpu_time | unit |",
                "| --- | ---: | ---: | --- |",
            ]
        )
        for row in selected:
            lines.append(
                f"| `{row['name']}` | `{row['real_time']}` | `{row['cpu_time']}` | `{row['time_unit']}` |"
            )

    lines.extend(
        [
            "",
            "## Axion Trace Artifacts",
            "",
            "| File | Lines | Bytes | SHA3-512 |",
            "| --- | ---: | ---: | --- |",
        ]
    )
    if axion:
        for row in axion:
            lines.append(
                f"| `{row['path']}` | `{row['lines']}` | `{row['size_bytes']}` | `{row['sha3_512']}` |"
            )
    else:
        lines.append("| _none_ | `0` | `0` | _n/a_ |")

    lines.extend(
        [
            "",
            "## Artifact Digest Table",
            "",
            "| Artifact | Bytes | SHA3-512 |",
            "| --- | ---: | --- |",
        ]
    )
    for row in artifacts:
        lines.append(f"| `{row['path']}` | `{row['size_bytes']}` | `{row['sha3_512']}` |")

    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
