# Full T81Lang Datatype Surface Stress Audit

**Date:** March 1, 2026
**Agent:** Deterministic Language Surface Audit Agent

This document contains an exhaustive exploration of every datatype exposed in T81Lang, identifying semantic, determinism, serialization, VM, lowering, persistence, and performance shortcomings based on actual codebase behavior.

---

## 1. Primary Objectives Met

The audit focused on determining for every datatype:
1. Whether its behavior is fully implemented across the Frontend, IR, VM, and persistence layers.
2. Whether the type is natively represented, polyfilled, or stubbed.
3. Whether its behavior matches its specification and documentation.

---

## 2. Type-by-Type Audit

### Type: `T81BigInt`, `T81Int`, `i2`, `i8`, `i16`, `i32`, `T81Uint`
#### Surface Classification
*   Fully Native
*   Deterministic

#### Findings
*   Integers natively lower to arithmetic TISC instructions and execute deterministically. `T81Int` uses `kPow3AccumTrits` internally to manage bounds correctly, throwing `std::overflow_error` upon violation.

#### Determinism Risk Level
*   None

#### Required Fixes
*   None

#### Priority
*   Low

---

### Type: `T81Float`
#### Surface Classification
*   Host-Dependent

#### Findings
*   While signed-zero has been normalized deterministically (as per previous audit), transcendental functions (`pow`, `sin`, etc.) rely on host `<cmath>`. This is a strict violation of cross-platform bitwise deterministic execution.

#### Determinism Risk Level
*   High

#### Required Fixes
*   Deterministic math rewrite for `<cmath>` reliance. Create soft-float equivalents or trap on usage in strict contexts.

#### Priority
*   Critical (breaks determinism)

---

### Type: `T81Fraction`
#### Surface Classification
*   Fully Native
*   Persistable

#### Findings
*   Fully supported via `fraction_pool`. Natively supported.

#### Determinism Risk Level
*   None

#### Required Fixes
*   None

#### Priority
*   Low

---

### Type: `T81Fixed`
#### Surface Classification
*   VM Alias

#### Findings
*   `T81Fixed` is aliased directly to the `Integer` primitive kind during IR lowering. There is no native `T81Fixed` VM representation.

#### Determinism Risk Level
*   Low

#### Required Fixes
*   Replace polyfill with native type in VM opcode additions if distinct fixed-point behavior is required, else document it strictly as an integer alias.

#### Priority
*   Medium (incomplete surface)

---

### Type: `T81Complex`
#### Surface Classification
*   Partial Native (VM opcode exists)

#### Findings
*   Frontend IR emits `MAKE_COMPLEX`, and VM has `MakeComplex` handling which returns a `ComplexHandle`.
*   **Missing Binary Pool Support:** There is no `complex_pool` in `binary_io.cpp`. `T81Complex` constants cannot be persisted into a compiled T81 binary.

#### Determinism Risk Level
*   Structural

#### Required Fixes
*   Binary pool support: Add `complex_pool` to `Program` and implement `write_complex`/`read_complex` in `binary_io.cpp`.

#### Priority
*   High (breaks correctness / incomplete surface)

---

### Type: `T81Quaternion`, `T81Prob`, `T81Qutrit`, `Cell`
#### Surface Classification
*   Stub

#### Findings
*   Frontend parser/lexer recognizes `T81Quaternion`, `T81Prob`, and `Cell`.
*   IR lowers them to a `NOP` and allocates a simple Integer/Float register.
*   The VM has no opcodes to handle quaternion or probability math. `Cell` has `cell_determinism` tests fixed previously, but these types are not fully natively plumbed in VM execution.

#### Determinism Risk Level
*   Moderate (unimplemented semantics)

#### Required Fixes
*   VM opcode additions for natively supporting these primitive operations, or remove from surface until ready.

#### Priority
*   Medium (incomplete surface)

---

### Type: `T81String`, `T81Bytes`, `T81Vector`
#### Surface Classification
*   Fully Native

#### Findings
*   Lower natively to string and vector opcodes (`STRVECNEW`, `STRLEN`, etc.). Persisted via `symbol_pool` and dynamically managed in the VM heap correctly.

#### Determinism Risk Level
*   Low

#### Required Fixes
*   None

#### Priority
*   Low

