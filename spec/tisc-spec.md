______________________________________________________________________

title: T81 Foundation Specification — TISC
nav:

- [Overview](t81-overview.md)
- [Data Types](t81-data-types.md)
- [TISC Specification](tisc-spec.md)
- [T81 Virtual Machine](t81vm-spec.md)
- [T81Lang](t81lang-spec.md)
- [Axion Kernel](axion-kernel.md)
- [Cognitive Tiers](cognitive-tiers.md)

______________________________________________________________________

[← Back to Spec Index](index.md)

# TISC — Ternary Instruction Set Computer

Version 0.3 — Draft (Standards Track)

Status: Draft → Standards Track\
Applies to: T81VM, T81Lang, Axion, Cognitive Tiers

This document defines the **Ternary Instruction Set Computer (TISC)** for the T81 Ecosystem.\
It is **normative** for all instruction encoding, execution semantics, and VM integration.

______________________________________________________________________

## 0. Design Principles

TISC MUST satisfy:

1. **Deterministic Execution**\
   Each instruction has a single, total, unambiguous semantic definition.

1. **Base-81 / Balanced Ternary Semantics**\
   Arithmetic and logic are ternary-native; binary implementation shortcuts MUST NOT change observable behavior.

1. **Zero Undefined Behavior**\
   Every operand combination either:

   - produces a canonical result, or
   - yields a deterministic fault.

1. **Axion Visibility**\
   All state changes, faults, and privileged operations MUST be observable by the Axion kernel.

1. **Layer Compatibility**\
   TISC must:

   - consume values conforming to the [Data Types](t81-data-types.md) spec
   - execute under [T81VM](t81vm-spec.md)
   - be a compilation target for [T81Lang](t81lang-spec.md)

______________________________________________________________________

## 1. Machine Model

The abstract TISC machine state is:

```text
STATE = (R, PC, SP, FLAGS, MEM, META)
```

- `R` — Register file (27 general-purpose registers)
- `PC` — Program counter
- `SP` — Stack pointer
- `FLAGS` — Condition flags and ternary status
- `MEM` — Memory segments (code, stack, heap, tensor, meta)
- `META` — Axion-visible metadata (trace, tags, fault history)

The T81VM is responsible for hosting and evolving this state; TISC defines how each instruction transforms it.

______________________________________________________________________

## 2. Register File

TISC defines **27 registers**, indexed `R0` through `R26`.

### 2.1 General-Purpose Registers

- `R0–R22`: General-purpose, caller- and callee-save conventions defined at the ABI level (out of scope here but MUST remain deterministic).

### 2.2 Special Registers (Architectural Conventions)

- `R23` — **ACC** (Accumulator)
  Common result target for arithmetic and logic.

- `R24` — **COND** (Condition / Flags Mirror)
  May be used as a read-only snapshot of last comparison or status; writes are defined as privileged or no-op depending on ABI.

- `R25` — **TMP** (Scratch / Temporary)
  Reserved for short-lived operations and micro-optimizations.

- `R26` — **ASR** (Axion System Register)
  Axion-privileged; holds Axion-related control words, verification codes, and safety markers.

Any attempt by unprivileged code to modify Axion fields in `R26` MUST trigger a deterministic fault.

______________________________________________________________________

## 3. Memory Model

Memory is a flat, addressable ternary space **partitioned into segments**:

- **CODE**: Executable instructions (read-only from non-privileged instructions)
- **STACK**: Call frames and local data
- **HEAP**: Dynamically allocated data
- **TENSOR**: Tensor/matrix storage
- **META**: Debug / Axion metadata, traces, tags

The exact physical representation is implementation-defined, but:

1. Addressing MUST be deterministic and reproducible.
1. Alignment MUST follow base-81 multiples for composite structures.
1. Out-of-bounds access MUST cause a deterministic fault (no undefined reads/writes).

______________________________________________________________________

