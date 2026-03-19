#!/usr/bin/env python3
"""
Trace Replay Gate — Stage 2 determinism verification.

For each canonical trace fixture in tests/fixtures/canonical_traces/:
  1. Compile the matching .t81 source to a temporary .tisc bytecode file.
  2. Run the VM with --trace to capture the live execution trace.
  3. Replay the live trace against the checked-in golden trace.
  4. Fail if any divergence is detected.

A passing run means the VM produces a bit-identical execution sequence
to the one recorded when the golden trace was generated.  Because the
golden traces are committed to the repository, any party who builds
the stack and runs this script can independently verify determinism
without needing a reference machine.

Usage:
    python3 scripts/ci/trace_repro_gate.py \
        --t81-bin build/t81 \
        --fixtures-dir tests/fixtures/t81lang_determinism \
        --traces-dir   tests/fixtures/canonical_traces \
        --workdir      build/trace-repro \
        [--hash-out    build/trace-repro/hash.txt]
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path


FIXTURE_NAMES = [
    "01_bigint_add",
    "02_fraction_sub",
    "03_float_literal",
    "05_bool_and_string",
    "08_bounded_loop_print",
]


def run(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, check=check)


def compile_fixture(t81_bin: Path, source: Path, out: Path) -> None:
    proc = run([str(t81_bin), "code", "build", str(source), "-o", str(out)], check=False)
    if proc.returncode != 0:
        raise RuntimeError(f"compile failed for {source.name}:\n{proc.stdout}")


def capture_trace(t81_bin: Path, tisc: Path, trace_out: Path) -> None:
    proc = run([str(t81_bin), "vm", "run", str(tisc), "--trace", "-o", str(trace_out)],
               check=False)
    if proc.returncode != 0:
        raise RuntimeError(f"vm run failed for {tisc.name}:\n{proc.stdout}")


def replay_trace(t81_bin: Path, tisc: Path, golden: Path) -> dict:
    proc = run([str(t81_bin), "trace", "replay", str(tisc), str(golden), "--json"], check=False)
    try:
        return json.loads(proc.stdout)
    except json.JSONDecodeError:
        raise RuntimeError(
            f"trace replay produced non-JSON output for {tisc.name}:\n{proc.stdout}"
        )


def main() -> int:
    ap = argparse.ArgumentParser(description="Trace Replay Gate")
    ap.add_argument("--t81-bin",      required=True)
    ap.add_argument("--fixtures-dir", required=True)
    ap.add_argument("--traces-dir",   required=True)
    ap.add_argument("--workdir",      required=True)
    ap.add_argument("--hash-out",     default=None,
                    help="Write aggregate SHA-256 of all verified golden traces here")
    args = ap.parse_args()

    t81_bin     = Path(args.t81_bin)
    fixtures    = Path(args.fixtures_dir)
    traces_dir  = Path(args.traces_dir)
    workdir     = Path(args.workdir)

    if not t81_bin.exists():
        print(f"[FAIL] t81 binary not found: {t81_bin}", file=sys.stderr)
        return 1
    if not fixtures.exists():
        print(f"[FAIL] fixtures dir not found: {fixtures}", file=sys.stderr)
        return 1
    if not traces_dir.exists():
        print(f"[FAIL] traces dir not found: {traces_dir}", file=sys.stderr)
        return 1

    workdir.mkdir(parents=True, exist_ok=True)

    passed = 0
    failed = 0
    aggregate = hashlib.sha256()

    for name in FIXTURE_NAMES:
        source = fixtures / f"{name}.t81"
        golden = traces_dir / f"{name}.trace"
        tisc   = workdir / f"{name}.tisc"
        live   = workdir / f"{name}.live.trace"

        if not source.exists():
            print(f"[SKIP] {name}: source not found ({source})")
            continue
        if not golden.exists():
            print(f"[FAIL] {name}: golden trace not found ({golden})", file=sys.stderr)
            failed += 1
            continue

        try:
            compile_fixture(t81_bin, source, tisc)
            capture_trace(t81_bin, tisc, live)
            result = replay_trace(t81_bin, tisc, golden)
        except RuntimeError as e:
            print(f"[FAIL] {name}: {e}", file=sys.stderr)
            failed += 1
            continue

        if result.get("ok"):
            entries = result.get("actual_entries", "?")
            print(f"[PASS] {name}: {entries} entries — bit-identical")
            # Fold the golden trace content into the aggregate hash so the
            # hash-out file represents the full verified fixture set.
            aggregate.update(name.encode("utf-8"))
            aggregate.update(b"\0")
            aggregate.update(golden.read_bytes())
            aggregate.update(b"\n")
            passed += 1
        else:
            kind  = result.get("kind", "unknown")
            idx   = result.get("mismatch_index")
            exp   = result.get("expected")
            actual = result.get("actual")
            msg = f"[FAIL] {name}: divergence ({kind})"
            if idx is not None:
                msg += f" at entry {idx}"
            if exp:
                msg += f"\n  expected: {exp}"
            if actual:
                msg += f"\n  actual:   {actual}"
            print(msg, file=sys.stderr)
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed out of {passed + failed} fixtures")

    if args.hash_out and passed > 0:
        hash_path = Path(args.hash_out)
        hash_path.parent.mkdir(parents=True, exist_ok=True)
        final_hash = aggregate.hexdigest()
        hash_path.write_text(final_hash + "\n", encoding="utf-8")
        print(f"Trace aggregate hash: {final_hash}")
        print(f"Written to: {hash_path}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
