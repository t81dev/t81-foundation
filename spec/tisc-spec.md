______________________________________________________________________

title: T81 Foundation Specification — TISC
nav:

- [Overview](t81-overview.md)
- [Data Types](t81-data-types.md)
- [TISC Specification](tisc-spec.md)
- [T81 Virtual Machine](t81vm-spec.md)
- [T81Lang](t81lang-spec.md)
- [Axion Governance Kernel](axion-kernel.md)
- [Cognitive Tiers](cognitive-tiers.md)

______________________________________________________________________

[← Back to Spec Index](index.md)

# TISC — Ternary Instruction Set Computer

Version v1.9.0-Stable

Status: Stable\
Last Revised: 2026-03-01\
Applies to: T81VM, T81Lang, Axion, Cognitive Tiers

> **Freeze Exception — 2026-03-01**\
> Scope: Additive corrections only — no existing opcode semantics changed.\
> Authorized by: @t81dev\
> Rationale: §5.14 adds seven bitwise opcodes (BITAND, BITOR, BITXOR, BITNOT,
> BITSHL, BITSHR, BITUSHR) that are implemented in the VM and emitted by the
> T81Lang compiler's lowering pass but were absent from the normative spec,
> causing external readers to miss valid instruction-set surface.

This document defines the **Ternary Instruction Set Computer (TISC)** for the T81 Ecosystem.\
It is **normative** for all instruction encoding, execution semantics, and VM integration.

______________________________________________________________________

## 0. Binary Host Execution Boundary

T81 is a **ternary semantic architecture executed on binary hardware**. This is an intentional architectural layer. TISC operates semantically as a ternary computer, yet the implementation relies on 2-bit packed trits and SWAR vectorization to interface with the binary substrate of host systems. This approach achieves maximum performance on x86 and ARM while preserving bit-exact, deterministic ternary state.

## 0.5. Design Principles

TISC MUST satisfy:

1. **Deterministic Execution**\
   Each instruction has a single, total, unambiguous semantic definition.

2. **Base-81 / Balanced Ternary Semantics**\
   Arithmetic and logic are ternary-native; binary implementation shortcuts MUST NOT change observable behavior.

3. **Zero Undefined Behavior**\
   Every operand combination either:

   - produces a canonical result, or
   - yields a deterministic fault.

4. **Axion Visibility**\
   All state changes, faults, and privileged operations MUST be observable by the Axion kernel.

5. **Layer Compatibility**\
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

- `R` — Register file (mandatory architectural window `R0–R80`, with optional implementation-defined extension banks)
- `PC` — Program counter
- `SP` — Stack pointer
- `FLAGS` — Condition flags and ternary status
- `MEM` — Memory segments (code, stack, heap, tensor, meta)
- `META` — Axion-visible metadata (trace, tags, fault history)

The T81VM is responsible for hosting and evolving this state; TISC defines how each instruction transforms it.

A simple TISC program to add two numbers might look like this:

```tisc
; Load the integer value 5 into register R0
LOADI R0, 5

; Load the integer value 10 into register R1
LOADI R1, 10

; Add the values in R0 and R1, store the result in R2
ADD R2, R0, R1

; Halt the machine
HALT
```

### Complex Arithmetic & Comparison Example

```tisc
; Floating point arithmetic
LOADI R0, 1.5           ; Load float handle
LOADI R1, 2.5           ; Load another float handle
FADD R2, R0, R1         ; R2 = 4.0 handle

; Comparison and conditional jump
LOADI R3, 10
CMP R2, R3              ; Compare 4.0 with 10
JN label_less           ; Jump if negative (4.0 < 10)
HALT

label_less:
LOADI R4, 1
HALT
```

### Tensor Operation Example

```tisc
; Assuming handles are pre-loaded or created via language frontend
LOADI R1, 1             ; Tensor handle 1
LOADI R2, 2             ; Tensor handle 2
TVECADD R0, R1, R2      ; Elementwise vector add
HALT
```

### Stack Management & Function Call Example

```tisc
; main entry
LOADI R0, 42            ; Argument for function
PUSH R0                 ; Push argument to stack
LOADI R1, label_func    ; Function address
CALL R1                 ; Call function
POP R2                  ; Pop result from stack (assuming ABI uses stack for return)
HALT

label_func:
POP R3                  ; Pop argument from stack
LOADI R4, 1
ADD R5, R3, R4          ; result = arg + 1
PUSH R5                 ; Push result to stack
RET                     ; Return to caller
```

### Axion Policy & Metadata Example

```tisc
; Read current instruction count from Axion
; IMM_TAG 0: Instruction Count (hypothetical tag)
AXREAD R0, 0

; Set a new stack limit for current context
; IMM_TAG 1: Max Stack Depth (hypothetical tag)
LOADI R1, 1024
AXSET 1, R1

; Verify current state against ethical constraints
AXVERIFY R2
JZ R2, label_unsafe     ; If result is zero (fail), jump to handler
HALT

label_unsafe:
TRAP 1                  ; Trigger security trap
HALT
```

______________________________________________________________________

## 2. Register File

TISC defines a mandatory **architectural register window of 81 registers**,
indexed `R0` through `R80`.

### 2.1 General-Purpose Registers

- `R0`: Hardwired zero.
- `R1–R74`: General-purpose registers. Specific caller- and callee-save conventions are defined at the ABI level.

### 2.2 Axion System Window (R75–R80)

Registers `R75` through `R80` form the fixed **Axion System Window**. These registers are managed by the VM and Axion kernel to provide high-visibility architectural state.

| Register | Purpose |
|----------|---------------------------------|
| R75 | Global Tick |
| R76 | Lineage Root Hash |
| R77 | Current Entropy Signature |
| R78 | Active Constitutional Mask |
| R79 | Recursion Depth Counter |
| R80 | Axion Seal / Capability Word |

Any attempt by unprivileged instructions to directly modify registers in the
`R75–R80` range MUST be ignored or trigger a deterministic **Security Fault**,
depending on the active Axion policy.

### 2.3 Implementation-Defined Extended Register Banks

Implementations MAY expose additional internal registers beyond `R80`
(for example, implementation-local scratch/context banks), but they are
**non-portable** and outside the mandatory architectural contract.

Requirements:

1. The semantics of `R0–R80` MUST remain unchanged and portable.
2. Programs that target the canonical deterministic profile MUST NOT rely on
   registers outside `R0–R80`.
3. If an implementation exposes extended registers, this MUST be documented as
   an implementation profile and MUST NOT silently alter the meaning of
   canonical opcodes over `R0–R80`.

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
2. Alignment MUST follow base-81 multiples for composite structures.
3. Out-of-bounds access MUST cause a deterministic fault (no undefined reads/writes).

______________________________________________________________________

## 4. Instruction Encoding

For the current canonical runtime/toolchain profile (v1.1), each TISC
instruction is encoded as a fixed-width **13-byte record**:

```text
[ OPCODE:u8 | A:i32 | B:i32 | C:i32 ]   // little-endian i32 operands
```

- `OPCODE` — instruction selector (`uint8_t`)
- `A`, `B`, `C` — signed 32-bit operands

The byte-level encoding above is the normative interchange representation used
by `encode/decode` in the current implementation profile.

Conceptual ternary field decompositions MAY still be used for architectural
reasoning, but they MUST map deterministically to the canonical 13-byte form.

Decoding requirements:

- Decoding MUST be deterministic and total.
- Any unrecognized opcode or invalid operand combination MUST map to a
  **Decode Fault**, not undefined behavior.

### 4.1 Native Ternary Transition Profile (Planned)

T81’s long-term direction includes native ternary instruction-word execution.
To preserve compatibility and auditability during that transition:

1. Any native ternary instruction layout MUST define a deterministic, lossless
   mapping to/from the canonical 13-byte interchange form.
