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

Version 1.2 — Draft

Status: Draft\
Last Revised: 2026-03-01\
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
   **Note:** Floating-point division and transcendental functions (`sin`, `cos`, etc.) rely on host-platform behavior and may vary across architectures; strict bit-exact determinism is guaranteed only for integer/fraction arithmetic and float storage.

2. **Pure-by-default**\
   Functions are pure unless explicitly marked as effectful.

3. **Ternary-native semantics**\
   All numbers, fractions, logical values, and composite structures follow the T81 Data Types specification.

4. **Statically typed + Canonical**\
   All values must be canonical at every boundary; compile-time enforcement where possible.

5. **VM-compatible**\
   All generated TISC MUST:

   - be type-correct
   - be shape-correct
   - respect determinism and Axion policies

6. **Tier-aware**\
   Code MAY declare cognitive-tier intent, enabling Axion to enforce or monitor complexity bounds.

______________________________________________________________________

## 1. Core Grammar

A simplified normative grammar follows.\
(Full formal grammar appears in Appendix A.)

```ebnf
program       ::= declaration*

declaration   ::= fn_decl
                | type_decl
                | record_decl
                | enum_decl
                | var_decl
                | let_decl
                | statement

fn_decl       ::= annotation* "fn" identifier
                  [ "[" generic_params "]" ]
                  "(" parameters ")" [ "->" type ] block

annotation    ::= "@" identifier [ "(" annotation_args ")" ]
annotation_args ::= annotation_arg { "," annotation_arg }
annotation_arg  ::= [ identifier ":" ] expr

generic_params ::= identifier { "," identifier }
parameters    ::= [ parameter { "," parameter } ]
parameter     ::= identifier ":" type

block         ::= "{" statement* [ expr ] "}"

statement     ::= let_decl
                | var_decl
                | assign
                | return_stmt
                | if_stmt
                | while_stmt
                | for_stmt
                | loop_stmt
                | break_stmt
                | continue_stmt
                | expr ";"

let_decl      ::= "let" identifier [ ":" type ] "=" expr ";"
var_decl      ::= "var" identifier [ ":" type ] [ "=" expr ] ";"
assign        ::= lvalue "=" expr ";"
lvalue        ::= identifier | lvalue "." identifier | lvalue "[" expr "]"
return_stmt   ::= "return" [ expr ] ";"
break_stmt    ::= "break" ";"
continue_stmt ::= "continue" ";"

if_stmt       ::= "if" "(" expr ")" block [ "else" ( block | if_stmt ) ]
while_stmt    ::= "while" "(" expr ")" block
for_stmt      ::= "for" identifier "in" expr block
loop_stmt     ::= annotation* "loop" block

expr             ::= logical_or_expr
logical_or_expr  ::= logical_and_expr { "||" logical_and_expr }
logical_and_expr ::= bitwise_or_expr { "&&" bitwise_or_expr }
bitwise_or_expr  ::= bitwise_xor_expr { "|" bitwise_xor_expr }
bitwise_xor_expr ::= bitwise_and_expr { "^" bitwise_and_expr }
bitwise_and_expr ::= range_expr { "&" range_expr }
range_expr       ::= equality_expr [ ".." equality_expr ]
equality_expr    ::= relational_expr { ("==" | "!=") relational_expr }
relational_expr  ::= shift_expr { ("<" | "<=" | ">" | ">=") shift_expr }
shift_expr       ::= additive_expr { ("<<" | ">>" | ">>>") additive_expr }
additive_expr    ::= term { ("+" | "-") term }
term             ::= factor { ("*" | "/" | "%") factor }
factor           ::= unary { "**" unary }   (* right-associative *)
unary         ::= ( "!" | "-" | "~" ) unary | call
call          ::= primary { "(" [ expr { "," expr } ] ")"
                           | "[" expr "]"
                           | "." identifier }
primary       ::= literal
                | identifier
                | paren_expr
                | if_expr
                | block_expr
                | match_expr
                | record_literal
                | enum_literal

if_expr       ::= "if" "(" expr ")" block [ "else" ( block | if_expr ) ]
block_expr    ::= block
match_expr    ::= "match" "(" expr ")" "{" match_arm { ( "," | ";" ) match_arm } [ ( "," | ";" ) ] "}"
match_arm     ::= pattern [ "if" expr ] "=>" expr
pattern       ::= "_"                             (* wildcard *)
                | identifier                      (* binding *)
                | identifier "(" pattern ")"      (* enum variant with payload *)
                | "{" field_pattern { "," field_pattern } "}"   (* record destructure *)
field_pattern ::= identifier ":" identifier

literal       ::= integer_literal | t81int_literal | float_literal | t81float_literal
                | string_literal | symbol_literal | vector_literal | "true" | "false"

integer_literal  ::= digit+
t81int_literal   ::= digit+ "t81"                (* T81BigInt literal *)
float_literal    ::= digit+ "." digit* [ exponent ]
t81float_literal ::= digit+ "." digit* [ exponent ] "t81"  (* T81Float literal *)
exponent         ::= ( "e" | "E" ) [ "+" | "-" ] digit+
string_literal   ::= '"' character* '"'           (* T81String literal *)
symbol_literal   ::= ":" identifier              (* Symbol literal — colon prefix *)

vector_literal ::= "[" ( [ expr { "," expr } ] | expr ";" expr ) "]"
record_literal ::= identifier "{" field_init { "," field_init } [ "," ] "}"
field_init     ::= identifier ":" expr
enum_literal   ::= identifier "." identifier [ "(" expr ")" ]

paren_expr    ::= "(" expr ")"
```

