# SCG-07 Phase 3 Evidence Receipt: F1 T81Limb27 Multiplication Correctness Repair

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [SCG-07 Phase 3 Evidence Receipt: F1 T81Limb27 Multiplication Correctness Repair](#scg-07-phase-3-evidence-receipt-f1-t81limb27-multiplication-correctness-repair)
  - [1. Executive Summary](#1-executive-summary)
  - [2. Inventory of Changed Files](#2-inventory-of-changed-files)
  - [3. Finding F1 Root Cause & Repair Disposition](#3-finding-f1-root-cause-&-repair-disposition)
    - [3.1 Pre-Repair Defect Mechanism](#31-pre-repair-defect-mechanism)
    - [3.2 Phase 3 Mathematical Repair](#32-phase-3-mathematical-repair)
  - [4. Verification Evidence](#4-verification-evidence)
    - [4.1 Before and After Results for F1 Reproducer](#41-before-and-after-results-for-f1-reproducer)
    - [4.2 Test Execution Matrix](#42-test-execution-matrix)
    - [4.3 Independent Oracle Comparison](#43-independent-oracle-comparison)
  - [5. CI Attribution & Inherited Baseline Failures](#5-ci-attribution-&-inherited-baseline-failures)
  - [6. Scope Constraints & Remaining Limitations](#6-scope-constraints-&-remaining-limitations)

<!-- T81-TOC:END -->


**Document ID:** T81-SCG-07-PHASE3-RECEIPT-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-07)
**Baseline Anchor:** Commit `1604fd204346c0ca09d14abe6be6f2417170f741` (origin/main, merging PR #737)
**Date:** 2026-11-09
**Status:** IMPLEMENTATION CANDIDATE (NOT MERGED) — READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This receipt documents the bounded, candidate implementation for **SCG-07 Phase 3: F1 T81Limb27 Booth Multiplication Correctness Repair** in `t81::core::T81Limb27`.
Following the empirical reproduction in SCG-06 (Issue #717, PR #733 / PR #735), the exception safety repairs in SCG-07 Phase 1 (PR #736), the `T81BigInt` signed subtraction repair in SCG-07 Phase 2 (PR #737), and maintainer policy decisions from SCG-04, this repair resolves finding F1 without modifying normative specifications, ISA opcodes, or unrelated arithmetic components.

The implementation is rebased directly atop baseline commit `1604fd204346c0ca09d14abe6be6f2417170f741`. All canonical SCG-06 reproduction evidence and preceding SCG-07 Phase 1 and Phase 2 receipts are preserved unchanged.

---

## 2. Inventory of Changed Files

The PR diff is strictly isolated to the following 4 files against the merged PR #737 baseline:

| File | Change Description |
|---|---|
| `include/t81/types/T81Limb.hpp` | Repaired `T81Limb27::operator*(const T81Limb27& rhs)` to perform direct balanced-ternary multiply-accumulate across decoded trits into `accum`, followed by exact balanced-ternary carry normalization. Removed obsolete `Booth27Decision` and `BOOTH27_TABLE` whose sole consumer was the defective `T81Limb27::operator*`. |
| `tests/cpp/test_t81_limb27_mul.cpp` | Standalone regression test suite verifying: original F1 reproducer ($1 \times 2 = 2$); small values and complete sign-combination matrix ($\pm 1, \pm 2, \pm 3, \pm 9, \pm 27, 0$); powers-of-three multiplication; alternating balanced-ternary digit patterns; 27-trit boundary values ($\pm(3^{27}-1)/2$); dense carry-propagation chains; commutativity ($a \times b = b \times a$); and 10,000 randomized trials evaluated against an independent exact-integer arbitrary-precision oracle (`ExactBigIntOracle`, base $10^9$) using operands passed directly as explicit 27-trit digit vectors (without raw object-memory reinterpretation). |
| `CMakeLists.txt` | Registered `t81_limb27_mul_test` executable and test runner in CMake build configuration and added to `T81_TEST_TARGETS`. |
| `docs/research/continuity/SCG07_PHASE3_EVIDENCE_RECEIPT.md` | Formal receipt and verification report for independent audit. |

---

## 3. Finding F1 Root Cause & Repair Disposition

### 3.1 Pre-Repair Defect Mechanism
In `include/t81/types/T81Limb.hpp`:
```cpp
// Pre-repair lines 104-131:
struct Booth27Decision {
  int8_t mul;
  int8_t shift_trytes;
};
static constexpr std::array<Booth27Decision, 27 * 27> build_booth27_table() {
  std::array<Booth27Decision, 27 * 27> table{};
  for (int b0 = -13; b0 <= 13; ++b0) {
    for (int b1 = -13; b1 <= 13; ++b1) {
      int8_t mul = 0;
      int8_t shift = 0;
      int pattern = b0 + 3 * b1;
      if (pattern == 1 || pattern == 3) {
        mul = 1;
      } else if (pattern == 2 || pattern == 4) {
        mul = 1;
        shift = 1;
      } else if (pattern == -1 || pattern == -3) {
        mul = -1;
      } else if (pattern == -2 || pattern == -4) {
        mul = -1;
        shift = 1;
      }
      table[(b0 + 13) * 27 + (b1 + 13)] = {mul, shift};
    }
  }
  return table;
}
```
In `T81Limb27::operator*(const T81Limb27& rhs)` (pre-repair lines 770–827), tryte pairs `(b0, b1)` looked up decisions in `BOOTH27_TABLE`. When `pattern == 2` or `pattern == 4`, `shift_trytes` was set to `1`. In `operator*`, the multiplier computed `shift_trits = decision.shift_trytes * 3 = 3`. A 3-trit shift corresponds to multiplication by $3^3 = 27$.
Consequently, low-magnitude multiplier patterns (such as $1 \times 2$) were mis-scaled by a factor of 27:
- Expected: $1 \times 2 = 2$ (trits: `[-1, 1, 0, 0, ...]`)
- Observed: $1 \times 27 = 27$ (trits: `[0, 0, 0, 1, ...]`)

### 3.2 Phase 3 Mathematical Repair
The defective tryte-shifted accumulation is replaced with direct balanced-ternary multiply-accumulate across decoded trits into a wide 54-trit accumulator, followed by exact balanced-ternary carry normalization:
```cpp
  [[nodiscard]] T81Limb54 operator*(const T81Limb27& rhs) const noexcept {
    std::array<int, T81Limb54::TRITS> accum{};
    std::array<std::array<int8_t, 3>, TRYTES> a_trits{};
    std::array<std::array<int8_t, 3>, TRYTES> b_trits{};
    for (int j = 0; j < TRYTES; ++j) {
      a_trits[j] = detail::TRYTE_TO_TRITS[trytes_[j] + 13];
      b_trits[j] = detail::TRYTE_TO_TRITS[rhs.trytes_[j] + 13];
    }

    for (int i = 0; i < TRYTES; ++i) {
      for (int ti = 0; ti < 3; ++ti) {
        int8_t b_val = b_trits[i][ti];
        if (b_val == 0) continue;
        int bit = i * 3 + ti;
        for (int j = 0; j < TRYTES; ++j) {
          for (int tj = 0; tj < 3; ++tj) {
            int8_t a_val = a_trits[j][tj];
            if (a_val == 0) continue;
            accum[bit + j * 3 + tj] += b_val * a_val;
          }
        }
      }
    }

    int carry = 0;
    std::array<int8_t, T81Limb54::TRITS> result{};
    for (int i = 0; i < T81Limb54::TRITS; ++i) {
      int sum = accum[i] + carry;
      int rem = (sum + 1) % 3;
      if (rem < 0) rem += 3;
      rem -= 1;
      carry = (sum - rem) / 3;
      result[i] = static_cast<int8_t>(rem);
    }
    return T81Limb54::from_trits(result);
  }
```
This ensures:
1. Positional trit weights $3^{i+j}$ are applied exactly once.
2. Carry normalization iteratively resolves all carries up to 54 trits with zero remainder.
3. Obsolete `Booth27Decision` and `BOOTH27_TABLE` structures are cleanly eliminated with zero lingering references.

---

## 4. Verification Evidence

### 4.1 Before and After Results for F1 Reproducer

| Input A (trytes / int) | Input B (trytes / int) | Expected Output Trits | Pre-Repair Output Trits | Post-Repair Output Trits | Disposition |
|---|---|---|---|---|---|
| `[1, 0, ...]` ($1$) | `[2, 0, ...]` ($2$) | `[-1, 1, 0, 0]` ($2$) | `[0, 0, 0, 1]` ($27$) | `[-1, 1, 0, 0]` ($2$) | **RESOLVED** |

### 4.2 Test Execution Matrix

| Test Suite | Configuration | Command | Result |
|---|---|---|---|
| `t81_limb27_mul_test` | Debug | `cmake --build build --target t81_limb27_mul_test && ./build/t81_limb27_mul_test` | **PASS (100%)** |
| `t81_limb27_mul_test` | Release (`-DNDEBUG`) | `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build --target t81_limb27_mul_test && ./build/t81_limb27_mul_test` | **PASS (100%)** |
| ASan / UBSan | Address/Undefined Sanitizer | `g++ -std=c++20 -fsanitize=address,undefined -g -Iinclude tests/cpp/test_t81_limb27_mul.cpp -o /tmp/asan_limb27_test && /tmp/asan_limb27_test` | **PASS (zero leaks/errors)** |
| `t81_limb_bohemian_add_test` | Existing limb arithmetic | `./build/t81_limb_bohemian_add_test` | **PASS** |
| `t81_limb54_add_test` | Existing limb54 addition | `./build/t81_limb54_add_test` | **PASS** |
| `t81_limb_add_test` | Existing limb addition | `./build/t81_limb_add_test` | **PASS** |
| `t81_bigint_sub_test` | Phase 2 subtraction suite | `./build/t81_bigint_sub_test` | **PASS** |
| `t81_t81fraction_test` | Phase 1 rational suite | `./build/t81_t81fraction_test` | **PASS** |
| `clang-format-18` | Style conformance | `clang-format-18 --dry-run --Werror include/t81/types/T81Limb.hpp tests/cpp/test_t81_limb27_mul.cpp` | **PASS (0 violations)** |

### 4.3 Independent Oracle Comparison
The randomized test suite in `tests/cpp/test_t81_limb27_mul.cpp` executes 10,000 trials evaluated directly against an independent base-$10^9$ arbitrary-precision integer oracle (`ExactBigIntOracle`).
All operand test vectors are provided as explicit 27-trit arrays (`std::array<int8_t, 27>`) directly converted to oracle inputs via `trits_to_oracle()`, completely decoupled from T81Limb internals and eliminating raw object-memory inspection.
Across all test distributions (uniform random, sparse operands, dense non-zero, high/low boundary biased, and mixed trytes), 10,000/10,000 trials passed with 0 discrepancies and verified commutativity ($a \times b = b \times a$).

---

## 5. CI Attribution & Inherited Baseline Failures

The current baseline (`origin/main` at `1604fd204346c0ca09d14abe6be6f2417170f741`) exhibits known baseline failures in CI that are pre-existing and independent of this change:
1. **Missing Fixture Regressions:** Missing `models/tiny-random-llama/model.safetensors` causes failures in `t81_cli_contract_test_runner` and `run_model_artifact_review_smoke.sh`.
2. **Repository Governance Structure:** Committed directory `review/pr-656/` causes `scripts/governance/check_root_structure.py` (`t81_root_structure_policy_test`) to fail.
3. **Sparse Checkout Discrepancies:** Windows sparse checkout in `.github/workflows/ci.yml:476-505` omits `/development_tests/**`, failing CMake generation on missing `development_tests/test_real_direct_api.cpp`.
4. **External Environment Incompatibilities:** GCC 15 `std::function` regex automaton `-Werror=maybe-uninitialized` warning in Google Benchmark; macOS x86_64 GCC 14 libgtest ABI mismatch; and macOS ARM64 clang unsupported runner label `macos-14-arm64-us-default`.
5. **Transient External Infrastructure Failures:** Transient GitHub HTTP 504 Gateway Timeout or HTTP 503 errors during `lychee` markdown link resolution in `lint / spec & docs` (failing the aggregate `quality gate / required` job).

Zero new failures or regressions are introduced by the changes in this PR.

---

## 6. Scope Constraints & Remaining Limitations

1. **Defect Scope:** Only finding F1 (`T81Limb27` multiplication) is addressed in Phase 3. Core limb multiplication defect F2 (`T81Limb54::reference_mul` carry normalization) and F3 (`T81Limb54::karatsuba` intermediate carry discard) remain untouched for Phase 4.
2. **Untouched Findings:** Findings F4, F5, F6, and F7 remain closed as previously verified.
3. **Capacity Boundaries:** `T81Limb27::operator*` takes two 27-trit limbs and produces a 54-trit `T81Limb54`, which fully accommodates the maximum possible product $(-(3^{27}-1)/2) \times (-(3^{27}-1)/2) < (3^{54}-1)/2$ without overflow or truncation.
4. **Final Disposition:** **READY FOR INDEPENDENT REVIEW**.
