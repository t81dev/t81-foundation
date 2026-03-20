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
    "BUILD.bazel",
    "CMakeDoxyfile.in",
    "CMakeDoxygenDefaults.cmake",
    "CMakeLists.txt",
    "CMakePresets.json",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "LICENSE",
    "Makefile",
    "README.es.md",
    "README.md",
    "README.pt-BR.md",
    "README.ru.md",
    "README.zh-CN.md",
    "SECURITY.md",
    "SUPPORT_TAXONOMY.md",
    "T81_CAPABILITIES_REPORT.md",
    "artifacts",
    "assets",
    "benchmarks",
    "book",
    "cmake",
    "contracts",
    "core",
    "docs",
    "examples",
    "experiments",
    "experimental",
    "include",
    "internal",
    "kernel",
    "lang",
    "legacy",
    "logs",
    "notebooks",
    "package-lock.json",
    "package.json",
    "pdf",
    "pyproject.toml",
    "qemu",
    "runtime",
    "scripts",
    "spec",
    "src",
    "tests",
    "third_party",
    "tooling",
    "tools",
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
