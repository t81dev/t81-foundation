# Testing Strategy

> **Source of Truth:** This document defines the **testing taxonomy and coverage expectations**.

**Last Updated:** February 19, 2026

## 1. Testing Taxonomy

We categorize tests by their architectural scope and verification goal.

| Type | Directory | Goal | Example |
| :--- | :--- | :--- | :--- |
| **Unit Tests** | `tests/cpp/` | Verify individual class/function correctness. | `tests/cpp/test_T81BigInt.cpp` |
| **Property Tests** | `tests/cpp/` | Verify mathematical invariants (e.g., ring properties). | `tests/cpp/bigint_properties_test.cpp` |
| **Integration Tests** | `tests/cpp/` | Verify subsystem interaction (CLI -> VM). | `tests/cpp/cli_debugger_test.cpp` |
| **Spec Conformance** | `tests/cpp/` | Verify adherence to normative spec behavior. | `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| **Determinism Gates** | `scripts/ci/` | Verify bit-exact reproducibility across environments. | `t81lang_repro_gate.py` |
| **Fuzz Tests** | `tests/cpp/` | Discover edge cases via random input generation. | `tests/cpp/frontend_fuzz_test.cpp` |
| **Resource Guardrails** | `tests/cpp/` | Verify bounded resource usage (memory/time). | `tests/cpp/bigint_allocation_guardrail_test.cpp` |

## 2. Coverage Position

T81 prioritizes **Semantic Coverage** over raw Line Coverage.

- **Critical:** All Axion policy boundaries, VM opcode dispatch, and canonical serialization paths MUST be covered.
- **Critical:** Determinism gates MUST pass on all supported platforms.
- **High:** Public API surface (`include/t81/`) should have usage examples in tests.

We verify coverage effectiveness through **Property-Based Testing** (RapidCheck) and **Fuzzing**, rather than chasing 100% line execution of error paths that should be unreachable.

## 3. CI Matrix

Our Continuous Integration pipeline (`.github/workflows/ci.yml`) validates the following matrix on every PR:

| OS | Compiler | Purpose |
| :--- | :--- | :--- |
| **Ubuntu 24.04** | GCC 14 | Primary Linux build & unit tests (214+ tests). |
| **Ubuntu 24.04** | Clang 18 | Primary Linux build & **Determinism Gate**. |
| **Ubuntu 24.04 (ARM64)** | Clang 18 | Cross-architecture **Determinism Gate**. |
| **macOS 14 (ARM64)** | Apple Clang | macOS build compatibility. |
| **macOS 13 (x86_64)** | GCC 14 | Legacy macOS support. |

Additionally, we run:
- **Sanitizers:** ASan / UBSan (Clang 18).
- **Static Analysis:** `clang-tidy-18`.
- **Fuzzing:** `libFuzzer` target.
- **Formatting:** `clang-format-18`.
