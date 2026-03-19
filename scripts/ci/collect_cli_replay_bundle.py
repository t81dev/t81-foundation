#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.cli.replay-bundle.v1"


@dataclass
class CommandResult:
    name: str
    argv: list[str]
    rc: int
    expected_rc: int
    rc_match_expected: bool
    duration_ms: int
    stdout_sha256: str
    stderr_sha256: str
    combined_sha256: str
    artifacts: dict[str, dict[str, Any]]


def sha256_hex_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def canonical_json_bytes(obj: Any) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def run_command(
    name: str,
    argv: list[str],
    cwd: Path,
    expected_rc: int = 0,
    artifact_paths: dict[str, Path] | None = None,
) -> tuple[CommandResult, bytes, bytes]:
    started = time.perf_counter()
    proc = subprocess.run(argv, cwd=cwd, capture_output=True, text=False, check=False)
    duration_ms = int((time.perf_counter() - started) * 1000)
    stdout = proc.stdout or b""
    stderr = proc.stderr or b""

    artifacts: dict[str, dict[str, Any]] = {}
    for label, path in (artifact_paths or {}).items():
        resolved = path.resolve()
        artifacts[label] = {
            "path": str(resolved),
            "exists": resolved.exists(),
        }
        if resolved.exists():
            artifacts[label]["sha256"] = sha256_file(resolved)
            artifacts[label]["size_bytes"] = resolved.stat().st_size

    return (
        CommandResult(
            name=name,
            argv=argv,
            rc=proc.returncode,
            expected_rc=expected_rc,
            rc_match_expected=(proc.returncode == expected_rc),
            duration_ms=duration_ms,
            stdout_sha256=sha256_hex_bytes(stdout),
            stderr_sha256=sha256_hex_bytes(stderr),
            combined_sha256=sha256_hex_bytes(stdout + b"\n---stderr---\n" + stderr),
            artifacts=artifacts,
        ),
        stdout,
        stderr,
    )


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Collect replayable evidence bundle for core t81 governed CLI workflows.")
    p.add_argument("--t81-bin", required=True, help="Path to the main t81 binary")
    p.add_argument("--out-dir", required=True, help="Output directory for artifacts and bundle")
    p.add_argument("--runs", type=int, default=2, help="Number of repeated bundle runs")
    p.add_argument(
        "--program",
        default="examples/hello_world.t81",
        help="T81 source fixture used for build/run/determinism/trace/canonfs checks",
    )
    p.add_argument(
        "--model-fixture",
        default="tests/fixtures/llama_cpp_repro/model.gguf",
        help="Model fixture used for AI workflow checks",
    )
    return p.parse_args()


def ensure_exists(path: Path, label: str) -> Path:
    resolved = path.resolve()
    if not resolved.exists():
        raise SystemExit(f"{label} not found: {resolved}")
    return resolved


def extract_canonfs_hash(stdout: bytes) -> str:
    text = stdout.decode("utf-8", errors="replace").strip()
    if not text.startswith("sha3-256:"):
        raise RuntimeError(f"unexpected canonfs hash output: {text!r}")
    return text


def sanitize_command(result: CommandResult) -> dict[str, Any]:
    return {
        "name": result.name,
        "argv": result.argv,
        "rc": result.rc,
        "expected_rc": result.expected_rc,
        "rc_match_expected": result.rc_match_expected,
        "duration_ms": result.duration_ms,
        "stdout_sha256": result.stdout_sha256,
        "stderr_sha256": result.stderr_sha256,
        "combined_sha256": result.combined_sha256,
        "artifacts": result.artifacts,
    }


def determinism_projection(command: dict[str, Any]) -> dict[str, Any]:
    return {
        "name": command["name"],
        "rc": command["rc"],
        "expected_rc": command["expected_rc"],
        "rc_match_expected": command["rc_match_expected"],
        "stdout_sha256": command["stdout_sha256"],
        "stderr_sha256": command["stderr_sha256"],
        "combined_sha256": command["combined_sha256"],
        "artifacts": {
            label: {
                "exists": payload.get("exists", False),
                "sha256": payload.get("sha256", ""),
                "size_bytes": payload.get("size_bytes", 0),
            }
            for label, payload in sorted(command["artifacts"].items())
        },
    }


def validate_determinism(runs: list[dict[str, Any]]) -> tuple[bool, list[str]]:
    if len(runs) < 2:
        return True, []
    mismatches: list[str] = []
    baseline = [determinism_projection(cmd) for cmd in runs[0]["commands"]]
    for run in runs[1:]:
        candidate = [determinism_projection(cmd) for cmd in run["commands"]]
        if candidate != baseline:
            for idx, (base_cmd, cand_cmd) in enumerate(zip(baseline, candidate), start=1):
                if base_cmd != cand_cmd:
                    mismatches.append(f"run#{run['index']}:command#{idx}:{cand_cmd['name']}")
    return len(mismatches) == 0, mismatches


