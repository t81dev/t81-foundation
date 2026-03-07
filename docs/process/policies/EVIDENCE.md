# Evidence Matrix

> **Source of Truth:** This document maps our **claims** to their **specifications**, **tests**, and **CI artifacts**. It serves as a proof-of-correctness index.

**Last Updated:** February 10, 2026

## 1. Core Claims

| Claim | Spec Authority | Verification Test | CI Workflow |
| :--- | :--- | :--- | :--- |
| **Deterministic Compilation** | `spec/tisc/` | `tests/cpp/e2e_compile_determinism_test.cpp` | `t81lang-cross-arch-bit-identity` |
| **Deterministic Execution** | `spec/vm/` | `tests/cpp/vm_determinism_property_test.cpp` | `t81lang-cross-arch-bit-identity` |
| **Cross-Arch Bit-Identity** | `spec/numerics/` | `scripts/ci/t3k_repro_gate.py` | `t3k-cross-arch-bit-identity` |
| **Axion Policy Enforcement** | `spec/axion/` | `tests/cpp/axion_log_determinism_test.cpp` | `build-and-test` |
| **Runtime Contract Sync** | `docs/runtime-semantics-boundary.md` | `scripts/check-runtime-contract-sync.py` | `spec-and-docs` |
| **Spec Conformance** | `spec/lang/` | `tests/cpp/t81lang_conformance_baseline_test.cpp` | `build-and-test` |
| **Strict Profile Enforcement** | `spec/determinism-profile.md` | `tests/cpp/strict_profile_test.cpp` | `t81-strict-profile-gate` |
| **Deterministic Trap Semantics** | `spec/determinism-profile.md` | `tests/cpp/vm_trap_test.cpp` | `build-and-test` |
| **Symbol Content-Addressed Mode** | `spec/determinism-profile.md` | `tests/cpp/symbol_content_address_test.cpp` | `build-and-test` |
| **Error Determinism Guarantee** | `spec/determinism-profile.md` | `tests/cpp/error_determinism_test.cpp` | `build-and-test` |

## 2. Artifacts & Reproducibility

We guarantee the following artifacts are bit-identical across all supported platforms when built with the same compiler version.

| Artifact | Source | Hash Validation | Expected SHA256 (Canonical) |
| :--- | :--- | :--- | :--- |
| **TISC Binary (.tisc)** | `t81 compile` | `t81 repro-hash` | *Varies by version (see CI logs)* |
| **T3_K Quantized Model** | `t81 weights` | `scripts/ci/t3k_repro_gate.py` | *Varies by version (see CI logs)* |
| **Trace Log** | `t81 trace` | `tests/cpp/vm_trace_test.cpp` | *Stable per minor version* |

## 3. Coverage Analysis

Our testing strategy covers:
- **Unit:** 100% of public numeric API (`include/t81/types/`).
- **Integration:** End-to-end CLI workflows (`compile` -> `run` -> `trace`).
- **Property:** Ring properties for `T81BigInt`, `T81Float`, `T81Prob`.
- **Fuzzing:** Frontend parser and TISC decoder resilience.

See [TESTING.md](../reference/TESTING.md) for detailed strategy.
