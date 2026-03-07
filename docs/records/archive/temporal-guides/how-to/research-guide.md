# T81 Foundation: Researcher's Guide

This document explores the mathematical foundations, cognitive models, and formal safety mechanisms that define the T81 Ecosystem. It is intended for researchers in computer science, mathematics, and AI safety.

______________________________________________________________________

## 1. Balanced Ternary Mathematics

The T81 stack is built fundamentally on **balanced ternary** arithmetic. Unlike binary (base-2) or standard ternary (base-3 with digits 0, 1, 2), balanced ternary uses the digits (trits) **-1, 0, and 1** (often represented as `-`, `0`, `+`).

### 1.1 Advantages for Cognition
- **Symmetry:** Balanced ternary is inherently symmetric around zero. This eliminates the need for a separate "sign bit" and simplifies many arithmetic operations.
- **Rounding:** Truncation in balanced ternary is equivalent to rounding to the nearest integer, reducing cumulative errors in large-scale tensor operations.
- **Efficiency:** Base 3 is mathematically closer to *e* (the most efficient radix for number representation) than base 2.

### 1.2 Canonical Data Types
T81 implements **90 canonical data types** (the "TISC-90" set) that ensure every value—from a single trit to a high-rank tensor—has a unique, deterministic binary representation.

______________________________________________________________________

## 2. Cognitive Tiers of Execution

T81 organizes computation into five distinct **Cognitive Tiers**, defined in `include/t81/cog/tier.hpp`. This model allows the Axion safety layer to scale its scrutiny based on the complexity and "agency" of the code.

| Tier | Name | Description | Axion Enforcement |
| :--- | :--- | :--- | :--- |
| **1** | **Deterministic** | Pure arithmetic and fixed control flow. | Basic instruction counting. |
| **2** | **Managed** | Dynamic memory and bounded recursion. | Segment-trace validation (RFC-0020). |
| **3** | **Reflective** | Code that inspects its own state/trace. | Match-guard requirements (RFC-0019). |
| **4** | **Self-Referential** | High-tier cognitive loops (`Tier4Loop`). | Alignment-clause verification. |
| **5** | **Agentic** | Full goal-directed agency and alignment. | Strict `require-alignment` policies. |

______________________________________________________________________

## 3. Axion: A Formal Safety Kernel

Axion is not just a monitor; it is a **formal safety kernel** that treats program execution as a sequence of provable transitions.

### 3.1 Trace-Based Verification
Every HanoiVM operation emits an `AxionEvent`. These events are collected into a deterministic trace that the **Policy Engine** evaluates against a set of S-expression predicates.

### 3.2 The Policy DSL
Policies are authored in a domain-specific language that allows researchers to specify exactly which events must occur before a privileged operation (like a Tier 4 loop) is allowed to proceed.

```lisp
(policy
  (tier 4)
  (require-alignment (reason "self-referential loop verified")))
```

______________________________________________________________________

## 4. Deterministic AI & Llama Integration

T81 enables **Bit-Identical Reproducibility** for large language models. By implementing transformer kernels (RMSNorm, RoPE, Softmax) directly in balanced ternary-native C++, we ensure that a model like Llama-3.2 produces the exact same output on every architecture, from x86_64 to ARM SVE.

Researchers can use the ` DistributedT81Tensor` and `HanoiVM` to explore how ternary numerics affect the convergence and safety characteristics of advanced AI models.

______________________________________________________________________

## 5. Further Research Areas
- **Ternary Hardware Synthesis:** Mapping HanoiVM to physical ternary logic gates.
- **Formal Proof of Determinism:** Using the `t81-verify` harness (RFC-0008) to formally prove that TISC lowering is semantics-preserving.
- **Tier 5 Alignment Strategies:** Developing robust `require-alignment` clauses for autonomous agentic systems.
