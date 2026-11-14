# Language-Surface Inventory Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Language-Surface Inventory Audit](#language-surface-inventory-audit)
  - [Gaps Identified](#gaps-identified)
  - [Determinism Classification](#determinism-classification)
    - [Deterministic & Verified](#deterministic-&-verified)
    - [Deterministic (New Tests Added)](#deterministic-new-tests-added)
    - [Deterministic but Incompletely Tested](#deterministic-but-incompletely-tested)
    - [Surface-Exposed but VM-Incomplete (Polyfills/Aliases)](#surface-exposed-but-vm-incomplete-polyfillsaliases)
    - [Experimental / Reflective Deterministic Defaults](#experimental--reflective-deterministic-defaults)

<!-- T81-TOC:END -->


| Type | Backend Exists | Exposed in T81Lang | VM Opcode Coverage | Canonical Serialization | Determinism Tests | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Primitives** | | | | | | |
| `i2` (Trit) | YES | YES | YES | YES | PARTIAL | Stable |
| `i8` (Tryte) | YES | YES | YES | YES | PARTIAL | Stable |
| `i16` | YES | YES | YES | YES | PARTIAL | Stable |
| `i32` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81BigInt` | YES | YES | YES (aliased to i64) | YES (as i64) | YES | Stable |
| `T81Float` | YES | YES | YES | YES | YES | Stable |
| `T81Fraction` | YES | YES | YES | YES | YES | Stable |
| `T81Fixed` | YES | YES | YES (as Int) | YES (as Int) | YES | Beta |
| `T81Complex` | YES | YES | YES | YES | YES | Beta |
| `T81Quaternion` | YES | YES | YES (NOP) | YES + serialize_canonical | YES | Beta |
| `T81Prob` | YES | YES | YES (NOP) | YES | YES | Beta |
| `T81Qutrit` | YES | YES | YES | YES (via T81Int) | YES | Beta |
| `T81Uint` | YES | YES | YES | YES + serialize_canonical | YES | Stable |
| `Cell` | YES | YES | YES (NOP) | YES | YES | Stable |
| **Collections** | | | | | | |
| `T81String` | YES | YES | YES | YES | YES | Stable |
| `T81Bytes` | YES | YES | YES | YES | YES | Stable |
| `T81Vector` | YES | YES | YES | YES | YES | Stable |
| `T81Matrix` | YES | YES | YES | PARTIAL | UNKNOWN | Beta |
| `T81Tensor` | YES | YES | YES | PARTIAL | UNKNOWN | Beta |
| `T81List` | YES | YES | YES | YES | PARTIAL | Beta |
| `T81Map` | YES | YES | YES | YES | YES | Beta |
| `T81Set` | YES | YES | YES | YES | YES | Beta |
| `T81Tree` | YES | YES | YES | YES | PARTIAL | Beta |
| `T81Graph` | YES | YES (handle API) | YES (via builtin call) | YES (Native + Lang canonical) | PARTIAL | Draft |
| **Symbolic** | | | | | | |
| `T81Symbol` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81Symbolic` | YES | YES | YES | YES | NO | Experimental |
| `T81Polynomial`| YES | YES | YES | YES | NO | Experimental |
| **System** | | | | | | |
| `Option` | YES | YES | YES | N/A | YES | Stable |
| `Result` | YES | YES | YES | N/A | YES | Stable |
| `T81Time` | YES | YES | YES | YES | NO | Experimental |
| `T81Entropy` | YES | YES | YES | YES | NO | Experimental |
| `T81Promise` | YES | YES | YES | YES | NO | Experimental |
| `T81Agent` | YES | YES | YES | YES | NO | Experimental |

## Gaps Identified

1.  **Collections Implementation Gap:** `T81List`, `T81Map`, `T81Set`, `T81Tree`, and `T81Graph` are exposed via `std.collections.*` intrinsics but are implemented as `Vector[String]` polyfills in the compiler lowering (`ir_generator.hpp`). They do not utilize the native C++ types (`T81Map`, `T81Set`, etc.) in the VM, resulting in inefficient $O(N)$ operations and lack of true type-specific serialization.
2.  **Canonical Serialization Gap:** The collection types (`Map`, `Set`, `Graph`) lack canonical serialization in their Language/VM polyfill representation because their underlying Vector representation relies on insertion order, which is not sorted by key/content. The native C++ `T81Map` and `T81Graph` classes support `serialize_canonical`, but it is unused by the language runtime.
3.  **BigInt Precision Gap:** `T81BigInt` is backed by a robust `BigInt` C++ class, but the VM aliasing maps `bigint` operations to standard 64-bit integer opcodes (`ADD`, `SUB`, etc.) which do not support arbitrary precision handles. Literals > 64-bit are truncated or unsupported in the current IR generation.
4.  **Complex Number Persistence Gap:** `T81Complex` is supported in the VM (via `MAKE_COMPLEX`), but lacks binary pool serialization support in `binary_io.cpp`, meaning complex values cannot be persisted in the program binary constants.
5.  **Host-Math Dependence:** `T81Float` (and by extension `T81Complex`, `T81Vector`) relies on `std::cmath` for transcendental functions unless `T81_DETERMINISTIC` is defined.

## Determinism Classification

### Deterministic & Verified
*   **Primitives:** `i2`, `i8`, `i16`, `i32`, `T81Float`.
*   **Collections:** `T81String`, `T81Bytes`.
*   **System:** `Option`, `Result` (verified via `match` and binary pool serialization).
*   **Verification:** Confirmed by SHA256 logs in `tests/fixtures/t81lang_determinism` and compiler reproducibility tests.

### Deterministic (New Tests Added)
*   **Primitives:** `T81Uint`, `T81BigInt`, `Cell`, `T81Fraction`.
*   **Math:** `T81Quaternion`, `T81Prob`, `T81Qutrit`, `T81Fixed`, `T81Complex`.
*   **Collections:** `T81Vector`, `T81Map` (Native), `T81Set` (Native).
*   **Verification:** Verified via `tests/determinism/` suite (Adversarial + Canonical Serialization).

### Deterministic but Incompletely Tested
*   **Symbolic:** `T81Symbol` (partial).
*   **Note:** No evidence of nondeterminism found, but coverage is partial.

### Surface-Exposed but VM-Incomplete (Polyfills/Aliases)
*   **Collections:** `T81List`, `T81Map`, `T81Set`, `T81Tree`, `T81Graph` (Vector polyfills).
*   **Primitives:** `T81BigInt` (Aliased to 64-bit int).
*   **Status:** Exposed via handle API but implemented as overlays/aliases. Canonical serialization is missing or limited.

### Experimental / Reflective Deterministic Defaults
*   **System:** `T81Time` (returns stable zero), `T81Entropy` (returns stable zero), `T81Promise` (stubbed).
*   **Math:** `T81Complex` (runtime only, no persistence).
*   **Symbolic:** `T81Symbolic`, `T81Polynomial` (heap objects, no canonical persistence).
