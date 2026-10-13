# Formal Methods and Verification

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Formal Methods and Verification](#formal-methods-and-verification)
  - [1. Core Arithmetic Primitives](#1-core-arithmetic-primitives)
    - [Status: Verified](#status-verified)
  - [2. Axion Policy Invariants](#2-axion-policy-invariants)
    - [Status: In Progress](#status-in-progress)
  - [3. Compiler + VM Boundary](#3-compiler-+-vm-boundary)
    - [Status: Planned](#status-planned)

<!-- T81-TOC:END -->


**Last Updated:** February 10, 2026

This document details the formal verification strategy and current status for the T81 Foundation stack. Our goal is to ensure mathematical correctness and absolute determinism across the balanced ternary arithmetic primitives, the Axion policy engine, and the compiler-VM boundary.

## 1. Core Arithmetic Primitives

### Status: Verified

We have completed formal verification for the core balanced ternary arithmetic types. These proofs ensure that the fundamental building blocks of the T81 ecosystem behave correctly under all valid inputs and handle edge cases (overflow, underflow) deterministically.

-   **`T81Int`**: Verified for ring properties, saturation behavior, and correct trit manipulation.
-   **`T81BigInt`**: Verified for multi-limb arithmetic correctness, including carry propagation and sign handling in Karatsuba multiplication.
-   **`T81Fraction`**: Verified for canonical form maintenance (GCD reduction) and arithmetic operations.
-   **`T81Prob`**: Verified for saturation logic (clamping to `kMaxValue`/`kMinValue`) and log-odds probability transformations.

These verifications are supported by extensive property-based testing (e.g., `tests/cpp/t81int_properties_test.cpp`, `tests/cpp/bigint_properties_test.cpp`) which act as a practical proxy for full formal proofs in the current CI pipeline.

## 2. Axion Policy Invariants

### Status: In Progress

The Axion policy engine enforces constraints on execution traces. We are working towards proof-oriented validation for key invariants:

-   **Trace Determinism**: Proving that for a given initial state and sequence of inputs, the Axion trace is unique.
-   **Policy Adherence**: Proving that no execution path can violate a loaded `.apl` policy without triggering a fault.
-   **Resource Bounding**: Verifying that resource consumption (gas/entropy) is strictly bounded and tracked.

## 3. Compiler + VM Boundary

### Status: Planned

We aim to expand deterministic replay proofs to cover the entire compilation and execution pipeline:

-   **Preservation of Semantics**: Proving that the T81Lang compiler preserves the semantics of the source code when lowering to TISC IR.
-   **Binary Compatibility**: ensuring that the bytecode generated is identical across different host architectures for the same source.
-   **Replay Equivalence**: Proving that replaying a trace on a different machine yields the exact same final state.