---

### Type: `T81Matrix`, `T81Tensor`
#### Surface Classification
*   Performance Reality Disconnect

#### Findings
*   Tensors are heavily supported, have `tensor_pool` for persistence, and dedicated opcodes (`TMATMUL`, `TVECADD`).
*   **Performance:** `T81Tensor.hpp` implements `matmul` as a naive $O(N^3)$ triple-loop without BLAS/tiling. Matrix operations are highly unoptimized. Tensors are frequently copied deeply during IR lowering rather than referenced.

#### Determinism Risk Level
*   Low

#### Required Fixes
*   Performance hardening: Replace naive `matmul` loop with cache-friendly tiled implementations.

#### Priority
*   Low (performance/cleanup)

---

### Type: `T81List`
#### Surface Classification
*   Polyfill

#### Findings
*   IR lowers `List` to `STRVECNEW`.
*   There is no native `List` container in the VM execution loop; it executes as a string vector.

#### Determinism Risk Level
*   Structural

#### Required Fixes
*   Replace polyfill with native type.

#### Priority
*   Medium (incomplete surface)

---

### Type: `T81Map`, `T81Set`
#### Surface Classification
*   Polyfill / Falsely Claims Determinism

#### Findings
*   **Performance / Storage Disconnect:** The VM provides `MapNew` and `SetNew` opcodes, but internally they allocate and manipulate a `std::vector<std::string>`.
*   **Performance:** `MapPut`, `MapGet`, `SetAdd`, `SetHas` perform an $O(N)$ linear scan over the vector to find keys.
*   **Determinism Guarantee Failure:** The C++ `T81Map` uses `std::hash` for generic keys, breaking cross-platform determinism invariants.
*   **Binary Persistence:** No maps or sets can be natively persisted to the constant pool.

#### Determinism Risk Level
*   High

#### Required Fixes
*   Replace `Vector[String]` polyfill with native Hash Array Mapped Trie (HAMT) or deterministic tree structures in the VM.
*   Enforce Canonical sorting on serialization.
*   Add binary pool support for composite constants.

#### Priority
*   Critical (breaks determinism / performance)

---

### Type: `T81Tree`, `T81Graph`
#### Surface Classification
*   Stub / Polyfill

#### Findings
*   IR lowers `Tree` and `Graph` to `STRVECNEW`.
*   The VM has no opcodes for `Tree` or `Graph` navigation, edge addition, or traversal. Graph construction is simply pushed onto a string vector.

#### Determinism Risk Level
*   Structural

#### Required Fixes
*   VM opcode additions for `Graph` and `Tree` types.
*   Replace string vector polyfill with native representation in VM heap.

#### Priority
*   High (breaks correctness / incomplete surface)

---

### Type: `T81Symbol`
#### Surface Classification
*   Fully Native

#### Findings
*   Interned deterministic symbols. Persistable.

#### Determinism Risk Level
*   None

#### Required Fixes
*   None

#### Priority
*   Low

---

### Type: `T81Symbolic`, `T81Polynomial`
#### Surface Classification
*   Stub / Experimental

#### Findings
*   IR lowering produces `SYMLOAD`, but VM defines `SymLoad` and `SymRewrite` as part of the "Cognitive Tiers", which are experimental stubs throwing `TierFault` or `DecodeFault` if not enabled.
*   Cannot be stored in binary constant pools.

#### Determinism Risk Level
*   Low (since they trap correctly, but practically unusable)

#### Required Fixes
*   VM opcode implementation and Canonical Serialization Enforcement.

#### Priority
*   Medium (incomplete surface)

---

### Type: `Option`, `Result`
#### Surface Classification
*   VM Alias / Polyfill

#### Findings
*   Handled via enum variant payload creation natively (`MAKE_ENUM_VARIANT_PAYLOAD`).

#### Determinism Risk Level
*   Low

#### Required Fixes
*   None

#### Priority
*   Low

---

### Type: `T81Time`, `T81Entropy`, `T81Promise`, `T81Agent`
#### Surface Classification
*   Stub / Experimental

#### Findings
*   IR emits a literal string handle (e.g., `"std.async.promise"`).
*   No native VM representation. Concurrency (`T81Thread`, `T81Promise`) and Distributed logic (`T81Agent`) are stubs that are explicitly listed in `EXPERIMENTAL_SURFACE_INVENTORY.md` as non-deterministic.

