______________________________________________________________________

title: "RFC-0012 — TernaryTensor Type and Balanced-Trit Quantization"
version: Draft
applies_to:

- T81Lang Specification
- T81 Data Types Specification
- TISC Specification

______________________________________________________________________

# Summary

This RFC proposes the introduction of two core, AI-native features into T81Lang:

1.  A new composite type, `TernaryTensor[T, Rank]`, designed for highly efficient storage of neural network weights.
2.  A new language expression, `quantize(expr as type)`, for converting standard numeric types into balanced ternary digits (`Trit`).

These features provide the foundational primitives for building and representing ternary neural networks directly in the language.

# Motivation

The T81 Ecosystem is explicitly designed for AI workloads, which benefit significantly from low-precision, high-density data representations. Balanced ternary weights (-1, 0, +1) offer a powerful combination of model compactness and computational efficiency.

To make this a first-class feature, T81Lang needs:
1.  **A specialized data structure:** Standard `Tensor` types are not optimized for the unique packing strategies of ternary data. A dedicated `TernaryTensor` is required to signal intent to the compiler and VM, enabling aggressive, hardware-specific optimizations (e.g., trit-packed SIMD operations).
2.  **An explicit conversion mechanism:** The process of converting high-precision numbers (like `T81Float`) into low-precision trits is a critical step in the AI pipeline known as quantization. A built-in `quantize` expression makes this a safe, deterministic, and visible part of the language, rather than an ad-hoc library function.

# Guide-level explanation

This RFC adds a new type and a new expression.

First, a new primitive type `Trit` is introduced. It represents a balanced ternary digit with a value of -1, 0, or 1.

Second, a new composite type `TernaryTensor` is added. It's like a regular `Tensor`, but it's specifically designed to hold `Trit` values in a highly compressed format. This is perfect for storing the weights of a neural network.

Third, a new `quantize` expression allows you to convert regular numbers into these trits. For example, you could take a `T81Float` like `-0.8` and quantize it to a `Trit` value of `-1`.

Here is a simple example of how these features would be used to create a tensor of ternary weights:

```t81
// Start with a vector of regular floating-point numbers
let float_weights = [-0.8t81, 0.1t81, 0.9t81];

// Use the quantize expression to convert them to a TernaryTensor of Trits
let ternary_weights = quantize(float_weights as TernaryTensor[Trit, 3]);

// ternary_weights now contains the canonical trit values: [-1t81, 0t81, 1t81]
```

# Reference-level implementation

## 1. Specification Changes

The following changes would be made to the `t81lang-spec.md` upon acceptance of this RFC.

### A. Grammar (`§1 Core Grammar`)

The `factor` production in the grammar will be extended to include `quantize_expr`:

```ebnf
factor ::= literal
         | identifier
         | fn_call
         | unary_op factor
         | paren_expr
         | quantize_expr  // New

quantize_expr ::= "quantize(" expr "as" type ")"
```

### B. Type System (`§2 Type System`)

#### New Primitive Type (`§2.2 Primitive Types`)

A new primitive type `Trit` will be added:
-   **Trit**: Represents a balanced ternary digit. It is a subtype of `T81Int` but its canonical value space is restricted to {-1, 0, 1}.

#### New Composite Type (`§2.4 Composite Types`)

A new composite type `TernaryTensor` will be added:
-   **TernaryTensor[T, Rank]**: A tensor variant optimized for storing trits. When `T` is `Trit`, the compiler and VM will use a packed memory layout (e.g., base-81 grouping, achieving ~2.63 bits per trit).

#### New Canonicalization Rule (`§2.6 Canonicalization Rules`)

-   Values produced by a `quantize` expression must be canonicalized to the balanced ternary range {-1, 0, 1}.

### C. Compilation Pipeline (`§5 Compilation Pipeline`)

#### Type Checking (`§5.3 Type Checking`)

-   The `quantize(expr as type)` expression is valid only if `expr` is of a numeric type (`T81Int`, `T81Float`, `T81Fraction`) or a `Vector`/`Tensor` of numeric types, and `type` is `Trit` or a `TernaryTensor` of `Trit`.
-   The result of a valid `quantize` expression is the type specified after `as`.

#### TISC Lowering (`§5.7 TISC Lowering`)

A new row will be added to the lowering table:

| IR Construct | TISC Output |
| --- | --- |
| `quantize(expr)` | `TQUANTIZE_BALANCED`, canonicalize to -1/0/1 |

The `TQUANTIZE_BALANCED` opcode will perform the deterministic conversion from a numeric value to a balanced trit.

## 2. T81 Data Types Specification Changes

The `t81-data-types.md` spec will be updated to include `Trit` as a canonical primitive and `TernaryTensor` as a canonical composite type, specifying its memory layout and packing strategy.

# Drawbacks

-   This adds a new expression and two new types to the language, slightly increasing its complexity. However, the complexity is justified by the significant performance and memory benefits for the target AI domain.

# Rationale and alternatives

-   **Why not use a library function?** A built-in `quantize` expression makes the operation visible to the compiler and Axion. This allows for powerful, deterministic optimizations and safety checks that would not be possible with an opaque function call.
-   **Why a new `TernaryTensor` type?** A distinct type allows the type system to enforce that only trit-packed data is used in ternary-specific operations (like the future `TMATMUL_TRIT`). It also provides a clear signal to the VM's memory manager to use a specialized, compressed memory layout.

# Future Possibilities

-   This RFC lays the groundwork for `RFC-0013`, which will introduce a ternary-optimized matrix multiplication operator (`**`) that operates on `TernaryTensor`.
-   Future work could explore different quantization strategies (e.g., asymmetric, affine) by extending the `quantize` syntax.

# Open Questions

1.  What is the precise, bit-for-bit canonical memory layout for a packed `TernaryTensor`? This will need to be formally defined in the `t81-data-types.md` and `t81vm-spec.md`.
2.  What is the exact rounding algorithm for the `TQUANTIZE_BALANCED` TISC opcode (e.g., round-half-to-even, round-half-up)? This must be specified to ensure determinism.
