#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


HASH256_RE = re.compile(r"(sha3-256:[^\s]+)")
SCHEMA = "t81.ai.governed-llama-flow.v1"


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd, text=True, capture_output=True, check=False)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Run governed llama flow: canonize-file + policy + llama-run.")
    p.add_argument("--t81-bin", required=True, help="Path to t81 binary (llama-enabled build)")
    p.add_argument("--model", required=True, help="Path to GGUF model")
    p.add_argument("--prompt", default="Say hello from governed inference.", help="Prompt text")
    p.add_argument("--out-dir", required=True, help="Output directory for artifacts")
    p.add_argument("--max-tokens", type=int, default=16)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--threads", type=int, default=1)
    p.add_argument("--temperature", type=float, default=0.0)
    p.add_argument("--top-k", type=int, default=1)
    p.add_argument("--top-p", type=float, default=1.0)
    p.add_argument(
        "--llama-hash-probe",
        default="scripts/ci/llama_model_hash.py",
        help="Path to canonical llama model-hash probe script",
    )
    return p.parse_args()


def sha256_text(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8")).hexdigest()


def probe_model_hash(t81_bin: Path, model: Path, probe_script: Path) -> str:
    probe = run([sys.executable, str(probe_script), "--t81-bin", str(t81_bin), str(model)], cwd=Path.cwd())
    if probe.returncode != 0:
        raise SystemExit(f"llama_model_hash probe failed rc={probe.returncode}: {probe.stderr.strip()}")
    out = (probe.stdout or "").strip()
    if not out.startswith("sha3-512:"):
        raise SystemExit(f"llama_model_hash probe returned unexpected output: {out}")
    return out


def main() -> int:
    args = parse_args()
    root = Path.cwd()
    t81 = Path(args.t81_bin).resolve()
    model = Path(args.model).resolve()
    out = Path(args.out_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)

    if not t81.exists():
        raise SystemExit(f"t81 binary not found: {t81}")
    if not model.exists():
        raise SystemExit(f"model not found: {model}")

    canon = run([str(t81), "canonize-file", str(model)], cwd=root)
    (out / "canonize.stdout.log").write_text(canon.stdout or "", encoding="utf-8")
    (out / "canonize.stderr.log").write_text(canon.stderr or "", encoding="utf-8")
    if canon.returncode != 0:
        raise SystemExit(f"canonize-file failed rc={canon.returncode}")

    m = HASH256_RE.search((canon.stdout or "") + "\n" + (canon.stderr or ""))
    if not m:
        raise SystemExit("failed to parse sha3-256 hash from canonize-file output")
    canon_hash = m.group(1)
    probe_script = Path(args.llama_hash_probe).resolve()
    if not probe_script.exists():
        raise SystemExit(f"llama hash probe script not found: {probe_script}")
    model_hash = probe_model_hash(t81, model, probe_script)

    policy = out / "policy.apl"
    policy.write_text(
        "\n".join(
            [
                "(policy",
                "  (tier 1)",
                f"  (allowed-tensor-hashes [\"{model_hash}\"]))",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    llama_cmd = [
        str(t81),
        "llama-run",
        str(model),
        args.prompt,
        "--policy",
        str(policy),
        "--max-tokens",
        str(args.max_tokens),
        "--seed",
        str(args.seed),
        "--threads",
        str(args.threads),
        "--temperature",
        str(args.temperature),
        "--top-k",
        str(args.top_k),
        "--top-p",
        str(args.top_p),
        "--expected-model-hash",
        model_hash,
    ]
    ll = run(llama_cmd, cwd=root)
    (out / "llama_run.stdout.log").write_text(ll.stdout or "", encoding="utf-8")
    (out / "llama_run.stderr.log").write_text(ll.stderr or "", encoding="utf-8")

    payload = {
        "schema": SCHEMA,
        "status": "pass" if ll.returncode == 0 else "fail",
        "model": str(model),
        "canon_hash_sha3_256": canon_hash,
        "model_hash": model_hash,
        "policy_file": str(policy),
        "command": llama_cmd,
        "llama_returncode": ll.returncode,
        "llama_stdout_sha256": sha256_text(ll.stdout or ""),
        "llama_stderr_sha256": sha256_text(ll.stderr or ""),
    }
    (out / "governed_llama_flow.json").write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

    print(f"governed llama flow status: {payload['status']}")
    print(f"model_hash: {model_hash}")
    print(f"artifact: {(out / 'governed_llama_flow.json')}")
    return 0 if ll.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
