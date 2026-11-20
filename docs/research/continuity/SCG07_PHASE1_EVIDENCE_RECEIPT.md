# SCG-07 Phase 1 Evidence Receipt: F6/F7 Exception-Safety Repair

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [SCG-07 Phase 1 Evidence Receipt: F6/F7 Exception-Safety Repair](#scg-07-phase-1-evidence-receipt-f6f7-exception-safety-repair)
  - [1. Executive Summary](#1-executive-summary)
  - [2. Inventory of Changed Files](#2-inventory-of-changed-files)
  - [3. Dispositions for F6 and F7](#3-dispositions-for-f6-and-f7)
    - [3.1 Finding F6: `T81Fraction` Arithmetic Operator Exception Boundary](#31-finding-f6-`t81fraction`-arithmetic-operator-exception-boundary)
    - [3.2 Finding F7: `T81Fraction::to_double()` Large-Width Truncation and Abort](#32-finding-f7-`t81fractionto_double`-large-width-truncation-and-abort)
  - [4. Verification Evidence](#4-verification-evidence)
    - [4.1 Regression Suite Execution](#41-regression-suite-execution)
    - [4.2 Non-Regressions in Existing Test Suite](#42-non-regressions-in-existing-test-suite)
  - [5. Baseline CI Comparison and Attribution](#5-baseline-ci-comparison-and-attribution)
    - [5.1 Category A: Independently Verified Codebase Regressions (Inherited from Baseline)](#51-category-a-independently-verified-codebase-regressions-inherited-from-baseline)
    - [5.2 Category B: Environmental and Runner-Infrastructure Failures](#52-category-b-environmental-and-runner-infrastructure-failures)
  - [6. Merge-Policy Blockers and Residual Risks](#6-merge-policy-blockers-and-residual-risks)

<!-- T81-TOC:END -->


**Document ID:** T81-SCG-07-PHASE1-RECEIPT-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-07)
**Baseline Anchor:** Commit `7ba82e033d072e421b4851d156989203e503047e` (origin/main, merging PR #735)
**Date:** 2026-11-09
**Status:** IMPLEMENTATION CANDIDATE (NOT MERGED) — READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This receipt documents the bounded, candidate implementation for **SCG-07 Phase 1: F6/F7 Exception-Safety Repair in `t81::v1::T81Fraction`** (PR #736).
Following the verification in SCG-06 (PR #733 / PR #735) and maintainer policies D1=1B and D4=4B from SCG-04, this repair resolves the exception specification mismatch and conversion truncation issues identified under findings F6 and F7 without modifying normative specifications, ISA opcodes, or unrelated arithmetic components.

The implementation is rebased directly atop baseline commit `7ba82e033d072e421b4851d156989203e503047e`. All canonical SCG-06 reproduction evidence under `docs/research/continuity/evidence/scg06/` and `SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md` has been verified and preserved 100% unchanged against its canonical `SHA256SUMS.txt`.

---

## 2. Inventory of Changed Files

The PR diff is strictly isolated to the following 4 files against the merged PR #735 baseline:

| File | Change Description |
|---|---|
| `include/t81/types/T81Fraction.hpp` | Replaced throwing `noexcept` declarations on arithmetic operators (`+`, `-`, `*`, `/`, unary `-`), constructors, `canonicalize()`, and `operator<=>` with standard exception-propagating declarations. Replaced `to_double()` dependency on truncated `to_int64()` with safe wide-range `int_to_double()` accumulator. |
| `tests/cpp/test_t81fraction.cpp` | Added standalone regression test suite verifying F6 overflow catchability requiring strictly `std::overflow_error` without process abort, F7 wide-range conversion evaluated against an independent exact-integer rational oracle across widths 13..324, and normal rational arithmetic. |
| `CMakeLists.txt` | Registered `t81_t81fraction_test` executable and test runner in CMake build configuration. |
| `docs/research/continuity/SCG07_PHASE1_EVIDENCE_RECEIPT.md` | Published formal receipt and verification findings for independent audit. |

---

## 3. Dispositions for F6 and F7

### 3.1 Finding F6: `T81Fraction` Arithmetic Operator Exception Boundary
- **Pre-Repair Defect Mechanism:** Following SCG-03, `T81Int<N>` multiplication and arithmetic operations throw `std::overflow_error` (via `axion::trap_overflow`) on overflow. Because `T81Fraction` arithmetic operators were marked `noexcept`, an escaping exception invoked C++ `std::terminate()`, abruptly aborting the process (`SIGABRT`).
- **Phase 1 Resolution:** Removed `noexcept` specifiers from throwing methods:
  - `operator+`, `operator-`, `operator*`, `operator/`, unary `operator-`
  - `canonicalize()`, `abs_int()`, `gcd()`
  - `T81Fraction(const Int&, const Int&)` and `explicit T81Fraction(I)`
  - `operator<=>` (cross-multiplication can overflow `T81Int<N>`)
  - `from_double()` (continued fraction accumulation can overflow `T81Int<N>`)
- **Regression Assertion Tightening:** Test assertions in `tests/cpp/test_t81fraction.cpp` strictly require `std::overflow_error` without catch-all generic `std::exception` fallback.
- **Disposition:** **REPAIRED (EXCEPTION CONTRACT RECONCILED)**. Exceptions are catchable at the caller level across Debug and Release configurations without invoking `std::terminate()`.

### 3.2 Finding F7: `T81Fraction::to_double()` Large-Width Truncation and Abort
- **Pre-Repair Defect Mechanism:** `T81Fraction::to_double()` converted `num_` and `den_` by calling `to_int64()`. Because `to_int64()` throws `std::overflow_error` whenever $|x| > \frac{3^{40}-1}{2}$ (> 40 trits), calling `to_double()` on supported large-width fractions (e.g. `T81Fraction<81>`) with > 40 trits threw an uncaught exception out of a `noexcept` function, causing `std::terminate()`.
- **Phase 1 Resolution:** Implemented direct trit accumulator helper `int_to_double(const Int& val) noexcept`:
  - Accumulates balanced ternary digits directly into IEEE-754 binary64 float range.
  - Does not invoke `to_int64()`, eliminating the 40-trit barrier.
  - Successfully evaluates all admitted widths $13 \le \text{TotalTrits} \le 324$ within IEEE-754 binary64 range (since $3^{324} \approx 10^{154} < \text{DBL_MAX} \approx 1.8 \times 10^{308}$).
  - `to_double() const noexcept` is preserved as `noexcept` because `int_to_double` does not throw.
- **Accuracy and Scope Clarification:**
  - **Exception Safety Boundary:** Completely restored. Unconditional crashes and `std::terminate()` aborts are eliminated for all supported fraction widths up to 324 trits.
  - **Numerical Precision Limits:** Evaluated against an independent arbitrary-precision exact-integer rational oracle (`ExactBigIntOracle`, using base-$10^9$ limb vector arithmetic decoupled from T81 internal types) and analytical powers. Accuracy is bounded by standard IEEE-754 binary64 precision (~53 bits of mantissa, relative tolerance $10^{-12}$).
  - We explicitly document this as a finite-range floating-point approximation; we do **NOT** claim universally correct rounding for arbitrary balanced ternary rationals (which remains governed by future RFC-00D5 / SCG-04 D2 work).
- **Disposition:** **REPAIRED (EXCEPTION-SAFETY BOUNDARY RESTORED / NUMERICAL CONVERSION APPROXIMATION BOUNDED)**.

---

## 4. Verification Evidence

### 4.1 Regression Suite Execution
The dedicated regression suite `t81_t81fraction_test` was compiled and executed across both Debug and Release configurations:

- **Debug Build (`CMAKE_BUILD_TYPE=Debug`):**
  - `test_f6_multiplication_overflow_catchable`: PASSED. Caught strictly `std::overflow_error` (`T81Int overflow`) from $3^{45} \times 3^{45} > 81\text{ trits}$ without process abort. Validated normal product $3^{20} \times 3^{20} = 3^{40}$ succeeds.
  - `test_f6_addition_subtraction_overflow`: PASSED. Caught strictly `std::overflow_error` on $(3^{80}/1) + (3^{80}/1)$ and $- (3^{80}/1) - (3^{80}/1)$.
  - `test_f7_to_double_boundary_widths`: PASSED. Verified against both analytical values and the independent exact-integer rational oracle (`ExactBigIntOracle`) within IEEE-754 precision tolerance ($10^{-12}$):
    - Width 13: 0.0, 13.5, -13.5.
    - Width 40: $3^{39} \approx 4.05256 \times 10^{18}$.
    - Width 41: $3^{40} \approx 1.21577 \times 10^{19}$ and $-3^{40}$.
    - Width 81: $3^{45} \approx 2.95431 \times 10^{21}$, $3^{80} \approx 1.47809 \times 10^{38}$, fractional $1 / 3^{45}$, ratio $3^{80} / 3^{45} = 3^{35}$, and non-trivial ratio $(3^{70} - 1) / (3^{35} + 2)$.
    - Width 324: $3^{150} \approx 3.69988 \times 10^{71}$, $-3^{150}$, and non-trivial ratio $(3^{300} - 3^{100} + 1) / (3^{100} + 2)$ positive and negative.
  - `test_normal_rational_arithmetic`: PASSED. Reduction (3/4, -3/4), zero denominator canonicalization (0/1), addition, subtraction, multiplication, division, and ordering comparisons.
- **Release Build (`CMAKE_BUILD_TYPE=Release`):**
  - Full suite passed with identical behavior and zero aborts.
- **Formatting Hygiene:**
  - `clang-format-18 --dry-run --Werror include/t81/types/T81Fraction.hpp tests/cpp/test_t81fraction.cpp` passed with zero violations.

### 4.2 Non-Regressions in Existing Test Suite
- `t81_fraction_test`: PASSED.
- `t81_fraction_properties_test`: PASSED.
- `t81_t81int_test`: PASSED.
- `t81_numeric_format_test`: PASSED.

---

## 5. Baseline CI Comparison and Attribution

To verify that PR #736 introduces zero regressions against the exact merged baseline `7ba82e033d072e421b4851d156989203e503047e` (PR #735), all CI check run failures on check suite `42548303909` were analyzed and attributed across two distinct categories:

### 5.1 Category A: Independently Verified Codebase Regressions (Inherited from Baseline)
These failures stem from verifiable defects in checked-in test scripts, missing fixture files, or workflow configurations already present on `origin/main`:
1. `cli_contract_test_runner` (line 549): Asserts existence of `safetensors_fixture` (`models/tiny-random-llama-exported.safetensors` or `models/tiny-random-llama/model.safetensors`). Both directories and fixtures are absent from the git repository.
2. `run_model_artifact_review_smoke.sh`: Exits with code 1 due to `missing source fixture at models/tiny-random-llama/model.safetensors`.
3. `t81_root_structure_policy_test`: `tests/ci/check_root_structure_test.py` calls `scripts/governance/check_root_structure.py`, which fails because an untracked/unrecognized directory `review/` exists in the repo root on `origin/main` (committed in earlier PR #656 review).
4. `build / windows-x86_64 / clang-cl` and `build / windows-x86_64 / msvc`: CMake generation fails at `src/direct_api/CMakeLists.txt:26` because `development_tests/test_real_direct_api.cpp` is missing under the sparse checkout path defined at `.github/workflows/ci.yml:476-505`.

### 5.2 Category B: Environmental and Runner-Infrastructure Failures
These failures are triggered by external toolchain versions, platform ABI mismatches, or deprecated runner labels on GitHub Actions:
1. `gate / tritwise-determinism / avx2-asan`: GCC 15 `-Werror=maybe-uninitialized` false-positive in libstdc++ `regex_automaton.h` / `std_function.h` when compiling Google Benchmark v1.9.5 (`benchmark_register.cc.o`).
2. `build / macos-x86_64 / gcc`: Linker symbol resolution failure on macOS x86_64 caused by an ABI mismatch between Homebrew's LLVM libc++ (used when building `libgtest.a`) and GCC 14's libstdc++.
3. `build / macos-arm64 / clang`: Cancelled immediately upon job startup because runner label `macos-14-arm64-us-default` is no longer supported by GitHub Actions.

In all jobs where test execution proceeded, `t81_t81fraction_test` and all fraction-related arithmetic tests passed 100%.

---

## 6. Merge-Policy Blockers and Residual Risks

1. **Required Quality Gate Policy Blocker:**
   - The repository quality gate workflow `.github/workflows/ci.yml` mandates that required check runs (`gate / tritwise-determinism / avx2-asan`, `build / sanitizers`, `build / linux-x86_64 / clang`, etc.) pass before automatic merging.
   - Because these jobs fail due to inherited upstream baseline defects (missing fixtures, root directory structure violation, Windows sparse checkout omission, and GCC 15 Google Benchmark warnings), PR #736 **CANNOT** pass the required quality gate aggregator without maintainer waivers or upstream baseline repairs.
   - **Resolution Policy:** Maintainers must either (a) merge baseline fixes for the fixture, structure, and sparse checkout defects, or (b) apply administrative branch-protection waivers for PR #736 based on this independent attribution evidence.
2. **F7 Numerical Conversion Approximation Bounds:**
   - Exception safety is completely restored: `to_double() const noexcept` will never throw or call `std::terminate()` for any admitted width $13 \le \text{TotalTrits} \le 324$.
   - Numerical precision is explicitly bounded by standard IEEE-754 binary64 limits (~53 bits of mantissa). Values with $>53$ significant bits are rounded according to host floating-point arithmetic. Universal correct rounding for balanced ternary rationals remains unaddressed and reserved for RFC-00D5 / SCG-04 D2.
3. **Zero Scope Expansion:**
   - PR #736 strictly modifies only the 4 authorized files. No ISA changes, OS kernel modifications, or unrelated arithmetic changes have been introduced.
