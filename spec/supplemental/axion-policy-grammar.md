# Axion Policy Language (APL) Grammar & Verification Targets

**Status:** Draft
**Version:** 0.1.0

This document codifies the grammar for the Axion Policy Language (APL) and identifies the formal verification targets for the T81 core primitives.

## 1. APL Grammar Spec (S-Expression)

APL is a declarative, S-expression based DSL used to define safety and alignment constraints for the HanoiVM.

### 1.1. Core Predicates

```lisp
<policy>        ::= (policy <statement>*)
<statement>     ::= (tier <integer>)
                  | (max-instructions <integer>)
                  | (max-recursion <integer>)
                  | (max-stack <integer>)
                  | (allowed-tensor-hashes [<string>*])
                  | <requirement>
                  | <hint>

<requirement>   ::= (require-loop (file <string>) (line <integer>) (bound <integer>))
                  | (require-segment-event (action <action>) (segment <segment>))
                  | (require-alignment (reason <string>))
                  | (require-axion-event (reason <string>))
                  | (require-match-guard (variant <string>))

<hint>          ::= (hint-loop-unroll <integer>)
                  | (hint-parallel-tensor <integer>)

<action>        ::= "read" | "write" | "alloc" | "free"
<segment>       ::= "code" | "stack" | "heap" | "tensor" | "meta"
```

### 1.2. Verification Targets for Core Arithmetic

To ensure the integrity of the HanoiVM, the following core balanced ternary arithmetic primitives must be formally verified:

1.  **T81Int<N> Addition/Subtraction**:
    -   Prove that for all `a, b \in T81Int<N>`, `a + b` correctly handles carry/borrow and matches the mathematical definition of balanced ternary addition.
    -   Prove deterministic overflow behavior (trapping).
2.  **T81BigInt Multiplication**:
    -   Verify Karatsuba implementation against the baseline grade-school multiplication for arbitrary precision.
    -   Prove sign-magnitude normalization correctness.
3.  **Fraction Canonicalization**:
    -   Prove that for any `Fraction f`, `f.canonicalize()` results in a unique representation (GCD reduction and denominator sign normalization).
4.  **T81Float bit-identicality**:
    -   Verify that `T81Float` storage and basic operations (`+`, `-`, `*`) are side-channel-free and produce identical results across supported hardware backends.
    -   **Note:** Division and transcendentals are currently excluded from strict bit-identity verification due to host dependency.

## 2. Axion Predicates for Tier 4

Tier 4 promotion requires validation of the process's self-model.

```lisp
(require-self-model-integrity (hash <canon-hash-81>))
(require-reflection-cycle (count <integer>) (depth <integer>))
```
