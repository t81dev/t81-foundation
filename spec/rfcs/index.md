# T81 RFC Index

This index tracks RFC status and intended disposition.

## Status Legend

- `draft`: active authoring
- `proposed`: under formal review
- `accepted`: design accepted, pending/ongoing integration
- `integrated`: merged into normative specs
- `superseded`: replaced by a newer RFC
- `rejected`: closed without adoption

## RFC Catalog

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-0000 | T81 Base-81 Ternary Computing Stack | draft | Foundational umbrella document |
| RFC-0001 | Architecture Principles | draft | Candidate for partial integration into `t81-overview.md` |
| RFC-0002 | Deterministic Execution Contract | draft | Cross-layer invariants |
| RFC-0003 | Axion Safety Model | draft | Axion threat/safety model |
| RFC-0004 | Canonical Tensor Semantics | draft | Tensor semantics harmonization |
| RFC-0005 | TISC v0.4 Extensions | draft | ISA extension planning |
| RFC-0006 | Deterministic GC | draft | VM reclaim semantics |
| RFC-0007 | T81Lang Standard Library | draft | Stdlib contract definition |
| RFC-0008 | Formal Verification Harness | superseded | Superseded by RFC-0027 (spec-as-executable is the concrete realization) |
| RFC-0009 | Axion Policy Language (APL) | draft | Superseded path by RFC-0022 once accepted |
| RFC-0010 | TISC Float/Fraction Ops | accepted | Accepted with caveat: host `double` path for `FDIV`/transcendentals is not yet cross-arch bit-deterministic |
| RFC-0011 | T81Lang Grammar Modernization | draft | Grammar evolution proposal |
| RFC-0012 | Ternary Tensor Quantization | superseded | Superseded by RFC-0026 (QMATMUL covers quantized matmul path) |
| RFC-0013 | Ternary Matmul | superseded | Superseded by RFC-0026 (QMATMUL) |
| RFC-0014 | Neural Primitives | superseded | Superseded by RFC-0026 (ATTN + EMBED cover the neural primitive surface) |
| RFC-0015 | Agentic Constructs | draft | First-class agent model |
| RFC-0016 | SIMD Limb | proposed | SIMD arithmetic proposal |
| RFC-0017 | Introduce T81 Native | proposed | Register-native type proposal |
| RFC-0018 | T81 Native SIMD Arithmetic | proposed | Follow-on SIMD arithmetic details |
| RFC-0019 | Axion Match Logging | accepted | Runtime/spec/CLI depend on canonical match metadata and guard-audit strings |
| RFC-0020 | Axion Segment Trace | accepted | Runtime/spec/CLI depend on canonical segment-trace strings |
| RFC-0021 | Tier4 Cognition | draft | Tier-4 reflection/cognition proposal |
| RFC-0022 | Axion Policy Language Evolution | accepted | CLI/compiler/runtime policy surface is active; supersedes RFC-0009 in practice |
| RFC-0023 | T81Lang Print Canonical Runtime | draft | Deterministic print/runtime surface |
| RFC-0024 | C++23 Wording Alignment | draft | Documentation/process wording alignment |
| RFC-0025 | Policy-Gated Tensor Loading via CanonFS | accepted | `TLOADHASH` + `allowed-tensor-hashes` are active; operational hardening continues |
| RFC-0026 | AI-Native Inference Opcodes | accepted | Phase-1 opcode surface is implemented; remaining follow-on work is narrow `WLOAD` promotion review plus RFC-0030 float-domain policy |
| RFC-0027 | Spec-as-Executable Conformance Model | accepted | Conformance suite is wired into CMake/CTest; optional annotation follow-ons remain |
| RFC-0028 | Deterministic Trace JIT | draft | Trace-JIT deterministic execution model |
| RFC-0029 | T81Lang Feature Registry Drift Prevention | draft | Feature registry consistency mechanisms |
| RFC-0030 | Deterministic Math Subsystem | draft | Canonical arithmetic operations |
| RFC-0031 | Deterministic AI Execution Contract | draft | Composes RFC-0002/0003/0004/0025/0026/00A0 into a single AI execution contract |
| RFC-0032 | AI Subsystem Promotion Pathway | proposed | Normative promotion specification for `experiments/ai/` components; 5-phase roadmap |
| RFC-00B0 | Axion HAL Specification | accepted | First non-hosted promotion path and HAL contract for the Axion OS stack |
| RFC-00B1 | Ternary MMU | accepted | TVA layout, radix page table, and MMU fault model for Axion |
| RFC-00B2 | Device Driver Architecture | accepted | Phase 4 storage/display/network boundary and guest-device model |
| RFC-00B3 | Axion Kernel Architecture | draft | Kernel boundary and integration path after subsystem bring-up |

## Experimental RFCs (A-series)

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-00A0 | AI Experiment Sandbox | draft | Formal boundaries for AI experimentation in `/experiments/ai/` |
| RFC-00A1 | Deterministic Evidence Protocol | draft | Evidence collection and verification for AI determinism |
| RFC-00A2 | AI Benchmark Specification | draft | Standardized AI performance benchmarks |
| RFC-00A3 | Model Artifact Provenance | draft | Model supply chain provenance tracking |
| RFC-00A4 | Ternary Quantization Codec | draft | Ternary-native quantization codecs |
| RFC-00A5 | LLM Backend Adapter | draft | Engine-agnostic LLM backend interface |
| RFC-00A6 | Axion Policy Hooks | draft | Policy enforcement hooks for AI operations |
| RFC-00A7 | UX Integration | draft | User experience integration for AI features |
| RFC-00A8 | AI Native VM Opcodes | draft | Additional AI-specific VM opcodes |

## Active Consolidations

- APL track: RFC-0022 is the forward path; RFC-0009 is retained for provenance until formal supersession.
- AI-native track: RFC-0026 supersedes RFC-0012, RFC-0013, RFC-0014. Those are closed for new edits.
- Conformance track: RFC-0027 supersedes RFC-0008. Spec-as-executable is the concrete realization.

## Authoring

- Start from `spec/rfcs/template.md`.
- Update this index whenever RFC status changes.
