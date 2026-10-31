# Determinism Fix Report: T81 Core Data Types Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Determinism Fix Report: T81 Core Data Types Audit](#determinism-fix-report-t81-core-data-types-audit)
  - [1. Executive Summary](#1-executive-summary)
  - [2. Determinism Surface Table](#2-determinism-surface-table)
  - [3. Per-Type Notes](#3-per-type-notes)
    - [3.1. Low-Level Primitives (`Cell`, `T81Int`)](#31-low-level-primitives-`cell`-`t81int`)
    - [3.2. Floating Point (`T81Float`)](#32-floating-point-`t81float`)
    - [3.3. Containers (`Map`, `Set`, `Graph`)](#33-containers-`map`-`set`-`graph`)
  - [4. Minimal Failing Examples (Pre-Fix)](#4-minimal-failing-examples-pre-fix)
    - [4.1. Cell Overflow](#41-cell-overflow)
    - [4.2. Float Signed Zero](#42-float-signed-zero)
  - [5. Patch Summary](#5-patch-summary)
  - [6. Regression Test Summary](#6-regression-test-summary)
  - [7. Remaining Risks / Experimental Surfaces](#7-remaining-risks--experimental-surfaces)

<!-- T81-TOC:END -->

**Date:** March 1, 2026
**Agent:** Determinism Stress + Remediation Agent

## 1. Executive Summary

This audit verified the deterministic behavior of the T81 core data types. The investigation focused on canonicalization, serialization stability, and cross-platform consistency.

**Key Findings:**
*   **Passed:** `T81BigInt` (including GMP compatibility), `T81Fraction`, `T81Fixed` (aliased to integer), and `T81Vector` (static/dynamic).
*   **Fixed:**
    *   `Cell` type now correctly handles overflow, preventing undefined behavior (UB).
    *   `T81Float` now enforces a positive zero canonicalization to avoid signed zero ambiguities.
    *   `Map` and `Set` containers are now strictly typed in the frontend, preventing accidental usage as generic vectors and ensuring correct lowering to deterministic opcodes.
*   **Experimental:** The `Hanoi` kernel and `Cognitive Tier` components remain experimental stubs with no determinism guarantees.

## 2. Determinism Surface Table

| Type | Status | Severity | Invariant Broken | Root Cause | Fix Applied | Tests Added |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Cell` | **Fixed** | Critical | Overflow handling | Undefined Behavior on signed overflow | Added explicit overflow checks and clamping | `tests/cpp/cell_determinism_test.cpp` |
| `T81Float` | **Fixed** | High | Canonicalization | Signed zero ambiguity (`-0.0` vs `0.0`) | Enforced positive zero on construction/op | `tests/cpp/t81float_determinism_test.cpp` |
| `T81BigInt` | Stable | - | - | - | - | Verified via existing tests |
| `T81Map` | **Fixed** | Medium | Type Safety | Frontend allowed `Vector` ops on `Map` | Enforced strict `Map` type in Semantic Analyzer | `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| `T81Set` | **Fixed** | Medium | Type Safety | Frontend allowed `Vector` ops on `Set` | Enforced strict `Set` type in Semantic Analyzer | `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| `T81Graph` | Stable | - | - | - | - | Verified strict typing |

## 3. Per-Type Notes

### 3.1. Low-Level Primitives (`Cell`, `T81Int`)
*   **Cell:** The original implementation relied on C++ signed overflow behavior, which is UB. We introduced `Cell::safe_add` and similar helpers to clamp or wrap deterministically.
*   **T81Int:** Wraps `T81BigInt` for arbitrary precision; determinism inherits from the GMP/BigInt backend.

### 3.2. Floating Point (`T81Float`)
*   **Signed Zero:** IEEE 754 allows `-0.0`. T81 canonicalization now forces `0.0` to ensure bit-exact serialization matches.
*   **Transcendental Functions:** Rely on host `cmath`. This is a known experimental surface (see `EXPERIMENTAL_SURFACE_INVENTORY.md`). For auditing purposes, we verified basic arithmetic determinism.

### 3.3. Containers (`Map`, `Set`, `Graph`)
*   **Type Enforcement:** The frontend previously polyfilled these as `Vector[String]`, allowing `std.collections.len()` to work. The audit revealed this broke type invariants. We updated the Semantic Analyzer to distinguish `Map`, `Set`, `Tree`, and `Graph` as unique types, requiring specific builtins (e.g., `map_size`, `set_add`).
*   **Ordering:** Iteration order for `Map` and `Set` must be canonicalized before serialization. The current VM implementation uses sorted keys for deterministic serialization.

## 4. Minimal Failing Examples (Pre-Fix)

### 4.1. Cell Overflow
```cpp
Cell c(9223372036854775807); // Max int64
c = c + 1; // UB in C++, result undefined
```
**Fix:** Explicit check ensures wrapping or saturation defined by the spec.

### 4.2. Float Signed Zero
```cpp
T81Float a = 0.0;
T81Float b = -1.0 * 0.0; // -0.0
assert(serialize(a) == serialize(b)); // Fails if binary repr differs
```
**Fix:** `T81Float` constructor and operators now normalize `-0.0` to `0.0`.

## 5. Patch Summary

*   `include/t81/types/Cell.hpp`: Added overflow protection.
*   `include/t81/types/T81Float.hpp`: Added zero normalization.
*   `include/t81/frontend/ir_generator.hpp`: Added `collections_list_size` and `collections_tree_size` lowering; patched float parsing for macOS.
*   `lang/frontend/semantic_analyzer.cpp`: Added symbol table entries for container-specific size functions.
*   `tests/cpp/`: Updated conformance and IR generator tests to reflect stricter type rules.

## 6. Regression Test Summary

*   **Run:** `cmake --build build --target t81lang_conformance_baseline_test && build/t81lang_conformance_baseline_test`
*   **Coverage:**
    *   Base81 literal parsing.
    *   Container type strictness (Map/Set/Graph/Tree).
    *   Builtin alias resolution.
    *   IR lowering for all standard library modules.

## 7. Remaining Risks / Experimental Surfaces

1.  **Transcendental Math:** `T81Float` still uses host `sin`/`cos`/etc. Cross-platform bit-exactness is NOT guaranteed for these operations.
2.  **Concurrency:** `T81Thread` and `T81Promise` are stubs. Thread scheduling determinism is not implemented.
3.  **Distributed:** `T81Network` and distributed tensor ops are experimental stubs.

This concludes the Phase 1 Determinism Audit.
