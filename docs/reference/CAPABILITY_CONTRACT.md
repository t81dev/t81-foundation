# T81 Foundation — Capability Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — Capability Contract](#t81-foundation-—-capability-contract)
  - [Intended Use](#intended-use)
- [1. Purpose](#1-purpose)
- [2. Execution Stack Overview](#2-execution-stack-overview)
- [3. Determinism Scope](#3-determinism-scope)
  - [3.1 Compile-Time Deterministic (CI-Enforced)](#31-compile-time-deterministic-ci-enforced)
  - [3.2 Runtime Deterministic (Pure Logic)](#32-runtime-deterministic-pure-logic)
  - [3.3 Host-Dependent](#33-host-dependent)
  - [3.4 Hardware-Dependent](#34-hardware-dependent)
- [4. Governance & Policy Enforcement](#4-governance-&-policy-enforcement)
  - [4.1 Axion Capabilities](#41-axion-capabilities)
  - [4.2 Invocation Model](#42-invocation-model)
  - [4.3 Isolation Model](#43-isolation-model)
- [5. CanonFS Integrity Model](#5-canonfs-integrity-model)
  - [5.1 Write Path (Enforced)](#51-write-path-enforced)
  - [5.2 Read Path (Verified by Default)](#52-read-path-verified-by-default)
  - [5.3 Persistence](#53-persistence)
- [6. Performance Model](#6-performance-model)
  - [6.1 Interpreter](#61-interpreter)
  - [6.2 Threaded Trace (“JIT”)](#62-threaded-trace-“jit”)
  - [6.3 Tensor Engine](#63-tensor-engine)
- [7. Cognitive Tiers](#7-cognitive-tiers)
- [8. Hard Invariants](#8-hard-invariants)
- [9. Negative Space (Explicit Non-Capabilities)](#9-negative-space-explicit-non-capabilities)
- [10. Failure Modes](#10-failure-modes)
- [11. Intended Use](#11-intended-use)
- [12. Not Suitable For](#12-not-suitable-for)

<!-- T81-TOC:END -->


## Intended Use

This document is intended for:

- AI systems integrating with T81
- Automated code assistants
- Security review tools
- Contributors evaluating enforcement strength

Any system reasoning about T81 must treat this document as authoritative for capability boundaries.

If behavior is not explicitly guaranteed here, it must be treated as unsupported.

**Version:** 1.0
**Last Updated:** February 2026
**Scope:** Derived strictly from current implementation (code + tests + CI).
**Authority:** Descriptive of implementation. Not normative spec text.

---

# 1. Purpose

This document defines:

* What the T81 system **can do**
* What it **cannot do**
* What is **enforced**
* What is **assumed**
* What is **experimental**
* What is **stubbed**
* What is **deterministic**
* What is **host- or hardware-dependent**

This document is binding for interpretation of system capabilities.

---

# 2. Execution Stack Overview

| Layer               | Role                              | Status                             |
| ------------------- | --------------------------------- | ---------------------------------- |
| T81Lang             | Source → TISC compiler            | Implemented                        |
| TISC                | Bytecode instruction set          | Implemented                        |
| T81VM               | Bytecode interpreter + trace mode | Implemented                        |
| Axion               | Runtime policy enforcement        | Implemented (Software-enforced)    |
| CanonFS             | Content-addressed storage         | Implemented (Beta integrity model) |
| Determinism Profile | Compile-time strict mode          | Implemented                        |
| JIT                 | Threaded trace interpreter        | Experimental (Not native code)     |
| Cognitive Tiers     | Symbolic → Infinite               | Partial / Stub                     |

---

# 3. Determinism Scope

Determinism is **scoped**, not global.

## 3.1 Compile-Time Deterministic (CI-Enforced)

| Surface                  | Enforcement           | CI  |
| ------------------------ | --------------------- | --- |
| TISC Bytecode Generation | Hash-based repro gate | Yes |
| T3_K Quantization        | Hash-based repro gate | Yes |

These surfaces are bit-exact across supported architectures.

---

## 3.2 Runtime Deterministic (Pure Logic)

| Surface                             | Enforcement             | CI               |
| ----------------------------------- | ----------------------- | ---------------- |
| T81Int Arithmetic                   | Mathematical invariants | Yes (unit tests) |
| dmath Transcendentals (fixed-point) | Pure C++ logic          | No               |

Deterministic unless compiler optimization alters behavior.

---

## 3.3 Host-Dependent

| Surface                               | Dependency            |
| ------------------------------------- | --------------------- |
| T81Float inverse/hyperbolic functions | Host libm in non-strict builds; deterministic dmath path when `T81_DETERMINISTIC` is enabled |
| T81Float rounding                     | Host stdlib           |
| Memory pointer values                 | Host allocator / ASLR |
| Thread scheduling                     | OS scheduler          |

Not guaranteed deterministic across OS/libc versions.

---

## 3.4 Hardware-Dependent

| Surface              | Dependency                          |
| -------------------- | ----------------------------------- |
| Tensor Engine (GGML) | SIMD / FMA / backend implementation |

No cross-platform bit-exact guarantee.

---

# 4. Governance & Policy Enforcement

## 4.1 Axion Capabilities

Axion can block:

* Instruction count
* Virtual stack usage
* Virtual recursion depth
* Reflection/meta-writes
* Tensor loads (via allowlist)

Axion cannot block:

* Native C++ infinite loops (if policy call skipped)
* Host OOM conditions
* Segfaults
* Side channels
* OS-level interactions

---

## 4.2 Invocation Model

Interpreter Mode:

* `PolicyEngine::evaluate()` invoked per instruction dispatch.

JIT Trace Mode:

* Invoked at trace entry and exit.
* Invoked per instruction inside trace via policy callback in trace executor.
* Deny outcomes fail closed (`SecurityFault`).

Syscalls:

* Evaluated per syscall.

---

## 4.3 Isolation Model

* Hardware Isolation: **None**
* OS Sandboxing: **None**
* Memory Segmentation: Software-checked only
* Enforcement Domain: User-space process

This system does not provide hardware-level containment.

---

# 5. CanonFS Integrity Model

## 5.1 Write Path (Enforced)

* Address = SHA3-512(content) truncated to 256 bits.
* Hash computed on write.
* Content-addressable structure enforced.

## 5.2 Read Path (Verified by Default)

* Read path re-hashes payload bytes and compares against `CanonRef` hash.
* Hash mismatch returns deterministic `DecodeError`.
* Verification is enabled by default for both in-memory and persistent drivers.
* Diagnostic override exists: `T81_CANONFS_READ_VERIFY=0`.

## 5.3 Persistence

* File-based (.blk files).
* Atomic rename not guaranteed.
* No crash-consistency journal.

---

# 6. Performance Model

## 6.1 Interpreter

* Deterministic.
* Fully policy-checked per instruction.

## 6.2 Threaded Trace (“JIT”)

* Threaded-code interpreter.
* Does not emit machine code.
* Equivalent semantics verified by tests.
* Policy checks must be embedded explicitly.

## 6.3 Tensor Engine

* Hardware-accelerated via GGML.
* Determinism not guaranteed across hardware.
* Policy only gates initial load (TLOADHASH).

---

# 7. Cognitive Tiers

| Tier                 | Status                  | Risk             |
| -------------------- | ----------------------- | ---------------- |
| Tier 1 (Symbolic)    | Implemented             | Low              |
| Tier 2 (Reflective)  | Partial                 | Medium           |
| Tier 3 (Recursive)   | Partial                 | High             |
| Tier 4 (Distributed) | Experimental            | High             |
| Tier 5 (Infinite)    | Stub (returns constant) | False Capability |

Tier 5 does not perform convergence analysis.
Some unimplemented Tier/extension opcodes are intentionally fail-closed in VM
execution (deterministic `SecurityFault`) rather than permissive placeholders.

---

# 8. Hard Invariants

The following must never be violated:

* CanonFS Write Identity: address == SHA3(content)
* Bytecode Stability (CI enforced)
* Policy invocation per interpreter dispatch
* VM memory bounds checks on segment access
* Determinism repro gates must pass before merge

---

# 9. Negative Space (Explicit Non-Capabilities)

The system does NOT:

* Provide hardware-level memory isolation
* Provide OS sandboxing
* Emit machine-code JIT
* Guarantee transcendental bit-exactness
* Guarantee tensor determinism across platforms
* Guarantee atomic persistence
* Guarantee Tier 5 convergence logic
* Guarantee thread scheduling determinism
* Provide implemented async/network execution semantics for `NSEND/NRECV/VWAIT/VYIELD`
* Provide implemented neural execution semantics for `TNEURALFWD/TNEURALBWD`

---

# 10. Failure Modes

* Host libm drift breaks float determinism.
* Omitted policy call bypasses resource limits.
* Host stack overflow precedes virtual recursion guard.
* CanonFS backing store tampering is detected when read verification is enabled (default).
* JIT trace omission of policy checks enables unbounded execution.

---

# 11. Intended Use

High-assurance symbolic or cognitive research workloads where:

* Deterministic compilation
* Canonical numeric representation
* Auditability via Axion traces

are prioritized over:

* Hardware isolation
* Native-code performance
* Real-time guarantees

---

# 12. Not Suitable For

* Hard real-time control systems
* Cryptographic key isolation
* Multi-tenant secure sandboxing
* High-throughput numeric simulation
* Host-level adversarial environments without external sandboxing