## 4. Instruction Encoding

Each TISC instruction is encoded as a fixed-size **81-trit word**.

Conceptually the word is divided into fields:

```text
[ OPC (0..80) | MODE | RD | RS1 | RS2 / IMM_TAG | IMM_EXT / UNUSED ]
```

- `OPC` — Opcode index (0–80) selecting a semantic class
- `MODE` — Addressing and operand mode info
- `RD` — Destination register
- `RS1` — Source register 1
- `RS2` — Source register 2 (or high bits of immediate)
- `IMM_*` — Immediate data or extended encoding bits

Exact bit/trit layouts are implementation-defined, but:

- Decoding MUST be deterministic and total.
- Any unrecognized opcode or invalid MODE/operand combination MUST map to a **Decode Fault**, not undefined behavior.

______________________________________________________________________

## 5. Opcode Classes

This section defines the **normative opcode classes and their semantics**.
Implementations MAY add extensions via future RFCs but MUST NOT alter existing behavior.

For each instruction:

- `Operands` specify input/output registers or memory locations
- `Semantics` define the exact state transition
- `Faults` specify when a deterministic fault MUST occur

### 5.1 Notation

- `R[x]` — value in register x
- `MEM[a]` — value at memory address a
- `→` — denotes state transition
- `⊥` — denotes a deterministic fault

______________________________________________________________________

### 5.2 Arithmetic Instructions

All arithmetic uses **T81BigInt** semantics unless otherwise noted. Registers that
refer to `T81Float` or `T81Fraction` values SHALL contain canonical **pool
handles** (1-based indices into the literal/value pools defined by the program
image). Any opcode that dereferences a handle MUST fault with
`IllegalInstruction` if the handle is zero, negative, or out of range.

#### ADD

- **Form**: `ADD RD, RS1, RS2`
- **Semantics**:
  `R[RD] := canonical(R[RS1] + R[RS2])`
- **Faults**: None (BigInt is arbitrary precision)

#### SUB

- **Form**: `SUB RD, RS1, RS2`
- **Semantics**:
  `R[RD] := canonical(R[RS1] − R[RS2])`
- **Faults**: None

#### MUL

- **Form**: `MUL RD, RS1, RS2`
- **Semantics**:
  `R[RD] := canonical(R[RS1] × R[RS2])`
- **Faults**: None

#### DIV

- **Form**: `DIV RD, RS1, RS2`
- **Semantics**:
  If `R[RS2] = 0` → fault.
  Else: `R[RD] := canonical(trunc_div(R[RS1], R[RS2]))`
- **Faults**: Division-by-zero fault.

#### MOD

- **Form**: `MOD RD, RS1, RS2`
- **Semantics**:
  If `R[RS2] = 0` → fault.
  Else: `R[RD] := canonical(R[RS1] mod R[RS2])`
- **Faults**: Division-by-zero fault.

#### NEG

- **Form**: `NEG RD, RS`
- **Semantics**:
  `R[RD] := canonical(−R[RS])`
- **Faults**: None

#### INC / DEC

- **Form**: `INC RD` / `DEC RD`
- **Semantics**:
  `INC`: `R[RD] := canonical(R[RD] + 1)`
  `DEC`: `R[RD] := canonical(R[RD] − 1)`
- **Faults**: None

#### FADD / FSUB / FMUL / FDIV

- **Form**: `FADD RD, RS1, RS2` (and analogous `FSUB`, `FMUL`, `FDIV`)
- **Semantics**:
  - Resolve `R[RS1]` and `R[RS2]` as handles into the float pool.
  - Apply canonical `T81Float` arithmetic (Section 2.3 of Data Types).
  - Write the resulting canonical float back as a handle in `R[RD]`. The VM MAY
    reuse an existing equal value but MUST do so deterministically.
- **Faults**:
  - Invalid handle → `IllegalInstruction`.
  - Division by zero (`FDIV` with canonical zero divisor) → `DivideByZero`.