This grammar MUST be parsed deterministically.
No ambiguous operator precedence is allowed; all precedence rules are normative and listed in Appendix A.1.

**Key notation notes:**

- Conditions in `if`, `while` MUST be enclosed in parentheses: `if (expr)`.
- Symbol literals use the colon-prefix syntax (`:name`), not backticks.
- `T81BigInt` numeric literals require the `t81` suffix: `42t81`, `-17t81`.
- `T81Float` numeric literals MAY use the `t81` suffix: `3.14t81`. Without the suffix, an unqualified float literal is inferred as `T81Float` when context demands it, otherwise as the host float type.
- Enum construction uses dot notation: `Color.Red`, `Shape.Circle(5t81)`.

### Vector Literal Typing Rules

- All elements in a vector literal MUST be of the same type, or be promotable to a common type according to the Numeric Widening rules (§2.7).
- The type of an empty vector literal (`[]`) is inferred from its context. If the context does not provide a type, the program is ill-formed.
- The repeat syntax `[val; count]` requires `count` to be an integer type.

### Vector Literal Semantics

- Vector literals are immutable at construction.
- Vector literals are value-type constructs. When passed to a function, they are passed by value (i.e., a copy is made).
- `[val; count]` constructs a vector of length `count` where every element is initialized to `val`.

Logical conjunction/disjunction short-circuit left-to-right. When the left operand of `&&`
evaluates to false, or the left operand of `||` evaluates to true, the right operand MUST
NOT be evaluated. The final result is a canonical boolean encoded as a `T81BigInt`.

