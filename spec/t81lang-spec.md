______________________________________________________________________

title: T81 Foundation Specification — T81Lang
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

# T81Lang Specification

Version 0.2 — Draft (Standards Track)

Status: Draft → Standards Track\
Applies to: TISC, T81VM, Axion, Data Types

T81Lang is the **high-level, deterministic, ternary-native programming language** of the T81 Ecosystem.\
It compiles **exclusively** to TISC and guarantees predictable semantics under T81VM.

This document defines:

- language syntax and grammar
- deterministic semantics
- type system and canonical rules
- purity, effects, and safety
- compilation pipeline
- Axion visibility and tier interactions

______________________________________________________________________

## 0. Language Properties

T81Lang is designed with the following properties:

1. **Deterministic**\
   Every expression has a single, unambiguous meaning. No hidden I/O, nondeterminism, or environment leakage.

1. **Pure-by-default**\
   Functions are pure unless explicitly marked as effectful.

1. **Ternary-native semantics**\
   All numbers, fractions, logical values, and composite structures follow the T81 Data Types specification.

1. **Statically typed + Canonical**\
   All values must be canonical at every boundary; compile-time enforcement where possible.

1. **VM-compatible**\
   All generated TISC MUST:

   - be type-correct
   - be shape-correct
   - respect determinism and Axion policies

1. **Tier-aware**\
   Code MAY declare cognitive-tier intent, enabling Axion to enforce or monitor complexity bounds.

______________________________________________________________________

## 1. Core Grammar

A simplified normative grammar follows.\
(Full formal grammar appears in Appendix A.)

```ebnf
program       ::= { function | declaration }*

function      ::= "fn" identifier "(" parameters ")" "->" type block
parameters    ::= [ parameter { "," parameter } ]
parameter     ::= identifier ":" type

block         ::= "{" statement* "}"

statement     ::= let_decl
                | assign
                | return
                | if_stmt
                | loop_stmt
                | expr ";"

let_decl      ::= "let" identifier ":" type "=" expr ";"
assign        ::= identifier "=" expr ";"
return        ::= "return" expr ";"

if_stmt       ::= "if" expr block [ "else" block ]
loop_stmt     ::= "loop" block

expr          ::= equality_expr
equality_expr ::= relational_expr { ("==" | "!=") relational_expr }
relational_expr ::= additive_expr { ("<" | "<=" | ">" | ">=") additive_expr }
additive_expr ::= term { ("+" | "-") term }
term          ::= factor { "*" factor }
factor        ::= literal
                | identifier
                | fn_call
                | unary_op factor
                | paren_expr

fn_call       ::= identifier "(" [ expr { "," expr } ] ")"

literal       ::= integer | float | fraction | symbol

paren_expr    ::= "(" expr ")"
```

This grammar MUST be parsed deterministically.
No ambiguous operator precedence is allowed; all precedence rules are normative and listed in Appendix A.1.

______________________________________________________________________

## 2. Type System

The T81Lang type system directly corresponds to the T81 Data Types spec.

### 2.1 Primitive Types

- `T81Int`
- `T81Float`
- `T81Fraction`
- `Symbol`

These map 1:1 to the Data Types primitive categories.

### 2.2 Composite Types

- `Vector[T]`
- `Matrix[T]`
- `Tensor[T, Rank]`
- `Record { ... }`
- `Enum { ... }`

Composite types MUST:

- have static shapes where applicable (vectors, matrices, tensors)
- follow canonicalization rules when constructed

### 2.3 Structural Types

- `Option[T]`
- `Result[T, E]`

### 2.4 Canonicalization Rules

All values MUST be canonical at:

- construction
- mutation
- function return
- VM boundary crossing

If a value is non-canonical, the compiler MUST either:

- normalize it at compile time, or
- emit code that performs canonicalization at runtime, or
- reject the program with a type error

______________________________________________________________________

## 3. Purity and Effects

### 3.1 Pure Functions

A function is pure if:

- it reads no global or external state
- it performs no I/O
- its result depends only on its parameters
- it produces no VM-visible side effects except deterministic computation

Pure functions MAY be optimized aggressively.

### 3.2 Effectful Functions

Marked with:

```t81
@effect
fn write_log(x: T81Int) -> Unit { ... }
```

Effectful operations include:

- memory mutation
- VM I/O channels (to be defined)
- Axion interactions
- tensor heavy ops (if declared impure via cost annotations)

### 3.3 Tiered Purity

Optional annotation:

```t81
@tier(2)
```

This indicates cognitive-tier complexity expectations and allows Axion to regulate recursion and resource scaling.

______________________________________________________________________

## 4. Name Resolution

T81Lang uses lexical scoping.
Name resolution is deterministic:

1. Look up in local scope
1. Look up in parent scopes
1. Look up in module scope
1. Resolve via imports (imports MUST be explicit and acyclic)

Shadowing is allowed but MUST be resolved deterministically by nearest scope.

______________________________________________________________________

## 5. Compilation Pipeline

Compilation to TISC follows **eight deterministic stages**.

### Stage 1 — Lexing

Produces a canonical stream of tokens.

### Stage 2 — Parsing

Produces an AST conforming to the grammar.

### Stage 3 — Type Checking

Guarantees:

- no type mismatches
- all shapes are valid
- canonical forms are upheld
- arithmetic expressions are only legal when both operands share a primitive type
  and TISC exposes a matching opcode (`ADD/MUL` for `T81Int`, `FADD/FMUL` for
  `T81Float`, `FRACADD/FRACMUL` for `T81Fraction`). Mixed-type arithmetic MUST
  be rejected unless an explicit conversion (`I2F`, `I2FRAC`, etc.) is inserted.
