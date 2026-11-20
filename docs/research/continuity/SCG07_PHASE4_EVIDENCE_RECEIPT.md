# SCG-07 Phase 4 Evidence Receipt: F2 T81Limb54 Reference Multiplication Correctness Repair

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [SCG-07 Phase 4 Evidence Receipt: F2 T81Limb54 Reference Multiplication Correctness Repair](#scg-07-phase-4-evidence-receipt-f2-t81limb54-reference-multiplication-correctness-repair)
  - [1. Executive Summary](#1-executive-summary)
  - [2. Inventory of Changed Files](#2-inventory-of-changed-files)
  - [3. Finding F2 Root Cause & Repair Disposition](#3-finding-f2-root-cause-&-repair-disposition)
    - [3.1 Pre-Repair Defect Mechanism](#31-pre-repair-defect-mechanism)
    - [3.2 Phase 4 Mathematical Repair](#32-phase-4-mathematical-repair)
  - [4. Verification Evidence](#4-verification-evidence)
    - [4.1 Before and After Results for SCG-06 Reproducer (300 Trials)](#41-before-and-after-results-for-scg-06-reproducer-300-trials)
    - [4.2 Independent Oracle Methodology & Truncation Verification](#42-independent-oracle-methodology-&-truncation-verification)
    - [4.3 Test Execution Matrix](#43-test-execution-matrix)
  - [5. CI Attribution & Inherited Baseline Failures](#5-ci-attribution-&-inherited-baseline-failures)
  - [6. Scope Constraints & Remaining Limitations](#6-scope-constraints-&-remaining-limitations)

<!-- T81-TOC:END -->


**Document ID:** T81-SCG-07-PHASE4-RECEIPT-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-07)
**Baseline Anchor:** Commit `5cc8d9457fd19ff2d85a5bda1fc1678d8e0124d1` (origin/main, merging PR #738)
**Date:** 2026-11-09
**Status:** IMPLEMENTATION CANDIDATE (NOT MERGED) — READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This receipt documents the bounded, candidate implementation for **SCG-07 Phase 4: F2 T81Limb54 Reference Multiplication Correctness Repair** in `t81::core::T81Limb54::reference_mul`.
Following the empirical reproduction in SCG-06 (Issue #717, PR #733 / PR #735), the exception safety repairs in SCG-07 Phase 1 (PR #736), the `T81BigInt` signed subtraction repair in SCG-07 Phase 2 (PR #737), the `T81Limb27` multiplication repair in SCG-07 Phase 3 (PR #738), and maintainer policy decisions from SCG-04, this repair resolves finding F2 without modifying normative specifications, ISA opcodes, or unrelated arithmetic components.

The implementation is rebased directly atop baseline commit `5cc8d9457fd19ff2d85a5bda1fc1678d8e0124d1`. All canonical SCG-06 reproduction evidence and preceding SCG-07 Phase 1, Phase 2, and Phase 3 receipts are preserved unchanged.

---

## 2. Inventory of Changed Files

The PR diff is strictly isolated to the following 4 files against the merged PR #738 baseline:

| File | Change Description |
|---|---|
| `include/t81/types/T81Limb.hpp` | Repaired `T81Limb54::reference_mul(const T81Limb54& a, const T81Limb54& b)`: replaced the defective single-step subtraction carry loop with exact balanced-ternary carry normalization across the 54-trit result domain, preserving the public API, `noexcept` specification, and canonical balanced-ternary residue modulo $3^{54}$ semantics. |
| `tests/cpp/test_t81_limb54_reference_mul.cpp` | Standalone regression test suite verifying: original SCG-06 F2 reproducers (all-ones dense vectors, 300 diverse SplitMix64 trials); small values and complete sign matrix ($\pm 1, \pm 2, \pm 3, \pm 9, \pm 27, 0$); powers of three across the 54-trit range; alternating balanced-ternary patterns; 54-trit boundary extremes ($\pm(3^{54}-1)/2$); dense cascading carry chains; tryte-boundary carry propagation; final-carry and out-of-range overflow/truncation handling; commutativity and identity properties; strict output canonicality (digits in $\{-1, 0, 1\}$); and 10,000 randomized trials evaluated against an independent exact-integer arbitrary-precision oracle (`ExactBigIntOracle`, base $10^9$). |
| `CMakeLists.txt` | Registered `t81_limb54_reference_mul_test` executable and test runner in CMake build configuration and added to `T81_TEST_TARGETS`. |
| `docs/research/continuity/SCG07_PHASE4_EVIDENCE_RECEIPT.md` | Formal receipt and verification report for independent audit. |

---

## 3. Finding F2 Root Cause & Repair Disposition

### 3.1 Pre-Repair Defect Mechanism
In `include/t81/types/T81Limb.hpp`:
```cpp
// Pre-repair lines 496-536:
inline T81Limb54 T81Limb54::reference_mul(const T81Limb54& a, const T81Limb54& b) noexcept {
  std::array<int, TRITS * 2> accum{};
  auto A = a.to_trits();
  auto B = b.to_trits();
  for (int i = 0; i < TRITS; ++i) {
    if (A[i] == 0) continue;
    for (int j = 0; j < TRITS; ++j) {
      accum[i + j] += static_cast<int>(A[i]) * static_cast<int>(B[j]);
    }
  }
  int carry = 0;
  for (int i = 0; i < TRITS; ++i) {
    int sum = accum[i] + carry;
    if (sum >= 2) {
      accum[i] = sum - 3;
      carry = 1;
    } else if (sum <= -2) {
      accum[i] = sum + 3;
      carry = -1;
    } else {
      accum[i] = sum;
      carry = 0;
    }
  }
  carry = 0;
  std::array<int8_t, TRITS> result{};
  for (int i = 0; i < TRITS; ++i) {
    int sum = accum[i] + carry;
    if (sum == 2) {
      result[i] = -1;
      carry = 1;
    } else if (sum == -2) {
      result[i] = 1;
      carry = -1;
    } else {
      result[i] = static_cast<int8_t>(sum);
      carry = 0;
    }
  }
  return from_trits(result);
}
```
In schoolbook balanced-ternary convolution of two 54-trit limbs, partial-product sums at position $k$ can reach magnitudes up to $\pm 54$.
The normalization loop used a single-step conditional adjustment (`if (sum >= 2) { accum[i] = sum - 3; carry = 1; }`). If `accum[i] = 54` and `carry = 0`, `accum[i]` became 51 and `carry` became 1.
Subsequent passes failed to reduce values exceeding $\pm 2$, leaving large non-canonical numbers in `accum[i]`. When passed through the secondary loop and `from_trits()`, non-canonical digits (reaching values up to $\pm 52$) were directly encoded into trytes and stored in `T81Limb54`, causing both representation corruption and numerical value errors.

### 3.2 Phase 4 Mathematical Repair
The defective normalization and secondary loops are replaced with exact balanced-ternary carry normalization across the 54-trit result domain:
```cpp
inline T81Limb54 T81Limb54::reference_mul(const T81Limb54& a, const T81Limb54& b) noexcept {
  std::array<int, TRITS * 2> accum{};
  auto A = a.to_trits();
  auto B = b.to_trits();
  for (int i = 0; i < TRITS; ++i) {
    if (A[i] == 0) continue;
    for (int j = 0; j < TRITS; ++j) {
      accum[i + j] += static_cast<int>(A[i]) * static_cast<int>(B[j]);
    }
  }
  int carry = 0;
  std::array<int8_t, TRITS> result{};
  for (int i = 0; i < TRITS; ++i) {
    int sum = accum[i] + carry;
    int rem = (sum + 1) % 3;
    if (rem < 0) rem += 3;
    rem -= 1;
    carry = (sum - rem) / 3;
    result[i] = static_cast<int8_t>(rem);
  }
  return from_trits(result);
}
```
This ensures:
1. Positional trit weights $3^{i+j}$ are accumulated correctly.
2. Every intermediate sum `sum = accum[i] + carry` is decomposed into a unique canonical trit `rem` in $\{-1, 0, 1\}$ and integer carry `carry = (sum - rem) / 3` such that `sum == rem + 3 * carry`.
3. Carries propagate cleanly across all 54 trit positions.
4. For products within 54 trits, the exact product is returned.
5. For products exceeding 54 trits, the output represents the exact, unique canonical balanced-ternary residue modulo $3^{54}$, consistent with 54-trit fixed-width arithmetic.
6. The public API and `noexcept` specification are preserved.

---

## 4. Verification Evidence

### 4.1 Before and After Results for SCG-06 Reproducer (300 Trials)

In the empirical reproduction conducted in SCG-06 across 300 diverse pseudo-random trials using the exact historical seed (`0x123456789abcdef0ULL`), generator (`SplitMix64`), ordering, and 5 operand distribution modes (sparse, small width, dense, alternating, boundary), failures were classified into four disjoint categories. The post-repair results on this exact 300-case reproducer suite demonstrate complete resolution:

| Metric / Cohort | Pre-Repair (`5cc8d94`) | Post-Repair (Phase 4) | Disposition |
|---|---|---|---|
| Total Trials Tested | 300 | 300 | Complete |
| Produced Non-Canonical Digits (reaching $\pm 52$) | 130 (43.3%) | 0 (0.0%) | **RESOLVED** |
| Mathematical Value Mismatches modulo $3^{54}$ | 141 (47.0%) | 0 (0.0%) | **RESOLVED** |
| Both Non-Canonical Digits & Value Mismatch | 106 (35.3%) | 0 (0.0%) | **RESOLVED** |
| Non-Canonical Digits Only | 24 (8.0%) | 0 (0.0%) | **RESOLVED** |
| Mathematical Value Mismatch Only | 35 (11.7%) | 0 (0.0%) | **RESOLVED** |
| Passed Both Canonicality & Mathematical Value Checks | 135 (45.0%) | 300 (100.0%) | **100% PASS** |

### 4.2 Independent Oracle Methodology & Truncation Verification
The regression suite in `tests/cpp/test_t81_limb54_reference_mul.cpp` uses an independent base-$10^9$ arbitrary-precision integer oracle (`ExactBigIntOracle`).
Operands are evaluated as exact mathematical integers ($A = \sum a_i 3^i$, $B = \sum b_i 3^i$) and their exact product $P = A \times B$ is computed.
The expected 54-trit result is independently derived by computing the canonical balanced-ternary expansion of $P$ modulo $3^{54}$:
$$P = \sum_{i=0}^{53} r_i 3^i + 3^{54} Q$$
where $r_i \in \{-1, 0, 1\}$ and $|R| \le \frac{3^{54}-1}{2}$.
The test confirms:
1. Every emitted digit belongs strictly to $\{-1, 0, 1\}$.
2. The reconstructed 54-trit limb value equals the independently computed residue $R$.
3. Both positive and negative overflow cases yield exact residues matching the documented 54-trit truncation contract.
4. Carries propagate without loss across all 54 positions, with truncation outside the 54-trit window discarding high-order multiples of $3^{54}$ while retaining the exact canonical residue. (Note: the implementation contract returns a 54-trit limb and does not expose an implementation carry-out/quotient return value; verification is performed on the canonical 54-trit residue).

### 4.3 Test Execution Matrix

| Test Suite | Configuration | Command | Result |
|---|---|---|---|
| `t81_limb54_reference_mul_test` | Debug | `cmake --build build --target t81_limb54_reference_mul_test && ./build/t81_limb54_reference_mul_test` | **PASS (100%)** |
| `t81_limb54_reference_mul_test` | Release (`-DNDEBUG`) | `g++ -std=c++20 -O3 -DNDEBUG -Iinclude tests/cpp/test_t81_limb54_reference_mul.cpp -o /tmp/test_ref_mul_release && /tmp/test_ref_mul_release` | **PASS (100%)** |
| ASan / UBSan | Address/Undefined Sanitizer | `g++ -std=c++20 -fsanitize=address,undefined -g -Iinclude tests/cpp/test_t81_limb54_reference_mul.cpp -o /tmp/test_ref_mul_asan && /tmp/test_ref_mul_asan` | **PASS (zero leaks/errors)** |
| `t81_limb27_mul_test` | Phase 3 F1 regression suite | `./build/t81_limb27_mul_test` | **PASS (10,000 trials)** |
| `t81_limb54_booth_mul_test` | Existing Booth multiplication | `./build/t81_limb54_booth_mul_test` | **PASS (4,096 trials)** |
| `t81_limb54_add_test` | Existing limb54 addition | `./build/t81_limb54_add_test` | **PASS** |
| `t81_limb_bohemian_add_test` | Existing limb addition | `./build/t81_limb_bohemian_add_test` | **PASS** |
| `t81_limb_add_test` | Existing limb addition | `./build/t81_limb_add_test` | **PASS** |
| `t81_bigint_sub_test` | Phase 2 subtraction suite | `./build/t81_bigint_sub_test` | **PASS (1,200 trials)** |
| `t81_t81fraction_test` | Phase 1 rational suite | `./build/t81_t81fraction_test` | **PASS** |
| `clang-format-18` | Style conformance | `clang-format-18 --dry-run --Werror include/t81/types/T81Limb.hpp tests/cpp/test_t81_limb54_reference_mul.cpp` | **PASS (0 violations)** |

Across all 10,000 randomized trials (covering uniform, sparse, dense non-zero, boundary biased, low-order restricted, out-of-range overflow, and alternating distributions), 10,000/10,000 trials matched the independent oracle with zero discrepancies and zero non-canonical trits, confirming commutativity ($a \times b = b \times a$).

---

## 5. CI Attribution & Inherited Baseline Failures

The current baseline (`origin/main` at `5cc8d9457fd19ff2d85a5bda1fc1678d8e0124d1`) exhibits known baseline failures in CI that are pre-existing and independent of this change:
1. **Missing Fixture Regressions:** Missing `models/tiny-random-llama/model.safetensors` causes failures in `t81_cli_contract_test_runner` and `run_model_artifact_review_smoke.sh`.
2. **Repository Governance Structure:** Committed directory `review/pr-656/` causes `scripts/governance/check_root_structure.py` (`t81_root_structure_policy_test`) to fail.
3. **Sparse Checkout Discrepancies:** Windows sparse checkout in `.github/workflows/ci.yml:476-505` omits `/development_tests/**`, failing CMake generation on missing `development_tests/test_real_direct_api.cpp`.
4. **External Environment Incompatibilities:** GCC 15 `std::function` regex automaton `-Werror=maybe-uninitialized` warning in Google Benchmark; macOS x86_64 GCC 14 libgtest ABI mismatch; and macOS ARM64 clang unsupported runner label `macos-14-arm64-us-default`.
5. **Transient External Infrastructure Failures:** Transient GitHub HTTP 504 Gateway Timeout or HTTP 503 errors during `lychee` markdown link resolution in `lint / spec & docs` (failing the aggregate `quality gate / required` job).

Zero new failures or regressions are introduced by the changes in this PR.

---

## 6. Scope Constraints & Remaining Limitations

1. **Defect Scope:** Only finding F2 (`T81Limb54::reference_mul` carry normalization) is addressed in Phase 4. F1 (`T81Limb27` multiplication) remains resolved from Phase 3. F3 (`T81Limb54::karatsuba` intermediate carry discard) remains for future work.
2. **Untouched Findings:** Findings F4, F5, F6, and F7 remain closed as previously verified.
3. **Capacity Boundaries:** `T81Limb54::reference_mul` takes two 54-trit limbs and produces a 54-trit `T81Limb54`. When the mathematical product exceeds $\pm\frac{3^{54}-1}{2}$, the returned value is the exact canonical balanced-ternary residue modulo $3^{54}$.
4. **Final Disposition:** **READY FOR INDEPENDENT REVIEW**.
