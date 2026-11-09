# SCG-07 Phase 2 Evidence Receipt: F5 T81BigInt Signed Subtraction Repair

**Document ID:** T81-SCG-07-PHASE2-RECEIPT-001-R1
**Author:** T81 Continuity Engineering Task Force (SCG-07)
**Baseline Anchor:** Commit `e24eba723c221dc895e20307d41a8c4ba97405d6` (origin/main, merging PR #736)
**Date:** 2026-11-09
**Status:** IMPLEMENTATION CANDIDATE (NOT MERGED) — READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This receipt documents the bounded, candidate implementation for **SCG-07 Phase 2: F5 T81BigInt Signed Subtraction Repair** in `t81::v1::T81BigInt`.
Following the empirical reproduction in SCG-06 (Issue #717, PR #733 / PR #735), the exception safety repairs in SCG-07 Phase 1 (PR #736), and maintainer policy selections D1=1B and D4=4B from SCG-04, this repair resolves finding F5 without modifying normative specifications, ISA opcodes, or unrelated arithmetic components.

The implementation is rebased directly atop baseline commit `e24eba723c221dc895e20307d41a8c4ba97405d6`. All canonical SCG-06 reproduction evidence and preceding SCG-07 Phase 1 evidence are preserved unchanged.

---

## 2. Inventory of Changed Files

The PR diff is strictly isolated to the following 4 files against the merged PR #736 baseline:

| File | Change Description |
|---|---|
| `include/t81/types/T81BigInt.hpp` | Repaired single-limb fallback path in `operator+(const T81BigInt&, const T81BigInt&)` to inspect `val.sign_trit() == Trit::N`, inverting `negative_ = true` and storing `-val` so the limb magnitude remains non-negative in canonical form. |
| `tests/cpp/test_t81bigint_sub.cpp` | Standalone regression test suite verifying: original F5 reproducer ($(3^{79}+7) - (3^{80}+11)$); F5 single-limb cancellation ($(3^{79}+7) - (3^{79}+11) = -4$); complete sign-combination matrix (+-+, +- -, - -+, - - -); zero and integer boundaries; multi-limb borrow propagation (160 and 243 trits); and 1,200 randomized trials evaluated against an independent exact-integer arbitrary-precision oracle (`ExactBigIntOracle`, base $10^9$). |
| `CMakeLists.txt` | Registered `t81_bigint_sub_test` executable and test runner in CMake build configuration. |
| `docs/research/continuity/SCG07_PHASE2_EVIDENCE_RECEIPT.md` | Formal receipt and verification report for independent audit. |

---

## 3. Finding F5 Root Cause & Repair Disposition

### 3.1 Pre-Repair Defect Mechanism
`T81BigInt` represents multi-precision balanced ternary integers using magnitude-oriented limbs (`limbs_`) and a separate sign flag (`negative_`). Under the canonical representation invariant:
1. Every limb in `limbs_` must have a non-negative most significant trit (`sign_trit() != Trit::N`).
2. The sign of the integer is represented exclusively by `negative_`.

In `operator+(const T81BigInt& a, const T81BigInt& b)` (which `operator-` calls by negating the right operand), single-limb values with operand significant trits up to `kLimbTrits` (81 trits) entered the fallback block:
```cpp
// Pre-repair lines 1037-1046:
try {
  T81BigInt res;
  res.limbs_.clear();
  res.limbs_.push_back(a.negative_ ? -a.limbs_[0] : a.limbs_[0]);
  Limb rhs = b.negative_ ? -b.limbs_[0] : b.limbs_[0];
  res.limbs_[0] = res.limbs_[0] + rhs;
  res.negative_ = false;
  res.normalize();
  return res;
} catch (const std::overflow_error&) {
  // Fall through
}
```
When evaluating $X - Y$ where $X < Y$ (such as $(3^{79}+7) - (3^{80}+11)$ or $(3^{79}+7) - (3^{79}+11)$):
- The signed sum `res.limbs_[0] + rhs` produced a negative limb (`val.sign_trit() == Trit::N`).
- `res.negative_` was unconditionally set to `false`.
- `res.normalize()` called `verify_invariants()`, which checked `limbs_.back().sign_trit() == Trit::N`. In Debug builds, this violated the invariant and called `std::terminate()`.
- In Release builds (`-DNDEBUG`), invariant checks were compiled away, leaving a negative limb stored with `negative_ = false`. Subsequent operations (such as `to_decimal_string()` or division) invoked `to_std_chunks()`, which attempted to borrow from higher limbs and threw `std::logic_error("to_std_chunks: borrow beyond MSB (input effectively negative)")`.

### 3.2 Phase 2 Mathematical Repair
The fallback path now evaluates the signed limb sum and canonicalizes the sign:
```cpp
Limb lhs = a.negative_ ? -a.limbs_[0] : a.limbs_[0];
Limb rhs = b.negative_ ? -b.limbs_[0] : b.limbs_[0];
Limb val = lhs + rhs;
if (val.sign_trit() == Trit::N) {
  res.negative_ = true;
  res.limbs_.push_back(-val);
} else {
  res.negative_ = false;
  res.limbs_.push_back(val);
}
res.normalize();
return res;
```
If `val` is negative, `res.negative_` is set to `true` and the magnitude is stored as `-val` (which has a positive most significant trit). If `val` is zero or positive, `res.negative_` is `false` and `val` is stored directly. `res.normalize()` then trims any leading zeros and verifies invariants.

---

## 4. Verification Evidence

### 4.1 Test Execution Matrix

| Test Suite | Configuration | Command | Result |
|---|---|---|---|
| `t81_bigint_sub_test` | Debug | `cmake --build build --target t81_bigint_sub_test && ./build/t81_bigint_sub_test` | **PASS (100%)** |
| `t81_bigint_sub_test` | Release (`-DNDEBUG`) | `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build --target t81_bigint_sub_test && ./build/t81_bigint_sub_test` | **PASS (100%)** |
| ASan / UBSan | Address/Undefined Sanitizer | `g++ -std=c++20 -fsanitize=address,undefined -g -Iinclude tests/cpp/test_t81bigint_sub.cpp -o /tmp/asan_sub_test && /tmp/asan_sub_test` | **PASS (zero leaks/errors)** |
| `t81_bigint_test` | Roundtrip suite | `./build/t81_bigint_test` | **PASS** |
| `t81_bigint_properties_test` | Properties & invariants | `./build/t81_bigint_properties_test` | **PASS** |
| `t81_bigint_division_edge_properties_test` | Division edge properties | `./build/t81_bigint_division_edge_properties_test` | **PASS** |
| `t81_bigint_division_semantics_test` | Division semantics | `./build/t81_bigint_division_semantics_test` | **PASS** |
| `t81_bigint_gcd_divmod_property_test` | GCD / divmod property suite | `./build/t81_bigint_gcd_divmod_property_test` | **PASS** |
| `t81_bigint_modular_inverse_test` | Modular inverse suite | `./build/t81_bigint_modular_inverse_test` | **PASS** |
| `t81_bigint_v1_baseline_test` | Baseline test suite | `./build/t81_bigint_v1_baseline_test` | **PASS** |
| `t81_t81fraction_test` | Phase 1 rational test suite | `./build/t81_t81fraction_test` | **PASS** |
| `clang-format-18` | Style conformance | `clang-format-18 --dry-run --Werror include/t81/types/T81BigInt.hpp tests/cpp/test_t81bigint_sub.cpp` | **PASS (0 violations)** |

### 4.2 Independent Oracle Comparison
The regression test compares all 1,200 randomized test vectors against an independent base-$10^9$ arbitrary-precision integer oracle (`ExactBigIntOracle`). All test cohorts (Cohort A: arbitrary 64-bit integers; Cohort B: multi-limb power-of-three boundaries 40..120 trits; Cohort C: sign-inversion and involution) match with 0 discrepancies.

---

## 5. Scope Constraints & Remaining Limitations

1. **Defect Scope:** Only finding F5 (`T81BigInt` signed subtraction) is addressed in Phase 2. Core limb multiplication defects (F1 Booth table scaling, F2 `reference_mul` carry normalization, and F3 Karatsuba carry discard) remain untouched for Phase 3 and Phase 4.
2. **Normative Stability:** No changes have been made to normative specifications under `spec/` or ISA opcode allocations.
3. **Capacity Boundaries:** `T81BigInt` multi-limb representation supports arbitrary precision limited only by host system memory and chunk allocation limits.
