______________________________________________________________________

title: "RFC-0014 — Neural Forward and Training Primitives"
version: Draft
applies_to:

- T81Lang Specification
- TISC Specification
- Axion Kernel Specification

______________________________________________________________________

# Summary

This RFC proposes the introduction of high-level, AI-specific constructs into T81Lang: an `infer` expression and a `train` statement. These constructs provide a formal syntax for the two primary modes of neural network operation: performing a forward pass (inference) and updating weights (training). These will lower to new, specialized TISC opcode sequences and introduce new hooks for Axion to monitor and regulate AI workloads.

# Motivation

While `RFC-0012` and `RFC-0013` provide the data types and mathematical operations for building neural networks, the control flow for training and inference is still implicit. To make T81Lang a truly AI-native language, it needs first-class constructs for these core tasks.

1.  **Safety and Determinism:** A dedicated `train` block allows the compiler and Axion to enforce that weight mutations (gradient updates) are effectful, deterministic, and contained. An `infer` expression makes it clear that a pure, deterministic forward pass is being performed.
2.  **Compiler Optimization:** Explicit `train` and `infer` constructs provide powerful hints to the compiler. For example, a `train` block can be unrolled by the compiler or JIT for batch processing efficiency, and an `infer` expression can be heavily optimized as a pure function.
3.  **Axion Integration:** These constructs create clear boundaries for Axion to apply policies. Axion can monitor loss entropy within a `train` block, verify the purity of an `infer` call, and apply tier-based complexity limits to the underlying operations.

# Guide-level explanation

This RFC introduces a new expression and a new statement for working with neural networks.

First, the `infer` expression is for performing inference—that is, running a neural network forward to get a prediction. It takes a network and an input, and returns the output. It is treated as a pure expression, meaning it has no side effects and produces a value that can be assigned.

```t81
// net is a neural network model
// input_data is a TernaryTensor
let prediction = infer net(input_data) -> TernaryTensor[Trit, 10];
```

Second, the `train` statement is for training a network. It defines a block of code where weight updates (gradient descent) can happen. This is explicitly marked as an effectful operation.

```t81
// net is a neural network model
// training_data is a batch of inputs and labels
train net(training_data) {
  // ... gradient calculation and weight updates ...
  // This block is effectful and monitored by Axion.
}
```

These constructs make the intent of the code much clearer and safer.

# Reference-level implementation

## 1. Specification Changes

The following changes would be made to the `t81lang-spec.md` upon acceptance of this RFC.

### A. Grammar (`§1 Core Grammar`)

The `statement` production will be extended to include `train_stmt`. The `primary` production from `RFC-0013` will be extended to include the new `infer_expr`.

```ebnf
statement     ::= let_decl
                | assign
                | return
                | if_stmt
                | loop_stmt
                | train_stmt   // New
                | expr ";"

train_stmt  ::= "train" identifier "(" [ expr { "," expr } ] ")" block

primary       ::= literal
                | identifier
                | fn_call
                | paren_expr
                | quantize_expr
                | infer_expr   // New

infer_expr    ::= "infer" identifier "(" expr ")" "->" type
```

This grammar is now fully consistent with the expression hierarchy defined in `RFC-0013`.

### B. Purity and Effects (`§3 Purity and Effects`)

-   **`infer` expressions are pure.** The compiler must verify that the underlying network call performs a pure forward pass.
-   **`train` blocks are effectful.** Any operations within a `train` block that mutate neural network weights are considered effectful and must be flagged as such.

### C. Compilation Pipeline (`§5 Compilation Pipeline`)

#### Type Checking (`§5.3 Type Checking`)

-   The `infer` expression requires that the first identifier resolves to a neural network type and that the expression in parentheses is a valid input tensor for that network. The return type must match the network's output signature. The type of the entire expression is the type specified after `->`.
-   The `train` statement requires that the first identifier resolves to a neural network type and the arguments are valid training data (e.g., inputs and labels).

#### TISC Lowering (`§5.7 TISC Lowering`)

New rows will be added to the lowering table:

| IR Construct | TISC Output |
| --- | --- |
| `train(net, data)` | `TNEURAL_BWD` loop with gradient accumulation |
| `infer(net, input)` | `TNEURAL_FWD` with trit activation |

The `TNEURAL_FWD` and `TNEURAL_BWD` opcodes are new, high-level TISC instructions that encapsulate the complex control flow of neural network forward and backward passes, respectively.

### D. Axion Integration (`§7 Axion Integration`)

-   Axion will gain new hooks to observe `train` and `infer` operations.
-   `AX_TRAIN_BEGIN`/`AX_TRAIN_END`: These hooks will allow Axion to monitor weight updates, loss entropy, and gradient stability within a training loop.
-   `AX_INFER_BEGIN`/`AX_INFER_END`: These hooks allow Axion to verify the purity and determinism of an inference call.

# Drawbacks

-   These are highly domain-specific constructs, which adds complexity to the core language. However, T81Lang is explicitly designed for AI, making this a justified addition.

# Rationale and alternatives

-   **Why not use functions like `train(...)` and `infer(...)`?** Standard function calls do not provide the same level of compiler and Axion visibility. A dedicated `infer` expression and `train` statement allow for special purity rules (`infer` is pure, `train` is effectful) and unique control flow (a `train` *block*). This distinction is fundamental to the safety and optimization goals of T81Lang.
-   **Grammar Consistency:** The `train_stmt` grammar intentionally omits the trailing semicolon that was present in the original proposal. This is to ensure that its syntax is consistent with other block-based statements in the language, such as `if_stmt` and `loop_stmt`, which do not have a semicolon after the closing brace. This makes the language grammar more regular and easier to parse.

# Future Possibilities

-   This RFC completes the foundational set of AI primitives. The next logical step is `RFC-0015`, which will introduce `agent` declarations, a higher-level abstraction for bundling data (weights) and behavior (inference, training) into a single cognitive unit.

# Open Questions

1.  What is the precise API for the `training_data` passed to a `train` block? Will it be a tuple of `(inputs, labels)`, a stream, or another structure? This needs to be defined in the standard library RFC.
2.  How will Axion's policies for monitoring training (e.g., "stop if loss entropy exceeds X") be formally specified? This will likely require an extension to the Axion Policy Language (`RFC-0009`).
