#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.evidence.v1"


@dataclass
class CommandSpec:
    name: str
    argv: list[str]
    expected_rc: int


def sha256_hex_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def run_command(cmd: list[str], cwd: Path) -> dict[str, Any]:
    started = time.perf_counter()
    proc = subprocess.run(cmd, cwd=cwd, capture_output=True, text=False, check=False)
    duration_ms = int((time.perf_counter() - started) * 1000)
    stdout = proc.stdout or b""
    stderr = proc.stderr or b""
    return {
        "argv": cmd,
        "rc": proc.returncode,
        "duration_ms": duration_ms,
        "stdout_sha256": sha256_hex_bytes(stdout),
        "stderr_sha256": sha256_hex_bytes(stderr),
        "combined_sha256": sha256_hex_bytes(stdout + b"\n---stderr---\n" + stderr),
    }


def canonical_json_bytes(obj: Any) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def validate_determinism(runs: list[dict[str, Any]]) -> tuple[bool, list[str]]:
    if len(runs) < 2:
        return True, []
    mismatches: list[str] = []
    base = runs[0]["commands"]
    for ridx, run in enumerate(runs[1:], start=2):
        for cidx, cmd in enumerate(run["commands"]):
            baseline = base[cidx]
            if cmd["rc"] != baseline["rc"]:
                mismatches.append(f"run#{ridx}:{cmd['name']}:rc {cmd['rc']} != {baseline['rc']}")
            if cmd["combined_sha256"] != baseline["combined_sha256"]:
                mismatches.append(
                    f"run#{ridx}:{cmd['name']}:output hash {cmd['combined_sha256']} != {baseline['combined_sha256']}"
                )
    return len(mismatches) == 0, mismatches


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Collect canonical AI CLI determinism evidence bundle.")
    p.add_argument("--ai-bin", required=True, help="Path to t81_ai executable")
    p.add_argument("--out-dir", required=True, help="Output directory for evidence artifacts")
    p.add_argument("--runs", type=int, default=3, help="Number of repeated runs for determinism checking")
    p.add_argument(
        "--model-fixture",
        default="",
        help="Optional GGUF fixture path for fixture-locked replay vectors.",
    )
    return p.parse_args()


def resolve_model_fixture(out_dir: Path, arg_value: str) -> tuple[Path, str]:
    if arg_value:
        p = Path(arg_value).resolve()
        return p, "user"

    repo_root = Path(__file__).resolve().parents[2]
    preferred = repo_root / "tests/fixtures/llama_cpp_repro/model.gguf"
    if preferred.exists():
        return preferred, "tests_fixture"

    generated = out_dir / "test_model.gguf"
    generated.write_text("t81-ai-evidence-fixture\n", encoding="utf-8")
    return generated, "generated"


def main() -> int:
    args = parse_args()
    ai_bin = Path(args.ai_bin).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")
    if args.runs < 1:
        raise SystemExit("--runs must be >= 1")

    model_path, model_source = resolve_model_fixture(out_dir, args.model_fixture)
    if not model_path.exists():
        raise SystemExit(f"model fixture not found: {model_path}")

    missing_path = out_dir / "missing.gguf"
    if missing_path.exists():
        missing_path.unlink()

    replay_path = out_dir / "ai_evidence_replay_vector.json"

    specs = [
        CommandSpec("help", [str(ai_bin), "--help"], 0),
        CommandSpec("model_inspect", [str(ai_bin), "model", "inspect", str(model_path)], 0),
        CommandSpec("verify_existing", [str(ai_bin), "verify", str(model_path)], 0),
        CommandSpec("verify_determinism_existing", [str(ai_bin), "verify", "determinism", str(model_path)], 0),
        CommandSpec(
            "inference_vector",
            [
                str(ai_bin),
                "inference",
                "run",
                "--model",
                "rfc00a1-fixture",
                "--model-file",
                str(model_path),
                "--prompt",
                "rfc00a1 vector prompt",
            ],
            0,
        ),
        CommandSpec(
            "workflow_run_vector",
            [str(ai_bin), "workflow", "run", "rfc00a1-vector", "--seed", "0", "--out", str(replay_path)],
            0,
        ),
        CommandSpec("workflow_replay_vector", [str(ai_bin), "workflow", "replay", str(replay_path)], 0),
        CommandSpec("workflow_report_vector", [str(ai_bin), "workflow", "report", str(replay_path)], 0),
        CommandSpec("verify_missing", [str(ai_bin), "verify", str(missing_path)], 1),
    ]

    runs: list[dict[str, Any]] = []
    for run_idx in range(args.runs):
        cmd_results = []
        for spec in specs:
            result = run_command(spec.argv, cwd=Path.cwd())
            result["name"] = spec.name
            result["expected_rc"] = spec.expected_rc
            result["rc_match_expected"] = result["rc"] == spec.expected_rc
            cmd_results.append(result)
        runs.append({"index": run_idx + 1, "commands": cmd_results})

    rc_ok = all(cmd["rc_match_expected"] for run in runs for cmd in run["commands"])
    deterministic, mismatches = validate_determinism(runs)
    status = "pass" if rc_ok and deterministic else "fail"

    bundle = {
        "schema": SCHEMA_VERSION,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "meta": {
            "os": platform.platform(),
            "python": platform.python_version(),
            "repo": os.environ.get("GITHUB_REPOSITORY", "local"),
            "commit": os.environ.get("GITHUB_SHA", "local"),
            "event": os.environ.get("GITHUB_EVENT_NAME", "local"),
            "workflow": os.environ.get("GITHUB_WORKFLOW", "local"),
            "run_id": os.environ.get("GITHUB_RUN_ID", "local"),
        },
        "inputs": {
            "ai_bin": str(ai_bin),
            "ai_bin_sha256": sha256_file(ai_bin),
            "model_fixture": str(model_path),
            "model_fixture_source": model_source,
            "model_fixture_sha256": sha256_file(model_path),
            "replay_vector_artifact": str(replay_path),
            "runs": args.runs,
        },
        "checks": {
            "expected_rc_match": rc_ok,
            "deterministic_outputs": deterministic,
            "status": status,
            "mismatches": mismatches,
        },
        "runs": runs,
    }
    if replay_path.exists():
        bundle["fixture_locked_replay_vector"] = {
            "artifact_path": str(replay_path),
            "artifact_sha256": sha256_file(replay_path),
        }

    # Canonical hash excludes timestamp only.
    canonical_payload = dict(bundle)
    canonical_payload.pop("generated_at_utc", None)
    bundle["bundle_sha256"] = sha256_hex_bytes(canonical_json_bytes(canonical_payload))

    bundle_path = out_dir / "ai_evidence_bundle.json"
    hash_path = out_dir / "ai_evidence_bundle.sha256"
    summary_path = out_dir / "ai_evidence_summary.md"

    bundle_path.write_text(json.dumps(bundle, indent=2, sort_keys=True), encoding="utf-8")
    hash_path.write_text(f"{bundle['bundle_sha256']}  ai_evidence_bundle.json\n", encoding="utf-8")
    summary_path.write_text(
        "\n".join(
            [
                "# AI Evidence Bundle",
                "",
                f"- schema: `{bundle['schema']}`",
                f"- status: `{status}`",
                f"- deterministic_outputs: `{deterministic}`",
                f"- expected_rc_match: `{rc_ok}`",
                f"- bundle_sha256: `{bundle['bundle_sha256']}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"ai evidence status: {status}")
    print(f"bundle: {bundle_path}")
    print(f"hash:   {hash_path}")
    print(f"summary:{summary_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
