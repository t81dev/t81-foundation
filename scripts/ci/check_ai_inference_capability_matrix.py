#!/usr/bin/env python3
"""Emit inference format/mode capability matrix for AI CLI runtime."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "t81.ai.inference-capability-matrix.v1"
EXPECTATION_SCHEMA_VERSION = "t81.ai.inference-capability-expectations.v1"
UNSUPPORTED_SENTINEL = "No backend supports requested format/mode"


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def effective_support_state(payload: dict[str, Any], requested_mode: str, fallback_status: str) -> tuple[str, str]:
    strict_core_eligible = bool(payload.get("strict_core_eligible", False))
    effective_class = str(payload.get("effective_determinism_class", "")).strip()
    if requested_mode == "strict_deterministic" and not strict_core_eligible:
        return (
            "unsupported",
            effective_class or "strict_deterministic request downgraded by runtime boundary",
        )
    return fallback_status, effective_class


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate AI inference format/mode capability matrix.")
    p.add_argument("--out-dir", required=True, help="Output directory for matrix artifacts")
    p.add_argument("--ai-bin", required=True, help="Path to t81_ai binary")
    p.add_argument("--model-file", required=True, help="Model file used for capability probes")
    p.add_argument("--prompt", default="Deterministic inference capability probe.", help="Probe prompt")
    p.add_argument(
        "--formats",
        default="gguf,t3k",
        help="Comma-separated inference formats to probe (default: gguf,t3k)",
    )
    p.add_argument(
        "--modes",
        default="strict_deterministic",
        help="Comma-separated inference modes to probe (default: strict_deterministic)",
    )
    p.add_argument(
        "--required",
        action="append",
        default=[],
        help="Required format:mode entry (repeatable). Defaults to none.",
    )
    p.add_argument(
        "--expectations-file",
        default="",
        help="Optional capability expectation contract JSON.",
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
    required = args.required if args.required else []
    expectations_file = (
        Path(args.expectations_file).resolve()
        if args.expectations_file
        else Path(__file__).resolve().with_name("ai_inference_capability_expectations.json")
    )

    matrix_entries: list[dict[str, Any]] = []
    errors: list[str] = []

    for fmt in formats:
        for mode in modes:
            probe_id = f"{fmt}_{mode}".replace("/", "_").replace(":", "_")
            out_json = out_dir / f"ai_inference_capability_{probe_id}.json"
            cmd = [
                str(ai_bin),
                "inference",
                "run",
                "--model",
                f"capability-{probe_id}",
                "--model-file",
                str(model_file),
                "--format",
                fmt,
                "--mode",
                mode,
                "--prompt",
                str(args.prompt),
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
                provisional = "supported" if payload_status == "pass" else "degraded"
                support_state, effective_class = effective_support_state(payload, mode, provisional)
                selected_backend = str(payload.get("selected_backend", ""))
            elif proc.returncode != 0 and UNSUPPORTED_SENTINEL in stderr:
                support_state = "unsupported"
                artifact_path = ""
                effective_class = ""
            else:
                support_state = "error"
                effective_class = ""

            matrix_entries.append(
                {
                    "format": fmt,
                    "mode": mode,
                    "support_state": support_state,
                    "return_code": proc.returncode,
                    "selected_backend": selected_backend,
                    "payload_status": payload_status,
                    "effective_determinism_class": effective_class,
                    "artifact": artifact_path,
                    "stdout_sha256": sha256_text(stdout),
                    "stderr_sha256": sha256_text(stderr),
                    "stderr_snippet": stderr.strip().splitlines()[-1] if stderr.strip() else "",
                }
            )

    observed_lookup = {f"{item['format']}:{item['mode']}": item["support_state"] for item in matrix_entries}
    for req in required:
        state = observed_lookup.get(req, "missing")
        if state != "supported":
            errors.append(f"required inference capability {req} not supported (state={state})")

    expectation_results: list[dict[str, str]] = []
    if expectations_file.exists():
        expectations_payload = parse_json(expectations_file)
        if expectations_payload.get("schema") != EXPECTATION_SCHEMA_VERSION:
            errors.append(
                "expectations schema mismatch: "
                f"{expectations_payload.get('schema')} != {EXPECTATION_SCHEMA_VERSION}"
            )
        entries = expectations_payload.get("expectations")
        if not isinstance(entries, list):
            errors.append("expectations file must include expectations[]")
        else:
            for item in entries:
                if not isinstance(item, dict):
                    errors.append("expectation entry must be object")
                    continue
                key = f"{str(item.get('format', '')).strip()}:{str(item.get('mode', '')).strip()}"
                expected = str(item.get("expected_support_state", "")).strip()
                enforcement = str(item.get("enforcement", "required")).strip() or "required"
                observed = observed_lookup.get(key, "missing")
                passed = observed == expected
                expectation_results.append(
                    {
                        "key": key,
                        "expected_support_state": expected,
                        "observed_support_state": observed,
                        "enforcement": enforcement,
                        "result": "pass" if passed else "fail",
                    }
                )
                if not passed and enforcement in {"required", "allowlist"}:
                    errors.append(
                        f"inference capability expectation mismatch for {key}: "
                        f"expected={expected}, observed={observed}, enforcement={enforcement}"
                    )
    else:
        errors.append(f"missing expectations file: {expectations_file}")

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
        "expectations_file": str(expectations_file),
        "expectation_results": expectation_results,
        "matrix": matrix_entries,
        "errors": errors,
    }

    json_path = out_dir / "ai_inference_capability_matrix.json"
    md_path = out_dir / "ai_inference_capability_matrix.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# AI Inference Capability Matrix",
        "",
        f"- schema: `{SCHEMA_VERSION}`",
        f"- status: `{status}`",
        f"- required: `{', '.join(required) if required else '(none)'}`",
        f"- expectations_file: `{expectations_file}`",
        "",
        "| Format | Mode | Support | Effective Class | Backend | RC |",
        "| :--- | :--- | :--- | :--- | :--- | :--- |",
    ]
    for item in matrix_entries:
        lines.append(
            f"| `{item['format']}` | `{item['mode']}` | `{item['support_state']}` | "
            f"`{item.get('effective_determinism_class') or '-'}` | "
            f"`{item['selected_backend'] or '-'}` | `{item['return_code']}` |"
        )
    if expectation_results:
        lines.extend(
            [
                "",
                "| Expectation | Expected | Observed | Enforcement | Result |",
                "| :--- | :--- | :--- | :--- | :--- |",
            ]
        )
        for item in expectation_results:
            lines.append(
                f"| `{item['key']}` | `{item['expected_support_state']}` | "
                f"`{item['observed_support_state']}` | `{item['enforcement']}` | `{item['result']}` |"
            )
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    print(f"ai inference capability matrix status: {status}")
    print(f"artifact: {json_path}")
    print(f"summary:  {md_path}")
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