2. Profile identifiers MUST make encoding mode explicit at artifact boundaries
   (compiler output, loaders, and replay tools).
3. ISA semantics for `R0–R80` and opcode behavior MUST remain unchanged across
   encoding profiles.

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
  - **Note**: `FDIV` currently relies on host `double` precision and may not be bit-exact across platforms.
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

______________________________________________________________________

### 5.2.1 Normative Arithmetic Semantics (RFC-0049)

This section is **normative**.  It encodes the canonical arithmetic law
defined by [RFC-0049](rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md)
into the TISC specification.  All arithmetic opcodes in §5.2 and all
implementation surfaces that serve as backends for those opcodes MUST
conform to the rules below.

#### Canonical Trit Domain

The canonical value domain is balanced ternary: digits in `{-1, 0, +1}`.
No implementation may expose a non-canonical intermediate trit state through
a public or governed arithmetic surface.

#### Primitive Semantics

| Operation | Canonical definition |
| --------- | -------------------- |
| **Negation** | Flip every trit: `Pos↔Neg`, `Zero` unchanged.  Followed by leading-zero normalization. |
| **Addition** | Digit-wise balanced-ternary summation with carry propagation.  Carry values are always in `{-1, 0, +1}`.  Sum in `{−3,−2}` → trit=sum+3, carry=−1; sum in `{2,3}` → trit=sum−3, carry=+1; otherwise carry=0. |
| **Subtraction** | `a − b ≡ a + (−b)`.  No independent borrow convention may be introduced. |
| **Multiplication** | Partial-product generation using canonical trit multiplication, accumulated via the addition/carry law above.  Backend groupings (Karatsuba, schoolbook, lookup) must not change place-value interpretation. |
| **Comparison** | Value-based.  Two canonical representations of the same numeric value MUST compare equal.  Lexical byte comparison is valid only when the normalized encoding is proven to preserve canonical numeric order. |

#### Carry Propagation Rule

The carry propagation law must be consistent across scalar, packed, SWAR,
and SIMD execution.  Any lookup-table or chunk-based carry normalization
MUST produce a result identical to the trit-by-trit carry law above.

#### Overflow Policies (Explicit Per Surface)

Each governed arithmetic surface adopts one of the following policies.
The chosen policy MUST NOT vary by backend.

| Surface | Overflow policy | Notes |
| ------- | --------------- | ----- |
| `T81BigInt` (ADD, SUB, MUL, NEG opcodes) | **Unbounded** — arbitrary precision; no overflow possible. | Multi-limb expansion maintains exact value. |
| `T81Int<N>` (fixed-width ternary integer) | **Exception trap** — `std::overflow_error` on out-of-range. | Caller must catch or use `T81BigInt`. |
| `T81Float<M,E>` (FADD, FSUB, FMUL, FDIV opcodes) | **Special-value saturation** — produces `Infinity` or `NaE` (Not-an-Entity); never silent UB. | `NaE` propagates like IEEE NaN. |
| Raw trit-vector decode (`decode_i64`) | **Saturation** — clamps to `INT64_MAX` / `INT64_MIN` on overflow. | Decode only; not a governed arithmetic operation. |
| DIV / MOD (integer division) | **Explicit fault** — `DivisionFault` when divisor is zero. | Truncation toward zero for signed operands. |

#### Forbidden Behaviors

The following are **unconditionally forbidden** on any arithmetic surface:

- Silent host-language signed-overflow UB.
- Backend-specific implicit carry or borrow rules not reducible to the canonical law.
- Comparison results that depend on internal byte encoding, padding, or stale trit storage.
- Overflow behavior that varies between scalar, SWAR, or SIMD backends.

#### Relation to Backend Equivalence

RFC-0042 defines *when* two backends are equivalent.  RFC-0049 defines
*what result* they must be equivalent to.  Backend equivalence for any
arithmetic surface therefore requires identical canonical value, identical
fault/trap result where applicable, and identical externally-governed trace
semantics where arithmetic events are observable.

#### Conformance

Conformance is verified by `t81_arithmetic_backend_equivalence_test`
(RFC-0049 §AC4), which checks the scalar trit oracle against the T81BigInt
multi-limb packed path for addition, subtraction, multiplication, negation,
comparison, carry propagation, and overflow policy.

______________________________________________________________________

### 5.2.2 Normative Packed-Trit Encoding (RFC-0044)

This section is **normative**.  It records the stable packed-trit encoding
contract defined by [RFC-0044](rfcs/RFC-0044-stable-packed-trit-vector-interface.md).
All execution backends, VM tensor helpers, SWAR kernels, and SIMD kernels that
operate on packed trit storage MUST conform to this layout.

#### Stable Surface

The governed stable type is `t81::ComputeTritVector` (header
`include/t81/packed_trit_vector.hpp`).  The implementation currently lives in
`t81::experimental` and is re-exported via alias; the governed public surface is
the `t81::` namespace alias.  Code MUST NOT depend directly on
`t81::experimental::ComputeTritVector` outside the implementation file.

#### Encoding Invariants

| Property | Value |
| -------- | ----- |
| **Bits per trit** | 2 |
| **Trits per byte** | 4 |
| **Bit ordering** | LSB-first within each byte (trit 0 occupies bits [1:0], trit 1 bits [3:2], etc.) |
| **Valid patterns** | `00` → 0 (Zero), `01` → +1 (Pos), `11` → −1 (Neg) |
| **Forbidden pattern** | `10` — no valid trit value; MUST be rejected on entry |
| **Byte count** | `⌈trit_count / 4⌉` — deterministic function of trit count |
| **Tail bits** | Unused bits in the last byte MUST be zero (zero-extended, not sign-extended) |

#### Validation Contract

- `ComputeTritVector::from_packed(bytes, trit_count)` MUST return a deterministic
  error result if any trit slot contains the forbidden `10` pattern, or if any
  unused trailing bit is non-zero.
- `ComputeTritVector::from_trits(trits)` constructs from canonical `{-1, 0, +1}`
  values and is always valid if the input values are in range.
- `ComputeTritVector::to_trits()` decodes packed bytes back to canonical trit
  values; it MUST return an error result if an invalid `10` pattern is encountered
  in stored bytes.
- All in-place operations MUST re-apply tail masking after mutation to maintain
  the zero-extension invariant.

#### Mutability Classification

| Method category | Example | Side effects |
| --------------- | ------- | ------------ |
| **Pure** | `t_not()`, `t_and()`, `t_or()`, `t_xor()` (const dispatchers) | Returns new `ComputeTritVector`; caller owns result |
| **In-place** | `t_not_inplace()`, `t_and_inplace()`, `t_or_inplace()` | Mutates self in-place; re-applies tail mask |
| **Raw kernel** | `kernel_not_swar()`, `kernel_and_avx2()`, etc. | Takes pre-validated raw byte pointers; callers MUST ensure canonical input |

Raw kernel entry points require prevalidated canonical data.  They do not
re-validate their inputs.  Callers producing raw bytes by any means other than
`from_trits` or `from_packed` are responsible for maintaining the encoding
invariants documented above.

#### Backend Interoperability

Every backend that reads or writes packed-trit storage (scalar, SWAR, SIMD, VM
tensor helpers, JIT lowering) MUST consume the same canonical `t81::ComputeTritVector`
layout.  Backend-specific reinterpretations of the 2-bit trit encoding are
forbidden on the deterministic surface.

#### Conformance (RFC-0044)

Conformance is verified by `test_swar` (`tests/cpp/test_swar.cpp`), specifically
`test_rfc0044_invalid_pattern_rejection()`, which asserts that:

- `from_packed` rejects the forbidden `10` bit pattern in any trit slot
- `from_packed` rejects non-zero tail padding
- `from_packed` accepts valid bytes and round-trips to the same trit sequence
- `to_trits()` always produces values in `{-1, 0, +1}`

