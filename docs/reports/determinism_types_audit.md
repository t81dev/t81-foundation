# Determinism Fix Report

## 1. Executive Summary

This report documents the stress testing, analysis, and remediation of the T81 Core Data Types to ensure strict determinism across all platforms. The goal is to identify and eliminate any sources of nondeterminism, including host-dependent math, unstable iteration orders, and non-canonical representations.

## 2. Determinism Surface Table

| Type | Status | Severity | Invariant Broken | Root Cause | Fix Applied | Tests Added |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Cell` | Verified | - | - | - | Added shift overflow checks | `test_primitives.cpp` |
| `T81Int` | Verified | - | - | - | - | `test_primitives.cpp` |
| `T81BigInt` | Verified | - | - | - | - | `test_primitives.cpp` |
| `T81Float` | Verified | Medium | Host-dependent Math | `cmath` usage in transcendentals | Warning on host-math; `to_canonical_string` fixes | `test_float.cpp` |
| `T81Map` | Verified | High | Iteration Order | Unordered internal storage | `iter_sorted()` verified stable; `serialize_canonical` stable | `test_containers.cpp` |
| `T81Set` | Verified | High | Iteration Order | Unordered internal storage | - | `test_containers.cpp` |
| `T81Vector` | Verified | - | - | - | Added arithmetic type bridge for scalars | `test_vector.cpp` |
| `T81Fixed` | Verified | - | - | - | - | `test_math.cpp` |
| `T81Fraction`| Verified | - | - | - | - | `test_math.cpp` |
| `T81Complex` | Verified | - | - | - | - | `test_math.cpp` |

## 3. Per-Type Notes

*   **`Cell`**: 5-trit balanced ternary logic. Added runtime check for overflow in left-shift (`<<`) to prevent silent loss of non-zero trits.
*   **`T81Int`**: Uses packed trits. Verified arithmetic and string conversion.
*   **`T81BigInt`**: Uses `T81Int` limbs. Verified arithmetic (including Karatsuba) and canonical Base81 string roundtrip. `operator<<` and `string` ctor are not part of the core API, necessitating test adjustments.
*   **`T81Float`**: Transcendental functions (`acos`, `asin`, `pow`, etc.) rely on host `cmath` unless `T81_DETERMINISTIC` is defined. This remains a known limitation for Phase 1. Fixed `to_canonical_string` to correctly handle signed zero (`+0E0` vs `-0E0`).
*   **`T81Map`**: Iteration is inherently unstable due to open addressing. Canonical serialization (`serialize_canonical`) uses key sorting and was verified to be deterministic. Users must use `iter_sorted()` for deterministic traversal.
*   **`T81Vector`**: Verified geometric operations. Added template support to bridge arithmetic types (like `double`) to `T81Float` scalars automatically, improving usability.

## 4. Minimal Failing Examples (Remediated)

*   **`Cell` Left Shift Overflow**: `Cell::from_int(1) << 10` previously silently discarded high trits. Now throws `std::overflow_error`.
*   **`T81Float` Zero Canonicalization**: `zero` and `neg_zero` were indistinguishable in some contexts. `to_canonical_string` updated to explicitly output `+0E0`/`-0E0` based on sign bit.
*   **`T81Vector` Construction**: Failed to compile with `double` literals. Added `std::is_arithmetic_v` check to `component_to_scalar`.

## 5. Patch Summary

*   `include/t81/types/cell.hpp`: Added overflow check to `operator<<` and fixed `operator*` (minor refactor).
*   `include/t81/types/T81Float.hpp`: Updated `zero()` factory to set exp=0 explicitely (clarity) and `to_canonical_string` to handle signed zero.
*   `include/t81/types/T81Vector.hpp`: Improved constructor template constraints to accept arithmetic types.
*   `tests/determinism/*`: Added comprehensive test suite.

## 6. Regression Test Summary

Run the following new test binaries:
*   `tests/determinism/test_primitives`
*   `tests/determinism/test_float`
*   `tests/determinism/test_containers`
*   `tests/determinism/test_math`
*   `tests/determinism/test_vector`

And the script runner:
*   `t81 code run tests/determinism/test_script.t81`

## 7. Remaining Risks

*   **Host-Math Dependence**: `T81Float` transcendentals are not bit-exact across platforms without `T81_DETERMINISTIC` + `dmath` backend (which is partial).
*   **Map Iteration**: Direct iteration over `T81Map` remains non-deterministic. This is by design for performance, but requires developer discipline to use `iter_sorted()` for logic affecting consensus.
