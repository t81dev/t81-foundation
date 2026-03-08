______________________________________________________________________

title: T81 Strict Determinism Profile
nav:

- [Overview](t81-overview.md)
- [Data Types](t81-data-types.md)
- [TISC Specification](tisc-spec.md)
- [T81 Virtual Machine](t81vm-spec.md)
- [T81Lang](t81lang-spec.md)
- [Axion Kernel](axion-kernel.md)
- [Determinism Profile](determinism-profile.md)

______________________________________________________________________

[← Back to Spec Index](index.md)

# T81 Strict Determinism Profile

**Version 1.0 — Normative**

**Status:** Stable
**Applies To:** T81VM, Axion, Core Numerics

This document defines the **Strict Determinism Profile**, a normative subset of T81 semantics that guarantees bit-exact reproducibility across all conformant architectures and host platforms.

It provides the authoritative definition for "VM-Safe" behaviors and enumerates the specific constraints required to achieve **Tier A** determinism.

______________________________________________________________________

# 1. Determinism Tiers

T81 defines six tiers of deterministic assurance. The Strict Profile corresponds to **Tier A**.

| Tier | Name | Description | Host Dependency | Reproducibility |
|---|---|---|---|---|
| **A** | **Strict / Bit-Exact** | Pure integer/ternary logic, software-defined float math, canonical serialization. | **None** | **Absolute** (Cross-Arch, Cross-OS) |
| **B** | **Canonical Numeric** | Allowed host-optimized float arithmetic if IEEE-754 compliant. | Limited (FPU) | High (Arch-dependent float rounding) |
| **C** | **Host-Tolerant** | Default mode. Allows host `double` for div/transcendentals. | Yes (Math lib) | Functional (Values within epsilon) |
| **D** | **Async / Concurrent** | Deterministic scheduling, but allows host-defined thread grouping. | Scheduler | Logical sequence only |
| **E** | **Network / Dist** | Consensus-based state; local node variance allowed if consensus holds. | Network | State Machine |
| **F** | **Unconstrained** | Debug/Legacy. Access to wall-clock, host RNG, raw pointers. | **High** | **None** |

**Constraint:** All "Strict Mode" flags in CLI and Axion imply **Tier A**.

______________________________________________________________________

# 2. VM-Safe Types

Under the Strict Profile, types are classified as Safe, Conditional, or Forbidden.

## 2.1 Safe Types (Always Allowed)

*   `Trit`, `Tryte`
*   `T81Int<N>` (all sizes)
*   `T81BigInt`
*   `T81Fraction` (Canonicalized)
*   `T81Option[T]`, `T81Result[T,E]` (where T, E are Safe)
*   `List[T]`, `Vector[T]`, `Tensor[T]` (where T is Safe; Rank limits apply)
*   `String` (UTF-8, normalized)

## 2.2 Conditional Types (Guarded)

*   **`T81Float`**:
    *   **Allowed:** Construct from Integer, Add, Sub, Mul (Software backend).
    *   **Forbidden:** `Div`, `Sin`, `Cos`, `Log`, `Exp`, `Sqrt` **UNLESS** the `dmath` software backend is explicitly active.
    *   **Trap:** Attempting hardware-backed float ops triggers `FLOAT_OP_FORBIDDEN`.

*   **`T81Map[K,V]`**:
    *   **Allowed:** If K is comparable.
    *   **Constraint:** Iteration order MUST be sorted by K. Runtime pointer-order iteration is **Forbidden**.

### 2.2.1 Tensor Float-Domain Classification

Tensor numeric classification and arithmetic provenance are related but not
identical.

- `ExactTrit` and `ExactInt` denote semantically exact tensor values that are
  eligible for strict-core promotion under current tensor rules.
- `HostFloat` denotes a non-exact float-domain tensor result class.

In deterministic builds, a tensor classified as `HostFloat` MAY still be
produced by deterministic software math rather than host `<cmath>` or hardware
FPU behavior. Therefore:

- `HostFloat` does **not** automatically mean host-dependent arithmetic
- deterministic arithmetic does **not** automatically imply promotion to
  `ExactInt`

This distinction covers tensor kernels such as non-fixed `matmul` and other
float-domain operations that now execute with stronger deterministic guarantees
while remaining outside the exact integer/trit domain.

## 2.3 Forbidden Types

