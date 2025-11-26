______________________________________________________________________

title: "RFC-0013 — Ternary Matrix Multiply (`**`) and Trit-Packed Lowering"
version: Draft
applies_to:

- T81Lang Specification
- TISC Specification

______________________________________________________________________

# Summary

This RFC builds directly upon `RFC-0012` by proposing the introduction of a dedicated binary operator, `**`, for ternary-optimized matrix multiplication. This operator will be specifically designed to work with the `TernaryTensor` type and will lower to a new, highly-efficient `TMATMUL_TRIT` TISC opcode.

# Motivation

The primary operation in most neural network forward passes is matrix multiplication. While T81Lang has a generic `TMATMUL` opcode for standard tensors, it is not optimized for the unique properties of balanced ternary data.

To fully leverage the `TernaryTensor` type introduced in `RFC-0012`, the language needs a corresponding computational primitive. A dedicated `**` operator provides a clear, high-level syntax for this critical operation. This allows the compiler to:
1.  Enforce at compile-time that this operation is only used on compatible `TernaryTensor` types.
2.  Lower the operation to a specialized `TMATMUL_TRIT` TISC opcode that can execute significantly faster by using trit-packed SIMD instructions and avoiding generic multiplication hardware.

# Guide-level explanation

This RFC introduces a new operator, `**`, for performing matrix multiplication on the `TernaryTensor` types defined in the previous RFC. This operator is intended to be the workhorse for neural network calculations.

It has a higher precedence than the standard multiplication operator (`*`), reflecting its common use in scientific and AI computing.

Here is a simple example that builds on `RFC-0012`:

```t81
// An input vector, quantized to a TernaryTensor of Trits
let input = quantize([1, 0, -1, 1] as TernaryTensor[Trit, 4]);

// A weight matrix, also a TernaryTensor
let weights = quantize([
  [1, -1, 0],
  [0, 1, 1],
  [-1, 0, 1],
  [1, 1, 0]
] as TernaryTensor[Trit, 4, 3]);

// Use the new operator to perform the ternary matrix multiply
let output = input ** weights;

// output is now a TernaryTensor[Trit, 3] containing the result
```

# Reference-level implementation

## 1. Specification Changes

The following changes would be made to the `t81lang-spec.md` upon acceptance of this RFC.

### A. Grammar (`§1 Core Grammar`)

The `factor` production in the grammar will be modified to include `matmul_expr`, and a new `matmul_expr` production will be added. The `**` operator will have a higher precedence than `*`, `/`, and `%`.

```ebnf
term             ::= factor { ("*" | "/" | "%") factor }
factor           ::= unary { "**" unary }*   // Modified to use unary as base
unary            ::= literal
                   | identifier
                   | fn_call
                   | unary_op unary
                   | paren_expr
                   | quantize_expr
```
*(Note: A more complete grammar refactoring would place `**` in its own precedence level between `factor` and `unary`, but for this simplified grammar, this illustrates the intended precedence.)*

### B. Compilation Pipeline (`§5 Compilation Pipeline`)

#### Type Checking (`§5.3 Type Checking`)

-   The expression `a ** b` is valid only if both `a` and `b` are of type `TernaryTensor[Trit, Rank]`.
-   The shapes of the two tensors must be compatible for matrix multiplication (i.e., the inner dimensions must match).
-   The resulting type is a `TernaryTensor[Trit, Rank]` with the appropriate output shape.

#### TISC Lowering (`§5.7 TISC Lowering`)

A new row will be added to the lowering table:

| IR Construct | TISC Output |
| --- | --- |
| Ternary matmul `a ** b` | `TLOAD_TRIT`, `TMATMUL_TRIT` (trit-packed) |

The `TMATMUL_TRIT` opcode is a new, specialized instruction that performs matrix multiplication directly on trit-packed data, leveraging the VM's ternary ALU.

# Drawbacks

-   Introducing a new binary operator adds a small amount of complexity to the parser and the language specification. However, `**` is a standard operator for this purpose in many scientific computing languages (like Python), making it familiar to the target audience.

# Rationale and alternatives

-   **Why not overload the `*` operator?** Overloading `*` for both element-wise multiplication and matrix multiplication can lead to ambiguity. A dedicated `**` operator provides a clear and explicit signal of the programmer's intent, which is crucial for a deterministic language.
-   **Why not use a function like `matmul(a, b)`?** While a function is a viable alternative, a dedicated operator is more ergonomic and idiomatic for a core mathematical operation. It aligns T81Lang with best practices from other languages in the AI domain. The operator also provides a stronger hint to the compiler for optimization than a generic function call.

# Future Possibilities

-   This RFC, combined with `RFC-0012`, provides the core components for building neural network layers. This enables `RFC-0014`, which will introduce higher-level constructs like `train` and `infer` blocks.

# Open Questions

1.  What are the precise performance characteristics and cycle counts for the `TMATMUL_TRIT` opcode on the reference T81VM? This will need to be benchmarked and documented.
2.  Should the `**` operator be extended to handle `TernaryTensor` x `Tensor` multiplication in the future, and if so, what would the promotion and conversion rules be? (For this RFC, the scope is strictly `TernaryTensor` x `TernaryTensor`).