______________________________________________________________________

### 5.2.3 Backend Equivalence Contract (RFC-0042)

This section is **normative**.  It records the backend equivalence rules defined
by [RFC-0042](rfcs/RFC-0042-deterministic-backend-equivalence-contract.md).
All execution backends (scalar, SWAR, SIMD, JIT, future heterogeneous) MUST
satisfy this contract before being permitted to substitute for the canonical oracle.

#### Canonical Oracle Table

Each governed operation family has exactly one canonical oracle.  All backends
in that family MUST produce identical results to that oracle at every observable
boundary.

| Operation family | Canonical oracle | Location |
| ---------------- | ---------------- | -------- |
| Tritwise NOT | `ComputeTritVector::t_not_ref()` — scalar unpack–negate–repack | `include/t81/experimental/packed_trit_vector.hpp` |
| Tritwise AND | `ComputeTritVector::t_and_ref()` — scalar trit-min | same |
| Tritwise OR | `ComputeTritVector::t_or_ref()` — scalar trit-max | same |
| Tritwise XOR | `ComputeTritVector::t_xor_ref()` — scalar bounded difference | same |
| Integer ADD / SUB / MUL / NEG / DIV / MOD | RFC-0049 arithmetic oracle — digit-wise balanced-ternary with canonical carry | `include/t81/ternary/arith.hpp`; `T81BigInt` |
| Float FADD / FSUB / FMUL / FDIV | `T81Float<M,E>` soft-math path (when `T81_DETERMINISTIC` defined) | `include/t81/types/T81Float.hpp` |

#### Backend Dispatch Rules (Tritwise)

Dispatch is compile-time and size-gated.  Thresholds are expressed in **bytes**
(1 byte = 4 trits at 2 bits/trit).

| Architecture | Operation | Threshold | Backend selected |
| ------------ | --------- | --------- | ---------------- |
| x86\_64 + AVX2 | NOT, AND, OR | ≥ 64 B (≥ 256 trits) | AVX2 kernel; SWAR tail for remainder |
| x86\_64 + AVX2 | XOR | any size | LUT-based (no AVX2 XOR kernel) |
| AArch64 + NEON | OR | ≥ 64 B | NEON kernel; SWAR tail for remainder |
| AArch64 + NEON | NOT, AND | any size | SWAR (NEON kernels disabled) |
| All other | NOT, AND, OR, XOR | any size | SWAR |

Dispatch decisions MUST NOT depend on elapsed time, benchmark history, or
non-governed host environment features.  Dispatch is determined entirely by
compile-time architecture flags and the byte length of the operand.

#### Fallback Order

If a higher-tier backend cannot satisfy the equivalence contract it MUST fall
back to the nearest lower verified backend, in this order:

```text
JIT / heterogeneous  →  SIMD  →  SWAR  →  scalar
```

Silent semantic drift is forbidden.  If no verified backend is available the
system MUST fail closed with a deterministic error.

#### JIT and Heterogeneous Acceleration Constraint

Any JIT-lowered or accelerator-dispatched form MUST satisfy:

- identical register-visible result
- identical fault class and timing at the observable instruction boundary
- identical Axion-visible audit meaning
- identical CanonHash-relevant trace contribution

The equivalence requirement is formalized in
`docs/developer-guide/internals/jit-equivalence-plan.md` and is governed by
RFC-0042.  JIT or accelerator work that cannot satisfy these requirements MUST
NOT be promoted to the verified surface.

#### Conformance (RFC-0042)

Backend equivalence is verified by:

- `t81_tritwise_backend_equivalence_test` — scalar vs SWAR vs AVX2/NEON for NOT/AND/OR/XOR across sizes 1–4097 trits
- `t81_arithmetic_backend_equivalence_test` — scalar trit oracle vs T81BigInt multi-limb path for ADD/SUB/MUL/NEG/comparison/carry/overflow

______________________________________________________________________

### 5.2.4 Vectorized Ternary Operations (RFC-0050)

This section is **normative**.  It records the semantic governance rules defined by
[RFC-0050](rfcs/RFC-0050-vectorized-ternary-operations-for-tisc.md).

#### Semantic, Not Hardware, Vectorization

Vector operations in TISC are **semantic operations over declared-width ternary lanes**.
They are not AVX-specific, NEON-specific, or otherwise backend-specific instructions.
The ISA contract names vector intent and vector semantics; backend selection is an
implementation detail governed by RFC-0042.

#### Canonical Lane Model

Every vectorized ternary operation is defined over:

| Property | Requirement |
| -------- | ----------- |
| **Lane count / width** | Declared per opcode or operand metadata; canonical, not inferred |
| **Lane ordering** | Architecture-independent; MUST NOT vary by endianness, register layout, or packed physical arrangement |
| **Element interpretation** | Canonical balanced-trit `{-1, 0, +1}` per lane; arithmetic lanes inherit RFC-0049 semantics |
| **Result shape** | Same width as operands unless the opcode explicitly defines a reduction |

#### Opcode Surface (Illustrative)

The following mnemonics name the governed semantic intent.  Final encoding and
availability per ISA revision are defined in the opcode table.  Mnemonic selection
MUST remain consistent with existing opcode naming conventions.

| Mnemonic | Operation | Canonical semantic authority |
| -------- | --------- | ---------------------------- |
| `TVNOT` | Lane-wise ternary NOT | Tritwise oracle (RFC-0042 §1) |
| `TVAND` | Lane-wise ternary AND | Same |
| `TVOR` | Lane-wise ternary OR | Same |
| `TVXOR` | Lane-wise ternary XOR | Same |
| `TVADD` | Lane-wise balanced-ternary ADD | RFC-0049 arithmetic oracle per lane |
| `TVSUB` | Lane-wise balanced-ternary SUB | Same |
| `TVMUL` | Lane-wise balanced-ternary MUL | Same |

#### Explicit vs Implicit Vectorization

| Form | When required | Governing rule |
| ---- | ------------- | -------------- |
| **Explicit vector opcodes** | Vector width, trace meaning, policy behavior, or fault behavior is externally relevant | This section + RFC-0042 + RFC-0043 |
| **Implicit backend vectorization** | Purely internal optimization with no governed semantic effect at the DCP boundary | RFC-0042 §4 (Allowed Backend Substitutions) + RFC-0047 §4 (Allowed Transformations) |

#### Fault and Validation Semantics

Vector operations MUST fail closed for:

- invalid width declaration
- incompatible operand shapes
- out-of-range lane metadata
- unsupported explicit vector width on a given runtime configuration

Allowed fallback: deterministic SWAR or scalar execution if the opcode contract permits it;
otherwise, a deterministic fault.  Fallback behavior MUST be explicit and **trace-stable**:
if a vector opcode falls back to scalar execution, the semantic trace remains the vector
opcode event, not a scalar event.

#### Trace and Policy Semantics

- Trace hashing MUST NOT depend on the physical backend used beneath the vector semantic opcode.
- Vector operations MUST produce stable semantic trace categories independent of whether AVX2, NEON, or SWAR handles the operation internally.
- Axion / policy hooks MUST reason about semantic vector operations, not backend intrinsic details.

#### Relation to SWAR and SIMD Backends

[RFC-0040](rfcs/RFC-0040-swar-formalization.md) and
[RFC-0041](rfcs/RFC-0041-simd-formalization.md) define stable **implementation-facing**
SWAR and SIMD surfaces.  They are companions at the implementation layer, not de facto
ISA definitions.

- SWAR / SIMD MAY implement explicit vector opcodes.
- SWAR / SIMD MAY also remain internal execution backends for scalar-intent operations under RFC-0042 dispatch rules.
- Explicit vector opcodes MUST NOT expose SWAR- or SIMD-specific artifacts at the DCP surface.

#### JIT and Lowering Interaction

JIT lowering of vector opcodes is governed by RFC-0047:

