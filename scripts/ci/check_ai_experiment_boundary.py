#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
CMAKE_ADD_SUBDIR_RE = re.compile(r"add_subdirectory\s*\(\s*experiments/ai(?:\s+|\))", re.IGNORECASE)
SCRIPT_EXTS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".cmake", ".txt"}
SCRIPT_FILES = {"CMakeLists.txt"}
BLOCKED_DEP_PREFIXES = ("experiments/ai/", "experiments\\ai\\")

GUARDED_SOURCE_ROOTS = (
    "core",
    "kernel",
    "runtime",
    "include/t81",
    "src",
    "tests",
    "spec",
    "tooling",
    "tools",
)

FORBIDDEN_CORE_DIFF_PREFIXES = (
    "core/",
    "kernel/",
    "runtime/",
    "include/t81/",
    "src/",
    "tests/",
    "spec/",
)


@dataclass
class Violation:
    code: str
    path: str
    line: int
    detail: str


def iter_guarded_files(repo_root: Path) -> list[Path]:
    files: list[Path] = []
    for rel_root in GUARDED_SOURCE_ROOTS:
        root = repo_root / rel_root
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.name in SCRIPT_FILES or path.suffix in SCRIPT_EXTS:
                files.append(path)
    top_level_cmake = repo_root / "CMakeLists.txt"
    if top_level_cmake.exists():
        files.append(top_level_cmake)
    return files


def check_forbidden_ai_references(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []
    for path in iter_guarded_files(repo_root):
        rel = path.relative_to(repo_root).as_posix()
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        for idx, line in enumerate(lines, start=1):
            include_match = INCLUDE_RE.match(line)
            if include_match:
                include_target = include_match.group(1)
                if include_target.startswith(BLOCKED_DEP_PREFIXES):
                    violations.append(
                        Violation(
                            code="CORE_INCLUDES_AI_SANDBOX",
                            path=rel,
                            line=idx,
                            detail=include_target,
                        )
                    )
                continue

            lowered = line.lower()
            if "experiments/ai/" in lowered or "experiments\\ai\\" in lowered:
                if rel == "CMakeLists.txt" and CMAKE_ADD_SUBDIR_RE.search(line):
                    # Top-level CMake add_subdirectory is checked separately with
                    # guard-aware parsing.
                    continue
                violations.append(
                    Violation(
                        code="CORE_REFERENCES_AI_SANDBOX",
                        path=rel,
                        line=idx,
                        detail=line.strip(),
                    )
                )
    return violations


def check_top_level_cmake_guard(repo_root: Path) -> list[Violation]:
    cmake = repo_root / "CMakeLists.txt"
    if not cmake.exists():
        return [
            Violation(
                code="MISSING_TOPLEVEL_CMAKE",
                path="CMakeLists.txt",
                line=1,
                detail="missing top-level CMakeLists.txt",
            )
        ]

    violations: list[Violation] = []
    lines = cmake.read_text(encoding="utf-8", errors="ignore").splitlines()
    ai_gate_depth = 0

    for idx, raw in enumerate(lines, start=1):
        line = raw.strip()
        lower = line.lower()
        if lower.startswith("if(") and "t81_enable_ai_experiments" in lower:
            ai_gate_depth += 1
            continue
        if lower.startswith("endif"):
            if ai_gate_depth > 0:
                ai_gate_depth -= 1
            continue
        if CMAKE_ADD_SUBDIR_RE.search(line) and ai_gate_depth == 0:
            violations.append(
                Violation(
                    code="AI_SUBDIR_UNGUARDED",
                    path="CMakeLists.txt",
                    line=idx,
                    detail=line,
                )
            )

    return violations


def changed_files(repo_root: Path, base_ref: str, head_ref: str) -> list[str]:
    cmd = ["git", "diff", "--name-only", f"{base_ref}...{head_ref}"]
    proc = subprocess.run(
        cmd,
        cwd=repo_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"{' '.join(cmd)} failed: {proc.stderr.strip()}")
    return [line.strip() for line in proc.stdout.splitlines() if line.strip()]


def check_diff_boundary(repo_root: Path, base_ref: str, head_ref: str) -> list[Violation]:
    files = changed_files(repo_root, base_ref, head_ref)
    if not files:
        return []

    touched_ai = [p for p in files if p.startswith("experiments/ai/")]
    touched_core = [p for p in files if p.startswith(FORBIDDEN_CORE_DIFF_PREFIXES)]
    if touched_ai and touched_core:
        return [
            Violation(
                code="AI_PR_TOUCHES_CORE",
                path="(diff)",
                line=1,
                detail=f"ai={len(touched_ai)} core={len(touched_core)}",
            )
        ]
    return []


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Enforce AI experiment sandbox boundary invariants (RFC-00A0)."
    )
    parser.add_argument(
        "--repo-root",
        default=str(Path(__file__).resolve().parents[2]),
        help="Repository root path",
    )
    parser.add_argument(
        "--check-pr-diff",
        action="store_true",
        help="If running on pull_request, fail when AI sandbox changes are mixed with core path changes.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()

    violations: list[Violation] = []
    violations.extend(check_forbidden_ai_references(repo_root))
    violations.extend(check_top_level_cmake_guard(repo_root))

    if args.check_pr_diff:
        event_name = os.environ.get("GITHUB_EVENT_NAME", "").strip()
        base_ref = os.environ.get("GITHUB_BASE_REF", "").strip()
        head_sha = os.environ.get("GITHUB_SHA", "HEAD").strip() or "HEAD"
        if event_name == "pull_request" and base_ref:
            try:
                diff_violations = check_diff_boundary(repo_root, f"origin/{base_ref}", head_sha)
            except RuntimeError as exc:
                print(f"[WARN] skipped diff boundary check: {exc}")
                diff_violations = []
            violations.extend(diff_violations)
        else:
            print("AI boundary diff check: skipped (not pull_request context)")

    print("AI experiment boundary check (RFC-00A0)")
    if violations:
        print(f"Violations: {len(violations)}")
        for v in violations:
            print(f"  [FAIL] {v.code} {v.path}:{v.line} :: {v.detail}")
        return 1

    print("No AI sandbox boundary violations found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
