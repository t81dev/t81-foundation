# AI RFC Backlog

Last Updated: 2026-03-05
Owner: @t81dev
Scope: AI integration RFC implementation sequencing and compliance tracking

This backlog tracks active AI integration RFC work only. It is separate from structural hardening in `HARDENING_BACKLOG.md`.

---

## Priority Order

| Priority | RFC | Title | Current State | Next Deliverable | Owner | Target |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| P0 | RFC-00A0 | AI Experiment Sandbox and Repository Boundaries | **Implemented (Guard Active)** — CI boundary guard landed via `scripts/ci/check_ai_experiment_boundary.py` and workflow wiring | Monitor for violations and keep guard roots aligned with repository topology | @t81dev | 2026-03-08 |
| P0 | RFC-00A1 | Deterministic Evidence and Reproducibility Protocol for AI Workloads | **Implemented (Baseline)** — canonical evidence bundle script + workflow artifact upload landed | Expand baseline to include trace-level VM evidence and promotion-grade fixture locking | @t81dev | 2026-03-10 |
| P0 | RFC-00A3 | Model Artifact Identity and Provenance (GGUF/Safetensors Policy) | **Implemented (Baseline)** — CI provenance hash gate + deny-on-mismatch self-test landed | Extend to CanonFS-backed artifact identity and signed provenance chain | @t81dev | 2026-03-12 |
| P1 | RFC-0025 | Policy-Gated Tensor Loading via CanonFS | **Implemented (Runtime Evidence Lane Added)** — AI CI runs VM `TLOADHASH` conformance + `t81 canonize-*` toolchain artifact; optional fixture-gated governed `llama-run` evidence lane added | Promote optional lane to required once sanctioned fixture availability is standardized across CI | @t81dev | 2026-03-15 |
| P1 | RFC-00A6 | Axion Policy Hooks for Inference and Tooling Events | **Implemented (Baseline)** — deterministic policy event contract gate with reason-code coverage artifact | Integrate baseline reason-code contract with runtime Axion event stream and signed audit ledger | @t81dev | 2026-03-18 |
| P1 | RFC-00A5 | LLM Backend Adapter Interface (Engine-Agnostic) | **Implemented (Baseline)** — deterministic adapter-contract gate and artifact in AI CI | Bind contract to runtime adapter implementation and backend capability introspection from binary | @t81dev | 2026-03-20 |
| P2 | RFC-00A7 | UX Integration for AI in T81 | **Implemented (Runtime-Bound Contract)** — AI CI now validates runtime command surface (`model inspect`, `verify determinism`, `workflow run/replay/report`, `observability trace`) and emitted replay/trace artifacts | Expand runtime UX from contract-minimum commands to full RFC command portfolio and backend capability introspection | @t81dev | 2026-03-22 |
| P2 | RFC-00A2 | AI Benchmark Specification and Reporting Format | **Implemented (Baseline Contract)** — deterministic benchmark report contract gate + artifact active in AI CI | Replace baseline with runtime benchmark execution and trend/regression thresholds | @t81dev | 2026-03-24 |
| P2 | RFC-00A4 | Ternary Quantization Codec Contract | **Implemented (Baseline Contract)** — deterministic codec manifest contract gate + artifact active in AI CI | Add encode/decode fixture corpus validation against runtime quantization pipeline | @t81dev | 2026-03-27 |
| P3 | RFC-0026 | AI-Native Inference Opcodes | **Implemented (Phase-1 Partial Runtime Semantics + Conformance Gate)** — `tisc::Opcode` includes `ATTN`/`QMATMUL`/`EMBED`; `ATTN` now executes deterministic phase-1 semantics (provisional operand encoding), `QMATMUL`/`EMBED` remain fail-closed; AI CI runs both ATTN semantic and fail-closed conformance tests | Land canonical 4-source opcode encoding and implement `QMATMUL`/`EMBED` runtime semantics with shape/output conformance vectors | @t81dev | 2026-03-30 |
| P3 | RFC-00A8 | AI-Native VM Opcode Exploration | **Implemented (Alignment + Runtime Evidence Report)** — RFC-00A8 exploration constrained by RFC-0026 subset contract and CI-published opcode runtime report (`ai_opcode_runtime_report.*`) plus `/experiments/ai/opcodes/IMPLEMENTATION_REPORT.md` | Replace source-scan evidence with VM dispatch/conformance execution evidence as ATTN/QMATMUL/EMBED land in `tisc::Opcode` + VM handlers | @t81dev | 2026-04-02 |

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
| AI-S1 | Sandbox governance automation | RFC-00A0 | **Completed (2026-03-05)** — CI fails when AI code escapes sandbox boundary |
| AI-S2 | Determinism evidence baseline | RFC-00A1 | **Completed (2026-03-05)** — AI run emits canonical evidence bundle and hash-stable manifest |
| AI-S3 | Model provenance gate | RFC-00A3, RFC-0025 | **Completed (2026-03-05)** — model load gate denies invalid provenance hash |
| AI-S4 | Policy event instrumentation | RFC-00A6 | **Completed (2026-03-05)** — deterministic AI policy-event reason-code contract gate active in CI |

---

## Compliance Snapshot (2026-03-05)

- Baseline CI-gated RFCs: `RFC-00A0`, `RFC-00A1`, `RFC-00A2`, `RFC-00A3`, `RFC-00A4`, `RFC-00A5`, `RFC-00A6`, `RFC-00A7`, `RFC-0025`, `RFC-0026`, `RFC-00A8`
- Partial RFCs: none (all listed AI RFCs now have baseline CI gates; deep runtime integration remains open)
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