- Lowering may replace a vector opcode with backend-specific sequences **only** under RFC-0047 §4–§5 (allowed / forbidden transformations).
- State reconstruction on side-exit MUST cover the vector register file width (RFC-0047 §7).
- Policy boundaries inside vectorized loops MUST NOT be bypassed or merged away (RFC-0047 §8).

#### Promotion Gate

No explicit vector opcode family may be promoted to **Verified / DCP** scope unless:

1. arithmetic or tritwise semantics are defined independently of backend
2. lane ordering is canonical and architecture-independent
3. trace behavior is stable across backends
4. fallback / fault behavior is explicit
5. backend equivalence is validated through the RFC-0042 equivalence test matrix and the RFC-0043 conformance framework

#### Conformance (RFC-0050)

Equivalence testing for explicit vector opcode families MUST extend the RFC-0042
backend-equivalence matrix to include scalar vs SWAR, scalar vs SIMD, and SWAR vs SIMD
for the vector semantic form.  When JIT lowers vector opcodes, the matrix MUST also
include interpreter vs JIT per RFC-0047 §12.  No vector opcode family is DCP-eligible
until this matrix is satisfied.

______________________________________________________________________

### 5.2.5 Heterogeneous Acceleration Governance (RFC-0051)

This section is **normative**.  It records the governance boundary rules defined by
[RFC-0051](rfcs/RFC-0051-deterministic-heterogeneous-acceleration.md).

#### Default Classification: Governed non-DCP

Any heterogeneous accelerator execution path (GPU, Metal, CUDA, HIP, SYCL, Vulkan
compute, or equivalent) is classified as **governed non-DCP** by default.

No accelerator backend is automatically DCP-eligible, semantically trusted, or
interchangeable with verified CPU backends.  It must earn equivalence and promotion
explicitly under the RFC-0048 §6 promotion rules.

#### Allowed Accelerator Kernel Classes

An accelerator kernel class is eligible for governed execution only if all of the
following can be defined:

- canonical input serialization
- canonical kernel boundary
- canonical output interpretation
- explicit fault / fallback rules
- proof obligations satisfiable under RFC-0043

Eligible example classes:

- lane-local tritwise transforms (with RFC-0042 scalar-oracle equivalence)
- canonical packed-trit arithmetic kernels (inheriting RFC-0049 arithmetic oracle per lane)
- width-bounded tensor primitives with explicit shape contracts
- backend-stable reductions with an explicit canonical ordering model

**Not eligible by default:**

- vendor-tuned floating-point heuristics
- unordered atomic accumulation
- nondeterministic work-stealing kernels
- sampling or hardware-RNG-driven execution

#### Memory Transfer Rules

All host ↔ accelerator memory movement MUST be deterministic and explicitly modeled:

- canonical byte / trit serialization MUST be applied before transfer
- transfer boundaries MUST be visible to the governed execution model
- hidden mutation outside RFC-0045 visibility rules is forbidden
- device-visible memory MUST be deterministically initialized
- padding, alignment, and unused bits MUST be handled deterministically

#### Kernel Launch and Scheduling Semantics

Accelerator launches MUST obey RFC-0046 ordering rules:

- kernel launch order is semantically defined where externally relevant
- completion order MUST NOT alter canonical results
- queueing, batching, or stream selection MUST NOT change governed semantics
- fallback from asynchronous to synchronous execution MUST preserve trace and fault identity

#### Reduction and Cross-Workgroup Constraints

Reductions are the highest-risk accelerator surface.

Allowed only if:

- the reduction order is explicitly defined, or
- regrouping is proven equivalent under RFC-0049 arithmetic semantics and RFC-0046 ordering rules

Forbidden:

- backend-local unordered reduction accepted as "close enough"
- race-resolved accumulation whose result depends on scheduler timing

#### Fault, Fallback, and Availability

Every accelerator surface MUST define behavior for:

- unsupported device
- unavailable driver
- kernel-compilation failure
- timeout or synchronization failure

Allowed responses: deterministic fallback to a permitted CPU backend, or deterministic
hard fault.  Silent backend substitution that changes governed semantics is forbidden.
Best-effort execution with downgraded correctness is forbidden.

#### DCP Promotion Gate

An accelerator backend MAY become **Verified / DCP** only when all of the following
are satisfied:

1. backend equivalence proven against the scalar oracle per RFC-0042
2. memory movement audited against RFC-0045
3. scheduling audited against RFC-0046
4. conformance evidence exists across supported vendors / architectures per RFC-0043
5. public boundary docs updated per RFC-0048

Until all five conditions are met, the surface MUST remain governed non-DCP.

#### Vendor Neutrality

Vendor APIs (CUDA, Metal, HIP, SYCL, Vulkan compute) are implementation mechanisms
only.  No public deterministic claim may depend on vendor-specific semantic wording.
All semantic contracts MUST be stated above the vendor API layer.

#### Binding Cross-References

RFC-0042 (backend equivalence), RFC-0043 (conformance framework), RFC-0045 (memory
model), RFC-0046 (scheduling ordering), and RFC-0048 (surface classification) are all
binding constraints on any accelerator path.  An accelerator backend may not relax any
rule established by those RFCs.

______________________________________________________________________

### 5.2.6 Canonical Dataflow and State-Driven Execution (RFC-0052)

This section is **normative**.  It records the architectural model defined by
[RFC-0052](rfcs/RFC-0052-canonical-dataflow-and-state-driven-execution.md).

#### State-Driven Computation

Computation in T81 may be triggered only through:

- explicit program invocation
- canonical task-graph submission
- governed state transition on a **registered** dataflow surface
- CanonFS-backed state / materialization events where explicitly allowed

No implicit or hidden reactive behavior is permitted outside registered dataflow surfaces.

#### Canonical Dependency Graph

Dataflow execution is defined over a canonical dependency graph with:

| Property | Requirement |
| -------- | ----------- |
| **Node identity** | Deterministic; MUST NOT depend on container iteration order or host address |
| **Edge identity** | Deterministic; expresses explicit dependency type |
| **Ready-node ordering** | Canonical; obeys RFC-0046 ordering constraints |
| **Graph semantics** | MUST NOT depend on host hash order or scheduler timing |

#### Node Semantics

Every dataflow node MUST define:

- its input state dependencies
- its output state transitions
- its fault behavior
- whether it is pure, effect-bounded, or policy-mediated

Node execution MUST be reproducible from canonical input state plus canonical graph metadata.

#### CanonFS Participation

CanonFS participates in dataflow only through explicit governed surfaces.

Allowed:

- loading canonical artifacts as inputs
- materializing outputs to canonical storage
- policy-gated dependency resolution via canonical identifiers

Forbidden:

- implicit filesystem watch semantics
- host-local path ordering affecting graph semantics
- opportunistic external mutation bypassing canonical state rules

#### Ready-State and Activation Semantics

A node becomes ready only when all of the following are true:

1. all required dependencies are satisfied
2. all required policy gates are satisfied
3. its activation condition evaluates true under canonical state

Ready-node selection MUST obey RFC-0046 ordering constraints.

#### Propagation Semantics

When a node commits output state:

- downstream readiness is recomputed deterministically
- propagation order is canonical
- faulted outputs MUST NOT silently activate downstream nodes
- retry or requeue behavior MUST be explicit and deterministic

#### Fault Propagation

Faults in dataflow execution MUST define:

- whether the fault blocks downstream activation
- whether fallback state exists
- whether retry is permitted
- whether the graph enters a terminal or recoverable condition

Fault propagation is deterministic and auditable.  A fault MUST be visible through the
standard observability surfaces (node activation records, canonical summary state).

#### DPE as a Dataflow Realization

RFC-DPE defines concrete deterministic parallel execution mechanics.  RFC-0052 defines
the architectural model above them:

- DPE epochs may serve as one realization of dataflow scheduling
- dataflow semantics do not replace DPE; they organize when and why DPE runs work
- any DPE optimization remains subordinate to canonical dependency and commit semantics

#### DCP Boundary Rule

Dataflow surfaces are **governed non-DCP** by default.

Rules:

- any DCP claim for a dataflow surface requires conformance, ordering, memory, and
  propagation proof under RFC-0043, RFC-0045, RFC-0046, and RFC-0048 respectively
- local service orchestration and future distributed propagation MUST NOT inherit DCP
  claims automatically

______________________________________________________________________

### 5.2.7 Distributed Deterministic Execution Protocol (RFC-0053)

This section is **normative**.  It records the constitutional constraints defined by
[RFC-0053](rfcs/RFC-0053-distributed-deterministic-execution-protocol.md).

#### Distributed Execution Is One Canonical Computation

A distributed T81 execution MUST be interpreted as a single canonical computation with
multiple execution participants.  It is NOT loosely coordinated local computations.

Consequences:

- state identity is global to the computation
- commit order is globally meaningful
- node-local scheduling MUST NOT redefine semantic order

#### Global State and Commit Identity

Distributed execution requires:

- canonical state identifiers (MUST NOT depend on local paths, local clocks, or ephemeral addresses)
- canonical epoch or commit identifiers
- canonical input artifact identifiers
- explicit versioning or generation semantics

#### Global Ordering Rule

Distributed execution MUST define a canonical commit order that all nodes can derive or verify.

| Ordering property | Requirement |
| ----------------- | ----------- |
| Input order | Canonical |
| Dependency order | Canonical |
| Conflict resolution order | Canonical |
| Network arrival order | **MUST NOT define semantics** |

#### Conflict Resolution

Conflicts between distributed updates MUST be resolved by explicit deterministic rules.

Allowed: canonical commit order, explicit tie-break keys, deterministic winner selection
from canonical metadata, fail-closed conflict rejection.

Forbidden: "first packet wins," host clock or wall-clock precedence, scheduler timing
as a semantic tiebreak.

#### Replay and Evidence Requirements

Every distributed execution claiming deterministic status MUST produce:

- canonical input set
- canonical graph / state metadata
- canonical commit ledger
- node participation records
- deterministic result hash or equivalent replay summary

These artifacts MUST fit within the RFC-0043 conformance model.

#### Fault and Partition Behavior

Distributed execution MUST define deterministic behavior for node failure, message delay,
message duplication, partition / quorum loss, and retry / rejoin.

Allowed responses: deterministic abort; deterministic rollback to last canonical commit;
deterministic suspension until conditions are met.

Forbidden: speculative continuation with ambiguous semantics; silent local completion
with later reconciliation changing results.

#### Policy and Audit Participation

- Policy-visible operations MUST remain policy-visible when distributed.
- Node-local shortcuts MUST NOT bypass Axion or other governed policy checks.
- Audit records MUST preserve semantic, not transport-only, meaning.

#### DCP Boundary Rule (RFC-0053)

Distributed execution is **experimental / non-DCP by default**.

Promotion requirements are stricter than local execution:

- state synchronization adds new divergence risk
- network transport adds new timing and fault surfaces
- replay and commit-ledger proof obligations are larger

No distributed surface may be treated as DCP merely because its local executor is
DCP-verified.  Explicit promotion under RFC-0043 and RFC-0048 promotion rules is
required, including distributed-specific conformance evidence.

#### Relation to Companion RFCs

- RFC-0052: canonical dataflow / state model that distributed execution inherits
- RFC-DPE: deterministic local parallel execution mechanisms that may serve as building blocks
- RFC-0046: ordering model that distributed extensions MUST NOT relax

______________________________________________________________________

### 5.2.8 Native Ternary Hardware Target and Interop Contract (RFC-0055)

This section is **normative**.  It records the governance contract defined by
[RFC-0055](rfcs/RFC-0055-native-ternary-hardware-target-and-interop-contract.md).

#### Hardware Target Classes

T81 recognizes two distinct hardware target classes:

| Class | Description | Governing RFC |
| ----- | ----------- | ------------- |
| **General-purpose native ternary target** | Platform capable of hosting the T81 runtime, memory model, interrupt model, and governed OS-facing surfaces | RFC-0055 (this section) |
| **Narrow ternary accelerator target** | Executes only selected kernels or tensor operations beneath an existing governed runtime | RFC-0051 |

RFC-0055 governs only the general-purpose class.

#### TISC Remains the Semantic Authority

For any native ternary hardware target, the semantic authority remains TISC instruction
semantics, T81VM observable behavior, Axion policy and audit meaning, and canonical data
and trace contracts.

Allowed:

- direct TISC-on-hardware execution
- verified lowering from TISC into a hardware-native ISA (must satisfy RFC-0042 + RFC-0047)

Forbidden:

- a hardware target that changes user-visible semantics merely because the hardware has
  a different native instruction vocabulary

#### Hardware Integration Modes

| Mode | Description | Prerequisite |
| ---- | ----------- | ------------ |
| **Direct semantic target** | Hardware executes a TISC-faithful implementation directly | Preferred long-term mode |
| **Verified lowering target** | Lower-level ISA; verified lowering preserves TISC semantics | Must satisfy RFC-0042 + RFC-0047 |
| **Hosted compatibility mode** | Hardware behavior surfaced through guest/host HAL | Transitional only; MUST NOT be described as native T81 hardware execution |

#### Hardware Interop Layer Requirements

Every general-purpose native ternary hardware target MUST define an explicit interop
layer above the device ISA that covers:

- boot handoff contract
- memory discovery and mapping contract
- interrupt and trap translation
- device I/O exposure rules
- fault classification and fallback rules
- canonical serialization boundaries for host/hardware exchange

RFC-00B0 HAL is the current architectural starting point but is not itself sufficient
for real hardware promotion.

#### Boot, Memory, Interrupt, and Fault Obligations

- Boot: entry format, privilege levels, reset and trap entry, kernel/user boundary,
  ethics-first boot, and Axion-first enforcement points MUST be defined.
- Memory: address model, ternary page granularity, canonical endianness and packing,
  alignment, DMA rules (per RFC-00B1 direction for MMU).
- Interrupts: interrupt classes MUST map to canonical T81/Axion-visible meaning; device
  completion timing MUST NOT alter deterministic observation boundaries.
- Faults: fault classification and fallback rules MUST be deterministic.

#### Trace, Audit, and CanonFS Preservation

A native ternary hardware target is not eligible for governed promotion unless it preserves:

- canonical trace meaning
- Axion audit meaning
- CanonFS-visible object identity semantics
- deterministic error/fault classification at the observation boundary

Hardware performance counters are diagnostics only and MUST remain outside the DCP surface.

#### External ISA Compatibility Profile

If T81 targets an external ternary ISA, a compatibility profile MUST state:

- target ISA name and revision
- lowering boundary from TISC into the target ISA
- unsupported TISC features (if any) with explicit fault/fallback behavior
- conformance corpus proving semantic equivalence on the supported subset

Silent subset execution is forbidden.

#### Hardware Promotion Gate

All native ternary hardware targets are **experimental by default**.

Governed non-DCP promotion requires: documented interop layer; executable boot/memory/
interrupt/fault contracts; semantic equivalence proof on a bounded corpus; preserved Axion
and trace semantics; explicit listing of supported vs unsupported hardware behavior.

DCP promotion additionally requires RFC-0042, RFC-0043, RFC-0045, RFC-0046, and RFC-0048
obligations met specifically for the hardware target matrix.

______________________________________________________________________

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

> **Conformance programs:** [`spec/conformance/tisc/arithmetic-determinism.t81`](conformance/tisc/arithmetic-determinism.t81) · [`division-truncation.t81`](conformance/tisc/division-truncation.t81) · [`fraction-normalization.t81`](conformance/tisc/fraction-normalization.t81)

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

