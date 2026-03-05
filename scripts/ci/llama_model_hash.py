#!/usr/bin/env python3
"""Resolve the adapter-visible model hash for llama repro gating."""

from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from pathlib import Path

HASH_RE = re.compile(r"policy_violation hash=(sha3-512:[0-9a-f]+)")


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Generate expected model hash for llama repro gate using t81 llama-run probe"
    )
    ap.add_argument("model", help="Path to model.gguf")
    ap.add_argument("--t81-bin", required=True, help="Path to t81 binary built with llama support")
    ap.add_argument("--out", help="Optional output file (writes hash + newline)")
    args = ap.parse_args()

    model = Path(args.model)
    t81_bin = Path(args.t81_bin)
    if not model.exists():
        raise RuntimeError(f"missing model file: {model}")
    if not t81_bin.exists():
        raise RuntimeError(f"missing t81 binary: {t81_bin}")

    with tempfile.NamedTemporaryFile("w", suffix=".apl", delete=False) as tf:
        tf.write(
            "(policy\n"
            "  (tier 1)\n"
            "  (max-instructions 100000)\n"
            "  (max-stack 2048)\n"
            "  (allowed-tensor-hashes [\"sha3-512:0\"])\n"
            ")\n"
        )
        policy_path = Path(tf.name)

    cmd = [
        str(t81_bin),
        "llama-run",
        str(model),
        "hash probe",
        "--policy",
        str(policy_path),
        "--max-tokens",
        "1",
        "--seed",
        "0",
        "--threads",
        "1",
        "--temperature",
        "0",
        "--top-k",
        "1",
        "--top-p",
        "1.0",
    ]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    policy_path.unlink(missing_ok=True)

    match = HASH_RE.search(proc.stdout)
    if not match:
        print(proc.stdout)
        raise RuntimeError("could not resolve adapter hash from llama-run output")
    resolved = match.group(1)
    print(resolved)

    if args.out:
        out_path = Path(args.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(resolved + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