#### FRACADD / FRACSUB / FRACMUL / FRACDIV

- **Form**: `FRACADD RD, RS1, RS2` (and analogous `FRACSUB`, `FRACMUL`,
  `FRACDIV`)
- **Semantics**:
  - Resolve `R[RS1]` and `R[RS2]` as handles into the fraction pool.
  - Apply canonical `T81Fraction` operations (`add`, `sub`, `mul`, `div`) per
    Data Types Section 2.4, including normalization of numerator/denominator.
  - Store the canonical result as a pool handle in `R[RD]` (with deterministic
    deduplication if implemented).
- **Faults**:
  - Invalid handle → `IllegalInstruction`.
  - Division by zero (`FRACDIV` with zero numerator in divisor or canonical zero
    denominator) → `DivideByZero`.

#### CHKSHAPE

- **Form**: `CHKSHAPE RD, RS1, RS2`
- **Semantics**:
  - `R[RS1]` MUST contain a tensor handle, `R[RS2]` a canonical shape handle.
  - If the tensor’s shape matches the descriptor, write `1t81` to `R[RD]`,
    otherwise `0t81`.
- **Faults**:
  - Invalid handles or mismatched register tags → `IllegalInstruction`.

#### MAKE_OPTION_SOME

- **Form**: `MAKE_OPTION_SOME RD, RS`
- **Semantics**:
  Capture the canonical `Option[T]::Some` variant by reading the value stored in
  `R[RS]` (respecting its `ValueTag`), interning it in the VM’s option pool, and
  writing the resulting 1-based handle into `R[RD]`. The VM MUST deduplicate
  identical `(tag, payload)` pairs so equality compares semantic contents.
- **Faults**: Invalid register or payload tag → `IllegalInstruction`.

#### MAKE_OPTION_NONE

- **Form**: `MAKE_OPTION_NONE RD`
- **Semantics**:
  Write the canonical handle representing `Option[T]::None` into `R[RD]`. A VM
  MUST reuse a single handle for all `None` occurrences.
- **Faults**: Invalid destination register → `IllegalInstruction`.

#### MAKE_RESULT_OK

- **Form**: `MAKE_RESULT_OK RD, RS`
- **Semantics**:
  Record the `Result[T, E]::Ok` payload stored in `R[RS]`, intern it in the
  result pool, and write the deduplicated handle into `R[RD]`.
- **Faults**: Invalid register or payload tag → `IllegalInstruction`.

#### MAKE_RESULT_ERR

- **Form**: `MAKE_RESULT_ERR RD, RS`
- **Semantics**:
  Same as `MAKE_RESULT_OK` but marks the handle as the error variant.
- **Faults**: Invalid register or payload tag → `IllegalInstruction`.

#### OPTION_IS_SOME

- **Form**: `OPTION_IS_SOME RD, RS`
- **Semantics**:
  Read the option handle stored in `R[RS]`, write `1t81` to `R[RD]` when it
  represents `Option::Some`, otherwise write `0t81`. Flags follow the canonical
  integer written to `R[RD]`.
- **Faults**: Missing/invalid destination register or if `R[RS]` is not tagged as
  an option handle.

#### OPTION_UNWRAP

- **Form**: `OPTION_UNWRAP RD, RS`
- **Semantics**:
  Copy the payload of an `Option::Some` handle from `R[RS]` into `R[RD]`,
  preserving the payload’s canonical tag (int, float handle, fraction handle,
  symbol handle, etc.). Attempting to unwrap `None` MUST fault.
- **Faults**: Invalid registers, non-option source, or unwrapping a `None`
  handle → `IllegalInstruction`.

#### RESULT_IS_OK

- **Form**: `RESULT_IS_OK RD, RS`
- **Semantics**:
  Inspect the result handle stored in `R[RS]` and write `1t81` to `R[RD]` when
  it is `Result::Ok`, otherwise `0t81`. Flags mirror the integer result.
