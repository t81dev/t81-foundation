# T81 Foundation — RFC Index
Version 0.2

The T81 Foundation uses **RFCs (Request for Comments)** to propose, discuss, and standardize extensions or changes to the architecture.  
RFCs complement the main specification suite by capturing:

- new design proposals
- cross-layer improvements
- philosophical foundations
- safety and determinism frameworks
- formal extensions and experimental features

All RFCs follow the standard structure:

- RFC-NNNN — Title  
- Version — Draft / Proposal / Accepted  
- Applies To — affected layers  
- Summary  
- Motivation  
- Design / Specification  
- Rationale  
- Backwards Compatibility  
- Security Considerations  
- Open Questions  

---

# 1. Core RFCs (Foundational)

These RFCs define the **fundamental principles** of the T81 ecosystem.

### [RFC-0000](RFC-0000-t81-base-81-ternary-computing-stack.md)
**Title:** T81: Base-81 Ternary Computing Stack  
Defines a post-binary computing model leveraging balanced ternary (−1, 0, +1) and Base-81 encodings to maximize information density and determinism for AI-centric workloads.

### [RFC-0001](RFC-0001-architecture-principles.md)
**Title:** T81 Architecture Principles  
Defines the philosophical and engineering motivations behind T81: determinism, canonical semantics, tiered reasoning, and ternary-native computation.

### [RFC-0002](RFC-0002-deterministic-execution-contract.md)
**Title:** Deterministic Execution Contract (DEC)  
Defines the cross-layer determinism invariants that govern Data Types, TISC, VM, Lang, Axion, and Cognitive Tiers.

### [RFC-0003](RFC-0003-axion-safety-model.md)
**Title:** Axion Safety Model  
Defines Axion’s supervisory role, safety constraints, privilege model, recursion control, cognitive limits, and fault semantics.

### [RFC-0004](RFC-0004-canonical-tensor-semantics.md)
**Title:** Canonical Tensor Semantics  
Formalizes tensor shape/rank rules, tensor pools, and Axion-visible metadata so every layer treats tensors identically.

### [RFC-0005](RFC-0005-tisc-v0-4-extensions.md)
**Title:** TISC v0.4 Extensions  
Specifies the next ISA revision, including structural constructors, vector helpers, shape guards, and version-reporting opcodes.

### [RFC-0006](RFC-0006-deterministic-gc.md)
**Title:** Deterministic GC & Memory Reclamation  
Defines the deterministic garbage collector (mark/sweep order, safepoints, Axion hooks) required by the VM spec.

### [RFC-0007](RFC-0007-t81lang-standard-library.md)
**Title:** T81Lang Standard Library  
Introduces the canonical, versioned standard library modules for deterministic arithmetic, tensor helpers, Option/Result combinators, and Axion-safe I/O.

### [RFC-0008](RFC-0008-formal-verification-harness.md)
**Title:** T81 Formal Verification Harness  
Describes the canonical trace format, reference replayer, and SMT interface used to prove determinism end-to-end.

### [RFC-0009](RFC-0009-axion-policy-language.md)
**Title:** Axion Policy Language  
Defines the declarative DSL that encodes recursion/shape/opcode policies enforced by Axion at runtime.

### [RFC-0010](RFC-0010-tisc-float-fraction-ops.md)
**Title:** TISC Float and Fraction Operations  
Specifies the TISC opcodes and semantics for `T81Float` and `T81Fraction` arithmetic, ensuring deterministic handling of non-integer types.

### [RFC-0011](RFC-0011-t81lang-grammar-update.md)
**Title:** T81Lang Grammar Modernization  
Proposes the adoption of the advanced EBNF grammar from legacy sources to serve as the new baseline for the T81Lang C++ toolchain.

### [RFC-0012](RFC-0012-ternary-tensor-quantization.md)
**Title:** TernaryTensor Type and Balanced-Trit Quantization  
Introduces AI-native types (`Trit`, `TernaryTensor`) and a `quantize` expression for efficient, deterministic handling of neural network weights.

### [RFC-0013](RFC-0013-ternary-matmul.md)
**Title:** Ternary Matrix Multiply (`**`) and Trit-Packed Lowering  
Proposes the `**` operator for efficient, ternary-native matrix multiplication on `TernaryTensor` types.

### [RFC-0014](RFC-0014-neural-primitives.md)
**Title:** Neural Forward and Training Primitives  
Introduces high-level `train` and `infer` statements for safe, deterministic, and optimizable neural network operations.

### [RFC-0015](RFC-0015-agentic-constructs.md)
**Title:** First-Class Agents and Tiered Recursive Cognition  
Proposes the `agent` construct for encapsulating state and behaviors, enabling tiered cognition and recursive AI systems.

---

# 2. Upcoming RFCs (Planned)

The following numbers are reserved for future proposals that build on the
foundation above:

- **RFC-0100+** — experimental items listed in Section 3 below

---

# 3. Experimental RFCs (Optional Future)

These RFCs capture long-term or high-concept ideas that may graduate into standards.

- **RFC-0100 — Symbolic Reasoning Optimizer**
- **RFC-0101 — Deterministic Parallel Tensor Engine**
- **RFC-0102 — Tier-5 Reflective Metacognition**
- **RFC-0103 — Verified Transformations for T81Lang**
- **RFC-0104 — Cross-Language Canonical VM Target**
- **RFC-0105 — Axion Coherence Index & Cognitive Stability Metrics**

---

# 4. Contributing to RFCs

To propose an RFC:

1. Fork the repository.
2. Create a file under `rfcs/RFC-NNNN-title.md`.
3. Use the standard RFC template.
4. Submit a Pull Request.
5. Discuss the proposal in Issues and the PR thread.
6. Once accepted, move the RFC to the “Core” section.

RFC numbering is global and monotonic.

---

# 5. Status Levels

RFCs exist in one of the following states:

- **Draft** — Not finalized.
- **Proposal** — Open for community review.
- **Accepted** — Approved for inclusion in standards.
- **Integrated** — Fully specified and merged.
- **Superseded** — Replaced by a newer RFC.

---

# 6. Cross-References

- Main Spec Index → `../spec/index.md`
- TISC Spec → `../spec/tisc-spec.md`
- T81VM Spec → `../spec/t81vm-spec.md`
- Axion Kernel Spec → `../spec/axion-kernel.md`
- Cognitive Tiers → `../spec/cognitive-tiers.md`

---

# 7. Conclusion

This RFC Index serves as the authoritative directory for all active, planned, and future RFCs in the T81 Foundation.  
It unifies the proposal process, improves traceability, and provides a clear roadmap for the evolution of the architecture.