Division (`/`) follows the canonical semantics defined for the operand type in
[`tisc-spec.md`](tisc-spec.md#52-arithmetic-instructions) and deterministically
faults on a zero divisor. The modulo operator (`%`) is defined only for
integer types; attempting to use `%` with any other type MUST be rejected at compile
time.

______________________________________________________________________

## 2. Type System

The T81Lang type system directly corresponds to the T81 Data Types spec.

### 2.1 Generic Type Syntax (Authoritative)

The **only** authoritative syntax for generic instantiation is square brackets:

```t81
Vector[T]
T81Vector[T, N]        // fixed-size vector, N is a compile-time integer
Map[Key, Value]
Option[T]
Result[Ok, Err]
Tensor[T]              // dynamic-rank tensor
```

Angle brackets `<...>` are **legacy** and must not appear in any new code or grammar.
The C++20 compiler frontend actively rejects this syntax to enforce the modern style.

- Generic parameters are separated by commas when more than one is required.
- The first parameter is always the element type.
- Subsequent parameters, if present, are **compile-time integer constants**.

### 2.2 Primitive Types

Primitive types are organized into three tiers.

#### 2.2.1 Ternary Core

These types carry full ternary determinism guarantees and map 1:1 to the Data Types spec.

| Type | Description |
| :--- | :--- |
| `T81BigInt` | Arbitrary-precision balanced ternary integer. Numeric literals require the `t81` suffix: `42t81`. |
| `T81Float` | Balanced ternary floating point. Float literals MAY use the `t81` suffix: `3.14t81`. |
| `T81Fraction` | Exact rational number (numerator/denominator pair). |
| `Symbol` | Interned, immutable symbolic identifier. Literals use the colon-prefix syntax: `:name`. |

#### 2.2.2 Text and Binary

| Type | Description |
| :--- | :--- |
| `T81String` | UTF-8 text string. String literals use double quotes: `"hello"`. |
| `T81Bytes` | Opaque byte sequence. Constructed explicitly: `T81Bytes("data")`. |

#### 2.2.3 Extended Numeric

These types are ternary-native or ternary-adjacent and provide specialized numeric domains.

| Type | Description |
| :--- | :--- |
| `T81Fixed` | Fixed-point arithmetic with deterministic rounding. |
| `T81Complex` | Complex number (a + bi). |
| `T81Quaternion` | Hypercomplex number (a + bi + cj + dk). |
| `T81Prob` | Probability value, bounded to [0, 1]. |
| `T81Qutrit` | Ternary quantum state; values MUST be in {-1, 0, +1}. The compiler validates this at construction. |
| `T81Uint` | Unsigned integer. `T81Uint - T81Uint` yields `T81BigInt` to avoid wrap semantics. |

#### 2.2.4 Binary Interop Types

These types follow binary (not ternary) semantics and exist for practical interoperability.
They do NOT carry ternary determinism guarantees and MUST NOT be used in DCP-verified
surfaces that require strict bit-exact ternary reproducibility.

| Type | Description |
| :--- | :--- |
| `i32` | 32-bit signed binary integer. |
| `i16` | 16-bit signed binary integer. |
| `i8` | 8-bit signed binary integer. |
| `i2` | 2-bit signed binary integer. |
| `bool` | Boolean (`true` / `false`). |

### 2.3 Vector and Fixed-Size Vector Types

- `Vector[T]` — dynamic-length vector; element type `T`.
- `T81Vector[T, N]` — fixed-size vector; `N` is a compile-time integer constant.

`Vector[T]` is the standard resizable container. `T81Vector[T, N]` is used when the
size is known at compile time and deterministic allocation is required. Elements of both
are accessed with index syntax: `v[0]`.

### 2.4 Composite Collection Types

All collection types use the square-bracket generic syntax.

| Type | Description |
| :--- | :--- |
| `Matrix[T]` | 2-D tensor; equivalent to a rank-2 `Tensor[T]`. |
| `Tensor[T]` | N-dimensional tensor of element type `T`. |
| `List[T]` | Linked list of element type `T`. |
| `Map[K, V]` | Hash map from key type `K` to value type `V`. |
| `Set[T]` | Hash set of element type `T`. |
| `Tree[T]` | Tree structure of element type `T`. |
| `Graph` | Directed graph (vertices and edges are `T81String` handles). |

Composite types MUST:

- have static shapes where applicable (matrices, tensors, fixed vectors)
- follow canonicalization rules when constructed

User-defined composite types are declared with `record` and `enum` (see §3.2).

### 2.5 Structural Types: Option[T] and Result[T, E]

These are **first-class core types** and MUST be present in the grammar.
They use exactly the same square-bracket syntax as user generics.

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

### 2.6 Canonicalization Rules

All values MUST be canonical at:

- construction
- mutation
- function return
- VM boundary crossing

If a value is non-canonical, the compiler MUST either:

- normalize it at compile time, or
- emit code that performs canonicalization at runtime, or
- reject the program with a type error

### 2.7 Numeric Widening

Implicit widening follows a rank hierarchy (narrowest to widest):

```text
T81Qutrit < i2 < i8 < i16 < i32 < T81Uint < T81BigInt < T81Fraction < T81Fixed < T81Float
```

The compiler MAY implicitly widen a value to any wider type in the chain when context
requires it (e.g., declarations, parameter passing, binary arithmetic). Widening MUST be
performed by inserting the appropriate conversion opcodes (`I2F`, `I2FRAC`, etc.).

Special cases:

- `T81Uint - T81Uint` → `T81BigInt` (avoids unsigned wrap semantics).
- `%` (modulo) is legal **only** when both operands are integer types.
- Bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`, `>>>`) require integer operands.

No narrowing conversions are inserted implicitly. The compiler MUST NOT automatically
convert floats or fractions back to integers.

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
fn write_log(x: T81BigInt) -> Unit { ... }
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

This indicates cognitive-tier complexity expectations (1 = symbolic, 5 = infinite) and
allows Axion to regulate recursion and resource scaling.

### 3.4 Type Declaration Attributes

`record` and `enum` declarations MAY carry the following attributes:

```t81
@schema(1)
@module(geometry.shapes)
record Point {
  x: T81BigInt;
  y: T81BigInt;
}
```

- `@schema(N)` — declares a schema version integer for data compatibility tracking.
  `N` MUST be a positive integer literal.
- `@module(path)` — declares the module namespace using a dot-separated path (e.g.,
  `geometry.shapes`). This is informational metadata; the compiler does not enforce
  module isolation from this annotation alone.

______________________________________________________________________

## 4. Name Resolution

T81Lang uses lexical scoping.
Name resolution is deterministic:

1. Look up in local scope
2. Look up in parent scopes
3. Look up in module scope
4. Resolve via imports (imports MUST be explicit and acyclic)

Shadowing is allowed but MUST be resolved deterministically by nearest scope.

______________________________________________________________________

## 5. Compilation Pipeline

Compilation to TISC follows **eight deterministic stages**.

Determinism enforcement is not advisory: repository CI MUST run compile
reproducibility gates over the canonical fixture pack
(`tests/fixtures/t81lang_determinism/*.t81`) and verify the aggregate fixture
hash against the tracked golden file
(`tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt`) on both Linux
`x86_64` and Linux `arm64` runners.

### Deterministic Compilation Profile (Traceability)

This subsection defines bounded traceability invariants for compiler bytecode
emission. It does not expand language semantics beyond this specification.

1. Source-stability invariant:
   - For a fixed source program and fixed compiler configuration, repeated
     compilation MUST produce byte-identical TISC output.
2. Pipeline determinism invariant:
   - Stage 1 through Stage 7 outputs MUST be deterministic functions of source
     input and explicit configuration only.
3. Literal-pool determinism invariant:
   - Lowering MUST emit and reference literal pools deterministically; no
     host-order or nondeterministic insertion effects are allowed.
4. Control-flow lowering determinism invariant:
   - Branch labels, arm selection lowering, and short-circuit lowering MUST be
     stable for equivalent AST/IR input.

Acceptance evidence for this profile is anchored to existing verification
surfaces:

- `tests/cpp/e2e_compile_determinism_test.cpp`
- `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`
- `scripts/ci/t81lang_repro_gate.py`
- `tests/fixtures/t81lang_determinism/`

Determinism guarantees remain bounded by
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md` and
`docs/product/DETERMINISTIC_CORE_PROFILE.md`.

### Stage 1 — Lexing

Produces a canonical stream of tokens.

### Stage 2 — Parsing

Produces an AST conforming to the grammar.

### Stage 3 — Semantic Analysis & Type Checking

This stage is responsible for verifying the semantic correctness of the program. It performs **name resolution**, **scope analysis**, and **type checking**.

The semantic analyzer **MUST** enforce the following guarantees:

- **No Type Mismatches:** All expressions and statements must adhere to the type system.
- **Valid Shapes:** All tensor and vector operations must use compatible shapes.
- **Canonical Forms:** All constructed values must be in their canonical form.
- **Arithmetic Correctness:**
  - Arithmetic expressions are only legal when both operands share a compatible primitive type and TISC exposes a matching opcode (`ADD/SUB/MUL/DIV/MOD` for `T81BigInt`, `FADD/FSUB/FMUL/FDIV` for `T81Float`, `FRACADD/FRACSUB/FRACMUL/FRACDIV` for `T81Fraction`).
  - When an expression mixes `T81BigInt` with either `T81Float` or `T81Fraction`, the compiler MUST insert a deterministic widening conversion (`I2F` or `I2FRAC`) so the operands share the non-integer type before lowering.
  - Any other mixed-type arithmetic MUST be rejected.
  - The modulo operator (`%`) is legal **only** when both operands are integer types.
  - Division by zero MUST surface the VM’s deterministic `DivideByZero` fault.
- **Literal Typing:** Literal expressions for `T81Float`, `T81Fraction`, and `Symbol` MUST be tagged with their declared types.
- **Comparison Correctness:**
  - Comparison expressions (`==`, `!=`, `<`, `<=`, `>`, `>=`) MUST return a canonical boolean encoded as a `T81BigInt`.
  - Operands MUST share the same primitive numeric type, with the same widening rules as arithmetic.
  - `Symbol` operands are legal ONLY for equality/inequality.
- **Assignment and Function Call Correctness:**
  - When storing into variables, passing call arguments, or returning from a function, integer values MAY be widened to `T81Float` or `T81Fraction` per the widening rank hierarchy (§2.7).
  - Narrowing conversions (e.g., float to integer) MUST NOT be inserted implicitly.
- **Logical Operations:** Logical conjunction/disjunction (`&&`, `||`) MUST evaluate operands left-to-right, short-circuit deterministically, and produce a canonical boolean `T81BigInt` result.
- **Structural Type Correctness (`Option`/`Result`):**
  - `Some(expr)` infers `Option[T]` from `expr`'s type `T` or matches a contextual `Option` type.
  - `None` has no payload and therefore REQUIRES a contextual `Option[T]` type.
  - `Ok(expr)` and `Err(expr)` also require a contextual `Result[T, E]` type to check the payload against the correct branch.

A single example can illustrate many of these rules in practice:

```t81
// A function that returns an Option type
fn find_positive(val: T81BigInt) -> Option[T81BigInt] {
    if (val > 0t81) {
        return Some(val);
    } else {
        return None;
    }
}

// A variable declaration with an explicit type
let my_value: T81BigInt = -5t81;

// Using a match expression to safely unwrap the Option
let result_text: Symbol = match (find_positive(my_value)) {
    Some(_) => :found_positive,
    None    => :not_positive,
};
```

### Vector & Matrix Operations Example

```t81
fn process_vectors(a: Vector[T81BigInt], b: Vector[T81BigInt]) -> T81BigInt {
    // Vectors support element-wise operations if they have compatible shapes
    let sum = a + b;

    // Dot product of two vectors
    return std.tensor.vec_add(a, b);
}

fn matrix_transform(m: Matrix[T81Float], v: Vector[T81Float]) -> Vector[T81Float] {
    // Matrix multiplication via TISC TMATMUL
    return m * v;
}
```

### Generic Type Example

```t81
// A generic container for any T81 type
record Box[T] {
    content: T;
}

fn open_box[T](b: Box[T]) -> T {
    return b.content;
}

let int_box: Box[T81BigInt] = Box { content: 42t81 };
let value: T81BigInt = open_box(int_box);
```

### Record and Enum Example

```t81
record Point {
    x: T81BigInt;
    y: T81BigInt;
}

enum Shape {
    Circle(T81BigInt);          // Circle with radius
    Rectangle(Point, Point);    // Rectangle with top-left and bottom-right points
}

fn area_squared(s: Shape) -> T81BigInt {
    return match (s) {
        Circle(r)         => 3t81 * r * r,
        Rectangle(p1, p2) => (p2.x - p1.x) * (p2.y - p1.y),
    };
}
```

### Loop and Boundedness Example

```t81
fn factorial(n: T81BigInt) -> T81BigInt {
    var result: T81BigInt = 1t81;
    var i: T81BigInt = n;

    @bounded(100) // Expect this loop to run at most 100 times
    loop {
        if (i <= 1t81) {
            return result;
        }
        result = result * i;
        i = i - 1t81;
    }
}
```

### Safety Limit Examples

T81Lang code is subject to Axion safety policies. The following examples
illustrate code patterns that may trigger deterministic Axion traps if policy
limits are exceeded.

#### Recursion Depth Limit

Deep recursion triggers a `SecurityFault` when the stack depth exceeds the
`(max-recursion <n>)` policy limit.

```t81
// This will trap if max-recursion is set to a small value (e.g., 10)
fn deep_recursion(n: T81BigInt) -> T81BigInt {
    if (n <= 0t81) { return 0t81; }
    return 1t81 + deep_recursion(n - 1t81);
}
```

#### Instruction Count Limit

Infinite or extremely long loops trigger a `SecurityFault` when the total
executed instruction count exceeds the `(max-instructions <n>)` limit.

```t81
fn infinite_loop() {
    @bounded(infinite)
    loop {
        // performs work indefinitely until Axion veto
    }
}
```

#### Stack Usage Limit

Large local variable allocations trigger a `StackFault` (if exceeding physical
segment) or `SecurityFault` (if exceeding `(max-stack <n>)` policy).

```t81
fn large_stack_usage() {
    // This may trigger a StackFault depending on VM configuration
    // and Axion max-stack policy.
    let data: Vector[T81BigInt] = [1t81, 2t81, 3t81, 4t81, 5t81, 6t81, 7t81, 8t81, 9t81, 10t81];
    // ... complex local state ...
}
```

### Effectful Function Example

```t81
@effect
fn log_and_calculate(x: T81BigInt) -> T81BigInt {
    // Calling an intrinsic effectful function for logging
    write_axion_log("Calculating for value", x);

    return x * x;
}
```

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
| `a + b` (`T81BigInt`) | `LOADI`, `ADD` sequence |
| `a * b` (`T81BigInt`) | `MUL` |
| `a + b` (`T81Float`) | literal handle load, `FADD` |
| `a * b` (`T81Float`) | `FMUL` |
| `a + b` (`T81Fraction`) | literal handle load, `FRACADD` |
| `a * b` (`T81Fraction`) | `FRACMUL` |
| `a / b` (`T81BigInt`) | `DIV` (faults on zero divisor) |
| `a % b` (`T81BigInt`) | `MOD` (faults on zero divisor) |
| `a / b` (`T81Float`) | `FDIV` (faults on zero divisor) |
| `a / b` (`T81Fraction`) | `FRACDIV` (faults on zero divisor) |
| `a & b` | `BITAND` |
| `a \| b` | `BITOR` |
| `a ^ b` | `BITXOR` |
| `~a` | `BITNOT` |
| `a << b` | `BITSHL` (shift amount masked `& 0x3F`) |
| `a >> b` | `BITSHR` (arithmetic shift, amount masked) |
| `a >>> b` | `BITUSHR` (logical shift, amount masked) |
| comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`) | `CMP`, `SETF`, literal/branch sequence producing a `T81BigInt` boolean (`Symbol` allowed only for `==`/`!=`) |
| `T81BigInt → T81Float/T81Fraction` promotion | `I2F` / `I2FRAC` emitted before the consuming opcode/assignment |
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
a `T81BigInt` destination register so higher-level control flow can reuse the
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

- Condition MUST be wrapped in parentheses: `if (expr) { ... }`.
- The condition MUST be a canonical boolean (zero = false, non-zero = true).
- Branch lowering MUST produce deterministic TISC control flow.
- `if` may be used as either a statement or an expression. When used as an expression,
  both branches MUST produce the same type and neither may be omitted.

### 6.2 Match

`match` is an **expression** that evaluates a scrutinee against a series of arms. The
scrutinee is evaluated exactly once. All arms MUST produce the same type.

```t81
let total: T81BigInt = match (maybe_value) {
  Some(v) => v + 1t81,
  None    => 0t81,
};
```

**Arm patterns supported:**

| Pattern | Syntax | Applies to |
| :--- | :--- | :--- |
| Wildcard | `_` | Any type; discards value |
| Catch-all binding | `name` | Any type; binds value to `name` |
| Variant with payload | `VariantName(binding)` | Enum variants, `Some(binding)`, `Ok(binding)`, `Err(binding)` |
| Unit variant | `VariantName` or `None` | Enum unit variants, `None` |
| Record destructure | `{ field: binding, ... }` | Record types |
| Guard | `pattern if cond => expr` | Any arm; evaluated only after pattern matches |

**Option[T] rules:**

- MUST include exactly one `Some(binding)` arm and one `None` arm (order-insensitive).
- Use `_` to discard the `Some` payload: `Some(_)`.

**Result[T, E] rules:**

- MUST include exactly one `Ok(binding)` and one `Err(binding)` arm.
- Guards (`if cond`) are permitted on any arm, but exhaustiveness is still required:
  if all arms have guards, a compile-time error is raised.

**User-defined enum rules:**

- Every variant MUST be covered. Missing variants are compile-time errors.
- Duplicate variants are compile-time errors.

**Lowering:** branches via `OPTION_IS_SOME` / `RESULT_IS_OK` / enum tag checks; payload
is extracted with the corresponding `UNWRAP` opcode when a binding is present.

### 6.3 Loop

The bare `loop` construct creates an unbounded loop. Loops MUST:

- have deterministic entry/exit
- expose iteration counts to Axion
- be transformable into static CFG structures

Every `loop` MUST carry a `@bounded` annotation. The annotation value may be:

- `@bounded(N)` — compiler-visible static bound (N iterations maximum).
- `@bounded(infinite)` — explicitly infinite; Axion enforces via instruction-count policy.
- `@bounded(loop(guard_expr))` — runtime guard expression evaluated each iteration.

```t81
@bounded(100)
loop {
  if (i >= 10) { return result; }
  result = result * i;
  i = i - 1t81;
}
```

An unbounded `loop` without a `@bounded` annotation is a compile-time error.

### 6.4 While Loop

`while` evaluates a condition before each iteration. The condition MUST be wrapped in
parentheses. No annotation is required (the condition serves as the bound).

```t81
var i: i32 = 0;
while (i < 100) {
  curr = curr * 2t81;
  i = i + 1;
}
```

### 6.5 For Loop

`for` iterates over a collection or range, binding each element to an identifier.

```t81
for item in my_vector {
  print(item);
}
```

The iterable expression MUST produce a type that exposes an iteration protocol
(`Vector[T]`, `List[T]`, a range `a..b`, etc.).

### 6.6 Break and Continue

`break` exits the innermost enclosing `loop`, `while`, or `for`. `continue` skips the
remainder of the current iteration and resumes from the loop condition / next element.

```t81
@bounded(infinite)
loop {
  if (done()) { break; }
  if (skip()) { continue; }
  process();
}
```

Neither `break` nor `continue` may appear outside a loop body; doing so is a
compile-time error.

### 6.7 Recursion

Recursion is allowed but MUST:

- have explicit `@tier(N)` annotations if deep recursion is possible
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

## 9. Standard Library

The T81Lang standard library is accessed via dot-namespaced call paths (e.g.,
`std.collections.len(v)`). All stdlib functions are verified by the semantic analyzer
and are available without an explicit import. The tables below are normative for the
semantic analyzer; they do not enumerate every low-level overload.

### 9.1 std.collections

Operates on `Vector[T]`, `List[T]`, `Map[K, V]`, `Set[T]`, `Tree[T]`, and `Graph`.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.collections.len` | `(C) → i32` | Number of elements in any collection `C`. |
| `std.collections.is_empty` | `(C) → bool` | True if collection has zero elements. |
| `std.collections.first` | `(Vector[T] \| List[T]) → T` | First element. |
| `std.collections.last` | `(Vector[T] \| List[T]) → T` | Last element. |
| `std.collections.push` | `(Vector[T], T) → Vector[T]` | Append element; returns updated vector. |
| `std.collections.pop` | `(Vector[T]) → Vector[T]` | Remove last element; returns updated vector. |
| `std.collections.map` | `(...) → Map[K, V]` | Construct a Map. |
| `std.collections.map_put` | `(Map[K,V], K, V) → Map[K,V]` | Insert or update key. |
| `std.collections.map_get` | `(Map[K,V], K) → V` | Retrieve value by key. |
| `std.collections.map_has` | `(Map[K,V], K) → bool` | True if key exists. |
| `std.collections.map_remove` | `(Map[K,V], K) → Map[K,V]` | Remove key. |
| `std.collections.map_keys` | `(Map[K,V]) → Vector[K]` | All keys. |
| `std.collections.map_size` | `(Map[K,V]) → i32` | Number of entries. |
| `std.collections.set` | `(...) → Set[T]` | Construct a Set. |
| `std.collections.set_add` | `(Set[T], T) → Set[T]` | Add element. |
| `std.collections.set_has` | `(Set[T], T) → bool` | Membership test. |
| `std.collections.set_remove` | `(Set[T], T) → Set[T]` | Remove element. |
| `std.collections.set_size` | `(Set[T]) → i32` | Cardinality. |
| `std.collections.graph` | `() → Vector[T81String]` | Construct empty graph handle. |
| `std.collections.graph_add_edge` | `(G, T81String, T81String) → G` | Add directed edge. |
| `std.collections.graph_has_edge` | `(G, T81String, T81String) → bool` | Edge existence check. |
| `std.collections.graph_edge_count` | `(G) → i32` | Total edge count. |

### 9.2 std.text

Operates on `T81String`.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.text.len` | `(T81String) → i32` | String length in characters. |
| `std.text.is_empty` | `(T81String) → bool` | True if length is zero. |
| `std.text.concat` | `(T81String, T81String) → T81String` | Concatenate two strings. |
| `std.text.starts_with` | `(T81String, T81String) → bool` | Prefix test. |
| `std.text.ends_with` | `(T81String, T81String) → bool` | Suffix test. |
| `std.text.contains` | `(T81String, T81String) → bool` | Substring test. |
| `std.text.index_of` | `(T81String, T81String) → i32` | First occurrence index, or -1. |
| `std.text.replace` | `(T81String, T81String, T81String) → T81String` | Replace first match. |
| `std.text.split` | `(T81String, T81String) → Vector[T81String]` | Split by delimiter. |
| `std.text.join` | `(Vector[T81String], T81String) → T81String` | Join with separator. |
| `std.text.to_string` | `(T) → T81String` | Convert any printable value to string. |

### 9.3 std.bytes

Operates on `T81Bytes`. Mirrors `std.text` with byte semantics.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.bytes.len` | `(T81Bytes) → i32` | Byte length. |
| `std.bytes.is_empty` | `(T81Bytes) → bool` | True if empty. |
| `std.bytes.concat` | `(T81Bytes, T81Bytes) → T81Bytes` | Concatenate. |
| `std.bytes.starts_with` | `(T81Bytes, T81Bytes) → bool` | Prefix test. |
| `std.bytes.ends_with` | `(T81Bytes, T81Bytes) → bool` | Suffix test. |
| `std.bytes.contains` | `(T81Bytes, T81Bytes) → bool` | Subsequence test. |
| `std.bytes.split` | `(T81Bytes, T81Bytes) → Vector[T81Bytes]` | Split by delimiter. |
| `std.bytes.join` | `(Vector[T81Bytes], T81Bytes) → T81Bytes` | Join with separator. |
| `std.bytes.to_string` | `(T81Bytes) → T81String` | Decode as UTF-8. |
| `std.bytes.from_string` | `(T81String) → T81Bytes` | Encode as bytes. |

### 9.4 std.math

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.math.abs` | `(T81Float) → T81Float` | Absolute value. |
| `std.math.sqrt` | `(T81Float) → T81Float` | Square root. |
| `std.math.pow` | `(T81Float, T81Float) → T81Float` | Power. |
| `std.math.clamp` | `(T81Float, T81Float, T81Float) → T81Float` | Clamp to [lo, hi]. |
| `std.math.sin` / `cos` / `tan` | `(T81Float) → T81Float` | Trigonometric functions (host-platform; not bit-exact across architectures). |
| `std.math.exp` / `log` | `(T81Float) → T81Float` | Exponential / natural log. |
| `std.math.fraction.from_int` | `(T81BigInt) → T81Fraction` | Lift integer to fraction. |
| `std.math.fraction.to_int` | `(T81Fraction) → T81BigInt` | Truncate to integer. |
| `std.math.fraction.add` | `(T81Fraction, T81Fraction) → T81Fraction` | Exact addition. |
| `std.math.fraction.sub` | `(T81Fraction, T81Fraction) → T81Fraction` | Exact subtraction. |
| `std.math.fraction.mul` | `(T81Fraction, T81Fraction) → T81Fraction` | Exact multiplication. |
| `std.math.fraction.div` | `(T81Fraction, T81Fraction) → T81Fraction` | Exact division (faults on zero). |

### 9.5 std.symbol

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.symbol.intern` | `(T81String) → Symbol` | Intern a string as a Symbol. |
| `std.symbol.to_string` | `(Symbol) → T81String` | Convert Symbol back to its string name. |
| `std.symbol.eq` | `(Symbol, Symbol) → bool` | Symbol equality (identity check). |
| `std.symbol.ne` | `(Symbol, Symbol) → bool` | Symbol inequality. |

### 9.6 std.tensor

| Function | Signature | Description |
| :--- | :--- | :--- |
| `std.tensor.from_list` | `(Vector[T]) → Tensor[T]` | Construct rank-1 tensor from a vector. |
| `std.tensor.vec_add` | `(Tensor[T], Tensor[T]) → Tensor[T]` | Element-wise addition (lowers to `TVECADD`). |
| `std.tensor.matmul` | `(Tensor[T], Tensor[T]) → Tensor[T]` | Matrix multiplication (lowers to `TMATMUL`). |
| `std.tensor.load` | `(T81String) → i32` | Load tensor weights from a named resource; returns a VM handle. |

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

# Appendix A: Formal Grammar

This appendix contains the complete, normative EBNF grammar for T81Lang v1.2.
The grammar matches the recursive-descent parser in `lang/frontend/parser.cpp`.

## A.1 Lexical Elements

```ebnf
identifier ::= letter ( letter | digit | "_" )*
letter     ::= "a"..."z" | "A"..."Z" | "_"
digit      ::= "0"..."9"

integer_literal    ::= digit+
t81int_literal     ::= digit+ "t81"
float_literal      ::= digit+ "." digit* [ exponent ]
t81float_literal   ::= digit+ "." digit* [ exponent ] "t81"
exponent           ::= ( "e" | "E" ) [ "+" | "-" ] digit+
string_literal     ::= '"' character* '"'
symbol_literal     ::= ":" letter ( letter | digit )*

comment ::= "//" ( any character except newline )*
          | "/*" ( any character )* "*/"
```

**Notes:**

- `t81int_literal` is the canonical form for `T81BigInt` values (e.g., `42t81`).
- `symbol_literal` uses a leading colon: `:ok`, `:not_found`.
- Double-quoted strings are `T81String` values.

## A.2 Types

```ebnf
type ::= primitive_type
       | generic_type
       | identifier          (* user-defined record / enum / type alias *)

primitive_type ::=
    "T81BigInt" | "T81Float" | "T81Fraction" | "Symbol"
  | "T81String" | "T81Bytes"
  | "T81Fixed"  | "T81Complex" | "T81Quaternion" | "T81Prob"
  | "T81Qutrit" | "T81Uint"
  | "i32" | "i16" | "i8" | "i2" | "bool" | "void"

generic_type ::= generic_head "[" type_args "]"

generic_head ::=
    "Vector" | "T81Vector" | "Matrix" | "Tensor"
  | "List"   | "Map"       | "Set"    | "Tree" | "Graph"
  | "Option" | "Result"
  | identifier              (* user generic *)

type_args ::= type_arg ( "," type_arg )*
type_arg  ::= type | integer_literal | identifier
```

## A.3 Expressions

```ebnf
expression ::= assignment

assignment ::= lvalue "=" assignment
             | logical_or

lvalue ::= identifier
         | lvalue "." identifier
         | lvalue "[" expression "]"

logical_or  ::= logical_and  ( "||" logical_and  )*
logical_and ::= bitwise_or   ( "&&" bitwise_or   )*
bitwise_or  ::= bitwise_xor  ( "|"  bitwise_xor  )*
bitwise_xor ::= bitwise_and  ( "^"  bitwise_and  )*
bitwise_and ::= range_expr   ( "&"  range_expr   )*
range_expr  ::= equality     ( ".." equality     )?
equality    ::= comparison   ( ( "==" | "!=" ) comparison )*
comparison  ::= shift        ( ( "<" | "<=" | ">" | ">=" ) shift )*
shift       ::= additive     ( ( "<<" | ">>" | ">>>" ) additive )*
additive    ::= multiplicative ( ( "+" | "-" ) multiplicative )*
multiplicative ::= exponent  ( ( "*" | "/" | "%" ) exponent )*
exponent    ::= unary        ( "**" exponent )?      (* right-associative *)

unary ::= ( "!" | "-" | "~" ) unary
        | call

call ::= primary ( "(" arguments? ")" | "[" expression "]" | "." identifier )*

primary ::= literal
          | identifier
          | "(" expression ")"
          | if_expression
          | block_expression
          | match_expression
          | record_literal
          | enum_literal

arguments ::= expression ( "," expression )*

literal ::= t81int_literal
          | integer_literal
          | t81float_literal
          | float_literal
          | string_literal
          | symbol_literal
          | "true"
          | "false"
          | vector_literal

vector_literal ::= "[" ( expression ( "," expression )* | expression ";" expression )? "]"

record_literal ::= identifier "{" field_init ( "," field_init )* ","? "}"
field_init     ::= identifier ":" expression

enum_literal   ::= identifier "." identifier ( "(" expression ")" )?

if_expression    ::= "if" "(" expression ")" block [ "else" ( block | if_expression ) ]
block_expression ::= block

match_expression ::= "match" "(" expression ")" "{" match_arm ( ( "," | ";" ) match_arm )* ( "," | ";" )? "}"
match_arm        ::= pattern [ "if" expression ] "=>" expression

pattern ::= "_"
          | identifier
          | identifier "(" pattern ")"
          | "{" field_pattern ( "," field_pattern )* "}"
          | "None"
          | "Some"  "(" pattern ")"
          | "Ok"    "(" pattern ")"
          | "Err"   "(" pattern ")"

field_pattern ::= identifier ":" identifier
```

## A.4 Statements

```ebnf
statement ::= let_declaration
            | var_declaration
            | return_statement
            | break_statement
            | continue_statement
            | if_statement
            | while_statement
            | for_statement
            | loop_statement
            | expression_statement
            | block

let_declaration  ::= "let" identifier ( ":" type )? "=" expression ";"
var_declaration  ::= "var" identifier ( ":" type )? ( "=" expression )? ";"
return_statement ::= "return" expression? ";"
break_statement  ::= "break" ";"
continue_statement ::= "continue" ";"

if_statement    ::= "if" "(" expression ")" block ( "else" ( block | if_statement ) )?
while_statement ::= "while" "(" expression ")" block
for_statement   ::= "for" identifier "in" expression block
loop_statement  ::= annotation* "loop" block

expression_statement ::= expression ";"

block ::= "{" statement* expression? "}"
```

## A.5 Top-Level Declarations

```ebnf
program ::= declaration*

declaration ::= function_declaration
              | type_declaration
              | record_declaration
              | enum_declaration
              | var_declaration
              | let_declaration
              | statement

annotation      ::= "@" identifier ( "(" annotation_args ")" )?
annotation_args ::= annotation_arg ( "," annotation_arg )*
annotation_arg  ::= ( identifier ":" )? expression

function_declaration ::=
    annotation*
    "fn" identifier ( "[" generic_params "]" )? "(" parameters? ")" ( "->" type )? block

type_declaration   ::= "type" identifier ( "[" generic_params "]" )? "=" type ";"

record_declaration ::=
    annotation*
    "record" identifier ( "[" generic_params "]" )? "{" ( identifier ":" type ";" )* "}"

enum_declaration   ::=
    annotation*
    "enum" identifier "{" ( identifier ( "(" type ")" )? ";" )* "}"

generic_params ::= identifier ( "," identifier )*
parameters     ::= parameter ( "," parameter )*
parameter      ::= identifier ":" type
```

## A.1.1 Operator Precedence (Normative)

Highest to lowest binding strength:

| Level | Operators | Associativity |
| :---- | :-------- | :------------ |
| 1 (highest) | `()` `[]` `.` (call, index, field) | Left |
| 2 | `!` `-` `~` (unary prefix) | Right |
| 3 | `**` (exponentiation) | Right |
| 4 | `*` `/` `%` | Left |
| 5 | `+` `-` | Left |
| 6 | `<<` `>>` `>>>` | Left |
| 7 | `<` `<=` `>` `>=` | Left |
| 8 | `==` `!=` | Left |
| 9 | `&` | Left |
| 10 | `..` (range) | None |
| 11 | `^` | Left |
| 12 | `\|` | Left |
| 13 | `&&` | Left |
| 14 | `\|\|` | Left |
| 15 (lowest) | `=` (assignment) | Right |

______________________________________________________________________
