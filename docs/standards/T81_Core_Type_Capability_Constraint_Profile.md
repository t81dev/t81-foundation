# T81 Core Type Capability & Constraint Profile

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Core Type Capability & Constraint Profile](#t81-core-type-capability-&-constraint-profile)
  - [1. Type Taxonomy Overview](#1-type-taxonomy-overview)
  - [2. Capability Matrix](#2-capability-matrix)
  - [3. Per-Type Structured Profiles](#3-per-type-structured-profiles)
  - [`T81Int`](#`t81int`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Float`](#`t81float`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81BigInt`](#`t81bigint`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Tensor`](#`t81tensor`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Graph`](#`t81graph`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81List`](#`t81list`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Map`](#`t81map`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Vector`](#`t81vector`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Result`](#`t81result`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Symbol`](#`t81symbol`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Entropy`](#`t81entropy`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [`T81Time`](#`t81time`)
    - [Intended Use Cases](#intended-use-cases)
    - [Avoid When](#avoid-when)
    - [Determinism Notes](#determinism-notes)
    - [Architectural Risks](#architectural-risks)
  - [4. Cross-Type Constraint Analysis](#4-cross-type-constraint-analysis)
    - [1. The Stack vs. Heap Schism](#1-the-stack-vs-heap-schism)
    - [2. Identity & Determinism Gap](#2-identity-&-determinism-gap)
    - [3. Floating Point Purity](#3-floating-point-purity)
    - [4. Graph Scalability Illusion](#4-graph-scalability-illusion)
    - [5. Error Handling Side Effects](#5-error-handling-side-effects)
  - [5. Determinism Surface Summary](#5-determinism-surface-summary)

<!-- T81-TOC:END -->


## 1. Type Taxonomy Overview

The core T81 type system is divided into the following architectural layers:

- **Storage / Primitive**: `T81Int`, `T81Float`, `T81Fixed`, `T81Complex`. These provide the fundamental ternary numeric substrate.
- **Arithmetic**: `T81BigInt`, `T81Fraction`, `T81Polynomial`. Higher-level mathematical abstractions built on primitives.
- **Containers**: `T81Tensor`, `T81Graph`, `T81List`, `T81Map`, `T81Vector`. Structures for organizing data, optimized for either dense (Tensor), sparse (Graph), or dynamic (List/Map) access.
- **Symbolic / Identity**: `T81Symbol`, `T81String`. Types governing identity, interning, and text representation.
- **System / IO**: `T81Entropy`, `T81Time`. Interfaces to non-deterministic or side-effecting external systems.
- **Control Flow**: `T81Result`, `Option`. Monadic types for error handling and optionality.

---

## 2. Capability Matrix

| Type | Layer | Determinism Class | Canonical Serialization | Entropy Accounted | Reflection Surface | Governance Surface | Risk Class |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `T81Int` | Storage | Strict | `to_string` (base3) | [IMPLICIT] | None | None | Low |
| `T81Float` | Storage | Host-Dependent | [UNSPECIFIED] | [IMPLICIT] | None | None | Medium |
| `T81BigInt` | Arithmetic | Strict | `to_base81_string` | [IMPLICIT] | None | None | Medium |
| `T81Tensor` | Container | Element-Dependent (Strict for integer scalar; Host-Dependent if float or host math used) | [UNSPECIFIED] | [IMPLICIT] | None | None | Medium |
| `T81Graph` | Container | Strict (structural). Heap-backed for large graphs. Deterministic export via sorted adjacency. | [UNSPECIFIED] | [IMPLICIT] | None | None | High |
| `T81List` | Container | Strict | `operator<<` | [IMPLICIT] | None | None | Medium |
| `T81Map` | Container | Order-Dependent | serialize_canonical() (sorted keys) | [IMPLICIT] | None | None | Medium |
| `T81Vector` | Container | Host-Dependent | None | [IMPLICIT] | None | None | Medium |
| `T81Symbol` | Symbolic | Policy-Dependent | `to_string` (interned) | [IMPLICIT] | None | Intern Table | High |
| `T81Result` | Control Flow | Side-Effecting | `explain` | Explicit (Error) | `reflect()` | None | High |
| `T81Entropy` | System | Schedule-Dependent (multi-thread) / Strict given single-thread acquisition order | None | Explicit | None | Pool Access | High |
| `T81Time` | System | Host-Dependent | `narrate` | [IMPLICIT] | `reflect()` | Override | Medium |

---

## 3. Per-Type Structured Profiles

## `T81Int`

**Layer:** Storage
**Determinism Class:** Strict
**Primary Invariants:**
- `N` fixed at compile time (1..2048).
- Packed 4 trits per byte.
- `to_int64` throws on overflow (GCC/Clang) or uses simplified path (MSVC).
**Memory Model:** Stack-allocated, fixed size `std::array`.
**Failure Modes:** Overflow in conversion to `int64`.
**Canonical Representation:** Balanced ternary string.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Core arithmetic where fixed-width precision is sufficient.
- Bit-exact (trit-exact) logic.

### Avoid When
- Arbitrary precision is required (use `T81BigInt`).
- Floating point semantics are needed.

### Determinism Notes
Purely deterministic logic for all operators `+`, `-`, `*`, `/`, `%`. Division is restoring and deterministic.

### Architectural Risks
- MSVC path for `to_int64` is marked as "simplified" and may not be fully robust against overflow, potentially returning incorrect values for large inputs in `constexpr` contexts.

---

## `T81Float`

**Layer:** Storage
**Determinism Class:** Conditional
**Primary Invariants:**
- $M \ge 4$, $E \ge 4$, $M+E+1 \le 2048$.
- Backed by `T81Int`.
- Explicit NaE (Not-an-Entity) state.
**Memory Model:** Stack-allocated, fixed size.
**Failure Modes:** NaE propagation.
**Canonical Representation:** Canonical serialization is stable for a given internal representation. Arithmetic determinism depends on the active math path; canonical encoding itself is stable.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Scientific computing requiring ternary floating point.
- Neural network weights (e.g. `T81Float<72,9>`).

### Avoid When
- Strict cross-platform bit-exactness of division or transcendental functions is required (current implementation falls back to `double`).

### Determinism Notes
- Addition, subtraction, and multiplication are native and deterministic.
- Division and transcendental coverage remain conditional on the active math backend.
- Deterministic software-math paths now exist for significant portions of the tensor/AI execution surface even when results remain in a float-domain class.

### Architectural Risks
- Division fallback to `double` breaks strict determinism guarantees across different FPUs.
- `from_double` conversion logic has specific clamping (`kSafeTrits = 39`) which may lose precision for large mantissas unexpectedly.

---

## `T81BigInt`

**Layer:** Arithmetic
**Determinism Class:** Strict
**Primary Invariants:**
- Sign-magnitude representation.
- Magnitude stored as `std::vector<T81Int<81>>`.
- Magnitude is strictly non-negative.
**Memory Model:** Heap-allocated (vector of limbs).
**Failure Modes:** `std::bad_alloc` on massive numbers.
**Canonical Representation:** Base-81 string (`to_base81_string`).
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Cryptography.
- ID generation.
- Arbitrary precision arithmetic.

### Avoid When
- Zero-allocation guarantees are required (use `T81Int`).
- Real-time hard constraints are present (due to allocation).

### Determinism Notes
Strictly deterministic. Uses Karatsuba multiplication and Knuth's Algorithm D for division.

### Architectural Risks
- Unbounded memory usage for extremely large values.
- "Allocation Pathology" is mentioned in friend classes, implying known vectors for DoS via resource exhaustion.

---

## `T81Tensor`

**Layer:** Container
**Determinism Class:** Element-Dependent (Strict for integer scalar; Host-Dependent if float or host math used)
**Primary Invariants:**
- Rank and Dimensions fixed at compile time.
- Elements must satisfy `T81Element` concept (size $\le$ 32 bytes).
- 64-byte aligned contiguous storage.
**Memory Model:** Hybrid (Stack for small $\le$ 4KB, Heap `std::vector` for large).
**Failure Modes:** `std::bad_alloc` for large dimensions.
**Canonical Representation:** `serialize_canonical`.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Neural network layers.
- Fixed-size matrix operations.
- Hardware-accelerated kernels (theoretical).

### Avoid When
- Dynamic resizing is required.
- Dimensions are large enough to blow the stack.

### Determinism Notes
All operations (broadcast, slice, matmul, conv2d) are structurally deterministic.

### Architectural Risks
- `T81Element` constraint prevents storing complex objects, which is a feature for determinism but a constraint for flexibility.

---

## `T81Graph`

**Layer:** Container
**Determinism Class:** Strict (structural). Heap-backed for large graphs. Deterministic export via sorted adjacency.
**Primary Invariants:**
- Static `NodeCount` and `MaxDegree`.
- Adjacency list storage.
**Memory Model:** Hybrid (Stack for small, Heap `std::vector` for large).
**Failure Modes:**
- `std::bad_alloc`.
**Canonical Representation:** `serialize_canonical` (Sorted Adjacency List).
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Fixed-topology graph algorithms (PageRank, BFS).
- Sparse connectivity matrices.

### Avoid When
- Graph size exceeds ~65k nodes (due to `NodeID` limit).
- Dynamic topology changes (node addition/removal) are needed.

### Determinism Notes
Strictly deterministic tensor-like operations.

### Architectural Risks
- NodeID automatically widens to `uint32_t` for large graphs (>65535 nodes), resolving overflow issues.

---

## `T81List`

**Layer:** Container
**Determinism Class:** Strict
**Primary Invariants:**
- Backed by `std::vector<E>`.
- Element size $\le$ 256 bytes (enforced concept).
**Memory Model:** Heap-allocated contiguous buffer (`alignas(64)` wrapper).
**Failure Modes:** `std::bad_alloc`.
**Canonical Representation:** `[...]` via `operator<<`.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Dynamic sequences where compile-time size is impossible.
- Accumulators.

### Avoid When
- Stack allocation is strictly required (use `T81Tensor` or `std::array`).
- "Hardware-native" (tryte-aligned) storage is strictly required (current impl is a wrapper).

### Determinism Notes
Wrapper around `std::vector`. Deterministic if elements are.

### Architectural Risks
- Future migration to "true tryte-aligned ternary buffer" is mentioned in comments, implying current storage is provisional and ABI-unstable.

---

## `T81Map`

**Layer:** Container
**Determinism Class:** Order-Dependent
**Primary Invariants:**
- Open addressing, linear probing (step 1).
- Max load factor 0.729 ($3^{-1}$).
**Memory Model:** Heap-allocated `std::vector<Bucket>`.
**Failure Modes:** `std::bad_alloc`.
**Canonical Representation:** serialize_canonical() (sorted keys). Runtime iteration order remains insertion-dependent.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Associative lookups (Symbol -> Value).
- Vocabulary mapping.

### Avoid When
- **Iteration order determinism is required**.
- Determinism across runs is required (if hash function or insertion order varies).

### Determinism Notes
Iteration order is coupled to insertion history and hash function layout. If `T81Symbol` identities (which are insertion-order dependent) are used as keys, the map layout becomes nondeterministic across runs.

### Architectural Risks
- Linear probing on a heap vector introduces significant cache pressure compared to static structures.
- Nondeterministic iteration is a high risk for consensus logic.

---

## `T81Vector`

**Layer:** Container (Geometric)
**Determinism Class:** Host-Dependent
**Primary Invariants:**
- Fixed dimension (1..81).
- Stack allocated.
**Memory Model:** Stack array.
**Failure Modes:** None.
**Canonical Representation:** None.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** None.

### Intended Use Cases
- Geometric linear algebra (Dot/Cross products, Rotations).
- Physics simulations.

### Avoid When
- Strict bit-exactness of geometric ops (`length`, `angle`) is required (depends on `Scalar`).

### Determinism Notes
While structural operations (+, -) are deterministic, geometric operations (`length`, `normalized`, `angle`) default to using `T81Float` which falls back to host `double` for `sqrt`/`acos`.

### Architectural Risks
- Implicit dependency on `double` precision via `Scalar` bridge.

---

## `T81Result`

**Layer:** Control Flow
**Determinism Class:** Side-Effecting
**Primary Invariants:**
- Holds `T` or `T81Error`.
**Memory Model:** Stack/Heap (Error contains String).
**Failure Modes:** None (Monadic).
**Canonical Representation:** None.
**Entropy Semantics:** **Implicitly Expensive** (Error construction consumes entropy).
**Reflection Exposure:** `reflect()` enabled.
**Governance Exposure:** None.

### Intended Use Cases
- Error propagation.
- Functional pipeline composition (`map`, `and_then`).

### Avoid When
- Zero-cost error handling is required (Error construction is heavy).
- Deterministic/Pure functions are required (Error construction is side-effecting).

### Determinism Notes
**Critical Nondeterminism**: Constructing a `T81Error` (even for a copy) calls `acquire_entropy()` and `T81Time::now()`. This means entering a failure path permanently alters the global entropy state and is coupled to the system clock.

### Architectural Risks
- Error paths are not just slow; they are **state-mutating**. Triggering an error changes the PRNG sequence for subsequent operations, potentially causing "Butterfly Effects" in simulations.

---

## `T81Symbol`

**Layer:** Symbolic
**Determinism Class:** Policy-Dependent
**Primary Invariants:**
- 81-trit value.
- Interned via global `InternTable`.
**Memory Model:** Global static `unordered_map` (heap).
**Failure Modes:** Global lock contention.
**Canonical Representation:** Canonical export uses name; numeric ID remains runtime-local.
**Entropy Semantics:** None.
**Reflection Exposure:** None.
**Governance Exposure:** Intern Table access.

### Intended Use Cases
- Unique identifiers.
- Keyword arguments / Enums.
- Control flow tokens.

### Avoid When
- Deterministic cross-run identity is required without strict initialization order.
- High-frequency creation is needed (lock contention).

### Determinism Notes
Export determinism fixed. Runtime identity ordering still Order-Dependent. IDs are assigned sequentially via a global counter protected by a mutex. The numeric value of a symbol depends on the runtime order of its first interning. If threads race to intern new symbols, their numeric IDs will vary across runs.

### Architectural Risks
- **Identity Drift**: Serialized graphs containing symbols may be invalid if loaded into a process with a different interning history.
- Global mutable state (`InternTable`) is a single point of contention and nondeterminism source.

---

## `T81Entropy`

**Layer:** System
**Determinism Class:** Schedule-Dependent (multi-thread) / Strict given single-thread acquisition order
**Primary Invariants:**
- Move-only (Affine).
- Single consumption.
**Memory Model:** Stack value (81 trits).
**Failure Modes:** `std::terminate` on double consume.
**Canonical Representation:** None.
**Entropy Semantics:** Explicitly tracked.
**Reflection Exposure:** None.
**Governance Exposure:** `EntropyPool` access.

### Intended Use Cases
- Stochastic algorithms requiring provenance.
- Monte Carlo simulations.

### Avoid When
- True cryptographic entropy is required (implementation is a PRNG).
- High concurrency (atomic contention on global counter).

### Determinism Notes
The implementation is named `hardware_trng` but is actually a **deterministic PRNG** seeded with a constant (`0x517cc1b727220a95ULL`). While strictly deterministic given a sequence order, reliance on global atomic counters means thread scheduling dictates the entropy sequence received by any specific caller.

### Architectural Risks
- **False Security**: Naming suggests TRNG, code implements PRNG.
- Global atomic counter introduces serialization bottleneck.
- **Fixed**: Data races on `seed_` are resolved via mutex.

---

## `T81Time`

**Layer:** System
**Determinism Class:** Host-Dependent (Strict if synchronized override)
**Primary Invariants:**
- Wraps `std::chrono::steady_clock`.
**Memory Model:** Stack value.
**Failure Modes:** None.
**Canonical Representation:** `t+...us@EVENT`.
**Entropy Semantics:** [IMPLICIT].
**Reflection Exposure:** `reflect()` enabled.
**Governance Exposure:** `set_deterministic_time`.

### Intended Use Cases
- Logging and diagnostics.
- Timeouts.

### Avoid When
- Deterministic logic depends on time values (unless override is strictly managed).

### Determinism Notes
Host-Dependent by default. Strict under deterministic override (synchronized). Can be forced to deterministic mode via global static override.

### Architectural Risks
- **Fixed**: Data races on `deterministic_override_` are resolved via mutex.

---

## 4. Cross-Type Constraint Analysis

### 1. The Stack vs. Heap Schism
- `T81Tensor` and `T81Graph` now implement a hybrid storage model (Stack for small, Heap for large).
- `T81BigInt` and `T81List` are heap-allocated.
- **Resolution**: The system now robustly handles large static structures by automatically promoting them to heap storage, preventing stack overflow while preserving stack optimization for small instances.

### 2. Identity & Determinism Gap
- `T81Symbol` is fundamental to `T81Graph` and `T81Tensor` (for labeled dims/nodes), yet it introduces **runtime nondeterminism** via its interning mechanism.
- This weakens the strict determinism of `T81Graph` and `T81Tensor` when they rely on symbolic identities that may shift between runs.

### 3. Floating Point Purity
- `T81Float` claims to be a ternary float but relies on `double` for division and transcendentals.
- This creates a dependency on the host FPU, violating the "ternary-native" architectural goal and potentially causing divergence in distributed consensus if hosts differ (e.g., x86 vs ARM vs T81 hardware).

### 4. Graph Scalability Illusion
- **Resolved**: `T81Graph` now supports massive graphs via heap promotion and automatic `NodeID` widening (to `uint32_t`). `KnowledgeGraph` is now fully supported.

### 5. Error Handling Side Effects
- `T81Result` treats errors as heavy, side-effecting events (burning entropy). This creates a perverse incentive to avoid proper error reporting in tight loops to maintain deterministic entropy state, potentially leading to silent failures or fragile code.

---

## 5. Determinism Surface Summary

| Type | Safe for Core VM? | Policy Gating Required? | Nondeterminism Vector |
| :--- | :--- | :--- | :--- |
| `T81Int` | **YES** | No | None |
| `T81Float` | **NO** | Yes (Ban division/transcendentals) | Host FPU variance |
| `T81BigInt` | **YES** | Yes (Memory limits) | Allocation failure |
| `T81Tensor` | **Conditional** | Yes | Float ops allowed |
| `T81Graph` | **YES** | None | Memory Quota |
| `T81List` | **YES** | Yes (Memory limits) | Allocation failure |
| `T81Map` | **Conditional** | Yes | Iteration order (Insertion/Hash dependent) |
| `T81Vector` | **Conditional** | Yes | Float fallback |
| `T81Symbol` | **Conditional** | Yes | Policy: do not rely on numeric ID |
| `T81Result` | **NO** | Yes (Side effects) | Entropy burn on error |
| `T81Entropy` | **Conditional** | Yes | Schedule-dependent (multi-thread) |
| `T81Time` | **NO** | Yes (Must mock) | Host clock (unless injected) |

**Conclusion**: The core arithmetic layer (`T81Int`, `T81BigInt`) is robust. The container layer (`Tensor`, `Graph`) suffers from stack allocation constraints and type definition bugs. The system/symbolic layer (`Symbol`, `Entropy`, `Time`) introduces significant nondeterminism and thread-safety risks that must be mitigated by strict userland policy. The Control Flow layer (`Result`) introduces unique side-effect risks in error paths.
