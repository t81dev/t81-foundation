# T81 Core Type System Remediation Plan

**Status:** IMPLEMENTED (Partial Polyfill)
**Target:** T81 Core Architecture
**Scope:** Determinism, Memory Safety, and Canonical Serialization

---

## 1. Determinism Classification Normalization

| Type | Classification | Notes |
| :--- | :--- | :--- |
| `T81Int<N>` | **Strict** | Bitwise-exact behavior across all platforms. Canonical serialization implemented. |
| `T81Float<M,E>` | **Host-Dependent** | Relies on `double` for division and transcendentals (`sin`, `cos`, `log`). **Canonical Serialization implemented via internal trits.** |
| `T81BigInt` | **Strict** | Implements Knuth's Algorithm D and Karatsuba in software; no host float dependency. |
| `T81Fraction` | **Strict** | Canonicalizes via GCD; operations are integer-based. |
| `T81Complex` | **Host-Dependent** | Inherits `T81Float` dependencies. |
| `T81Quaternion` | **Host-Dependent** | Inherits `T81Float` dependencies. |
| `T81Vector<N>` | **Host-Dependent** | `length()` and `angle()` use `std::sqrt`, `std::acos` via `double`. |
| `T81Matrix` | **Host-Dependent** | Inherits `T81Float` dependencies if used with float scalar. |
| `T81Tensor` | **Element-Dependent** | Strict for integer scalar; Host-Dependent if float or host math used. Hybrid storage implemented. Canonical serialization added. |
| `T81Symbol` | **Order-Dependent** | ID assignment depends on interning order. `hash()` and `operator<=>` are ID-based. **Canonical Serialization uses Name.** |
| `T81Map` | **Order-Dependent** | **Fixed**: `serialize_canonical()` uses sorted keys (by name for symbols). |
| `T81Set` | **Order-Dependent** | Inherits `T81Map` behavior. |
| `T81Graph` | **Strict** | **Fixed**: NodeID widens to `uint32_t`. Heap storage used for large graphs. |
| `T81Time` | **Schedule-Dependent** | **Fixed**: Thread-safe override. |
| `T81Entropy` | **Schedule-Dependent** | Schedule-Dependent (multi-thread) / Strict given single-thread acquisition order. **Fixed**: Thread-safe seed/pool. |
| `T81IOStream` | **Order-Dependent** | **Fixed**: Thread-safe internal state. |
| `T81List` | **Strict** | Deterministic if element type is deterministic. |
| `T81String` | **Strict** | Text processing is deterministic. |

**Status**: Implemented: thread-safety hardening, stack-safety via hybrid storage, deterministic canonical export surfaces.
Deferred: host-float math determinism and content-addressable symbol identity.

---

## 2. Global State Mutation Audit

| Type | Global State Access | Determinism Impact | Thread-Safe? | Status |
| :--- | :--- | :--- | :--- | :--- |
| `T81Entropy` | `EntropyPool::seed_` | High (Order-Dependent) | **YES** (Mutex) | **FIXED** |
| `T81Time` | `deterministic_override_` | High (Schedule/Order) | **YES** (Mutex) | **FIXED** |
| `T81Symbol` | `InternTable` singleton | High (ID generation) | **YES** (Mutex) | Existing |
| `T81IOStream` | `cin`, `cout`, `cerr` | High (Interleaving) | **YES** (Mutex) | **FIXED** |

**Action Taken:** Added `std::mutex` protection to `EntropyPool`, `T81Time`, and `T81IOStream`. Removed `const` qualifier from `cin`/`cout`/`cerr` to support thread-safe operations.

---

## 3. SPEC–IMPLEMENTATION GAP Remediation

| Gap | Root Cause | Correction | Classification |
| :--- | :--- | :--- | :--- |
| **T81Float Double Dependency** | `operator/`, `sin`, `cos` cast to `double`. | Implement software ternary division & CORDIC/series for math. | **Deferred** (Long-term) |
| **T81Symbol Nondeterminism** | Intern IDs assigned via global counter. | Use content-addressable IDs (hash of string) or scoped tables. | **Deferred** |
| **T81Graph Stack Overflow** | `adj` array is `std::array` of size ~13MB (for 6561 nodes). | Change storage to `std::vector` (Heap) or external buffer. | **FIXED** |
| **T81Tensor Stack Overflow** | `data` array is C-style array in object. Large tensors explode stack. | Change storage to `std::vector` or `std::unique_ptr`. | **FIXED** |
| **T81Map Iteration Order** | Linear probing + `std::hash` / `symbol_hash`. | Enforce sorted iteration or use deterministic hash (SipHash) + fixed probing. | **FIXED** (via export) |
| **EntropyPool Race** | Unsynchronized static mutation. | Add `std::mutex` or `std::atomic` CAS loop for seed update. | **FIXED** |
| **IOStream Race** | Unsynchronized `timestamps_` list. | Add `std::mutex` to `T81IOStream`. | **FIXED** |

**Notes:**
- `T81Graph` and `T81Tensor` now use a hybrid storage model (Heap for large, Stack for small).
- `T81Map` now provides `iter_sorted()` and `serialize_canonical()` which guarantees stability even if iteration order varies.

---

## 4. Canonical Serialization Enforcement Plan

