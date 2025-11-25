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

expr             ::= logical_or_expr
logical_or_expr  ::= logical_and_expr { "||" logical_and_expr }
logical_and_expr ::= equality_expr { "&&" equality_expr }
equality_expr    ::= relational_expr { ("==" | "!=") relational_expr }
relational_expr  ::= additive_expr { ("<" | "<=" | ">" | ">=") additive_expr }
additive_expr    ::= term { ("+" | "-") term }
term             ::= factor { ("*" | "/" | "%") factor }
factor        ::= literal
                | identifier
                | fn_call
                | unary_op factor
                | paren_expr

fn_call       ::= identifier "(" [ expr { "," expr } ] ")"

literal       ::= integer | float | fraction | symbol | vector_literal
vector_literal ::= "[" [ expr { "," expr } ] "]"

paren_expr    ::= "(" expr ")"
```

This grammar MUST be parsed deterministically.
No ambiguous operator precedence is allowed; all precedence rules are normative and listed in Appendix A.1.

### Vector Literal Typing Rules

- All elements in a vector literal MUST be of the same type, or be promotable to a common type according to the Numeric Widening rules.
- The type of an empty vector literal (`[]`) is inferred from its context. If the context does not provide a type, the program is ill-formed.

### Vector Literal Semantics

- Vector literals are immutable.
- Vector literals are value-type constructs. When passed to a function, they are passed by value (i.e., a copy is made).

Logical conjunction/disjunction use canonical ternary booleans: `0t81`
represents false, any non-zero `T81Int` represents true, and the emitted code
MUST short-circuit left-to-right. When the left operand of `&&` evaluates to
false or the left operand of `||` evaluates to true, the right operand MUST NOT
be evaluated. The final result MUST be `0t81` or `1t81`.

Division (`/`) follows the canonical semantics defined for the operand type in
[`tisc-spec.md`](tisc-spec.md#52-arithmetic-instructions) and deterministically
faults on a zero divisor. The modulo operator (`%`) is defined only for
`T81Int`; attempting to use `%` with any other type MUST be rejected at compile
time.

______________________________________________________________________

## 2. Type System

The T81Lang type system directly corresponds to the T81 Data Types spec.

### 2.1 Primitive Types

- `T81Int`
- `T81Float`
- `T81Fraction`
- `Symbol`

These map 1:1 to the Data Types primitive categories.

### 2.2 Vector Type

- `Vector[T]`

The `Vector[T]` type is syntactic sugar for a rank-1 `Tensor` of type `T`. All operations on `Vector[T]` are equivalent to operations on a rank-1 `Tensor`.

### 2.3 Composite Types

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

`Option[T]` exposes two constructors:

- `Some(expr)` wraps a value of type `T`. When no contextual type exists, the
  compiler infers `Option[T]` directly from the operand.
- `None` represents the absence of a value and therefore REQUIRES a contextual
  `Option[T]` (e.g., a variable declaration or function parameter) so the
  compiler can verify downstream uses.

`Result[T, E]` provides explicit success/failure constructors:

- `Ok(expr)` yields the success branch (`T`) and MUST appear in a context that
  already specifies `Result[T, E]`.
- `Err(expr)` yields the error branch (`E`) under the same contextual-type
  requirement.

Both structural types lower to canonical handles backed by the VM. Handles are
deduplicated so equality comparisons operate on semantic contents rather than
allocation identity.

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

### 2.5 Numeric Widening

`T81Int` values MAY be widened to `T81Float` or `T81Fraction` whenever a context
requires the wider type (e.g., declarations, parameter passing, binary
arithmetic, or comparisons). Widening MUST be performed by inserting the
appropriate conversion opcodes:

- `I2F` for `T81Int → T81Float`
- `I2FRAC` for `T81Int → T81Fraction`

No other implicit conversions are permitted. In particular, the compiler MUST
NOT automatically convert floats or fractions back to integers; such narrowing
would need explicit syntax in a future revision.

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
  and TISC exposes a matching opcode (`ADD/SUB/MUL/DIV/MOD` for `T81Int`,
  `FADD/FSUB/FMUL/FDIV` for `T81Float`, `FRACADD/FRACSUB/FRACMUL/FRACDIV` for
  `T81Fraction`). When an expression mixes
  `T81Int` with either `T81Float` or `T81Fraction`, the compiler MUST insert a
  deterministic widening conversion (`I2F` or `I2FRAC`) so the operands share the
  non-integer type before lowering. Any other mixed-type arithmetic MUST be
  rejected, and the modulo operator (`%`) is legal **only** when both operands
  are `T81Int`. Division by zero MUST surface the VM’s deterministic
  `DivideByZero` fault; the compiler MAY fold constant expressions that avoid the
  fault but MUST NOT silently change the runtime semantics.

- literal expressions for `T81Float`, `T81Fraction`, and `Symbol` MUST be tagged
  with their declared types so lowering can emit the correct literal pool handle.

- comparison expressions (`==`, `!=`, `<`, `<=`, `>`, `>=`) MUST return `T81Int`
  (canonical boolean). Operands MUST share the same primitive numeric type
  (`T81Int`, `T81Float`, or `T81Fraction`). When a comparison mixes `T81Int`
  with `T81Float` or `T81Fraction`, operands MUST be widened via `I2F` or
  `I2FRAC` before emitting `CMP`. `Symbol` operands are legal ONLY for
  equality/inequality and MUST be rejected for ordering comparisons.

- When storing into variables, passing call arguments, or returning from a
  function, `T81Int` values MAY be widened to `T81Float` or `T81Fraction` via
  `I2F`/`I2FRAC` to satisfy the declared type. Narrowing conversions (float or
  fraction to integer) MUST NOT be inserted implicitly.

- logical conjunction/disjunction (`&&`, `||`) MUST evaluate operands left to
  right, short-circuit deterministically, and operate on canonical boolean
  `T81Int` values. Their result MUST be `0t81` when false and `1t81` when true.

- `Option[T]` and `Result[T, E]` constructors MUST follow deterministic typing
  rules:

  - `Some(expr)` infers `Option[T]` when `expr : T` unless an expected
    `Option[T]` appears in context, in which case the payload type MUST match.
  - `None` has no payload and therefore REQUIRES an expected `Option[T]`
    context; the compiler MUST reject standalone `None`.
  - `Ok(expr)` and `Err(expr)` also require an expected `Result[T, E]` context
    so the compiler can type-check the payload against the correct branch
    (`Ok` uses `T`, `Err` uses `E`).

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
| `a / b` (`T81Int`) | `DIV` (faults on zero divisor) |
| `a % b` (`T81Int`) | `MOD` (faults on zero divisor) |
| `a / b` (`T81Float`) | `FDIV` (faults on zero divisor) |
| `a / b` (`T81Fraction`) | `FRACDIV` (faults on zero divisor) |
| comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`) | `CMP`, `SETF`, literal/branch sequence producing a `T81Int` (`Symbol` allowed only for `==`/`!=`) |
| `T81Int → T81Float/T81Fraction` promotion | `I2F` / `I2FRAC` emitted before the consuming opcode/assignment |
| `Some(expr)` | Evaluate `expr`, `MAKE_OPTION_SOME` to produce canonical handle |
| `None` | Contextual type chooses `Option[T]`, emit `MAKE_OPTION_NONE` |
| `vector literal` | Evaluate elements, construct a rank-1 `T729Tensor`, add to `tensor_pool`, and load handle with `LoadImm`. |
| `Ok(expr)` | Evaluate `expr`, `MAKE_RESULT_OK` |
| `Err(expr)` | Evaluate `expr`, `MAKE_RESULT_ERR` |
| `match (value)` over `Option/Result` | Evaluate subject once, use `OPTION_IS_SOME` / `RESULT_IS_OK` plus conditional jumps; bind payloads via `OPTION_UNWRAP` or `RESULT_UNWRAP_OK` / `RESULT_UNWRAP_ERR` before lowering the selected arm |
| logical `a && b` | Evaluate `a`, `JumpIfZero` to skip RHS, evaluate `b` only when needed, write `0t81/1t81` deterministically |
| logical `a || b` | Evaluate `a`, `JumpIfNotZero` to skip RHS, evaluate `b` only when needed, write `0t81/1t81` deterministically |
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

### 6.2 Match

T81Lang exposes `match` expressions for structural types:

```t81
let total: T81Int = match (maybe_value) {
  Some(v) => v + 1t81,
  None => 0t81,
};
```

- The scrutinee inside `match ( … )` is evaluated exactly once.
- For `Option[T]` the arms MUST include exactly one `Some(binding)` arm and one
  `None` arm (order-insensitive). `binding` introduces a new immutable variable
  scoped to that arm; use `_` to ignore the payload.
- For `Result[T, E]` the arms MUST include exactly one `Ok(binding)` and one
  `Err(binding)` arm.
- Each arm MUST evaluate to the same type; the first arm determines the result
  type if no contextual type is provided.
- Lowering MUST branch via `OPTION_IS_SOME` / `RESULT_IS_OK`, use the matching
  `OPTION_UNWRAP` or `RESULT_UNWRAP_*` opcode when a payload binding is present,
  and jump over the inactive arm so that only the selected expression executes.
- Missing arms, duplicate variants, or mismatched types are compile-time errors.

### 6.3 Loop

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

### 6.4 Recursion

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