def run_bundle_once(
    t81_bin: Path,
    program: Path,
    model_fixture: Path,
    out_dir: Path,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    work_dir = out_dir / "work"
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)

    build_dir = work_dir / "build"
    ai_dir = work_dir / "ai"
    canonfs_root = work_dir / "canonfs"
    output_dir = work_dir / "outputs"
    for path in (build_dir, ai_dir, canonfs_root, output_dir):
        path.mkdir(parents=True, exist_ok=True)

    program_out = build_dir / "hello.tisc"
    trace_out = output_dir / "hello.trace"
    canonfs_get_out = output_dir / "hello_world_from_canonfs.t81"
    ai_inference_out = ai_dir / "inference_strict.json"

    commands: list[dict[str, Any]] = []

    result, stdout, _ = run_command(
        "code_build",
        [str(t81_bin), "code", "build", str(program), "-o", str(program_out)],
        cwd=Path.cwd(),
        artifact_paths={"program_tisc": program_out},
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "code_run_trace",
        [str(t81_bin), "code", "run", str(program_out), "--trace", "-o", str(trace_out)],
        cwd=Path.cwd(),
        artifact_paths={"trace": trace_out},
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "determinism_verify_run",
        [str(t81_bin), "determinism", "verify-run", str(program_out), "--json"],
        cwd=Path.cwd(),
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "trace_replay",
        [str(t81_bin), "trace", "replay", str(program_out), str(trace_out), "--json"],
        cwd=Path.cwd(),
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "canonfs_put_file",
        [str(t81_bin), "canonfs", "put-file", str(program), "--canonfs-root", str(canonfs_root)],
        cwd=Path.cwd(),
    )
    canonfs_hash = extract_canonfs_hash(stdout)
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "canonfs_verify",
        [str(t81_bin), "canonfs", "verify", canonfs_hash, "--json", "--canonfs-root", str(canonfs_root)],
        cwd=Path.cwd(),
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "canonfs_get",
        [str(t81_bin), "canonfs", "get", canonfs_hash, "-o", str(canonfs_get_out), "--json", "--canonfs-root", str(canonfs_root)],
        cwd=Path.cwd(),
        artifact_paths={"retrieved_source": canonfs_get_out},
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "ai_model_inspect",
        [str(t81_bin), "ai", "model", "inspect", str(model_fixture)],
        cwd=Path.cwd(),
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "ai_verify_determinism",
        [str(t81_bin), "ai", "verify", "determinism", str(model_fixture)],
        cwd=Path.cwd(),
    )
    commands.append(sanitize_command(result))

    result, stdout, _ = run_command(
        "ai_inference_run_strict",
        [
            str(t81_bin),
            "ai",
            "inference",
            "run",
            "--model",
            "rfc00a1-fixture",
            "--model-file",
            str(model_fixture),
            "--mode",
            "strict_deterministic",
            "--prompt",
            "replay bundle prompt",
            "--out",
            str(ai_inference_out),
        ],
        cwd=Path.cwd(),
        artifact_paths={"ai_inference": ai_inference_out},
    )
    commands.append(sanitize_command(result))

    checks = {
        "canonfs_hash": canonfs_hash,
        "canonfs_roundtrip_matches_source": sha256_file(program) == sha256_file(canonfs_get_out),
        "tisc_artifact_present": program_out.exists(),
        "trace_artifact_present": trace_out.exists(),
        "ai_inference_artifact_present": ai_inference_out.exists(),
    }
    return commands, checks


def main() -> int:
    args = parse_args()
    t81_bin = ensure_exists(Path(args.t81_bin), "t81 binary")
    program = ensure_exists(Path(args.program), "program fixture")
    model_fixture = ensure_exists(Path(args.model_fixture), "model fixture")
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.runs < 1:
        raise SystemExit("--runs must be >= 1")

    runs: list[dict[str, Any]] = []
    for idx in range(args.runs):
        commands, checks = run_bundle_once(t81_bin, program, model_fixture, out_dir)
        runs.append({"index": idx + 1, "commands": commands, "checks": checks})

    expected_rc_match = all(cmd["rc_match_expected"] for run in runs for cmd in run["commands"])
    deterministic_outputs, mismatches = validate_determinism(runs)
    roundtrip_ok = all(run["checks"]["canonfs_roundtrip_matches_source"] for run in runs)
    status = "pass" if expected_rc_match and deterministic_outputs and roundtrip_ok else "fail"

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
            "t81_bin": str(t81_bin),
            "t81_bin_sha256": sha256_file(t81_bin),
            "program_fixture": str(program),
            "program_fixture_sha256": sha256_file(program),
            "model_fixture": str(model_fixture),
            "model_fixture_sha256": sha256_file(model_fixture),
            "runs": args.runs,
        },
        "checks": {
            "expected_rc_match": expected_rc_match,
            "deterministic_outputs": deterministic_outputs,
            "canonfs_roundtrip_matches_source": roundtrip_ok,
            "status": status,
            "mismatches": mismatches,
        },
        "runs": runs,
    }

    canonical_payload = {
        "schema": bundle["schema"],
        "inputs": bundle["inputs"],
        "checks": bundle["checks"],
        "runs": [
            {
                "index": run["index"],
                "checks": run["checks"],
                "commands": [determinism_projection(cmd) for cmd in run["commands"]],
            }
            for run in runs
        ],
    }
    bundle["bundle_sha256"] = sha256_hex_bytes(canonical_json_bytes(canonical_payload))

    bundle_path = out_dir / "cli_replay_bundle.json"
    hash_path = out_dir / "cli_replay_bundle.sha256"
    summary_path = out_dir / "cli_replay_bundle.md"

    bundle_path.write_text(json.dumps(bundle, indent=2, sort_keys=True), encoding="utf-8")
    hash_path.write_text(f"{bundle['bundle_sha256']}  cli_replay_bundle.json\n", encoding="utf-8")
    summary_path.write_text(
        "\n".join(
            [
                "# CLI Replay Bundle",
                "",
                f"- schema: `{bundle['schema']}`",
                f"- status: `{status}`",
                f"- deterministic_outputs: `{deterministic_outputs}`",
                f"- expected_rc_match: `{expected_rc_match}`",
                f"- canonfs_roundtrip_matches_source: `{roundtrip_ok}`",
                f"- bundle_sha256: `{bundle['bundle_sha256']}`",
                "",
            ]
        ),
        encoding="utf-8",
    )

    print(f"cli replay bundle status: {status}")
    print(f"bundle:  {bundle_path}")
    print(f"hash:    {hash_path}")
    print(f"summary: {summary_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
