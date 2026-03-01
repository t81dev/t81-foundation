# Deterministic Core Profile v1

**Version:** 1.0 (Frozen)
**Status:** **Active**
**Reference:** `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`

This document defines the **Minimal Deterministic Core** of the T81 ecosystem.
Only components explicitly listed here as "Included" are guaranteed to be bit-exact reproducible across supported architectures.

## 1. Core Profile Definition

The "Deterministic Core" is the set of subsystems required to execute TISC bytecode with bit-exact state transitions.

| Component | Included | Frozen | Verified | Notes |
| :--- | :---: | :---: | :---: | :--- |
| **TISC ISA** | ✅ | ✅ | ✅ | Defined in `spec/tisc-spec.md`. Bit-exact opcodes. |
| **Data Types** | ✅ | ✅ | ✅ | `Trit`, `Tryte`, `T81BigInt`. Canonical encoding. |
| **Soft-Float (dmath)** | ✅ | ✅ | ✅ | `T81Float` arithmetic (no hardware FPU). |
| **Interpreter Execution** | ✅ | ✅ | ✅ | The reference `t81::vm::Interpreter`. |
| **Canonical Serialization** | ✅ | ✅ | ✅ | `CanonFS` binary format. |
| **JIT Compiler** | ❌ | ❌ | ❌ | Experimental. No equivalence guarantee yet. |
| **Cognitive Tiers** | ❌ | ❌ | ❌ | Higher-order logic (Axion/Hanoi) is evolving. |
| **Distributed Compute** | ❌ | ❌ | ❌ | Network timing/ordering is non-deterministic. |
| **Model Tooling** | ❌ | ❌ | ⚠️ | Quantization is verified, but tooling is auxiliary. |
| **Experimental Subsystems** | ❌ | ❌ | ❌ | Anything in `src/experimental/`. |

## 2. Invariants

1.  **Bit-Exactness**: All "Included" components MUST produce identical bitstreams for outputs on x86-64 and ARM64.
2.  **No Hardware Float**: The core MUST NOT use native hardware floating-point instructions for `T81Float` operations.
3.  **No Native I/O**: The core execution loop MUST be isolated from wall-clock time, network, or random OS events (entropy must be injected).

## 3. Stability Guarantee

The components marked **Frozen** are subject to the strict change control defined in `docs/governance/FREEZE_ENFORCEMENT.md`.
Any change to their behavior requires a **Major Version Bump**.
