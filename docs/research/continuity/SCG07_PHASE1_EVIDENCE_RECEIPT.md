# SCG-07 Phase 1 Evidence Receipt: F6/F7 Exception-Safety Repair

**Document ID:** T81-SCG-07-PHASE1-RECEIPT-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-07)
**Baseline Anchor:** Commit `7ba82e033d072e421b4851d156989203e503047e` (origin/main)
**Date:** 2026-11-09
**Status:** IMPLEMENTATION CANDIDATE (NOT MERGED) — READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This receipt documents the bounded, candidate implementation for **SCG-07 Phase 1: F6/F7 Exception-Safety Repair in `t81::v1::T81Fraction`**.
Following the verification in SCG-06 (PR #733 / PR #735) and the handoff architecture in `SCG07_TRUSTED_NUMERIC_FOUNDATION_PREPARATION.md`, this repair resolves the exception specification mismatch and conversion truncation issues identified under findings F6 and F7 without modifying normative specifications, ISA opcodes, or unrelated arithmetic components.

All canonical SCG-06 reproduction evidence under `docs/research/continuity/evidence/scg06/` and `SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md` has been verified and preserved 100% unchanged against its canonical `SHA256SUMS.txt`.

---

## 2. Inventory of Changes

| File | Change Description |
|---|---|
| `include/t81/types/T81Fraction.hpp` | Replaced throwing `noexcept` declarations on arithmetic operators (`+`, `-`, `*`, `/`, unary `-`), constructors, `canonicalize()`, and `operator<=>` with standard exception-propagating declarations. Replaced `to_double()` dependency on truncated `to_int64()` with safe wide-range `int_to_double()` accumulator. |
| `tests/cpp/test_t81fraction.cpp` | Added standalone regression test suite verifying F6 overflow catchability without `std::terminate()`, F7 wide-range conversion accuracy against mathematical references, and normal rational arithmetic across supported widths. |
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
- **Disposition:** **REPAIRED (EXCEPTION CONTRACT RECONCILED)**. Exceptions are catchable at the caller level across Debug and Release configurations without invoking `std::terminate()`.

### 3.2 Finding F7: `T81Fraction::to_double()` Large-Width Truncation and Abort
- **Pre-Repair Defect Mechanism:** `T81Fraction::to_double()` converted `num_` and `den_` by calling `to_int64()`. Because `to_int64()` throws `std::overflow_error` whenever $|x| > \frac{3^{40}-1}{2}$ (> 40 trits), calling `to_double()` on supported large-width fractions (e.g. `T81Fraction<81>`) with > 40 trits threw an uncaught exception out of a `noexcept` function, causing `std::terminate()`.
- **Phase 1 Resolution:** Implemented exact wide-width conversion helper `int_to_double(const Int& val) noexcept`:
  - Accumulates balanced ternary digits directly using `long double` weighted powers ($3^i$).
  - Does not invoke `to_int64()`, eliminating the 40-trit barrier.
  - Successfully evaluates all admitted widths $13 \le \text{TotalTrits} \le 324$ within IEEE-754 binary64 range (since $3^{324} \approx 10^{154} < \text{DBL_MAX} \approx 1.8 \times 10^{308}$).
  - `to_double() const noexcept` is preserved as `noexcept` because `int_to_double` does not throw.
- **Disposition:** **REPAIRED (WIDE CONVERSION BOUNDARY RESTORED)**. Validated against high-precision mathematical references across widths 13, 40, 41, 81, and 324.

---

## 4. Verification Evidence

### 4.1 Regression Suite Execution
The dedicated regression suite `t81_t81fraction_test` was compiled and executed across both Debug and Release configurations:

- **Debug Build (`CMAKE_BUILD_TYPE=Debug`):**
  - `test_f6_multiplication_overflow_catchable`: PASSED. Caught `std::overflow_error` (`T81Int overflow`) from $3^{45} \times 3^{45} > 81\text{ trits}$ without process abort. Validated normal product $3^{20} \times 3^{20} = 3^{40}$ succeeds.
  - `test_f6_addition_subtraction_overflow`: PASSED. Caught overflow on $(3^{80}/1) + (3^{80}/1)$ and $- (3^{80}/1) - (3^{80}/1)$.
  - `test_f7_to_double_boundary_widths`: PASSED. Verified against high-precision mathematical references ($10^{-12}$ relative tolerance):
    - Width 13: 0.0, 13.5, -13.5.
    - Width 40: $3^{39} \approx 4.05256 \times 10^{18}$.
    - Width 41: $3^{40} \approx 1.21577 \times 10^{19}$ and $-3^{40}$.
    - Width 81: $3^{45} \approx 2.95431 \times 10^{21}$, $3^{80} \approx 1.47809 \times 10^{38}$, fractional $1 / 3^{45}$, and ratio $3^{80} / 3^{45} = 3^{35}$.
    - Width 324: $3^{150} \approx 3.69988 \times 10^{71}$ and $-3^{150}$.
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

## 5. Compatibility and Residual-Risk Assessment

1. **Source Compatibility:**
   - Existing caller code using `T81Fraction` arithmetic continues to compile without changes.
   - Removing `noexcept` allows callers to wrap fraction arithmetic in `try { ... } catch (...)` blocks to handle overflows gracefully, whereas previously any overflow caused unrecoverable process termination.
2. **ABI / Exception Specification Compatibility:**
   - In C++17 and later, exception specifications are part of the function type. Code taking function pointers to `T81Fraction::operator*` would observe a type change (`noexcept` omitted). An audit of the codebase confirms no function pointers or templates requiring `noexcept` arithmetic operator signatures exist for `T81Fraction`.
3. **Residual Numerical Conversion Decisions (SCG-04 D2 / RFC-00D5):**
   - Phase 1 repairs `to_double()` for balanced ternary fractions up to the maximum template width 324, which fits within the IEEE-754 binary64 range ($\le 10^{308}$).
   - Any wider arbitrary-precision conversion beyond 324 trits or into ternary float formats is governed by RFC-00D5 and will be evaluated in subsequent SCG-07 phases.
