#!/usr/bin/env python3
"""Run reproducibility checks for `t81 llama-run` token streams."""

from __future__ import annotations

import argparse
import hashlib
import re
import subprocess
from pathlib import Path

TOKEN_IDS_RE = re.compile(r"^token_ids_csv:(.*)$", re.MULTILINE)
MODEL_HASH_RE = re.compile(r"^model_hash:\s*(\S+)\s*$", re.MULTILINE)
PROMPT_HASH_RE = re.compile(r"^prompt_hash:\s*(\S+)\s*$", re.MULTILINE)


def run_once(
    t81_bin: Path,
    model: str,
    policy: Path,
    prompt: str,
    max_tokens: int,
    seed: int,
    threads: int,
    top_k: int,
    top_p: float,
    temperature: float,
    expected_model_hash: str | None,
) -> str:
    cmd = [
        str(t81_bin),
        "internal",
        "llama-run",
        model,
        prompt,
        "--policy",
        str(policy),
        "--max-tokens",
        str(max_tokens),
        "--seed",
        str(seed),
        "--threads",
        str(threads),
        "--top-k",
        str(top_k),
        "--top-p",
        str(top_p),
        "--temperature",
        str(temperature),
    ]
    if expected_model_hash:
        cmd.extend(["--expected-model-hash", expected_model_hash])

    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        print(proc.stdout)
        raise RuntimeError("llama-run failed")
    return proc.stdout


def extract_line(regex: re.Pattern[str], text: str, field: str) -> str:
    match = regex.search(text)
    if not match:
        raise RuntimeError(f"missing {field} in llama-run output")
    return match.group(1).strip()


def main() -> int:
    ap = argparse.ArgumentParser(description="Run llama.cpp deterministic repro gate")
    ap.add_argument("--t81-bin", required=True)
    ap.add_argument("--model", required=True)
    ap.add_argument("--policy", required=True)
    ap.add_argument("--prompt", required=True)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--max-tokens", type=int, default=64)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--top-k", type=int, default=1)
    ap.add_argument("--top-p", type=float, default=1.0)
    ap.add_argument("--temperature", type=float, default=0.0)
    ap.add_argument("--expected-model-hash", default=None)
    ap.add_argument("--hash-out", required=True)
    args = ap.parse_args()

    t81_bin = Path(args.t81_bin)
    model_spec = args.model
    policy = Path(args.policy)
    hash_out = Path(args.hash_out)

    if not t81_bin.exists():
        raise RuntimeError(f"missing t81 binary: {t81_bin}")
    if not model_spec.startswith("sha3-256:"):
        model = Path(model_spec)
        if not model.exists():
            raise RuntimeError(f"missing model: {model}")
    if not policy.exists():
        raise RuntimeError(f"missing policy: {policy}")
    if args.runs < 2:
        raise RuntimeError("--runs must be >= 2")

    outputs: list[str] = []
    for _ in range(args.runs):
        out = run_once(
            t81_bin=t81_bin,
            model=model_spec,
            policy=policy,
            prompt=args.prompt,
            max_tokens=args.max_tokens,
            seed=args.seed,
            threads=args.threads,
            top_k=args.top_k,
            top_p=args.top_p,
            temperature=args.temperature,
            expected_model_hash=args.expected_model_hash,
        )
        outputs.append(out)

    model_hash = extract_line(MODEL_HASH_RE, outputs[0], "model_hash")
    prompt_hash = extract_line(PROMPT_HASH_RE, outputs[0], "prompt_hash")
    token_ids = extract_line(TOKEN_IDS_RE, outputs[0], "token_ids_csv")

    for idx, out in enumerate(outputs[1:], start=2):
        mh = extract_line(MODEL_HASH_RE, out, "model_hash")
        ph = extract_line(PROMPT_HASH_RE, out, "prompt_hash")
        ids = extract_line(TOKEN_IDS_RE, out, "token_ids_csv")
        if mh != model_hash:
            raise RuntimeError(f"run {idx}: model_hash drift: {mh} != {model_hash}")
        if ph != prompt_hash:
            raise RuntimeError(f"run {idx}: prompt_hash drift: {ph} != {prompt_hash}")
        if ids != token_ids:
            raise RuntimeError(f"run {idx}: token_ids drift detected")

    aggregate = hashlib.sha256()
    aggregate.update(model_hash.encode("utf-8"))
    aggregate.update(b"\n")
    aggregate.update(prompt_hash.encode("utf-8"))
    aggregate.update(b"\n")
    aggregate.update(token_ids.encode("utf-8"))
    final_hash = aggregate.hexdigest()

    hash_out.parent.mkdir(parents=True, exist_ok=True)
    hash_out.write_text(final_hash + "\n", encoding="utf-8")

    print(
        "llama.cpp repro gate passed: "
        f"runs={args.runs} model_hash={model_hash} prompt_hash={prompt_hash} sha256={final_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