- **Faults**: Invalid registers or non-result source handle.

#### RESULT_UNWRAP_OK

- **Form**: `RESULT_UNWRAP_OK RD, RS`
- **Semantics**:
  Copy the payload of a `Result::Ok` handle from `R[RS]` into `R[RD]` while
  preserving its payload tag. The VM MUST fault if `R[RS]` refers to an error
  variant.
- **Faults**: Invalid registers, non-result source, or attempting to unwrap an
  `Err` handle.

#### RESULT_UNWRAP_ERR

- **Form**: `RESULT_UNWRAP_ERR RD, RS`
- **Semantics**:
  Same as `RESULT_UNWRAP_OK`, but unwraps the `Err` payload. Fault if the handle
  represents `Ok`.
- **Faults**: Invalid registers, non-result source, or attempting to unwrap an
  `Ok` handle.

______________________________________________________________________

### 5.3 Ternary Logic Instructions

Logical operations operate over Trits or T81BigInt viewed as vectors of trits.

#### TNOT

- **Form**: `TNOT RD, RS`
- **Semantics**:
  Bitwise balanced-ternary negation:
  `T̄ ↔ T1`, `T0 → T0`
- **Faults**: None

#### TAND / TOR / TXOR

- **Form**: `TAND RD, RS1, RS2`
- **Semantics**:
  Applied tritwise using ternary min/max/XOR definitions.
- **Faults**: None; lengths must match; if mismatched, VM MUST canonicalize via padding rule specified in Data Types (implementation MUST be deterministic).

______________________________________________________________________

### 5.4 Comparison and Flags

#### CMP

- **Form**: `CMP RS1, RS2`

- **Semantics**:
  Compare `R[RS1]` and `R[RS2]` as canonical T81BigInt, T81Float, T81Fraction,
  Symbol, Option, or Result handles (types MUST match). Symbol comparisons MUST
  dereference both handles into the immutable symbol pool and compare the
  canonical symbol text lexicographically. Option comparisons order `None < Some` and recursively compare payloads. Result comparisons order `Err < Ok` and
  likewise compare payloads recursively.

  - `R[RS1] < R[RS2]` → FLAGS := {NEG = 1, ZERO = 0, POS = 0}
  - `R[RS1] = R[RS2]` → FLAGS := {NEG = 0, ZERO = 1, POS = 0}
  - `R[RS1] > R[RS2]` → FLAGS := {NEG = 0, ZERO = 0, POS = 1}

- **Faults**: Type mismatch fault if types are incompatible or if a symbol handle is zero/invalid.

#### SETF

- **Form**: `SETF RD`
- **Semantics**:
  `R[RD] := FLAGS` encoded as a canonical small integer. In this revision the VM
  MUST encode `NEG` as `-1`, `ZERO` as `0`, and `POS` as `+1`.
- **Faults**: None.

______________________________________________________________________

### 5.5 Move and Data Movement

#### MOV

- **Form**: `MOV RD, RS`
- **Semantics**:
  `R[RD] := R[RS]` (no modification, purely copy).
- **Faults**: None.

#### LOADI

- **Form**: `LOADI RD, IMM`
- **Semantics**:
  `R[RD] := canonical(IMM)` where IMM is a base-81 literal.
- **Faults**: Decode fault if IMM cannot represent a canonical value.

______________________________________________________________________

### 5.6 Memory Access

#### LOAD

- **Form**: `LOAD RD, [RS]`
- **Semantics**:
  Interpret `R[RS]` as an address;
  `R[RD] := MEM[R[RS]]` (canonicalized on load).
- **Faults**: Out-of-bounds, segment violation.

#### STORE

- **Form**: `STORE [RS], RD`
- **Semantics**:
  `MEM[R[RS]] := R[RD]`
