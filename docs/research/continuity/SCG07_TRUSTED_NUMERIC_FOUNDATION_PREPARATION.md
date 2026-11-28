# SCG-07: Trusted Numeric Foundation — Engineering Handoff Preparation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [SCG-07: Trusted Numeric Foundation — Engineering Handoff Preparation](#scg-07-trusted-numeric-foundation-—-engineering-handoff-preparation)
  - [1. Executive Summary & Purpose](#1-executive-summary-&-purpose)
    - [Fundamental Principle](#fundamental-principle)
  - [2. Inventory of Arithmetic Defects and Component Boundaries](#2-inventory-of-arithmetic-defects-and-component-boundaries)
  - [3. The Exception-Contract Incompatibility (F6 & F7)](#3-the-exception-contract-incompatibility-f6-&-f7)
    - [3.1 Architectural Problem](#31-architectural-problem)
    - [3.2 Dual-Surface Reconciliation Strategy](#32-dual-surface-reconciliation-strategy)
  - [4. Proposed Independent Regression Tests for F6 and F7](#4-proposed-independent-regression-tests-for-f6-and-f7)
    - [4.1 F6 Multiplicative Overflow Test Suite](#41-f6-multiplicative-overflow-test-suite)
    - [4.2 F7 Floating-Point Conversion Test Suite](#42-f7-floating-point-conversion-test-suite)
  - [5. `T81BigInt` Subtraction Sign & Borrow Invariants (F5)](#5-`t81bigint`-subtraction-sign-&-borrow-invariants-f5)
    - [5.1 Root Cause in `T81BigInt::operator+`](#51-root-cause-in-`t81bigintoperator+`)
    - [5.2 Required Invariant Contract](#52-required-invariant-contract)
  - [6. Dependencies on SCG-04 Architectural Decisions](#6-dependencies-on-scg-04-architectural-decisions)
  - [7. Proposed Repair Order and Acceptance Criteria](#7-proposed-repair-order-and-acceptance-criteria)
    - [Acceptance Criteria for SCG-07](#acceptance-criteria-for-scg-07)
  - [8. Explicit Authorization Requirements](#8-explicit-authorization-requirements)

<!-- T81-TOC:END -->


**Document ID:** T81-SCG-07-PREP-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-06 / SCG-07)
**Status:** DRAFT / PREPARATORY / NON-NORMATIVE
**Baseline Anchor:** Commit `2c3501e2807bb893076c5037231c3126b7f2bc72` (Merge PR #729)
**Preceding Audit:** `docs/research/continuity/SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md` (PR #733)
**Target Workstream:** SCG-07 Trusted Numeric Foundation

---

## 1. Executive Summary & Purpose

This document provides a bounded, non-normative engineering handoff for **SCG-07 (Trusted Numeric Foundation)**. It synthesizes the empirical reproduction evidence established in SCG-06 (Issue #717, PR #733) into an actionable repair sequencing plan and verification framework for subsequent maintainer authorization.

### Fundamental Principle
**Execution completion is an observation. Correctness is a claim requiring independent evidence.**
This document does **NOT** authorize or implement changes to production code, normative specifications, public API signatures, or ISA opcodes. It defines the technical boundaries, component invariants, exception contract reconciliations, and test prerequisites required before any repair PR may be initiated.

---

## 2. Inventory of Arithmetic Defects and Component Boundaries

The SCG-06 empirical reproduction established the exact status of findings F1–F7 across components:

| Finding | Component | Current Implementation State | Fault Mechanism | Component Boundary Impact |
|---|---|---|---|---|
| **F1** | `t81::core::T81Limb27` | **CONFIRMED UNCHANGED** | Booth table `build_booth27_table()` assigns `shift_trytes = 1` (scales by $3^3 = 27$) for patterns $\pm 2, \pm 4$ instead of multiplying by 2 or 4. | Internal limb multiplication; corrupts `T81Limb27::operator*`. |
| **F2** | `t81::core::T81Limb54` | **CONFIRMED UNCHANGED** | `reference_mul()` uses single-pass carry correction (`accum[i] = sum - 3; carry = 1;`). Fails on accumulators $|s| > 4$, leaving invalid non-canonical digits up to $\pm 15$ and producing mathematical value mismatches in 141/300 trials. | 54-trit reference multiplication; circular oracle validation in tests. |
| **F3** | `t81::core::T81Limb54` | **CONFIRMED UNCHANGED** | `karatsuba()` radical sum $x_0 + x_1$ discards 28th-trit carry in `T81Limb27::operator+`. In 68/68 cases with carry loss, products fail. Furthermore, 231/232 cases without carry loss also fail due to cascading multi-component defects (including demonstrated F1 Booth table scaling alongside intermediate subtraction/shift errors). | Karatsuba fast limb multiplication; multi-limb cross-lane integrity. |
| **F4** | `t81::T81Int<N>` | **CONFIRMED FIXED** | Fixed in SCG-03 (PR #729) via full $2N$-trit convolution accumulation and balanced ternary carry normalization before range check. Overflow reliably traps via `axion::trap_overflow("T81Int overflow")`. | Core fixed-width balanced ternary integer arithmetic. |
| **F5** | `t81::v1::T81BigInt` | **CONFIRMED UNCHANGED** | Single-limb subtraction fallback (`operator+` when adding opposite signs) sets `negative_ = false` and calls `normalize()`, which invokes `verify_invariants()`. For negative results (e.g. $(3^{79}+7) - (3^{79}+11) = -4$), the MSB limb is negative, triggering `std::terminate()` in Debug or uncaught `std::runtime_error("borrow beyond MSB")` in Release. | Multi-precision integer arithmetic; sign and borrow representation. |
| **F6** | `t81::v1::T81Fraction<N>` | **CONFIRMED FAILURE-MODE CHANGE** | Under pre-SCG-03, numerator overflow wrapped silently to 0. Post-SCG-03, `T81Int<N>::operator*` properly throws `std::overflow_error`. However, `T81Fraction::operator*` is explicitly marked `noexcept`. Escaping exceptions invoke `std::terminate()` (process abort, `SIGABRT`). | Fractional arithmetic operator surface; contract mismatch with throwing integers. |
| **F7** | `t81::v1::T81Fraction<N>` | **CONFIRMED UNCHANGED** | `to_double()` is marked `noexcept`, but calls `num_.to_int64()`, which throws `std::overflow_error` whenever $|num| > \frac{3^{40}-1}{2}$ (>40 trits). The exception escapes `noexcept`, invoking `std::terminate()`. | Floating-point conversion bridge; conversion survivability. |

---

## 3. The Exception-Contract Incompatibility (F6 & F7)

### 3.1 Architectural Problem
The SCG-03 repair of `T81Int` created a severe contract mismatch across library layers:
1. `T81Int<N>::operator*` now throws `std::overflow_error` upon arithmetic overflow.
2. `T81Fraction<N>::operator*` delegates directly to `num_ * rhs.num_` and `den_ * rhs.den_`, but declares:
   ```cpp
   [[nodiscard]] constexpr T81Fraction operator*(const T81Fraction& rhs) const noexcept;
   ```
3. In C++20, if an exception is thrown from inside a function declared `noexcept`, `std::terminate()` is invoked immediately, bypassing all caller `try/catch` handlers and terminating the host process.
4. Similarly, `T81Fraction<N>::to_double() const noexcept` delegates to `num_.to_int64()` and `den_.to_int64()`. For fractions exceeding 40 trits (which `T81Fraction<81>` standardly permits), `to_int64()` throws, instantly crashing the calling application.

### 3.2 Dual-Surface Reconciliation Strategy
In accordance with maintainer policy selection **D1=1B** (SCG-04 Maintainer Decision Record):
- **Surface A (Standard C++ Operators):** Must define clear, documented exception safety guarantees. Either `noexcept` must be conditionally removed from `T81Fraction` arithmetic operators (matching `T81Int`), or checked sentinel states must be adopted.
- **Surface B (Checked / Fallible Interface):** In accordance with maintainer policy selection **D4=4B**, checked arithmetic methods must return generic `Result<T, E>` without throwing exceptions and without invoking `std::terminate()`.

---

## 4. Proposed Independent Regression Tests for F6 and F7

SCG-07 must introduce standalone test fixtures that rigorously verify the boundary behavior without process aborts:

### 4.1 F6 Multiplicative Overflow Test Suite
- **Test Objective:** Verify that overflowing fractions do not silently corrupt values, nor unconditionally abort processes when callers handle errors.
- **Harness Architecture:**
  - Execute test cases in isolated subprocesses or dedicated C++ test suites (`tests/cpp/test_t81fraction.cpp`).
  - Test operand magnitudes:
    - Normal range: $\frac{3^{20}}{1} \times \frac{3^{20}}{1} = \frac{3^{40}}{1} \le 81\text{ trits}$ (must succeed).
    - Overflow boundary: $\frac{3^{40}}{1} \times \frac{3^{41}}{1} = \frac{3^{81}}{1}$ (capacity boundary).
    - Hard overflow: $\frac{3^{45}}{1} \times \frac{3^{45}}{1} = \frac{3^{90}}{1} > 81\text{ trits}$.
  - Expected behavior:
    - If operator throws: must catch `std::overflow_error` at call site without `std::terminate()`.
    - If checked API: must return `Result<T81Fraction, ArithmeticError>::err(Overflow)`.

### 4.2 F7 Floating-Point Conversion Test Suite
- **Test Objective:** Verify that `to_double()` operates safely across all valid balanced ternary fraction representations up to the full width $N$.
- **Harness Architecture:**
  - Test fractions with numerators spanning 1 to 81 trits:
    - Small width ($\le 40$ trits): exact comparison against `double(num.to_int64()) / double(den.to_int64())`.
    - Large width ($41 \le \text{trits} \le 81$): conversion must not call `to_int64()` directly. Must use scaling exponent arithmetic (e.g. `std::ldexp` or balanced ternary floating power accumulator) to compute IEEE-754 binary64 representation.
  - Assert that no exception escapes and no process abort occurs.

---

## 5. `T81BigInt` Subtraction Sign & Borrow Invariants (F5)

### 5.1 Root Cause in `T81BigInt::operator+`
`T81BigInt` represents multi-limb integers as `std::vector<T81Limb>` paired with a sign flag `negative_`.
In the single-limb fallback:
```cpp
// Fallback to single-limb
T81BigInt res;
res.limbs_.resize(1);
res.limbs_[0] = limbs_[0] + rhs.limbs_[0];
res.negative_ = false;
res.normalize();
return res;
```
When subtracting $Y$ from $X$ where $X < Y$, $X + (-Y)$ evaluates to a negative single limb.
Setting `res.negative_ = false` forces the limb to be interpreted as positive. `normalize()` subsequently invokes `verify_invariants()`:
```cpp
void verify_invariants() const {
  if (limbs_.empty()) return;
  // Most significant limb must not have sign trit matching negative_
  // In Debug: asserts limbs_.back().sign_trit() != Trit::N
}
```
Because the limb's most significant trit is negative, the invariant assertion triggers `std::terminate()`.

### 5.2 Required Invariant Contract
1. **Limb Canonical Sign:** All limbs in a normalized `T81BigInt` must be strictly magnitude-oriented, with the overall sign captured exclusively by `negative_`.
2. **Borrow Propagation:** If limb-wise subtraction results in an unborrowed negative MSB limb, the sign flag `negative_` must be inverted (`negative_ = true`) and the limb contents negated to restore positive-canonical representation.
3. **Debug / Release Equivalence:** Invariant checks must behave consistently across Debug and Release builds, avoiding divergent failure modes (`std::terminate()` vs uncaught `std::runtime_error`).

---

## 6. Dependencies on SCG-04 Architectural Decisions

SCG-07 repairs must strictly align with the Maintainer Decision Record established in SCG-04 (`docs/research/continuity/SCG04_MAINTAINER_DECISION_RECORD.md`):

1. **Decision D1 (Dual-Surface Arithmetic Architecture):**
   - Retain standard C++ arithmetic operators (`+`, `-`, `*`, `/`) with consistent exception semantics.
   - Introduce additive checked arithmetic methods (`checked_add`, `checked_sub`, `checked_mul`, `checked_div`) that do not throw exceptions.
2. **Decision D2 (RFC-00D5 Checked Numerical Conversion Profile):**
   - Provide explicit, bounded conversion between binary64 / integer types and balanced ternary representations.
   - Reconcile `to_double()` to follow checked conversion rules for high-magnitude fractions.
3. **Decision D4 (Generic Result Surface):**
   - Checked arithmetic surfaces in `T81Int`, `T81Fraction`, and `T81BigInt` must return generic `Result<T, E>` where `E` preserves exact failure causes (`Overflow`, `DivisionByZero`, `InvalidRepresentation`).

---

## 7. Proposed Repair Order and Acceptance Criteria

Repairs in SCG-07 should be partitioned into four sequential, independent phases to prevent cascading regressions:

```
+-------------------------------------------------------------------------------+
| Phase 1: Exception Safety Contract Reconciliation (F6, F7)                    |
| - Remove noexcept from throwing T81Fraction operators                         |
| - Implement wide-range to_double() without to_int64() truncation              |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
| Phase 2: Multi-Precision Subtraction and Invariants (F5)                      |
| - Repair single-limb fallback in T81BigInt operator+                          |
| - Enforce sign_trit normalization and borrow consistency                      |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
| Phase 3: Core Limb Multiplication and Carry Normalization (F1, F2)            |
| - Repair build_booth27_table() shift scaling in T81Limb27                     |
| - Implement complete multi-pass carry normalization in reference_mul()        |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
| Phase 4: Karatsuba Radical Sum Carry Propagation (F3)                         |
| - Handle 28th-trit carry in Karatsuba mid-term accumulation                   |
| - Validate against independent Python exact-integer oracle                    |
+-------------------------------------------------------------------------------+
```

### Acceptance Criteria for SCG-07
1. **Zero Silent Corruptions:** No operation wraps silently or returns truncated values without error indication.
2. **Zero Unexpected Aborts:** No valid or catchable error condition invokes `std::terminate()` or `SIGABRT` across Debug and Release builds.
3. **Exact Mathematical Equivalence:** All fixed operations pass 1,000 randomized test cases against the independent exact-integer oracle (`oracle.py`).
4. **CI Cleanliness:** All C++ changes format cleanly under `clang-format-18` and compile without warnings under `-Wall -Wextra -Werror`.

---

## 8. Explicit Authorization Requirements

Before any production modifications are undertaken:
1. **Maintainer Authorization Required:** Formal approval of SCG-07 scope by repository maintainers.
2. **Boundary Enforcement:**
   - No modifications to normative specifications (`spec/`).
   - No ISA opcode changes or allocations.
   - No performance or benchmarking claims.
3. **Commitment to Small, Defensible Pull Requests:** Each repair phase (Phases 1–4) must be submitted as a separate, independently reviewable PR with complete test coverage.
