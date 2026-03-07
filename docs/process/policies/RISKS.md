# Risk Register

> **Source of Truth:** This document tracks **known technical risks** and open questions. It signals transparency and intellectual honesty.

**Last Updated:** February 10, 2026

## 1. Technical Risks

| Risk | Impact | Mitigation Strategy |
| :--- | :--- | :--- |
| **JIT Compilation Determinism** | High | Trace-JIT optimization could introduce non-deterministic behavior. | Strict Axion policy checks on JIT output; cross-architecture fuzzing. |
| **Floating Point Consistency** | Medium | Different hardware FPUs may yield bit-level differences. | Use soft-float emulation or strict IEEE 754 compliance modes where necessary; T81Float uses integer-based representation. |
| **Performance Overhead** | Medium | Determinism checks (Axion) add runtime overhead. | Focus on "Auditability First"; optimize hot paths carefully; provide "Unsafe" modes (future) with clear warnings. |
| **Ecosystem Fragmentation** | Low | Forked runtimes diverging from spec. | Strong Runtime Contract (`contracts/runtime-contract.json`) and conformance suite. |

## 2. Open Questions

- **Distributed Determinism:** How to guarantee bit-exact replay across a distributed cluster of nodes?
- **Formal Verification Scope:** Which parts of the VM are critical for formal proofs (e.g., policy engine vs. arithmetic)?
- **Hardware Acceleration:** Can we offload to GPUs/TPUs without losing deterministic guarantees?

## 3. Mitigation Status

- **Axion Policy Engine:** Active. Catching runtime deviations.
- **CI Reproducibility Gates:** Active. Verifying compiler and runtime determinism.
- **Spec Governance:** Active. RFC process for all changes.
