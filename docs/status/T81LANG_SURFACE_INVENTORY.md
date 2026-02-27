# Language-Surface Inventory Audit

| Type | Backend Exists | Exposed in T81Lang | VM Opcode Coverage | Canonical Serialization | Determinism Tests | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Primitives** | | | | | | |
| `i2` (Trit) | YES | YES | YES | YES | PARTIAL | Stable |
| `i8` (Tryte) | YES | YES | YES | YES | PARTIAL | Stable |
| `i16` | YES | YES | YES | YES | PARTIAL | Stable |
| `i32` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81BigInt` | YES | YES | YES (aliased to i64) | YES (as i64) | PARTIAL | Stable |
| `T81Float` | YES | YES | YES | YES | YES | Stable |
| `T81Fraction` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81Fixed` | YES | YES | YES (as Int) | YES (as Int) | NO | Beta |
| `T81Complex` | YES | YES | YES | NO | NO | Beta |
| `T81Qutrit` | YES | YES | YES | YES | UNKNOWN | Beta |
| `T81Uint` | YES | YES | YES | YES | UNKNOWN | Stable |
| **Collections** | | | | | | |
| `T81String` | YES | YES | YES | YES | YES | Stable |
| `T81Bytes` | YES | YES | YES | YES | YES | Stable |
| `T81Vector` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81Matrix` | YES | YES | YES | PARTIAL | UNKNOWN | Beta |
| `T81Tensor` | YES | YES | YES | PARTIAL | UNKNOWN | Beta |
| `T81List` | YES | YES (handle API) | YES (via builtin call) | NO | NO | Draft |
| `T81Map` | YES | YES (handle API) | YES (via builtin call) | NO | NO | Draft |
| `T81Set` | YES | YES (handle API) | YES (via builtin call) | NO | NO | Draft |
| `T81Tree` | YES | YES (handle API) | YES (via builtin call) | NO | NO | Draft |
| `T81Graph` | YES | YES (handle API) | YES (via builtin call) | NO | NO | Draft |
| **Symbolic** | | | | | | |
| `T81Symbol` | YES | YES | YES | YES | PARTIAL | Stable |
| `T81Symbolic` | YES | YES | YES | NO | NO | Experimental |
| `T81Polynomial`| YES | YES | YES | NO | NO | Experimental |
| **System** | | | | | | |
| `Option` | YES | YES | YES | N/A | YES | Stable |
| `Result` | YES | YES | YES | N/A | YES | Stable |
| `T81Time` | YES | YES | YES | NO | NO | Experimental |
| `T81Entropy` | YES | YES | YES | NO | NO | Experimental |
| `T81Promise` | YES | YES | YES | NO | NO | Experimental |
| `T81Agent` | YES | YES | YES | NO | NO | Experimental |

## Gaps Identified

1.  **Collections Implementation Gap:** `T81List`, `T81Map`, `T81Set`, `T81Tree`, and `T81Graph` are exposed via `std.collections.*` intrinsics but are implemented as `Vector[String]` polyfills in the compiler lowering (`ir_generator.hpp`). They do not utilize the native C++ types (`T81Map`, `T81Set`, etc.) in the VM, resulting in inefficient $O(N)$ operations and lack of true type-specific serialization.
2.  **Canonical Serialization Gap:** The collection types (`Map`, `Set`, `Graph`) lack canonical serialization because their underlying Vector representation relies on insertion order, which is not sorted by key/content. The native C++ `T81Map` class supports `serialize_canonical`, but it is unused by the language runtime.
3.  **BigInt Precision Gap:** `T81BigInt` is backed by a robust `BigInt` C++ class, but the VM aliasing maps `bigint` operations to standard 64-bit integer opcodes (`ADD`, `SUB`, etc.) which do not support arbitrary precision handles. Literals > 64-bit are truncated or unsupported in the current IR generation.
4.  **Complex Number Persistence Gap:** `T81Complex` is supported in the VM (via `MAKE_COMPLEX`), but lacks binary pool serialization support in `binary_io.cpp`, meaning complex values cannot be persisted in the program binary constants.
5.  **Determinism Testing Gap:** While compiler determinism is tested (`test_ast_ir_compile_repeat_hash_gate`), there are no explicit runtime determinism fixtures for the Collections, `T81Fixed` (beyond int64), or `T81Complex` types.

## Determinism Classification

### Deterministic & Verified
*   **Primitives:** `i2`, `i8`, `i16`, `i32`, `T81Float`.
*   **Collections:** `T81String`, `T81Bytes`.
*   **System:** `Option`, `Result` (verified via `match` and binary pool serialization).
*   **Verification:** Confirmed by SHA256 logs in `tests/fixtures/t81lang_determinism` and compiler reproducibility tests.

### Deterministic but Incompletely Tested
*   **Primitives:** `T81Fraction`, `T81Uint`, `T81Fixed` (aliased to Int), `T81BigInt` (aliased to i64).
*   **Collections:** `T81Vector` (partial tests).
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
