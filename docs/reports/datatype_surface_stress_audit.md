# T81Lang Datatype Surface Stress Audit

## 1. Primary Objective

This document provides an exhaustive exploration of every datatype exposed in T81Lang, identifying semantic, determinism, serialization, VM, lowering, persistence, and performance shortcomings.

## 2. Primitives

### Type: `i2`, `i8`, `i16`, `i32` (Aliased to T81Int/BigInt)
#### Surface Classification
* VM Alias (mapped to arbitrary-precision BigInt)
#### Findings
* Architectural weakness: The language defines fixed-precision types, but the VM handles them as `T81BigInt` (arbitrary precision) or standard `T81Int`. This means implicit precision bounds are only checked at runtime, not natively enforced by a fixed-size opcode.
#### Determinism Risk Level
* Low
#### Required Fixes
* Replace polyfill with native fixed-precision types in VM, or enforce strict bounds checking in `IRGenerator` on every arithmetic op.
#### Priority
* Low (performance/cleanup)

### Type: `T81Uint`
#### Surface Classification
* VM Alias
#### Findings
* Clamped wrapper over `T81Int`. Can suffer from negative-value UB if the clamping logic isn't perfectly timed during intermediate calculations.
#### Determinism Risk Level
* Low
#### Required Fixes
* IR changes to enforce unsigned arithmetic boundaries instead of relying purely on wrapping.
#### Priority
* Low

### Type: `T81BigInt`
#### Surface Classification
* Fully Native
#### Findings
* Language-level parsing limits precision to 64-bit limits (`i64`) to prevent undefined arbitrary-precision scaling in the VM during literal parsing. This creates a disconnect between the VM's true capability (GMP compatibility) and the frontend's artificial limit.
#### Determinism Risk Level
* Low (but structurally disjoint)
#### Required Fixes
* Align literal parsing limits with VM limits, or explicitly document the 64-bit parsing limitation as spec.
#### Priority
* Medium (incomplete surface)

### Type: `T81Float`
#### Surface Classification
* Host-Dependent
#### Findings
* Transcendental functions (`cmath` like `expi`, `sqrt`) are gated behind `#ifndef T81_DETERMINISTIC`. In deterministic builds, they throw `std::domain_error`. Cross-platform bit-exactness is not guaranteed for transcendental math.
* Zero canonicalization explicitly outputs `+0E0` or `-0E0`, but IEEE 754 signed zero rules still create risks if not consistently normalized in all math ops.
#### Determinism Risk Level
* High
#### Required Fixes
* Deterministic math rewrite for transcendental functions to eliminate `cmath` host dependency.
#### Priority
* High (breaks correctness)

### Type: `T81Fraction`
#### Surface Classification
* Fully Native
#### Findings
* Exact rational number. Zero is encoded as `0/1`. Stable and fully native.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Fixed`
#### Surface Classification
* VM Alias
#### Findings
* Implemented as an alias for Integer types in the frontend and VM, inheriting integer serialization and determinism characteristics. Not a true fixed-point VM native type.
#### Determinism Risk Level
* None
#### Required Fixes
* VM opcode additions for true fixed-point scaled arithmetic if performance is needed.
#### Priority
* Low

### Type: `T81Complex`
#### Surface Classification
* Fully Native
#### Findings
* Supported in the VM via `MAKE_COMPLEX` opcode and includes canonical binary serialization support through `complex_pool` in `binary_io.cpp`. However, it shares the transcendental `cmath` dependency issues with `T81Float`.
#### Determinism Risk Level
* Moderate
#### Required Fixes
* Deterministic math rewrite for complex transcendentals.
#### Priority
* High

### Type: `T81Quaternion`
#### Surface Classification
* Stub
#### Findings
* Acts as a NOP during IR lowering and lacks dedicated VM opcodes. Complex data types like `T81Quaternion` lack constant pools in `tisc::Program`.
#### Determinism Risk Level
* Structural
#### Required Fixes
* VM opcode additions and binary pool support for Quaternions.
#### Priority
* Medium

### Type: `T81Prob`
#### Surface Classification
* Deterministic but Under-tested
#### Findings
* Native log-odds probability representation. Relies heavily on exact fixed-point math which needs aggressive boundary stress testing (kMinValue/kMaxValue transitions).
#### Determinism Risk Level
* Low
#### Required Fixes
* None structurally, but requires expanded edge-case testing.
#### Priority
* Low

### Type: `T81Qutrit`
#### Surface Classification
* VM Alias
#### Findings
* Alias for `T81Int<2>`.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `Cell`
#### Surface Classification
* Fully Native
#### Findings
* Left-shift operations now explicitly throw `std::overflow_error` if non-zero trits are shifted out.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

## 3. Collections

### Type: `T81String`
#### Surface Classification
* Fully Native
#### Findings
* Handled as normalized ASCII/Ternary text strings.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Bytes`
#### Surface Classification
* Fully Native
#### Findings
* Canonical byte buffer.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Vector`
#### Surface Classification
* Fully Native
#### Findings
* Constructors accept fundamental arithmetic types and automatically bridge them.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Matrix`, `T81Tensor`
#### Surface Classification
* Fully Native
#### Findings
* `Tensor.matmul` enforces strict shape compatibility in the VM (traps with `ShapeFault` on mismatch). Large matrices/tensors currently require deep-copying in some paths, masking performance issues.
#### Determinism Risk Level
* None
#### Required Fixes
* Performance hardening (eliminate unnecessary deep copies).
#### Priority
* Low

