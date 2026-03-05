# AI RFC Backlog

Last Updated: 2026-03-05
Owner: @t81dev
Scope: AI integration RFC implementation sequencing and compliance tracking

This backlog tracks active AI integration RFC work only. It is separate from structural hardening in `HARDENING_BACKLOG.md`.

---

## Priority Order

| Priority | RFC | Title | Current State | Next Deliverable | Owner | Target |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| P0 | RFC-00A0 | AI Experiment Sandbox and Repository Boundaries | **Partial** (boundary and build isolation evidenced in `AI_CLI_MILESTONE_EVIDENCE.md`) | Add automated boundary guard check in CI for `/experiments/ai/**` containment and forbidden core mutations | @t81dev | 2026-03-08 |
| P0 | RFC-00A1 | Deterministic Evidence and Reproducibility Protocol for AI Workloads | **Not started** (no canonical AI evidence pipeline in required CI) | Land canonical evidence schema + CLI export path + CI artifact upload for AI runs | @t81dev | 2026-03-10 |
| P0 | RFC-00A3 | Model Artifact Identity and Provenance (GGUF/Safetensors Policy) | **Not started** | Implement model provenance manifest and hash verification gate wired to CanonFS policy path | @t81dev | 2026-03-12 |
| P1 | RFC-0025 | Policy-Gated Tensor Loading via CanonFS | **Draft only** | Implement `TLOADHASH`/policy hash allowlist path with deterministic deny reasons and conformance tests | @t81dev | 2026-03-15 |
| P1 | RFC-00A6 | Axion Policy Hooks for Inference and Tooling Events | **Not started** | Add AI event hook taxonomy and deterministic audit events for model/inference/tool/resource actions | @t81dev | 2026-03-18 |
| P1 | RFC-00A5 | LLM Backend Adapter Interface (Engine-Agnostic) | **Not started** | Ship minimal engine-agnostic adapter contract and one conformance backend (llama.cpp experimental path) | @t81dev | 2026-03-20 |
| P2 | RFC-00A7 | UX Integration for AI in T81 | **Partial** (experimental CLI exists) | Expand CLI to RFC command families and add deterministic workflow replay/reporting | @t81dev | 2026-03-22 |
| P2 | RFC-00A2 | AI Benchmark Specification and Reporting Format | **Not started** | Add canonical benchmark spec doc + one CI benchmark workflow with reproducible report artifact | @t81dev | 2026-03-24 |
| P2 | RFC-00A4 | Ternary Quantization Codec Contract | **Not started** | Implement codec header/metadata validator and deterministic encode/decode fixture corpus | @t81dev | 2026-03-27 |
| P3 | RFC-0026 | AI-Native Inference Opcodes | **Draft only** | Freeze Phase-1 opcode subset (`QMATMUL`, `ATTN`, `EMBED`) semantics and add VM conformance stubs | @t81dev | 2026-03-30 |
| P3 | RFC-00A8 | AI-Native VM Opcode Exploration | **Exploratory only** | Align with RFC-0026 subset and publish non-normative experiment report under `/experiments/ai/opcodes` | @t81dev | 2026-04-02 |

---

## Dependency Chain

| RFC | Depends On | Blocking Reason |
| :--- | :--- | :--- |
| RFC-0025 | RFC-0004, RFC-0009, RFC-0020, RFC-0022 | Policy-gated tensor loading requires canonical tensor semantics and Axion policy/trace surfaces |
| RFC-0026 | RFC-0025 (operationally) | `WLOAD` policy gate and model provenance should exist before opcode promotion claims |
| RFC-00A5 | RFC-00A0, RFC-00A1, RFC-00A3 | Backend adapter must operate inside sandbox with reproducibility and artifact identity controls |
| RFC-00A6 | RFC-0022, RFC-0020 | AI policy hooks rely on policy grammar and trace semantics |
| RFC-00A7 | RFC-00A1, RFC-00A6 | UX workflows need evidence protocol and policy decision visibility |
| RFC-00A8 | RFC-0026 | VM opcode exploration should not diverge from standards-track AI opcode semantics |

---

## This Week Execution Slices

| Slice ID | Scope | RFCs | Acceptance Signal |
| :--- | :--- | :--- | :--- |
| AI-S1 | Sandbox governance automation | RFC-00A0 | CI fails when AI code escapes sandbox boundary |
| AI-S2 | Determinism evidence baseline | RFC-00A1 | AI run emits canonical evidence bundle and hash-stable manifest |
| AI-S3 | Model provenance gate | RFC-00A3, RFC-0025 | Model load denied on missing/invalid provenance hash |
| AI-S4 | Policy event instrumentation | RFC-00A6 | Axion logs deterministic AI event records with stable reason codes |

---

## Compliance Snapshot (2026-03-05)

- RFCs with partial implementation evidence: `RFC-00A0`, `RFC-00A7`
- RFCs still draft-only or exploratory: `RFC-0025`, `RFC-0026`, `RFC-00A1`, `RFC-00A2`, `RFC-00A3`, `RFC-00A4`, `RFC-00A5`, `RFC-00A6`, `RFC-00A8`
- Current gating principle: no AI feature promotion claims without deterministic evidence + provenance + policy enforcement

---

## Cross-References

- `docs/status/AI_CLI_MILESTONE_EVIDENCE.md`
- `docs/status/EXTENSION_PROFILE.md`
- `docs/status/CI_WORKFLOW_CONFIRMATION.md`
- `spec/rfcs/index.md`
- `spec/rfcs/RFC-0025-policy-gated-tensor-loading.md`
- `spec/rfcs/RFC-0026-ai-native-inference-opcodes.md`
- `spec/rfcs/RFC-00A0-ai-experiment-sandbox.md`
- `spec/rfcs/RFC-00A1-deterministic-evidence-protocol.md`
- `spec/rfcs/RFC-00A2-ai-benchmark-spec.md`
- `spec/rfcs/RFC-00A3-model-artifact-provenance.md`
- `spec/rfcs/RFC-00A4-ternary-quantization-codec.md`
- `spec/rfcs/RFC-00A5-llm-backend-adapter.md`
- `spec/rfcs/RFC-00A6-axion-policy-hooks.md`
- `spec/rfcs/RFC-00A7-ux-integration.md`
- `spec/rfcs/RFC-00A8-ai-native-vm-opcodes.md`