#### Determinism Risk Level
*   High

#### Required Fixes
*   Fully stub out execution logic to throw determinism violations if run in a strict mode, or implement strictly deterministic scheduler behavior.

#### Priority
*   Medium (incomplete surface)

---

## 3. Gap Matrix

| Type | Native? | Canonical? | Persistable? | Deterministic? | Risk |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `T81BigInt` / `i32` | Yes | Yes | Yes | Yes | Low |
| `T81Float` | Yes | Yes | Yes | **No (cmath)** | High |
| `T81Fixed` | **No (Alias)** | Yes | Yes | Yes | Low |
| `T81Complex` | Yes | Yes | **No** | Yes | Med |
| `T81Quaternion` | **No (Stub)** | No | No | N/A | Med |
| `T81Vector` | Yes | Yes | Yes | Yes | Low |
| `T81Tensor` / `Matrix` | Yes | Yes | Yes | Yes | Low |
| `T81List` | **No (Polyfill)** | Yes | **No** | Yes | Med |
| `T81Map` | **No (Polyfill)** | **No (std::hash)**| **No** | **No** | Critical |
| `T81Set` | **No (Polyfill)** | Yes | **No** | Yes | High |
| `T81Tree` / `Graph` | **No (Stub)** | No | **No** | Yes | High |
| `T81Symbol` | Yes | Yes | Yes | Yes | Low |
| `T81Symbolic` / `Poly` | **No (Stub)** | No | **No** | N/A | Med |
| `T81Promise` / `Agent` | **No (Stub)** | No | **No** | **No** | High |

---

## 4. Architectural Risk Report

1.  **Types that falsely claim determinism:** `T81Map` (uses `std::hash` for generic lookups), `T81Float` (uses host `<cmath>`).
2.  **Types exposed but VM-incomplete:** `T81Tree`, `T81Graph`, `T81Quaternion`, `T81Prob`, `Cell` (all stubbed as NOP or alias in IR/VM).
3.  **Performance Masking:** `T81Map` and `T81Set` are polyfilled atop `std::vector<std::string>`, masking $O(N)$ lookup times behind standard container nomenclature.
4.  **Types blocking Beta promotion:** `T81Complex` lacks persistence logic; `Map`/`Set` lack deterministic hashing and actual native VM implementation; `Tree`/`Graph` are wholly missing VM support.
5.  **Types requiring spec correction:** `T81List` is currently aliased to `STRVECNEW`; either rename to Vector or implement linked-list memory structures natively.

---

## 5. Ordered Remediation Plan

### Phase 1 — Determinism-Critical Fixes
*   Eliminate `std::hash` usage inside `T81Map` and `T81Set`; introduce deterministic Hash Array Mapped Trie (HAMT) hashing algorithms.
*   Rewrite `T81Float` transcendental functions (pow, sin, cos) to use deterministic soft-float equivalents instead of host `<cmath>`.

### Phase 2 — Canonical Serialization Enforcement
*   Enforce canonical sorting on serialization for Map/Set natively in the VM.
*   Add persistence capability: Create `complex_pool` to persist `T81Complex`.
*   Add persistence capability: Create composite pools to persist `T81Map`, `T81Set`, `T81List`, `T81Tree`, and `T81Graph` constants in `binary_io.cpp`.

### Phase 3 — VM Native Type Promotion
*   Implement native Opcode support for `T81Tree` and `T81Graph` operations (creation, edge addition, traversal).
*   Implement native Opcode support for `T81Quaternion`, `T81Prob`, and `Cell`.

### Phase 4 — Polyfill Elimination
*   Remove `Vector[String]` polyfill inside `vm.cpp` for `MapNew`, `MapGet`, `SetNew`, `SetAdd`. Replace with true Hash-Map memory allocation models.
*   Ensure `T81List` evaluates to an actual linked list memory structure rather than aliasing to string vectors.

### Phase 5 — Performance Hardening
*   Update `T81Tensor.hpp` to utilize cache-friendly blocked / tiled `matmul` algorithms instead of naive $O(N^3)$ loops.
*   Evaluate memory footprint and unnecessary deep-copy operations of Tensors during IR lowering.