### Type: `T81List`
#### Surface Classification
* Fully Native
#### Findings
* Lacks constant pools in `tisc::Program`.
#### Determinism Risk Level
* Low
#### Required Fixes
* Binary pool support.
#### Priority
* Medium

### Type: `T81Map`
#### Surface Classification
* Fully Native (Host-Dependent Keys)
#### Findings
* Relies on `std::hash` for non-symbol keys, breaking cross-platform determinism invariants.
* Iteration is non-deterministic at runtime, but `serialize_canonical()` sorts elements by key.
* Direct initialization with empty object literals (`{}`) causes type errors.
#### Determinism Risk Level
* Critical
#### Required Fixes
* Replace `std::hash` with a strictly deterministic, cross-platform hashing algorithm.
* Fix frontend type errors for empty initialization.
#### Priority
* Critical (breaks determinism)

### Type: `T81Set`
#### Surface Classification
* Fully Native
#### Findings
* Inherits `T81Map`'s `std::hash` cross-platform determinism break for non-symbol keys.
#### Determinism Risk Level
* Critical
#### Required Fixes
* Replace `std::hash` with deterministic hash.
#### Priority
* Critical

### Type: `T81Tree`, `T81Graph`
#### Surface Classification
* Polyfill / Stub
#### Findings
* Still fall back to `STRVECNEW` string vectors in the frontend. They are NOT natively backed by dedicated VM opcodes.
* Lack constant pools in `tisc::Program`.
#### Determinism Risk Level
* Structural
#### Required Fixes
* Replace polyfill with native VM opcodes (`TreeNew`, `GraphNew`, etc.).
* Binary pool support.
#### Priority
* High

## 4. Symbolic

