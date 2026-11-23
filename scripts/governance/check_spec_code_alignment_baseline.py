#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ALIGNMENT_BASELINE: dict[str, list[str]] = {
    "spec/tisc-spec.md": [
        "isa/encoding.cpp",
        "tests/cpp/tisc_opcode_matrix_test.cpp",
        "scripts/ci/check_tisc_freeze_integrity.py",
    ],
    "spec/t81vm-spec.md": [
        "vm/vm.cpp",
        "tests/cpp/vm_determinism_property_test.cpp",
        "tests/cpp/jit_trace_equivalence_test.cpp",
    ],
    "spec/axion-kernel.md": [
        "kernel/axion/policy_engine.cpp",
        "tests/cpp/vm_policy_parse_fail_closed_test.cpp",
        "tests/cpp/test_axion_opcodes.cpp",
    ],
    "spec/determinism-profile.md": [
        "docs/governance/DETERMINISM_SURFACE_REGISTRY.md",
        "scripts/ci/t81lang_repro_gate.py",
        "scripts/ci/t3k_repro_gate.py",
    ],
}


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    issues: list[str] = []

    for spec_path, impl_paths in sorted(ALIGNMENT_BASELINE.items()):
        spec = repo_root / spec_path
        if not spec.exists():
            issues.append(f"missing spec artifact: {spec_path}")
            continue
        for rel in impl_paths:
            if not (repo_root / rel).exists():
                issues.append(f"missing mapped implementation/evidence file: {rel} (from {spec_path})")

    if issues:
        print("spec-code alignment baseline check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("spec-code alignment baseline check PASSED")
    print(f"- validated mapped baseline specs: {len(ALIGNMENT_BASELINE)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