*   `RawPointer` / `HostAddress`
*   `SystemHandle` (File descriptors, sockets) unless wrapped in CanonFS/Axion handles.
*   `HostTime` / `Date` (unless derived from block height/step count).
*   `HardwareFloat` (Direct IEEE-754 mapping without canonicalization).

______________________________________________________________________

# 3. Forbidden Operations

The following operations MUST trigger a deterministic trap in Tier A execution:

1.  **Host Entropy Access**: Reading `/dev/random`, `std::random_device`, or unseeded memory.
2.  **Wall-Clock Access**: `std::chrono::now()`, `gettimeofday()`.
3.  **Address Observability**: Operations that reveal ASLR layout or heap pointer values (e.g., default `object.toString()` using address).
4.  **Unsorted Iteration**: Iterating over a hash map based on bucket order.
5.  **Non-Canonical Serialization**: Emitting non-normalized forms of BigInt/Fraction/Float.

______________________________________________________________________

# 4. Deterministic Failure Semantics

Failures must be as reproducible as successes. The Strict Profile defines standard **Deterministic Traps**.

## 4.1 Trap Taxonomy

| Trap Name | Trigger Condition | Axion Event Reason |
|---|---|---|
| **OOM_QUOTA** | Memory allocation exceeds deterministic quota (not host RAM). | `quota exceeded segment=heap limit=<N>` |
| **POLICY_DENIED** | Axion policy forbids operation (e.g., file write). | `policy violation op=<OP> target=<T>` |
| **FLOAT_OP_FORBIDDEN** | Forbidden hardware-backed float op in Tier A. | `float op forbidden opcode=<OP>` |
| **SYMBOL_ID_FORBIDDEN** | Accessing symbol by runtime ID instead of hash. | `symbol semantics violation id=<ID>` |
| **ENTROPY_VIOLATION** | Attempt to access non-deterministic RNG. | `entropy source forbidden` |
| **TIME_VIOLATION** | Attempt to read host clock. | `time source forbidden` |

## 4.2 Replay Invariant

For every Trap:
*   The instruction pointer (PC) MUST halt at the exact failing instruction.
*   The stack and memory state MUST be preserved exactly up to the fault.
*   The Axion log MUST contain the specific Trap Reason as the final entry.

______________________________________________________________________

# 5. Symbol Identity Stabilization

In Strict Mode, Symbols are **Content-Addressed**.

*   **Identity**: `Hash(StringContent)` (SHA-256 or defined canonical hash).
*   **Equality**: `A == B` iff `Hash(A) == Hash(B)`.
*   **Runtime IDs**: Transient integers used for performance (interning) MUST NOT be observable by user code.
    *   **Forbidden**: `Symbol.id()`, sorting by ID.
    *   **Allowed**: `Symbol.toString()`, sorting by String content.
*   **Serialization**: Symbols MUST serialize as their string content or canonical hash, never their runtime ID.

______________________________________________________________________

# 6. Error Determinism Fix

Errors (`Result::Err(E)`) must be value types.

*   **Constructors**: Error constructors MUST NOT capture:
    *   Stack traces with host pointers.
    *   Timestamps.
    *   Thread IDs.
*   **Representation**: Errors are structural records `{ code: CanonicalCode, message: String, context: Map[String, String] }`.
*   **Equality**: Two errors are equal if their structure is identical.

______________________________________________________________________

# 7. Canonical Serialization Unification

## 7.1 Wire Format

*   **Canonical Binary**: Base-81 packed format (TBD) or MsgPack with canonical sorting.
*   **Canonical Text**: JSON/TSON with keys sorted lexically.

## 7.2 CanonFS Boundary

*   All data crossing the CanonFS boundary (persistence) MUST be in Canonical Wire Format.
*   In-memory objects MUST be normalized (e.g., `BigInt` zero-stripped) before serialization.

______________________________________________________________________

# 8. Required Evidence & Enforcement

To claim compliance with the Strict Determinism Profile:

1.  **CI Enforcement**: `t81_repro_gate` MUST pass for Tier A workloads.
2.  **Fuzzing**: `frontend_fuzz_test` MUST NOT produce crashes or non-deterministic outputs.
3.  **Trace Validation**: Axion traces MUST match bit-for-bit across Linux (x64/ARM64) and macOS (ARM64).

______________________________________________________________________