### Type: `T81Symbol`
#### Surface Classification
* Fully Native
#### Findings
* `std.symbol.intern` correctly enforces distinct type identity from `T81String`.
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Symbolic`, `T81Polynomial`
#### Surface Classification
* Experimental / Stub
#### Findings
* Classified as drift from the current Draft Spec. Not fully supported in the VM.
#### Determinism Risk Level
* Structural
#### Required Fixes
* VM opcode additions and canonical serialization enforcement.
#### Priority
* Medium

## 5. System

### Type: `Option`, `Result`
#### Surface Classification
* Fully Native
#### Findings
* Verified as structurally sound and natively supported by opcodes (e.g. `MakeOptionSome`).
#### Determinism Risk Level
* None
#### Required Fixes
* None.
#### Priority
* Low

### Type: `T81Time`, `T81Entropy`
#### Surface Classification
* Host-Dependent / Stub
#### Findings
* `std.sys.entropy()` returns a constant `0` in the default test environment (deterministic mock). True behavior requires strict auditing to ensure real environments don't leak non-determinism.
#### Determinism Risk Level
* High
#### Required Fixes
* Ensure strict decoupling of host clock/entropy from VM consensus state.
#### Priority
* Critical

### Type: `T81Promise`, `T81Agent`
#### Surface Classification
* Experimental / Stub
#### Findings
* Considered drift from current Draft Spec. Thread scheduling determinism is not implemented.
#### Determinism Risk Level
* Structural
#### Required Fixes
* Implement deterministic threading/scheduling model.
#### Priority
* High

---

## 6. Gap Matrix

| Type | Native? | Canonical? | Persistable? | Deterministic? | Risk |
|---|---|---|---|---|---|
| `iN` / `T81Uint` | Alias | Yes | Yes | Yes | Low |
| `T81BigInt` | Yes | Yes | Yes | Yes (Limit in Parser) | Low |
| `T81Float` | Yes | Yes | Yes | No (Host `cmath`) | High |
| `T81Fraction` | Yes | Yes | Yes | Yes | None |
| `T81Fixed` | Alias | Yes | Yes | Yes | None |
| `T81Complex` | Yes | Yes | Yes | No (Host `cmath`) | Moderate |
| `T81Quaternion` | No | No | No | No (NOP) | Structural |
| `T81Prob` | Yes | Yes | Yes | Yes | Low |
| `T81Qutrit` | Alias | Yes | Yes | Yes | None |
| `Cell` | Yes | Yes | Yes | Yes | None |
| `T81String` | Yes | Yes | Yes | Yes | None |
| `T81Bytes` | Yes | Yes | Yes | Yes | None |
| `T81Vector` | Yes | Yes | Yes | Yes | None |
| `T81Matrix` | Yes | Yes | Yes | Yes | Low |
| `T81Tensor` | Yes | Yes | Yes | Yes | Low |
| `T81List` | Yes | Yes | No | Yes | Low |
| `T81Map` | Yes | Yes (Sorted) | Yes | No (`std::hash`) | Critical |
| `T81Set` | Yes | Yes (Sorted) | Yes | No (`std::hash`) | Critical |
| `T81Tree` | No | No | No | Yes (Polyfill) | Structural |
| `T81Graph` | No | No | No | Yes (Polyfill) | Structural |
| `T81Symbol` | Yes | Yes | Yes | Yes | None |
| `T81Symbolic` | Stub | No | No | No | Structural |
| `T81Polynomial`| Stub | No | No | No | Structural |
| `Option`/`Result`| Yes | Yes | Yes | Yes | None |
| `T81Time` | Stub | No | No | No | High |
| `T81Entropy` | Stub | No | No | No | High |
| `T81Promise` | Stub | No | No | No | Structural |
| `T81Agent` | Stub | No | No | No | Structural |

## 7. Architectural Risk Report

* **Types that falsely claim determinism**: `T81Map` and `T81Set` claim determinism via key sorting, but rely on `std::hash` for internal bucket placement, breaking cross-platform determinism invariants. `T81Float` and `T81Complex` rely on host `cmath` for transcendentals, breaking strict bit-exact determinism.
* **Types exposed but VM-incomplete**: `T81Tree` and `T81Graph` are polyfilled as `Vector[String]`. `T81Quaternion` acts as a NOP.
* **Types blocking Beta promotion**: The `std::hash` usage in `T81Map` and `T81Set`. The reliance on `cmath` for floats.
* **Types requiring spec correction**: `T81BigInt` limits in literal parsing need documentation. The experimental drift types (`T81Agent`, `T81Promise`, `T81Symbolic`) need formal spec inclusion or removal from the frontend.

## 8. Ordered Remediation Plan

* **Phase 1 — Determinism-Critical Fixes**: Replace `std::hash` in `T81Map`/`T81Set` with a deterministic algorithm. Remove `cmath` dependency for `T81Float`/`T81Complex`.
* **Phase 2 — Canonical Serialization Enforcement**: Add binary constant pool support for `T81List`, `T81Tree`, `T81Graph`, and `T81Quaternion`.
* **Phase 3 — VM Native Type Promotion**: Implement dedicated VM opcodes for `T81Tree` and `T81Graph`, ending the string-vector polyfill era. Implement `T81Quaternion` math ops.
* **Phase 4 — Polyfill Elimination**: Enforce true fixed-precision VM bounds for `i8`/`i16`/`i32` and `T81Fixed`.
* **Phase 5 — Performance Hardening**: Optimize Tensor deep-copying paths. Refine log-odds arithmetic in `T81Prob`.
