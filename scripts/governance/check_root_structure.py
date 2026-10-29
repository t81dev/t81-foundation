#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
import subprocess

EXPECTED_TOP_LEVEL = {
    ".clang-format",
    ".clang-tidy",
    ".cursorrules",
    ".devcontainer",
    ".editorconfig",
    ".gitattributes",
    ".github",
    ".gitignore",
    ".gitmodules",
    ".pre-commit-config.yaml",
    ".t81",
    "AGENTS.md",
    "BALANCED_TERNARY_COMPARATIVE_RESEARCH.md",
    "BUILD.bazel",
    "CMakeDoxyfile.in",
    "CMakeLists.txt",
    "CMakePresets.json",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "Dockerfile",
    "EXACT_RATIONAL_CONVERSION_FEASIBILITY.md",
    "INDEPENDENT_RATIONAL_CONVERSION_VALIDATION.md",
    "LICENSE",
    "Makefile",
    "PROJECT_STRUCTURE.md",
    "README.es.md",
    "README.md",
    "README.pt-BR.md",
    "README.ru.md",
    "README.zh-CN.md",
    "SECURITY.md",
    "SUPPORT_TAXONOMY.md",
    "SYSTEMS_AND_SUBSYSTEMS_REPORT.md",
    "T81_CAPABILITIES_REPORT.md",
    "TERNARY_COMPARATIVE_CLAIMS_AUDIT.md",
    "_config.yml",
    "artifacts",
    "benchmarks",
    "build.log",
    "cmake",
    "codec",
    "contracts",
    "crypto",
    "customer_tools",
    "development_tests",
    "docs",
    "drivers",
    "examples",
    "execution_reports",
    "experimental",
    "experiments",
    "ffi",
    "fs",
    "import.apl",
    "include",
    "inference",
    "install.ps1",
    "install.sh",
    "isa",
    "kernel",
    "lang",
    "logs",
    "market-demos",
    "market_analysis",
    "pyproject.toml",
    "record_hash.txt",
    "scripts",
    "skills",
    "spec",
    "src",
    "strategic_docs",
    "symbolic_algebraic.tisc",
    "symbolic_algebraic_exported.tisc",
    "ternaryos",
    "test_input.txt",
    "test_record.json",
    "tests",
    "third_party",
    "tiny-model-policy.apl",
    "tools",
    "vm",
}


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    proc = subprocess.run(
        ["git", "ls-tree", "--name-only", "HEAD"],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        print(proc.stderr)
        return proc.returncode

    current = {line.strip() for line in proc.stdout.splitlines() if line.strip()}
    missing = sorted(EXPECTED_TOP_LEVEL - current)
    extra = sorted(current - EXPECTED_TOP_LEVEL)

    if missing or extra:
        print("root structure check FAILED")
        if missing:
            print("- missing:")
            for name in missing:
                print(f"  - {name}")
        if extra:
            print("- unexpected:")
            for name in extra:
                print(f"  - {name}")
        return 1

    print("root structure check PASSED")
    print(f"- entries checked: {len(current)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
