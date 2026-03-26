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


def ensure_keys(obj: dict, keys: list[str], context: str) -> None:
    missing = [key for key in keys if key not in obj]
    if missing:
        raise RuntimeError(f"{context}: missing keys: {', '.join(missing)}")


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

    schema_paths = [
        repo_root / "spec/rfcs/RFC-00D1-canonfs-import-result-schema.json",
        repo_root / "spec/rfcs/RFC-00D1-canonfs-export-result-schema.json",
        repo_root / "spec/rfcs/RFC-00D1-canonfs-interchange-manifest-schema.json",
        repo_root / "spec/rfcs/RFC-00D1-canonfs-import-provenance-schema.json",
        repo_root / "spec/rfcs/RFC-00D1-canonfs-export-provenance-schema.json",
    ]
    for schema_path in schema_paths:
        if not schema_path.exists():
            failures.append(f"missing schema file: {schema_path.relative_to(repo_root)}")
            continue
        try:
            with schema_path.open("r", encoding="utf-8") as fh:
                json.load(fh)
        except json.JSONDecodeError as exc:
            failures.append(f"{schema_path.relative_to(repo_root)}: invalid JSON: {exc}")

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

        # RFC-00D1 CanonFS interchange seed
        canonfs_root = temp_path / "canonfs-root"
        canonfs_root.mkdir(parents=True, exist_ok=True)

        import_file = temp_path / "canonfs-import.txt"
        import_file.write_text("canonfs-json-contract", encoding="utf-8")
        import_proc = run_cmd(
            [str(t81_bin), "canonfs", "import", str(import_file), "--canonfs-root", str(canonfs_root), "--json"],
            cwd=repo_root,
        )
        if import_proc.returncode != 0:
            failures.append(f"canonfs import --json: unexpected exit={import_proc.returncode}")
        else:
            try:
                import_obj = ensure_json_object(import_proc.stdout, "canonfs import --json")
                ensure_keys(
                    import_obj,
                    [
                        "schema",
                        "status",
                        "source_kind",
                        "source_ref",
                        "imported_objects",
                        "provenance_ref",
                        "warnings",
                        "errors",
                        "policy_result",
                        "policy_profile",
                        "normalization_summary",
                    ],
                    "canonfs import --json",
                )
            except RuntimeError as exc:
                failures.append(str(exc))
            else:
                if import_obj.get("schema") != "t81.canonfs-import.v1":
                    failures.append(
                        "canonfs import --json: schema mismatch "
                        f"(got={import_obj.get('schema')}, expected=t81.canonfs-import.v1)"
                    )
                if import_obj.get("status") not in {"ok", "partial", "error"}:
                    failures.append("canonfs import --json: invalid status")
                if import_obj.get("policy_profile") not in {
                    "permissive",
                    "import-only",
                    "export-only",
                    "deny-all",
                }:
                    failures.append("canonfs import --json: invalid policy_profile")
                imported_objects = import_obj.get("imported_objects")
                if not isinstance(imported_objects, list) or not imported_objects:
                    failures.append("canonfs import --json: imported_objects must be a non-empty array")
                manifest_ref = import_obj.get("manifest_ref")
                if manifest_ref is not None and not isinstance(manifest_ref, str):
                    failures.append("canonfs import --json: manifest_ref must be string or null")
                else:
                    export_target = temp_path / "canonfs-export.txt"
                    object_ref = imported_objects[0] if isinstance(imported_objects, list) and imported_objects else None
                    if isinstance(object_ref, str):
                        export_proc = run_cmd(
                            [
                                str(t81_bin),
                                "canonfs",
                                "export",
                                object_ref,
                                "--canonfs-root",
                                str(canonfs_root),
                                "--out",
                                str(export_target),
                                "--json",
                            ],
                            cwd=repo_root,
                        )
                        if export_proc.returncode != 0:
                            failures.append(
                                f"canonfs export --json: unexpected exit={export_proc.returncode}"
                            )
                        else:
                            try:
                                export_obj = ensure_json_object(export_proc.stdout, "canonfs export --json")
                                ensure_keys(
                                    export_obj,
                                    [
                                        "schema",
                                        "status",
                                        "source_objects",
                                        "target_kind",
                                        "target_ref",
                                        "provenance_ref",
                                        "warnings",
                                        "errors",
                                        "policy_result",
                                        "policy_profile",
                                        "materialization_summary",
                                    ],
                                    "canonfs export --json",
                                )
                            except RuntimeError as exc:
                                failures.append(str(exc))
                            else:
                                if export_obj.get("schema") != "t81.canonfs-export.v1":
                                    failures.append(
                                        "canonfs export --json: schema mismatch "
                                        f"(got={export_obj.get('schema')}, expected=t81.canonfs-export.v1)"
                                    )
                                if export_obj.get("status") not in {"ok", "partial", "error"}:
                                    failures.append("canonfs export --json: invalid status")
                                if export_obj.get("policy_profile") not in {
                                    "permissive",
                                    "import-only",
                                    "export-only",
                                    "deny-all",
                                }:
                                    failures.append("canonfs export --json: invalid policy_profile")
                                source_objects = export_obj.get("source_objects")
                                if not isinstance(source_objects, list) or not source_objects:
                                    failures.append(
                                        "canonfs export --json: source_objects must be a non-empty array"
                                    )
                                if export_target.read_text(encoding="utf-8") != "canonfs-json-contract":
                                    failures.append("canonfs export --json: exported payload mismatch")

    if failures:
        print("cli json contracts: FAILED")
        for item in failures:
            print(f"  - {item}")
        return 1

    print("cli json contracts: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
