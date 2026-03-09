#!/usr/bin/env python3
"""Run T81Lang compile reproducibility gates over deterministic fixtures."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
from pathlib import Path


def compile_source(t81_bin: Path, source: Path, out_path: Path) -> None:
    cmd = [str(t81_bin), "code", "build", str(source), "-o", str(out_path)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        print(proc.stdout)
        raise RuntimeError(f"compile failed for {source}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Run T81Lang deterministic compile gates")
    ap.add_argument("--t81-bin", required=True)
    ap.add_argument("--fixtures-dir", required=True)
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--hash-out", required=True)
    ap.add_argument(
        "--expected-hash-file",
        default=None,
        help="Optional path to a checked-in expected aggregate hash; mismatches fail the gate.",
    )
    args = ap.parse_args()

    t81_bin = Path(args.t81_bin)
    fixtures_dir = Path(args.fixtures_dir)
    workdir = Path(args.workdir)
    hash_out = Path(args.hash_out)

    if not t81_bin.exists():
        raise RuntimeError(f"missing t81 binary: {t81_bin}")
    if not fixtures_dir.exists():
        raise RuntimeError(f"missing fixtures directory: {fixtures_dir}")

    workdir.mkdir(parents=True, exist_ok=True)
    fixture_paths = sorted(fixtures_dir.glob("*.t81"))
    if len(fixture_paths) < 5:
        raise RuntimeError("expected at least 5 determinism fixtures")

    aggregate = hashlib.sha256()
    for fixture in fixture_paths:
        stem = fixture.stem
        pass_a = workdir / f"{stem}.pass_a.tisc"
        pass_b = workdir / f"{stem}.pass_b.tisc"

        compile_source(t81_bin, fixture, pass_a)
        compile_source(t81_bin, fixture, pass_b)

        bytes_a = pass_a.read_bytes()
        bytes_b = pass_b.read_bytes()
        if bytes_a != bytes_b:
            raise RuntimeError(f"bytecode mismatch across compile passes for {fixture.name}")

        fixture_hash = hashlib.sha256(bytes_a).hexdigest()
        aggregate.update(fixture.name.encode("utf-8"))
        aggregate.update(b"\0")
        aggregate.update(fixture_hash.encode("ascii"))
        aggregate.update(b"\n")

    final_hash = aggregate.hexdigest()
    hash_out.parent.mkdir(parents=True, exist_ok=True)
    hash_out.write_text(final_hash + "\n", encoding="utf-8")

    if args.expected_hash_file:
        expected_path = Path(args.expected_hash_file)
        if not expected_path.exists():
            raise RuntimeError(f"missing expected hash file: {expected_path}")
        expected_hash = expected_path.read_text(encoding="utf-8").strip()
        if expected_hash != final_hash:
            raise RuntimeError(
                "unexpected t81lang reproducibility hash drift: "
                f"expected={expected_hash} actual={final_hash}"
            )

    print(f"T81Lang gates passed: fixtures={len(fixture_paths)} hash={final_hash}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
