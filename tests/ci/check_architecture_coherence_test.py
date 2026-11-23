#!/usr/bin/env python3
"""The architecture coherence checker must see dispatch cases in included .inc files.

Removing a case from an included fragment fails the check. A copy of that case
in a fragment vm/vm.cpp does not include does not satisfy it.
"""

from __future__ import annotations

import importlib.util
import io
import shutil
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path


CHECKER_INPUTS = (
    "include/t81/isa/opcodes.hpp",
    "include/t81/isa/ai_native_opcodes.hpp",
    "include/t81/axion/reasons.hpp",
    "include/t81/tensor/native.hpp",
    "vm/vm.cpp",
    "vm/tensor_helpers.cpp",
    "vm/policy_trace_bridge.cpp",
    "CMakeLists.txt",
    "tests/CMakeLists.txt",
    ".github/workflows/ci.yml",
    "scripts/ci/ai_wload_policy_evidence_expectations.json",
)

QMATMUL_CASE = "case t81::tisc::Opcode::QMATMUL:"


def load_checker(repo: Path):
    script = repo / "scripts/ci/check_architecture_coherence.py"
    spec = importlib.util.spec_from_file_location("check_architecture_coherence", script)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {script}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def snapshot_checker_inputs(src: Path, dst: Path) -> None:
    for rel in CHECKER_INPUTS:
        target = dst / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / rel, target)
    for inc in (src / "vm").rglob("*.inc"):
        target = dst / inc.relative_to(src)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(inc, target)
    src_cmake = src / "src"
    if src_cmake.is_dir():
        for cmake in src_cmake.rglob("CMakeLists.txt"):
            target = dst / cmake.relative_to(src)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(cmake, target)


def run_checker(module, repo: Path) -> tuple[int, str]:
    buffer = io.StringIO()
    with redirect_stdout(buffer):
        code = module.main(repo)
    return code, buffer.getvalue()


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    if not (repo / ".github/workflows/ci.yml").is_file():
        print("SKIP: .github/workflows/ci.yml not present in checkout")
        return 0
    module = load_checker(repo)

    code, output = run_checker(module, repo)
    if code != 0 or "architecture coherence check: PASSED" not in output:
        print("real repository coherence check failed")
        print(output)
        return 1

    with tempfile.TemporaryDirectory(prefix="t81-coherence-") as tmp:
        fixture = Path(tmp) / "repo"
        snapshot_checker_inputs(repo, fixture)
        code, output = run_checker(module, fixture)
        if code != 0:
            print("fixture copied from the repository did not pass")
            print(output)
            return 1

        inc = fixture / "vm/internal/vm_ai_opcodes.inc"
        text = inc.read_text(encoding="utf-8")
        if QMATMUL_CASE not in text:
            print(f"expected dispatch case missing from {inc}")
            return 1
        inc.write_text(text.replace(QMATMUL_CASE, "/* QMATMUL dispatch case removed */", 1), encoding="utf-8")
        if "Opcode::QMATMUL" in inc.read_text(encoding="utf-8"):
            print("QMATMUL dispatch token still present after case removal")
            return 1

        # A case that vm.cpp does not include must not count as dispatch.
        decoy = fixture / "vm/internal/not_included_opcodes.inc"
        decoy.write_text(f"        {QMATMUL_CASE} {{ break; }}\n", encoding="utf-8")

        code, output = run_checker(module, fixture)
        if code == 0 or "missing VM dispatch for QMATMUL" not in output:
            print("checker passed after the included QMATMUL dispatch case was removed")
            print(output)
            return 1
        if "architecture coherence check: PASSED" in output:
            print("checker reported PASSED after the dispatch case was removed")
            return 1

    print("architecture coherence dispatch test PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
