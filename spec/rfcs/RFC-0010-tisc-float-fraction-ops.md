______________________________________________________________________

title: "RFC-0010 — TISC Float & Fraction Arithmetic"
version: Draft
applies_to:

- T81 Data Types
- TISC Specification
- T81 Virtual Machine
- T81Lang

______________________________________________________________________

# Summary

Define first-class arithmetic opcodes for `T81Float` and `T81Fraction` so that:

1. TISC can add/sub/mul/div canonical floats and fractions directly (no manual handle shuffling).
1. T81Lang may lower non-`T81Int` expressions to deterministic TISC sequences.
1. T81VM tracks literal/value pools deterministically and surfaces results to Axion.

This RFC does not change literal syntax (already specified in `spec/v1.1.0-canonical.md §5.3`) but introduces new opcodes plus compiler/VM obligations.

# Motivation

- Current ISA only exposes conversion opcodes (`I2F`, `F2I`, `I2Frac`, `Frac2I`). There is no normative way to add two floats or multiply fractions without dropping into ad-hoc host code, violating determinism.
- T81Lang grammar (`spec/t81lang-spec.md §1`) explicitly allows float/fraction literals, yet the compilation pipeline (§5) cannot lower them.
- Axion observability depends on the VM performing canonical arithmetic (per `spec/t81vm-spec.md §4.6`). Without ISA support, implementations would diverge.

# Design / Specification

## 1. New Opcode Families

Extend `spec/tisc-spec.md §5.7` with the following opcodes (each 3-register form mirrors integer arithmetic):

| Opcode | Semantics | Notes |
|----------|---------------------------------------------------------------------------|-------|
| `FADD` | `R[RD] := canonical_float(R[RS1] + R[RS2])` | uses T81Float semantics |
| `FSUB` | `R[RD] := canonical_float(R[RS1] − R[RS2])` | |
| `FMUL` | `R[RD] := canonical_float(R[RS1] × R[RS2])` | |
| `FDIV` | `R[RD] := canonical_float(R[RS1] ÷ R[RS2])` | fault on zero divisor |
| `FRACADD`| `R[RD] := canonical_fraction(R[RS1] + R[RS2])` | values refer to fraction pool handles |
| `FRACSUB`| `R[RD] := canonical_fraction(R[RS1] − R[RS2])` | |
| `FRACMUL`| `R[RD] := canonical_fraction(R[RS1] × R[RS2])` | |
| `FRACDIV`| `R[RD] := canonical_fraction(R[RS1] ÷ R[RS2])` | fault on zero divisor |

**Operand Interpretation**

- Float opcodes operate on handle registers pointing into the VM float pool. VM resolves handles before performing deterministic arithmetic using `T81Float` rules (`spec/t81-data-types.md §2.3`).
- Fraction opcodes operate on fraction handles, normalize results (gcd, positive denominator) and return new/newly-deduplicated handles.

**Faults**

- Handle out of range → `Trap::IllegalInstruction`.
- Division by zero (denominator handle resolves to zero) → `Trap::DivideByZero`.

## 2. Literal Pools

`include/t81/tisc/program.hpp` already gained float/fraction/symbol pools. This RFC formalizes the requirement:

- When encoding a program, all float/fraction/symbol literals MUST be emitted into these pools with canonical values before any instructions execute.
- VM MUST preload `state.floats/fractions/symbols` from the program (per `t81vm-spec §4.6`), treat register values as 1-based handles, and maintain determinism when pools grow (e.g., as `FRACADD` produces a new fraction).

## 3. Compiler Responsibilities

`spec/t81lang-spec §5` gains the following obligations:

- Type checker MUST permit `T81Float`/`T81Fraction` locals, params, and returns if operations in scope map to supported opcodes (`FADD`, etc.).
- Lowering stage MUST:
  - Emit pool handles for literals (already implemented).
  - Emit appropriate float/fraction opcodes when expressions combine those types.
  - Reject cross-type arithmetic unless an explicit conversion opcode is inserted.

## 4. VM Responsibilities

Per `spec/t81vm-spec §4`:

- VM must implement the new opcodes with deterministic arithmetic over canonical `T81Float`/`T81Fraction` values.
- Results that already exist in the pool MAY reuse existing handles (optional dedup), otherwise append deterministically.
- Trace entries MUST include opcode + handle indices so Axion can inspect canonical values if necessary.

# Rationale

- Mirrors existing integer opcode structure, so tooling can reuse encoding/decoding paths.
- Keeps literal handling deterministic: pools act like read-only segments seeded at load time, then extended canonically when runtime results require new handles.
- Allows T81Lang features already promised by the grammar while staying aligned with T81 data-type semantics.

# Backwards Compatibility

- Existing integer-only programs remain unaffected.
- VM implementations lacking the new opcodes can trap deterministically when encountering them, but compliant engines must implement the semantics before executing ISA version ≥0.4.

# Security Considerations

- Arithmetic must be constant-time w.r.t. secret data if Axion policy requires it, but that is unchanged from integer ops.
- Handle reuse/deduplication MUST be deterministic to avoid covert channels.

# Open Questions

1. Do we require deduplication (interning) of float/fraction handles, or is append-only acceptable if program semantics tolerate duplicates?
1. Should we introduce comparison opcodes (`FCMP`, `FRACCMP`) in the same RFC or a follow-up?
1. How do these opcodes interact with Axion tier annotations—does heavy float work demand tier escalation?
