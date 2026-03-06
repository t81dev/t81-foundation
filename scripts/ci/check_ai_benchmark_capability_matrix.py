#!/usr/bin/env python3
"""Emit benchmark format/mode capability matrix for AI CLI runtime."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.benchmark-capability-matrix.v1"
UNSUPPORTED_SENTINEL = "No backend supports requested format/mode"


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI benchmark format/mode capability matrix.")
    p.add_argument("--out-dir", required=True, help="Output directory for matrix artifacts")
    p.add_argument("--ai-bin", required=True, help="Path to t81_ai binary")
    p.add_argument("--model-file", required=True, help="Model file used for capability probes")
    p.add_argument(
        "--formats",
        default="gguf,t3k",
        help="Comma-separated benchmark formats to probe (default: gguf,t3k)",
    )
    p.add_argument(
        "--modes",
        default="strict_deterministic",
        help="Comma-separated benchmark modes to probe (default: strict_deterministic)",
    )
    p.add_argument(
        "--required",
        action="append",
        default=[],
        help="Required format:mode entry (repeatable). Defaults to gguf:strict_deterministic.",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    ai_bin = Path(args.ai_bin).resolve()
    if not ai_bin.exists():
        raise SystemExit(f"AI binary not found: {ai_bin}")

    model_file = Path(args.model_file).resolve()
    if not model_file.exists():
        raise SystemExit(f"model file not found: {model_file}")

    formats = [item.strip() for item in str(args.formats).split(",") if item.strip()]
    modes = [item.strip() for item in str(args.modes).split(",") if item.strip()]
    if not formats or not modes:
        raise SystemExit("formats and modes must be non-empty")
    required = args.required if args.required else ["gguf:strict_deterministic"]

    matrix_entries: list[dict[str, Any]] = []
    errors: list[str] = []

    for fmt in formats:
        for mode in modes:
            probe_id = f"{fmt}_{mode}".replace("/", "_").replace(":", "_")
            out_json = out_dir / f"ai_benchmark_capability_{probe_id}.json"
            cmd = [
                str(ai_bin),
                "benchmark",
                "run",
                "--model",
                f"capability-{probe_id}",
                "--model-file",
                str(model_file),
                "--format",
                fmt,
                "--mode",
                mode,
                "--out",
                str(out_json),
            ]
            proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
            stdout = proc.stdout
            stderr = proc.stderr
            support_state = "unknown"
            payload_status = ""
            selected_backend = ""
            artifact_path = str(out_json)
            if proc.returncode == 0 and out_json.exists():
                payload = parse_json(out_json)
                payload_status = str(payload.get("status", ""))
                selected_backend = str(payload.get("selected_backend", ""))
                support_state = "supported" if payload_status == "pass" else "degraded"
            elif proc.returncode != 0 and UNSUPPORTED_SENTINEL in stderr:
                support_state = "unsupported"
                artifact_path = ""
            else:
                support_state = "error"

            matrix_entries.append(
                {
                    "format": fmt,
                    "mode": mode,
                    "support_state": support_state,
                    "return_code": proc.returncode,
                    "selected_backend": selected_backend,
                    "payload_status": payload_status,
                    "artifact": artifact_path,
                    "stdout_sha256": sha256_text(stdout),
                    "stderr_sha256": sha256_text(stderr),
                    "stderr_snippet": stderr.strip().splitlines()[-1] if stderr.strip() else "",
                }
            )

    required_lookup = {
        f"{entry['format']}:{entry['mode']}": entry.get("support_state", "unknown") for entry in matrix_entries
    }
    for req in required:
        state = required_lookup.get(req, "missing")
        if state != "supported":
            errors.append(f"required benchmark capability {req} not supported (state={state})")

    status = "pass" if not errors else "fail"
    payload = {
        "schema": SCHEMA_VERSION,
        "status": status,
        "generated_at": datetime.now(UTC).isoformat(),
        "ai_bin": str(ai_bin),
        "model_file": str(model_file),
        "formats": formats,
        "modes": modes,
        "required": required,
        "matrix": matrix_entries,
        "errors": errors,
    }

    json_path = out_dir / "ai_benchmark_capability_matrix.json"
    md_path = out_dir / "ai_benchmark_capability_matrix.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# AI Benchmark Capability Matrix",
        "",
        f"- schema: `{SCHEMA_VERSION}`",
        f"- status: `{status}`",
        f"- required: `{', '.join(required)}`",
        "",
        "| Format | Mode | Support | Backend | RC |",
        "| :--- | :--- | :--- | :--- | :--- |",
    ]
    for item in matrix_entries:
        lines.append(
            f"| `{item['format']}` | `{item['mode']}` | `{item['support_state']}` | "
            f"`{item['selected_backend'] or '-'}` | `{item['return_code']}` |"
        )
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai benchmark capability matrix status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
