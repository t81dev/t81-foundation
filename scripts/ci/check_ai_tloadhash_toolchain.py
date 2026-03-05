#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


HASH_RE = re.compile(r"(sha3-256:[^\s]+)")
SCHEMA_VERSION = "t81.ai.tloadhash-toolchain.v1"


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd, text=True, capture_output=True, check=False)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate RFC-0025 canonize-tensor toolchain baseline.")
    p.add_argument("--t81-bin", required=True, help="Path to t81 binary")
    p.add_argument("--out-dir", required=True, help="Directory for artifacts")
    p.add_argument(
        "--input-file",
        default="",
        help="Optional input file. If .t81w uses canonize-tensor, otherwise canonize-file.",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    t81_bin = Path(args.t81_bin).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not t81_bin.exists():
        raise SystemExit(f"t81 binary not found: {t81_bin}")

    if args.input_file:
        fixture = Path(args.input_file).resolve()
    else:
        default_model = Path("tests/fixtures/llama_cpp_repro/model.gguf").resolve()
        if default_model.exists():
            fixture = default_model
        else:
            fixture = out_dir / "tensor_fixture.bin"
            fixture.write_bytes(b"t81-rfc0025-canonize-file-fixture\n")

    if not fixture.exists():
        raise SystemExit(f"input file not found: {fixture}")

    command = "canonize-tensor" if fixture.suffix.lower() == ".t81w" else "canonize-file"
    proc = run([str(t81_bin), command, str(fixture)], cwd=Path.cwd())
    if proc.returncode != 0:
        print(proc.stdout)
        print(proc.stderr, file=sys.stderr)
        raise SystemExit(f"canonize-tensor failed with rc={proc.returncode}")

    combined = (proc.stdout or "") + "\n" + (proc.stderr or "")
    m = HASH_RE.search(combined)
    if not m:
        raise SystemExit("could not find sha3-256 hash in canonize-tensor output")
    canon_hash = m.group(1)

    policy_text = (
        "(policy\n"
        "  (tier 1)\n"
        f"  (allowed-tensor-hashes [\"{canon_hash}\"]))\n"
    )
    policy_file = out_dir / "policy.apl"
    policy_file.write_text(policy_text, encoding="utf-8")

    payload = {
        "schema": SCHEMA_VERSION,
        "status": "pass",
        "fixture_file": str(fixture),
        "command": command,
        "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
        "canon_hash": canon_hash,
        "policy_file": str(policy_file),
        "canonize_stdout_sha256": hashlib.sha256((proc.stdout or "").encode("utf-8")).hexdigest(),
    }
    json_file = out_dir / "ai_tloadhash_toolchain.json"
    md_file = out_dir / "ai_tloadhash_toolchain.md"
    json_file.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    md_file.write_text(
        "\n".join(
            [
                "# TLOADHASH Toolchain Evidence",
                "",
                f"- schema: `{SCHEMA_VERSION}`",
                "- status: `pass`",
                f"- canon_hash: `{canon_hash}`",
                f"- policy_file: `{policy_file}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"tloadhash toolchain status: pass")
    print(f"canon_hash: {canon_hash}")
    print(f"artifact: {json_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