- literal expressions for `T81Float`, `T81Fraction`, and `Symbol` MUST be tagged
  with their declared types so lowering can emit the correct literal pool handle.
- comparison expressions (`==`, `!=`, `<`, `<=`, `>`, `>=`) MUST return `T81Int`
  (canonical boolean) and both operands MUST be the same primitive numeric type
  (`T81Int`, `T81Float`, or `T81Fraction`).

### Stage 4 — Purity Analysis

Tracks pure vs impure operations.

### Stage 5 — Control Flow Normalization

All loops become structured CFG nodes; early exits are normalized into explicit jumps.

### Stage 6 — Intermediate Representation (IR)

The IR is ternary-native, SSA-like, and preserves canonical form.
Each IR instruction has:

- deterministic semantics
- no implicit side effects
- clear type consistency

### Stage 7 — TISC Lowering

Maps IR instructions to TISC sequences:

| IR Construct | TISC Output |
| ------------ | ------------------------------------------ |
| `a + b` (`T81Int`) | `LOADI`, `ADD` sequence |
| `a * b` (`T81Int`) | `MUL` |
| `a + b` (`T81Float`) | literal handle load, `FADD` |
| `a * b` (`T81Float`) | `FMUL` |
| `a + b` (`T81Fraction`) | literal handle load, `FRACADD` |
| `a * b` (`T81Fraction`) | `FRACMUL` |
| comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`) | `CMP`, `SETF`, literal/branch sequence producing a `T81Int` |
| vector add | `TVECADD` |
| matrix mul | `TMATMUL` |
| fn call | `CALL`, argument push, return capture |
| recursion | same as fn call; Axion receives call depth |

Lowering MUST also emit float/fraction/symbol literals into their respective
program pools before they are referenced by any instruction, and all references
in registers MUST be 1-based handles into those pools.
Comparisons first emit `CMP`/`SETF` to obtain the sign of the subtraction in a
register, then use deterministic branch sequences to write `0` or `1` back into
a `T81Int` destination register so higher-level control flow can reuse the
result.

The lowering pass MUST NOT introduce nondeterminism.

### Stage 8 — Code Generation

Produces:

- TISC binary
- metadata section (tier hints, purity map, shape map)
- Axion inspection records (optional but recommended)

______________________________________________________________________

## 6. Control Flow Semantics

All control flow is explicit and deterministic.

### 6.1 If/Else

- Condition MUST be a canonical boolean encoded as T81Int (0, 1, or -1 via ternary truth table).
- Branch lowering MUST produce deterministic TISC control flow.

### 6.2 Loop

Loops MUST:

- have deterministic entry/exit
- expose recursion depth and loop iteration counts to Axion
- be transformable into static CFG structures

Infinite loops are allowed *only* if annotated:

```t81
@bounded(infinite)
loop { ... }
```

Otherwise, an unbounded loop without explicit annotation is a compile-time error.

### 6.3 Recursion

Recursion is allowed but MUST:

- have explicit tier annotations if deep recursion is possible
- provide a deterministic termination guarantee if pure and finite

______________________________________________________________________

## 7. Axion Integration

Axion observes T81Lang at:

- compilation boundaries
- function entry/exit
- recursion depth
- effectful operations
- tensor heavy ops
- purity violations
- memory model constraints

### 7.1 Tier Metadata

Compilation emits tier metadata:

```text
tier-hints:
  function foo: tier 1
  function bar: tier 3
  function baz: tier 2 (recursive)
```

Axion MAY:

- veto unsafe recursion
- restructure execution scheduling
- enforce safety boundaries

### 7.2 Safety Hooks

All unsafe or effectful constructs compile to TISC sequences that call Axion hooks via:

- AXREAD
- AXSET
- AXVERIFY

before or after execution as required by Axion policy.

______________________________________________________________________

## 8. Interoperability Summary

T81Lang MUST:

- use [Data Types](t81-data-types.md) exactly
- lower to [TISC](tisc-spec.md) deterministically
- rely on [T81VM](t81vm-spec.md) for memory and execution
- inform and be regulated by [Axion](axion-kernel.md)
- enable structured reasoning for [Cognitive Tiers](cognitive-tiers.md)

______________________________________________________________________

# Cross-References

## Overview

- **Language Layer in Stack** → [`t81-overview.md`](t81-overview.md#2-architectural-layers)

## Data Types

- **Primitive & Composite Type Alignment** → [`t81-data-types.md`](t81-data-types.md#2-primitive-types)
- **Canonicalization Rules** → [`t81-data-types.md`](t81-data-types.md#5-canonicalization-rules-critical-normative-section)

## TISC

- **Lowering Targets** → [`tisc-spec.md`](tisc-spec.md#5-opcode-classes)
- **Execution Semantics** → [`tisc-spec.md`](tisc-spec.md#1-machine-model)

## T81VM

- **Memory Layout and Safety** → [`t81vm-spec.md`](t81vm-spec.md#4-memory-model)
- **Determinism Constraints** → [`t81vm-spec.md`](t81vm-spec.md#2-determinism-constraints)

## Axion

- **Tier Metadata & Verification** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)
- **Recursion and Safety Policies** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)

## Cognitive Tiers

- **Tier Semantics and Language Integration** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)

______________________________________________________________________