- **Faults**: Out-of-bounds, segment violation, write to CODE or forbidden META region.

#### PUSH / POP

- **Form**: `PUSH RS` / `POP RD`
- **Semantics**:
  `PUSH`: decrement SP, store `R[RS]` at `MEM[SP]`
  `POP`: load from `MEM[SP]` into `R[RD]`, increment SP
- **Faults**: Stack overflow/underflow.

______________________________________________________________________

### 5.7 Control Flow

All jumps are **deterministic** and MUST NOT permit self-modifying code.

#### JMP

- **Form**: `JMP RS`
- **Semantics**:
  `PC := R[RS]`
- **Faults**: Jump outside CODE segment → fault.

#### JZ / JNZ / JN / JP

- **Form**: `JZ RS` (jump if ZERO), `JNZ RS` (jump if not ZERO)
- **Semantics**:
  Branch based on FLAGS; if condition satisfied, `PC := R[RS]`, else `PC := PC + 1`.
- **Faults**: Same as `JMP`.

#### CALL / RET

- **Form**: `CALL RS` / `RET`
- **Semantics**:
  `CALL`: push `PC+1`, then `PC := R[RS]`
  `RET`: pop target into `PC`
- **Faults**: Stack underflow / invalid return address.

______________________________________________________________________

### 5.8 Tensor and Matrix Operations

Act on canonical tensor/matrix representations as defined in [Data Types](t81-data-types.md).

#### TVECADD

- **Form**: `TVECADD RD, RS1, RS2`
- **Semantics**:
  Elementwise addition of equal-length vectors.
- **Faults**: Shape mismatch.

#### TMATMUL

- **Form**: `TMATMUL RD, RS1, RS2`
- **Semantics**:
  Matrix multiplication; result canonicalized.
- **Faults**: Incompatible dimensions.

#### TTENDOT

- **Form**: `TTENDOT RD, RS1, RS2`
- **Semantics**:
  Tensor contraction along specified axes (encoded in MODE).
- **Faults**: Invalid axis, shape mismatch.

______________________________________________________________________

### 5.9 Conversion Instructions

Conversions MUST be explicit and deterministic.

#### I2F / F2I

- **Form**: `I2F RD, RS` / `F2I RD, RS`
- **Semantics**:
  - `I2F`: convert canonical integer `R[RS]` to a canonical `T81Float`, append
    (or deterministically reuse) it in the float pool, and return the handle in
    `R[RD]`.
  - `F2I`: resolve the handle in `R[RS]`, truncate toward zero per `T81Float`
    rules, and write the canonical integer into `R[RD]`.
- **Faults**: Overflow or non-representable values → deterministic error code or fault depending on ABI.

#### I2FRAC / FRAC2I

- **Form**: `I2FRAC RD, RS` / `FRAC2I RD, RS`
- **Semantics**:
  - `I2FRAC`: convert canonical integer `R[RS]` to a normalized `T81Fraction`
    handle.
  - `FRAC2I`: resolve the handle in `R[RS]`, ensuring the denominator is `1`,
    and write the canonical integer into `R[RD]`.
- **Faults**: Invalid fraction (e.g., denominator zero).

______________________________________________________________________

### 5.10 Axion-Privileged Instructions

These instructions are only valid in Axion-supervised or privileged contexts.

#### AXREAD

- **Form**: `AXREAD RD, IMM_TAG`
- **Semantics**:
  Read Axion metadata identified by `IMM_TAG` into `R[RD]`.
- **Faults**: Unauthorized tag access.

#### AXSET

- **Form**: `AXSET IMM_TAG, RS`
- **Semantics**:
  Request Axion to update a metadata or policy slot using `R[RS]`.
- **Faults**: Unauthorized mutation, inconsistent request.

#### AXVERIFY

- **Form**: `AXVERIFY RS`
- **Semantics**:
  Ask Axion to verify safety/ethics of the current state; result written into `R[RS]` as a canonical code.
