# PDF Implementation Proposal

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PDF Implementation Proposal](#pdf-implementation-proposal)
  - [1. Priority: Recursive AGI Codex (TΩNARY)](#1-priority-recursive-agi-codex-tωnary)
    - [Proposed Actions:](#proposed-actions)
  - [2. Priority: Full Semantic Inference Engine](#2-priority-full-semantic-inference-engine)
    - [Proposed Actions:](#proposed-actions)
  - [3. Secondary: Higher-Order Base Systems](#3-secondary-higher-order-base-systems)
    - [Proposed Actions:](#proposed-actions)
  - [4. Maintenance & Optimization](#4-maintenance-&-optimization)

<!-- T81-TOC:END -->


This document outlines a proposal for future implementation work based on the aspirational concepts found in the `pdf/` archive, particularly the "Aspirational" documents that have not yet been fully realized in the C++ runtime.

## 1. Priority: Recursive AGI Codex (TΩNARY)

**Source:** `TΩNARY – T81Recursive AGI Codex.pdf`
**Current Status:** Skeleton implementation in `experimental/tiers/cog/tier5/` (Infinite Tier).

The "Codex" represents the self-modifying, recursive core of the AGI system. While the `Infinite` tier skeleton exists, it currently lacks the logic to meaningfully "collapse" infinite recursion into finite execution.

### Proposed Actions:
1.  **Implement Infinite Convergence Logic**:
    - Replace the placeholder `collapse()` in `experimental/tiers/cog/tier5/infinite.cpp` with actual convergence detection (e.g., detecting fixed points in recursive functional streams).
    - Implement `CollapseSignature` generation based on the structural hash of the recursive pattern, not just a dummy string.
2.  **Codex Persistence**:
    - Use **CanonFS** to store the "Codex" as a content-addressed Merkle DAG of `T81Symbol`s and `T81Tensor`s.
    - Implement a mechanism to "load" a Codex version into the VM as the active policy/kernel.
3.  **Self-Modification Primitives**:
    - Expose `Reflect` and `Rewrite` opcodes to the T81Lang runtime, allowing the Codex to modify its own source (AST) safely under Axion Policy constraints.

## 2. Priority: Full Semantic Inference Engine

**Source:** `TYRNARY - T81Analysis.pdf` (implied), `TANNARY` (applications).
**Current Status:** Basic graph traversal in `T81Graph`, but no logical inference.

The early documents describe a system capable of logical deduction over the ternary graph structure.

### Proposed Actions:
1.  **Ternary Prolog Engine**:
    - Implement a resolution engine that operates on `T81Graph` nodes.
    - Support ternary truth values (True, False, Unknown/Maybe) in logical queries.
2.  **Pattern Matching**:
    - Extend the `MatchExpr` logic in T81Lang to work against graph patterns, not just algebraic data types.

## 3. Secondary: Higher-Order Base Systems

**Source:** `HEXANARY` (T2187), `SEPTANARY` (T6561), `OCTANARY` (T19683).
**Current Status:** Aspirational / Unimplemented.

These documents describe systems based on higher powers of 3 ($3^7, 3^8, 3^9$). While `T729Tensor` ($3^6$) is implemented, going higher offers diminishing returns for general computing but may have niche cryptographic or large-integer uses.

### Proposed Actions:
1.  **Generic `TNaryTensor<Base>`**:
    - Instead of implementing `T2187Tensor`, `T6561Tensor`, etc., individually, refactor `T729Tensor` into a generic template `TNaryTensor<PowerOf3>` that can handle any valid power.
    - This would satisfy all "Aspirational" type documents with a single generalized implementation.
2.  **BigInt Optimization**:
    - Evaluate if `T81BigInt` can utilize Base-2187 or Base-19683 limbs internally for faster large-integer arithmetic (reducing the number of limbs).

## 4. Maintenance & Optimization

1.  **T729Tensor Optimization**:
    - Verify SIMD usage in `T729Tensor` operations.
    - Ensure `T81Float` <-> `T729Tensor` conversions are lossless where expected.
2.  **Base-243 Codec Integration**:
    - Ensure the `Base243` codec is available to the `TLOADHASH` opcode for efficient loading of compressed tensor data from CanonFS.

---
**Next Step Recommendation:**
Focus on **Priority 1 (Recursive AGI Codex)** as it differentiates the T81 architecture from standard tensor frameworks and aligns with the existing Tier 5 skeleton work.
