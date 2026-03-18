#!/usr/bin/env python3
"""Validate machine-output JSON schema contracts for t81 CLI."""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path


def run_cmd(args: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=cwd, text=True, capture_output=True, check=False)


def ensure_json_object(output: str, context: str) -> dict:
    try:
        value = json.loads(output)
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{context}: invalid JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise RuntimeError(f"{context}: expected JSON object")
    return value


def ensure_json_array(output: str, context: str) -> list:
    try:
        value = json.loads(output)
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{context}: invalid JSON: {exc}") from exc
    if not isinstance(value, list):
        raise RuntimeError(f"{context}: expected JSON array")
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--t81-bin",
        type=Path,
        default=Path("build/t81"),
        help="Path to t81 binary.",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Repository root used as command cwd.",
    )
    args = parser.parse_args()

    t81_bin = args.t81_bin.resolve()
    repo_root = args.repo_root.resolve()
    if not t81_bin.exists():
        print(f"error: missing t81 binary: {t81_bin}")
        return 1

    failures: list[str] = []

    def check_object_schema(
        cmd: list[str], expected_schema: str, context: str, allowed_exits: set[int] | None = None
    ) -> None:
        if allowed_exits is None:
            allowed_exits = {0, 2}
        proc = run_cmd([str(t81_bin), *cmd], cwd=repo_root)
        if proc.returncode not in allowed_exits:
            failures.append(f"{context}: unexpected exit={proc.returncode}")
            return
        try:
            obj = ensure_json_object(proc.stdout, context)
        except RuntimeError as exc:
            failures.append(str(exc))
            return
        if obj.get("schema") != expected_schema:
            failures.append(
                f"{context}: schema mismatch (got={obj.get('schema')}, expected={expected_schema})"
            )

    check_object_schema(["env", "doctor", "--json"], "t81.doctor.v1", "env doctor --json")
    check_object_schema(
        ["code", "test", "--json", "--list", "--build-dir", str(t81_bin.parent)],
        "t81.test.v1",
        "code test --json --list",
    )

    with tempfile.TemporaryDirectory(prefix="t81-cli-json-contracts-") as temp_dir:
        temp_path = Path(temp_dir)

        # fmt
        fmt_file = temp_path / "sample.t81"
        fmt_file.write_text(
            "fn main() -> i32 {\nprint(\"x\");   \nreturn 0;\n}\n",
            encoding="utf-8",
        )
        check_object_schema(
            ["code", "fmt", "--json", str(fmt_file)],
            "t81.fmt.v1",
            "code fmt --json",
        )

        # pkg check
        manifest = temp_path / "package.t81"
        manifest.write_text(
            '(package\n  (name "ok_pkg")\n  (version "1.2.3")\n)\n',
            encoding="utf-8",
        )
        check_object_schema(
            ["internal", "pkg", "check", str(manifest), "--json"],
            "t81.pkg-check.v1",
            "internal pkg check --json",
        )

        # policy run
        policy_file = repo_root / "examples/system_integration.apl"
        if policy_file.exists():
            check_object_schema(
                ["policy", "run", str(policy_file), "--json"],
                "t81.policy-run.v1",
                "policy run --json",
            )
        else:
            failures.append("policy run --json: missing examples/system_integration.apl")

        # weights info (error-path contract for schema stability)
        check_object_schema(
            ["weights", "info", str(temp_path / "missing_fixture.t81w"), "--json"],
            "t81.weights-info.v1",
            "weights info --json",
            allowed_exits={1},
        )

        # trace export
        trace_file = temp_path / "trace.txt"
        trace_file.write_text("PC=0 NOP\n", encoding="utf-8")
        proc = run_cmd(
            [str(t81_bin), "trace", "export", str(trace_file), "--format", "json"], cwd=repo_root
        )
        if proc.returncode != 0:
            failures.append(f"trace export --format json: unexpected exit={proc.returncode}")
        else:
            try:
                arr = ensure_json_array(proc.stdout, "trace export --format json")
            except RuntimeError as exc:
                failures.append(str(exc))
                arr = []
            if not arr:
                failures.append("trace export --format json: empty array")
            elif not isinstance(arr[0], dict) or arr[0].get("schema") != "t81.trace-export-entry.v1":
                failures.append(
                    "trace export --format json: missing entry schema t81.trace-export-entry.v1"
                )

        # feedback submit/report
        feedback_path = temp_path / "feedback.jsonl"
        submit = run_cmd(
            [
                str(t81_bin),
                "feedback",
                "submit",
                "--rating",
                "4",
                "--note",
                "ci",
                "--path",
                str(feedback_path),
            ],
            cwd=repo_root,
        )
        if submit.returncode != 0:
            failures.append(f"feedback submit: unexpected exit={submit.returncode}")
        report = run_cmd(
            [str(t81_bin), "feedback", "report", "--path", str(feedback_path)], cwd=repo_root
        )
        if report.returncode != 0:
            failures.append(f"feedback report: unexpected exit={report.returncode}")
        else:
            try:
                obj = ensure_json_object(report.stdout, "feedback report")
            except RuntimeError as exc:
                failures.append(str(exc))
            else:
                if obj.get("schema") != "t81.feedback-report.v1":
                    failures.append(
                        "feedback report: schema mismatch "
                        f"(got={obj.get('schema')}, expected=t81.feedback-report.v1)"
                    )

    if failures:
        print("cli json contracts: FAILED")
        for item in failures:
            print(f"  - {item}")
        return 1

    print("cli json contracts: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