- **Faults**: None; failure is encoded, not thrown.

Any attempt to execute Axion instructions from non-privileged context MUST be treated as a **Security Fault**.

______________________________________________________________________

### 5.11 System and Miscellaneous

#### NOP

- **Form**: `NOP`
- **Semantics**: No state change except potential Axion trace entry.
- **Faults**: None.

#### HALT

- **Form**: `HALT`
- **Semantics**:
  Signal normal termination to T81VM; Axion records final state.
- **Faults**: None.

#### TRAP

- **Form**: `TRAP IMM_TAG`
- **Semantics**:
  Enter a well-defined trap handler; VM/Axion decide behavior (debug, inspect, terminate).
- **Faults**: None; trap semantics MUST be deterministic.

______________________________________________________________________

## 6. Fault Semantics

All faults are **deterministic** and **Axion-visible**.
TISC MUST NOT allow silent corruption or undefined behavior.

### 6.1 Fault Categories

- **Decode Fault**
  Invalid opcode, mode, or encoding.

- **Type Fault**
  Incompatible operand types (e.g., mixing T81Float and T81Fraction without explicit conversion).

- **Bounds Fault**
  Out-of-range memory access, invalid tensor indices.

- **Stack Fault**
  Stack underflow/overflow.

- **Division Fault**
  Division/modulo by zero.

- **Security Fault**
  Unauthorized Axion/system operation.

- **Shape Fault**
  Incompatible shapes for tensor/matrix operations.

### 6.2 Fault Handling

On any fault:

1. T81VM MUST stop normal instruction execution.

1. Fault record MUST be stored in Axion-visible metadata.

1. Axion MAY decide to:

   - terminate
   - roll back
   - escalate to cognitive tiers (in supervised analytic modes)

No fault may leave the machine in an unspecified or partially-updated state; either:

- the state transition is fully applied, or
- no part of it is applied and a fault is raised.

______________________________________________________________________

# Cross-References

## Overview

- **Layer Architecture** → [`t81-overview.md`](t81-overview.md#2-architectural-layers)
- **Determinism Requirements** → [`t81-overview.md`](t81-overview.md#3-determinism-requirements)

## Data Types

- **Operand Types** → [`t81-data-types.md`](t81-data-types.md#2-primitive-types)
- **Tensor/Matrix Structures for TISC Ops** → [`t81-data-types.md`](t81-data-types.md#3-composite-types)
- **Normalization Behavior** → [`t81-data-types.md`](t81-data-types.md#5-canonicalization-rules-critical-normative-section)

## T81 Virtual Machine

- **VM Execution Model for Instructions** → [`t81vm-spec.md`](t81vm-spec.md#1-execution-modes)
- **Deterministic Scheduling & Side-Effects** → [`t81vm-spec.md`](t81vm-spec.md#3-concurrency-model)
- **Fault Handling Pathways** → [`t81vm-spec.md`](t81vm-spec.md#2-determinism-constraints)

## T81Lang

- **Code Generation Targets for TISC** → [`t81lang-spec.md`](t81lang-spec.md#5-compilation-pipeline)
- **Type System Mapping to Operands** → [`t81lang-spec.md`](t81lang-spec.md#3-type-system)
- **Purity and Effect Constraints** → [`t81lang-spec.md`](t81lang-spec.md#1-language-properties)

## Axion Kernel

- **Privileged Instruction Handling** → [`axion-kernel.md`](axion-kernel.md#1-responsibilities)
- **Verification of Instruction Effects** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)
- **Recursion & Safety Boundaries for Jumps** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)

## Cognitive Tiers

- **Higher-Level Reasoning Built on TISC State Transitions** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)
- **Ternary Logic Foundations for Symbolic Recursion** → [`cognitive-tiers.md`](cognitive-tiers.md#2-constraints)

______________________________________________________________________