| Type | Canonical Required? | Minimal Form | Status |
| :--- | :--- | :--- | :--- |
| `T81Int` | Yes | Base-81 String (e.g. `12A`) | **Implemented** (trit string) |
| `T81BigInt` | Yes | Base-81 String | Existing |
| `T81Float` | Yes | Scientific Base-81 (e.g. `1.2A^B`) | **Implemented** (Trit representation). Canonical serialization does not call double; arithmetic determinism depends on the active math backend rather than the serialization format itself. |
| `T81Symbol` | Yes | String Name | **Implemented** |
| `T81Map` | Yes | Sorted Key-Value List | **Implemented** |
| `T81Graph` | Yes | Adjacency List (Sorted) | **Implemented** |
| `T81List` | Yes | List `[e1, e2, ...]` | Existing |
| `T81Tensor` | Yes | Shape + Data | **Implemented** |

**Action Items Completed:**
1. Implemented `T81Float::to_canonical_string()` without `double` conversion.
2. Implemented `T81Map::serialize_canonical()` which sorts keys before writing.
3. Ensured `T81Symbol` serializes as its string name.

---

## 5. Memory Domain Consistency Plan

| Type | Claimed Domain | Actual Domain | Inconsistency | Action |
| :--- | :--- | :--- | :--- | :--- |
| `T81Int` | Stack | Stack | None | None |
| `T81Float` | Stack | Stack | None | None |
| `T81BigInt` | Heap | Heap | None | None |
| `T81List` | Dynamic | Heap (`std::vector`) | None | None |
| `T81Map` | Dynamic | Heap (`std::vector`) | None | None |
| **`T81Graph`** | **Hardware/Static** | **Hybrid (Stack/Heap)** | **Resolved** | **Implemented** |
| **`T81Tensor`** | **Hardware/Static** | **Hybrid (Stack/Heap)** | **Resolved** | **Implemented** |
| `T81IOStream` | Static Global | Global | None | None |

**Resolution:**
- `T81Graph` and `T81Tensor` automatically switch to heap storage (`std::vector`) when size exceeds 4KB, preventing stack overflow while preserving optimization for small instances.

---

## 6. Determinism Boundary Definition for VM Core

| Type | VM-Safe | Conditions | Required Guards |
| :--- | :--- | :--- | :--- |
| `T81Int` | **YES** | None | None |
| `T81BigInt` | **YES** | None | Memory Quota (Heap) |
| `T81Float` | **CONDITIONAL** | No div/transcendentals | Strict Ops Only / Soft-Float Polyfill |
| `T81List` | **YES** | None | Memory Quota |
| `T81Map` | **Conditional** | **Fixed** | Use `serialize_canonical` for hashing/export |
| `T81Graph` | **YES** | None | Memory Quota |
| `T81Tensor` | **Conditional** | Float ops allowed | Memory Quota |
| `T81Symbol` | **Conditional** | Policy: do not rely on numeric ID | Use `serialize_canonical` (Name) |
| `T81Time` | **NO** | Unless Injected | Inject Time via VM Context |
| `T81Entropy` | **Conditional** | Schedule-Dependent | Inject Entropy via VM Context |
| `T81IOStream` | **NO** | Side-effecting | Ban. Use VM Output Buffer. |

---

## 7. Implementation Roadmap (Completed)

| Priority | Component | Change Type | Status | Description |
| :--- | :--- | :--- | :--- | :--- |
| **P0** | `T81Entropy` | Code | **DONE** | **Fix Data Race:** Added `std::mutex`. |
| **P0** | `T81IOStream` | Code | **DONE** | **Fix Data Race:** Added `std::mutex`. |
| **P0** | `T81Time` | Code | **DONE** | **Fix Data Race:** Added `std::mutex`. |
| **P1** | `T81Graph` | Code | **DONE** | **Fix Stack Overflow:** Hybrid storage (Vector/Array). |
| **P1** | `T81Tensor` | Code | **DONE** | **Fix Stack Overflow:** Hybrid storage. |
| **P1** | `T81Symbol` | Code | **DONE** | **Fix Determinism:** Added `serialize_canonical` (Name). |
| **P2** | `T81Map` | Code | **DONE** | **Fix Iteration:** Implemented `serialize_canonical()` with sorted keys. |
| **P2** | `T81Float` | Code | **DONE** | **Fix Determinism:** Implemented `to_canonical_string` without `double`. |

---

## 8. Stub Implementation Remediation

| Component | Stub Feature | Location | Status | Action |
| :--- | :--- | :--- | :--- | :--- |
| `AxionPolicy` | `CheckTier` Opcode | `kernel/axion/policy_engine.cpp` | **DONE** | Implemented tier check logic against `SyscallContext`. |
| `VM` | `AxCheck`, `AxReport` | `vm/vm.cpp` | **DONE** | Implemented functional logging behavior. |
| `VM` | `AxSign`, `AxCanon`, etc. | `vm/vm.cpp` | **Pending** | Currently stubs logging "Cognitive Opcode Stub Execution". |
| `VM` | Networking (`NSend`, `NRecv`) | `vm/vm.cpp` | **Pending** | Placeholders returning dummy values. |
| `VM` | Async (`VWait`, `VYield`) | `vm/vm.cpp` | **Pending** | Placeholders. |
| `T81Float` | Ternary-Decimal Conv | `include/t81/std/string.hpp` | **Deferred** | Uses `double` conversion fallback (marked TODO). |