> **Conformance program:** [`spec/conformance/tisc/ternary-logic-canonical.t81`](conformance/tisc/ternary-logic-canonical.t81)

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
  `R[RD] := FLAGS` encoded as a canonical small integer. The VM MUST encode `NEG`
  as `-1`, `ZERO` as `0`, and `POS` as `+1`.
- **Faults**: None.

> **Conformance program:** [`spec/conformance/tisc/comparison-total-order.t81`](conformance/tisc/comparison-total-order.t81)

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

> **Conformance program:** [`spec/conformance/tisc/bounds-fault-contract.t81`](conformance/tisc/bounds-fault-contract.t81)

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
- **See also**: `AGENT_INVOKE` (§5.16) — Axion-audited variant of `CALL` for
  T81Lang `agent` behavior invocations.

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

> **Conformance program:** [`spec/conformance/tisc/conversion-determinism.t81`](conformance/tisc/conversion-determinism.t81)

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

**Tier restriction (cross-reference):** `AXREAD` and `AXSET` are restricted to
Tier 2 and above per [`cognitive-tiers.md §1`](cognitive-tiers.md#tier-1--pure-deterministic-computation).
`AXVERIFY` is the only Axion privileged instruction permitted in Tier 1.

> **Conformance program:** [`spec/conformance/tisc/tier-restriction.t81`](conformance/tisc/tier-restriction.t81)

______________________________________________________________________

### 5.11 Trigonometric Instructions

Trigonometric operations on canonical `T81Float` values.

#### FSIN / FCOS / FTAN

- **Form**: `FSIN RD, RS` / `FCOS RD, RS` / `FTAN RD, RS`
- **Semantics**:
  - Resolve `R[RS]` as a handle to a `T81Float`.
  - Compute the sine, cosine, or tangent of the value (interpreted as radians).
  - Store the result as a canonical float handle in `R[RD]`.
  - **Note**: These operations currently rely on host `double` precision (`std::sin`, `std::cos`, etc.) and may not be bit-exact across platforms.
- **Faults**:
  - Invalid handle -> `IllegalInstruction`.
  - `FTAN` with asymptotic input -> VM defined behavior (large value or fault).

______________________________________________________________________

### 5.12 System and Miscellaneous

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

### 5.13 Governed Memory Access

Instructions that interact with content-addressed storage (CanonFS) under Axion governance.

#### TLOADHASH

- **Form**: `TLOADHASH RD, RS`
- **Semantics**:
  - `R[RS]` MUST contain a valid handle to a string symbol representing a `CanonHash`.
  - The Axion kernel MUST verify the hash against the active policy's `allowed-tensor-hashes` list.
  - If allowed, the VM retrieves the object from `CanonFS`, deserializes it into a canonical tensor, and stores the handle in `R[RD]`.
- **Faults**:
  - `DecodeFault`: Invalid `RS` handle or malformed object.
  - `SecurityFault`: Hash not allowed by Axion policy.
  - `BoundsFault`: Object not found in `CanonFS`.

______________________________________________________________________

### 5.14 Bitwise Integer Operations

These instructions perform standard binary bitwise operations on **T81BigInt**
operands. Operands are interpreted as two's-complement binary integers for
the purpose of bit-level manipulation; the result is stored as a canonical
`T81BigInt`.

> *Freeze Exception addition — 2026-03-01. These opcodes are emitted by the
> T81Lang compiler lowering table (§5 of [`t81lang-spec.md`](t81lang-spec.md#5-compilation-pipeline))
> and implemented in `core/vm/vm.cpp`. They were omitted from previous spec
> versions.*

#### BITAND

- **Form**: `BITAND RD, RS1, RS2`
- **Semantics**: `R[RD] := canonical(R[RS1] & R[RS2])` — bitwise AND of T81BigInt operands.
- **Faults**: None.

#### BITOR

- **Form**: `BITOR RD, RS1, RS2`
- **Semantics**: `R[RD] := canonical(R[RS1] | R[RS2])` — bitwise OR.
- **Faults**: None.

#### BITXOR

- **Form**: `BITXOR RD, RS1, RS2`
- **Semantics**: `R[RD] := canonical(R[RS1] ^ R[RS2])` — bitwise XOR.
- **Faults**: None.

#### BITNOT

- **Form**: `BITNOT RD, RS`
- **Semantics**: `R[RD] := canonical(~R[RS])` — bitwise complement.
- **Faults**: None.

#### BITSHL

- **Form**: `BITSHL RD, RS, R_AMT`
- **Semantics**: `R[RD] := canonical(R[RS] << (R[R_AMT] & 0x3F))` — left shift;
  shift amount is masked to 6 bits before use.
- **Faults**: None.

#### BITSHR

- **Form**: `BITSHR RD, RS, R_AMT`
- **Semantics**: `R[RD] := canonical(R[RS] >> (R[R_AMT] & 0x3F))` — arithmetic
  (sign-preserving) right shift; shift amount masked to 6 bits.
- **Faults**: None.

#### BITUSHR

- **Form**: `BITUSHR RD, RS, R_AMT`
- **Semantics**: `R[RD] := canonical(unsigned_right_shift(R[RS], R[R_AMT] & 0x3F))` —
  logical (unsigned, zero-fill) right shift.
- **Faults**: None.

> **Conformance programs:** [`spec/conformance/tisc/bitwise-determinism.t81`](conformance/tisc/bitwise-determinism.t81) · [`bitwise-shift-masking.t81`](conformance/tisc/bitwise-shift-masking.t81)

______________________________________________________________________

### 5.15 AI-Native Inference Operations (RFC-0026)

These instructions are **Tier 2+ only**. All operate on TensorHandle registers.
All are subject to Axion pre-instruction verification before execution begins.
Hardware floating-point is prohibited; all arithmetic uses the canonical
**T81Float soft-float** path defined in `spec/t81-data-types.md §4`.

> *Freeze Exception addition — 2026-03-07 (RFC-0026 AI-M1–M5). These opcodes
> are emitted by AI inference workload compilation paths and implemented in
> `core/vm/vm.cpp`. Opcode byte assignments: ATTN=0xBB, QMATMUL=0xBC,
> EMBED=0xBD, WLOAD=0xBE, GATHER=0xBF, SCATTER=0xC0.*

#### Packed Operand Encoding

When two register indices must be passed in a single 32-bit operand field,
the **PACK(X, Y)** encoding is used:

```text
packed = (X & 0xFF) | ((Y & 0xFF) << 8)
```

Both `X` and `Y` are decoded as unsigned 8-bit values. This encoding is used
by ATTN, QMATMUL, GATHER, and SCATTER.

#### ATTN — Scaled Dot-Product Attention

- **Form**: `ATTN RD, PACK(R_QK, R_V), 0`
- **Semantics**: `R[RD] := softmax(Q · Kᵀ / √dₖ) · V`
  where `R_QK` holds Q and K handles (packed), `R_V` holds the value tensor.
- **Determinism**: `√dₖ` MUST use the canonical T81Float square-root algorithm
  from `spec/t81-data-types.md §4`. Hardware FPU is prohibited.
- **Axion**: Emits `attn guard` AxionEvent with shape metadata before execution.
- **Faults**: `TypeFault` on incompatible head dimensions; `ShapeFault` on
  shape mismatch.

#### QMATMUL — Quantized Matrix Multiply

- **Form**: `QMATMUL RD, PACK(R_ACT, R_WT), R_SCALE`
- **Semantics**: `R[RD] := dequantize(R[R_WT], R[R_SCALE]) · R[R_ACT]`
  Dequantize-then-multiply order is normative; multiply-then-dequantize is
  non-conformant.
- **Axion**: Enforces that `R_WT` provenance matches the loaded model policy.
- **Faults**: `TypeFault` on shape mismatch; `CanonFault` if scale is outside
  the canonical range defined by the active Axion policy.

#### EMBED — Embedding Lookup

- **Form**: `EMBED RD, R_TABLE, R_IDX`
- **Semantics**: `R[RD] := gather_rows(R[R_TABLE], R[R_IDX])`
  `R_TABLE` has shape `[vocab, dim]`; `R_IDX` is a `T81BigInt` or
  `Vector[T81BigInt]` of token indices. Output shape: `[len(R_IDX), dim]`.
- **Faults**: `BoundsFault` on any out-of-bounds index.

#### WLOAD — Weight Load with Policy Gate

- **Form**: `WLOAD RD, R_SRC, R_POLICY`
- **Semantics**: Phase-1 materializes an independent weight-tensor copy from
  the source handle `R[R_SRC]`. `R[R_POLICY]` is reserved for follow-on policy
  dispatch and is not yet enforced by the current VM implementation.
- **CanonFS audit (AI-M4)**: When a CanonFS driver is attached to the VM,
  WLOAD emits `meta slot axion event segment=meta addr=<n> action=WeightLoad`
  via `log_canonfs_operation()`. This audit event is absent when no CanonFS
  driver is present.
- **Status note**: Full policy-gated CanonFS-backed weight loading remains a
  follow-on promotion step. Current phase-1 behavior provides deterministic
  materialization plus audit visibility, not final provenance enforcement.
- **Faults**: `TypeFault`.

#### GATHER — Sparse Gather

- **Form**: `GATHER RD, R_SRC, PACK(R_IDX, R_AXIS)`
- **Semantics**: Gathers a slice from tensor `R[R_SRC]` at index `R[R_IDX]`
  along axis `R[R_AXIS]`. For rank-2 tensors:
  - axis=0: output is row `R[R_IDX]` of `R[R_SRC]`
  - axis=1: output is column `R[R_IDX]` of `R[R_SRC]`
- **Determinism**: Given identical source tensor and index, output is
  bit-exact.
- **Faults**: `BoundsFault` on out-of-bounds index; `TypeFault` if index or
  axis registers hold non-integer values.

#### SCATTER — Sparse Scatter-Add

- **Form**: `SCATTER RD, R_DST, PACK(R_IDX, R_SRC)`
- **Semantics**: `R[RD] := scatter_add(R[R_DST], R[R_IDX], R[R_SRC])`
  Adds `R[R_SRC]` into `R[R_DST]` at index `R[R_IDX]`; `R[R_DST]` is not
  mutated in-place. `R[RD]` receives the updated tensor.
- **Aliasing detection (AI-M5)**: The VM MUST detect re-use of the same
  `(dst_handle, axis, index)` tuple within a single execution frame.
  Detection raises `SecurityFault` before any state mutation occurs. The
  tracking set (`ThreadContext::scatter_used`) is scoped to the current
  execution frame.
- **Faults**: `SecurityFault` (aliasing violation), `BoundsFault`,
  `TypeFault`.

> **Tests:** `tests/cpp/vm_ai_phase1_wload_canonfs_audit_test.cpp` ·
> `tests/cpp/vm_ai_phase1_gather_axis1_test.cpp` ·
> `tests/cpp/vm_ai_phase1_scatter_aliasing_test.cpp`

______________________________________________________________________

### 5.16 Agentic Constructs (RFC-0015)

These instructions support T81Lang's first-class `agent` declarations.
Every `AgentInvoke` emission MUST cause the Axion Policy Kernel to record an
audit event **before** the callee begins executing.

> *Freeze Exception addition — 2026-03-16 (RFC-0015). Opcode assigned
> immediately after `GcSafepoint` in the enumeration.
> Implemented in `lang/frontend/` (IRGen), `core/vm/vm.cpp`, and
> `runtime/jit/jit_compiler.cpp` (Axion-boundary exit group).*

#### AGENT_INVOKE

- **Form**: `AGENT_INVOKE RD, R_ADDR`
- **Semantics**:
  1. The Axion kernel records an `agent_invoke` AxionEvent carrying the
     call-site PC and the target address `R[R_ADDR]`.
  2. The current tier ceiling is checked; if exceeded, a `TierFault` is raised
     before any jump occurs.
  3. `PC+1` is pushed onto the call stack (identical to `CALL`).
  4. `PC := R[R_ADDR]`
  5. `RD` is reserved for future metadata; MUST be 0 in conforming programs.
- **Vs. CALL**: `AGENT_INVOKE` is semantically identical to `CALL` except for
  the mandatory pre-dispatch Axion audit event.  Optimizers MUST NOT silently
  lower `AGENT_INVOKE` to `CALL`.
- **JIT boundary**: The JIT compiler MUST exit to the interpreter on
  `AGENT_INVOKE` (Axion-boundary exit group) so the full audit sequence is
  guaranteed.
- **Faults**:
  - `TierFault`: Active tier ceiling exceeded.
  - `StackFault`: Call-stack overflow.
  - `DecodeFault`: `R[R_ADDR]` out of program bounds.

> **Tests:** `tests/cpp/agent_constructs_test.cpp` — [RFC-0015-04], [RFC-0015-05], [RFC-0015-06]

______________________________________________________________________

### 5.17 Ternary-Native Inference Operations (RFC-0034)

*Status: **accepted** — 2026-03-16. Full normative text below. Opcode bytes
assigned in `spec/tisc/opcode-registry.md §2.19`. Implementation in
`core/vm/vm.cpp`; math layer in `include/t81/tensor/ternary_native.hpp`.*

All opcodes in this class operate on `T729DynamicTensor` handles. All require
Tier 2+. All are subject to Axion pre-instruction shape and domain verification.
Weights must be in `{−1.0, 0.0, +1.0}` (T81Qutrit domain). Accumulators are
`T81BigInt`-exact — no floating-point multiply is performed.

| Mnemonic | Opcode Byte | Operands | Description |
| :--- | :--- | :--- | :--- |
| `TWMATMUL` | 0xC2 (194) | `RD, R_ACT, R_WT` | Ternary-weight matrix multiply: `RD = ACT · WT` using T81BigInt row accumulators; result cast to float. Weights must be T81Qutrit-domain. |
| `TQUANT` | 0xC3 (195) | `RD, R_SRC, R_THR` | Quantize tensor to ternary: abs(x) ≤ R\[THR\] → 0; x > THR → +1; x < -THR → -1. Result is T81Qutrit-domain. |
| `TATTN` | 0xC4 (196) | `RD, R_Q, PACK(R_K, R_V)` | Ternary Q/K attention: `scores = TWMATMUL(Q, Kᵀ)`; row-wise softmax; `out = softmax(scores) · V`. Q and K must be T81Qutrit-domain; V is float. |
| `TWEMBED` | 0xC5 (197) | `RD, R_TABLE, R_IDX` | Row-gather from T81Qutrit embedding table at integer index `R[IDX]`. Table must be 2-D; result is 1×cols T81Qutrit row. |
| `TERNACCUM` | 0xC6 (198) | `RD, R_WT, R_ACT` | Scalar 1-D ternary dot product of two flat tensors; T81BigInt-exact accumulator; result stored as T81Float handle. |
| `TACT` | 0xC7 (199) | `RD, R_SRC, R_MODE` | Ternary activation function selected by mode byte `R[MODE]` (see TACT Modes below); followed by Axion activation-ceiling gate. |

`TATTN` packs K and V register indices as `PACK(R_K, R_V) = (R_K & 0xFF) | ((R_V & 0xFF) << 8)` in the `C` operand.

#### TACT Modes

`R_MODE` selects the activation function. Undefined mode bytes raise `CanonFault`.

| Mode Byte | Name | Semantics |
| :--- | :--- | :--- |
| `0x01` | `TernaryStep` | `x > 0.5 → +1; x < −0.5 → −1; else 0` |
| `0x02` | `TanhQuantized` | `tanh(x) > 0.5 → +1; tanh(x) < −0.5 → −1; else 0` |

#### TACT Axion Activation-Ceiling Gate

After the mathematical transform, TACT submits the result to an Axion
post-execute `activation-ceiling` policy check. The verdict controls commit:

| Verdict | Effect |
| :--- | :--- |
| Allow | `RD` committed; PC advances normally |
| Quarantine | `RD` not committed; PC stalls; `SecurityFault` raised |
| Deny | `RD` not committed; `ActivationFault` raised |

Faults introduced by this class: `ActivationFault` (TACT Deny verdict),
`ShapeFault` (shape mismatch in TWMATMUL/TATTN), `BoundsFault` (TWEMBED
index out of range), `TierFault` (Tier < 2). All other faults follow §6.

### 5.17A SWAR Tensor Operations (RFC-0040)

*Status: **draft implementation** — 2026-03-18. Opcode bytes assigned in
`spec/tisc/opcode-registry.md §2.23`. Implementation in `core/vm/vm.cpp`;
stable SWAR API in `include/t81/swar/swar.hpp`.*

These opcodes expose RFC-0040 SWAR kernels as explicit VM instructions over
`T729DynamicTensor` handles whose numeric class is `ExactTrit`. They are not
scalar-trit aliases for `TNot`, `TAnd`, or `TOr`; they are explicit packed
tensor operations intended for deterministic small/medium exact-trit workloads.

| Mnemonic | Opcode Byte | Operands | Description |
| :--- | :--- | :--- | :--- |
| `TNOT_SWAR` | 0xD5 (213) | `RD, R_SRC, 0` | Unary negation of an `ExactTrit` tensor via stable SWAR kernels. Output shape matches input shape. |
| `TAND_SWAR` | 0xD6 (214) | `RD, R_LHS, R_RHS` | Elementwise ternary conjunction (`min`) of two same-shape `ExactTrit` tensors via SWAR. |
| `TOR_SWAR` | 0xD7 (215) | `RD, R_LHS, R_RHS` | Elementwise ternary disjunction (`max`) of two same-shape `ExactTrit` tensors via SWAR. |

Operational constraints:

- Inputs MUST be tensor handles.
- Inputs MUST have numeric class `ExactTrit`.
- Binary forms MUST have shape-compatible operands.
- Results preserve the input tensor shape and remain `ExactTrit`.

Fault behavior:

- `TypeFault`: operand tensor is not `ExactTrit`.
- `ShapeFault`: binary operand shapes differ.
- `DecodeFault`: operand is not a valid tensor handle or SWAR decode/encode fails.

### 5.18 Governed Foreign Function Interface (RFC-0036 + RFC-00B8)

*Status: **accepted** — 2026-03-18 alignment refresh. Implementation exists in
`core/vm/vm.cpp` (FFICall, FFIRegister, FFIPolicySet), `core/vm/ffi_dispatcher.cpp`,
and the RFC-0036 frontend path, with VM evidence covering success, failure, audit-trail,
quarantine, and real system-library calls. The runtime remains beta for promotion
purposes while broader schemas, ecosystem bindings, and the sandbox-boundary decision
remain open. Opcode bytes: §2.20.*

`FFI_CALL` enables T81Lang `foreign {}` blocks to invoke external functions
through the governed `FFIDispatcher`. Policy enforcement, resource quotas, and
audit trails are applied before and after every foreign call.

| Mnemonic | Opcode Byte | Operands | Description |
| :--- | :--- | :--- | :--- |
| `FFICall` | 0xC8 (200) | `R_DEST, ARG_COUNT, FUNC_SYMBOL` | Invoke foreign function; `text_literal` / encoded symbol pool entry carries the resolved name. Policy check before dispatch; audit event on completion. |
| `FFIRegister` | 0xC9 (201) | `R_LIB_NAME, R_VERSION_HASH` | Register foreign library by name and version hash in `FFILibraryRegistry`. |
| `FFIPolicySet` | 0xCA (202) | `R_POLICY_TYPE, R_POLICY_VALUE` | Set per-call FFI policy (determinism, quota, isolation). |

**FFI_CALL IR encoding:** at the IR level, `FFI_CALL` carries the function name
in the `text_literal` field of the instruction; operands are `{dest_reg, Immediate{arg_count}}`.
Binary emission preserves `arg_count` in operand `B` and stores the resolved symbol-pool
index in operand `C`. The VM dispatcher resolves that encoded symbol to a function via
`FFILibraryRegistry` at runtime.

**Policy qualifiers** map to Axion policy modes enforced at dispatch time:

- `deterministic` — Axion verifies output is identical for identical inputs.
- `governed` — Full pre/post Axion policy gate, same as effectful builtins.
- `quarantined` — Side-effects captured in audit trail before commit.

Faults: `FFINotInitialized`, `FFIPolicyDenied`, `FFITimeout`, `FFIMemoryExhausted`.

### 5.19 Ternary Lattice Cryptography (RFC-0038)

*Status: **proposed** — 2026-03-16. Full normative text in
`spec/rfcs/RFC-0038-lattice-crypto.md`. Math layer: `include/t81/tensor/lattice_crypto.hpp`.
Implementation: `core/vm/vm.cpp`.*

Negacyclic polynomial arithmetic over `{−1, 0, +1}` coefficients in `Z[x]/(x^n + 1)`.
No integer multiplications required — only add/sub/trit-flip. T81BigInt-exact accumulators.

| Mnemonic | Opcode Byte | Operands | Description |
| :--- | :--- | :--- | :--- |
| `POLYMUL` | 0xCB (203) | `RD, R_A, R_B` | Negacyclic polynomial multiply: `RD = A · B` in `Z[x]/(x^n+1)`. Both operands must be 1-D tensors of equal length `n`. T81BigInt accumulators; result cast to float. |
| `POLYMOD` | 0xCC (204) | `RD, R_A, R_Q` | Centered coefficient reduction: every coefficient `c` of `A` is mapped to `((c % q) + q) % q`, then shifted to `(−q/2, q/2]`. `R_Q` holds the modulus `q` (must be > 0). |

**Negacyclic wrap rule (normative):**

```text
C[k] = Σ_{i=0}^{n-1}  A[i] · B[(k−i+n) mod n] · neg(i, j)
  where  j = (k−i+n) mod n
         neg(i, j) = −1  if  i + j ≥ n  (wrap past x^n ≡ −1)
                      +1  otherwise
```

Ternary product `A[i] · B[j]` is computed as a trit-flip:

- 0 if either factor is 0
- +1 if both factors have the same sign
- −1 if factors have opposite signs

Faults: `ShapeFault` (non-1-D tensors, length mismatch), `DecodeFault` (q ≤ 0).
All Tier 2+.

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

2. Fault record MUST be stored in Axion-visible metadata.

3. Axion MAY decide to:

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

Current spec version: **v1.9.0**

- **Code Generation Targets for TISC** → [`t81lang-spec.md`](t81lang-spec.md#5-compilation-pipeline)
- **Type System Mapping to Operands** → [`t81lang-spec.md`](t81lang-spec.md#2-type-system)
- **Purity and Effect Constraints** → [`t81lang-spec.md`](t81lang-spec.md#3-purity-and-effects)

## Axion

- **Privileged Instruction Handling** → [`axion-kernel.md`](axion-kernel.md#1-responsibilities)
- **Verification of Instruction Effects** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)
- **Recursion & Safety Boundaries for Jumps** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)

## Cognitive Tiers

- **Higher-Level Reasoning Built on TISC State Transitions** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)
- **Ternary Logic Foundations for Symbolic Recursion** → [`cognitive-tiers.md`](cognitive-tiers.md#2-constraints)

______________________________________________________________________
