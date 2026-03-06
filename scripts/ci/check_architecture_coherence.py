#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


def must_contain(text: str, pattern: str, what: str, errors: list[str]) -> None:
    if re.search(pattern, text, flags=re.MULTILINE) is None:
        errors.append(f"missing {what}: /{pattern}/")


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    errors: list[str] = []

    opcodes_h = (repo / "include/t81/isa/opcodes.hpp").read_text(encoding="utf-8")
    vm_cpp = (repo / "core/vm/vm.cpp").read_text(encoding="utf-8")
    tensor_helpers = (repo / "core/vm/tensor_helpers.cpp").read_text(encoding="utf-8")
    axion_reasons = (repo / "include/t81/axion/reasons.hpp").read_text(encoding="utf-8")
    policy_trace_bridge = (repo / "core/vm/policy_trace_bridge.cpp").read_text(encoding="utf-8")
    tensor_native = (repo / "include/t81/tensor/native.hpp").read_text(encoding="utf-8")
    cmake = (repo / "CMakeLists.txt").read_text(encoding="utf-8")
    ci = (repo / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    ai_compat = (repo / "include/t81/isa/ai_native_opcodes.hpp").read_text(encoding="utf-8")
    wload_expect = json.loads(
        (repo / "scripts/ci/ai_wload_policy_evidence_expectations.json").read_text(
            encoding="utf-8"
        )
    )

    # ISA <-> VM coherence for RFC-0026 phase-1 opcodes.
    for op in ("ATTN", "QMATMUL", "EMBED"):
        must_contain(opcodes_h, rf"\b{op}\b", f"ISA opcode {op}", errors)
        must_contain(vm_cpp, rf"Opcode::{op}\b", f"VM dispatch for {op}", errors)

    # Ensure this compatibility header is no longer placeholder handler surface.
    if "TODO: Implement actual" in ai_compat or "create_opcode_handler" in ai_compat:
        errors.append("ai_native_opcodes compatibility header still contains placeholder handler surface")

    # Canonical policy engine must come from kernel path.
    must_contain(cmake, r"kernel/axion/policy_engine\.cpp", "kernel PolicyEngine source in build graph", errors)
    if "src/axion/policy_engine.cpp" in cmake:
        errors.append("src/axion/policy_engine.cpp is in build graph; canonical source must be kernel/axion")

    # CI quality gate must include cross-arch bit identity checks.
    must_contain(ci, r"- t3k-cross-arch-bit-identity", "quality-gate dependency on t3k cross-arch gate", errors)
    must_contain(
        ci,
        r"- t81lang-cross-arch-bit-identity",
        "quality-gate dependency on t81lang cross-arch gate",
        errors,
    )

    # WLOAD contract must enforce both allow and deny reason codes.
    required_codes = set(wload_expect.get("required_observed_wload_reason_codes", []))
    if "AI_POLICY_ALLOW_WLOAD_POLICY_GATE" not in required_codes:
        errors.append("WLOAD expectation missing allow reason code AI_POLICY_ALLOW_WLOAD_POLICY_GATE")
    if "AI_POLICY_DENY_WLOAD_UNSUPPORTED" not in required_codes:
        errors.append("WLOAD expectation missing deny reason code AI_POLICY_DENY_WLOAD_UNSUPPORTED")

    # Tensor provenance trace must remain observable in VM/Axion surfaces.
    must_contain(
        axion_reasons,
        r"canonical_tensor_provenance_reason|kTensorProvenance",
        "Axion tensor provenance reason builder",
        errors,
    )
    must_contain(
        policy_trace_bridge,
        r"event_type\s*=\s*\"tensor_provenance\"",
        "tensor provenance structured event emission",
        errors,
    )
    must_contain(
        policy_trace_bridge,
        r"reason_code\s*=\s*\"VM_TENSOR_PROVENANCE\"",
        "tensor provenance reason code emission",
        errors,
    )
    must_contain(vm_cpp, r"log_tensor_provenance\s*\(", "VM tensor provenance logging hook", errors)
    must_contain(
        cmake,
        r"\bt81_vm_tensor_provenance_trace_test\b",
        "tensor provenance regression target",
        errors,
    )
    must_contain(
        cmake,
        r"\bt81_vm_ai_phase1_shared_helper_parity_test\b",
        "phase-1 VM/shared helper parity regression target",
        errors,
    )
    must_contain(
        tensor_native,
        r"namespace t81::tensor_native",
        "shared tensor native decode surface",
        errors,
    )
    must_contain(
        tensor_helpers,
        r"tensor_native::decode\s*\(",
        "VM tensor helpers routing native decode through shared tensor layer",
        errors,
    )
    must_contain(
        cmake,
        r"\bt81_tensor_native_decode_test\b",
        "shared tensor native decode regression target",
        errors,
    )

    if errors:
        print("architecture coherence check: FAILED")
        for idx, err in enumerate(errors, start=1):
            print(f"{idx}. {err}")
        return 1

    print("architecture coherence check: PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
