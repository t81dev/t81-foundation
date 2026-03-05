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
| P0 | RFC-00A1 | Deterministic Evidence and Reproducibility Protocol for AI Workloads | **Implemented (Key-Backed Signed Multi-Lane Evidence Manifest + Promotion Window Attestation)** — canonical evidence bundle binds fixture GGUF replay vectors; AI CI includes VM trace-level ctest evidence (`t81_vm_trace_test`, `canonfs_axion_trace_test`), cross-lane continuity gate (`ai_cross_lane_evidence.*`), and signed multi-lane manifest artifact (`ai_evidence_manifest.*`) with promotion-window metadata, keyring-backed HMAC signature verification, and rotation-policy checks | Promote signing material from repository keyring to secret/KMS-managed keys with automated key-expiry alerting | @t81dev | 2026-03-10 |
| P0 | RFC-00A3 | Model Artifact Identity and Provenance (GGUF/Safetensors Policy) | **Implemented (CanonFS Identity + Signed Provenance Chain Baseline)** — CI provenance gate now binds model hash to CanonFS object identity and verifies a keyring-signed provenance chain with deterministic deny-on-mismatch self-test | Move provenance signing material to secret-managed keys and require multi-entry lineage chain verification across artifact promotion hops | @t81dev | 2026-03-12 |
| P1 | RFC-0025 | Policy-Gated Tensor Loading via CanonFS | **Implemented (Governed Runtime Lane + Multi-Seed Replay Attestation)** — AI CI runs VM `TLOADHASH` conformance + `t81 canonize-*` toolchain artifact, requires governed `llama-run` evidence using sanctioned fixture `tests/fixtures/llama_cpp_repro/model.gguf`, and now enforces deterministic multi-seed replay attestations with failure-taxonomy artifact (`governed_llama_replay_attestation.*`) | Promote replay/failure-taxonomy gate from repository-keyed evidence to secret-managed signing + escalation policy mapping | @t81dev | 2026-03-15 |
| P1 | RFC-00A6 | Axion Policy Hooks for Inference and Tooling Events | **Implemented (Baseline + Runtime Trace Binding)** — deterministic policy event contract gate with reason-code coverage artifact now validates a runtime observability trace binding (`ai_runtime_trace.json`) emitted by `t81_ai` | Extend runtime binding from single-trace contract checks to signed multi-event Axion ledger snapshots and replay verification | @t81dev | 2026-03-18 |
| P1 | RFC-00A5 | LLM Backend Adapter Interface (Engine-Agnostic) | **Implemented (Selection Replay Attestations)** — deterministic adapter-contract gate and artifact in AI CI now probes `t81_ai` (`--help`, `backend capabilities`, `backend select`, `model inspect`, `verify determinism`) with JSON schema validation, emitted selection trace artifact (`runtime_backend_selection_trace.json`), and multi-request deterministic replay attestation artifact (`runtime_backend_selection_replay.json`) | Promote replay attestations to signed backend-selection manifests bound to policy/runtime evidence snapshots | @t81dev | 2026-03-20 |
| P2 | RFC-00A7 | UX Integration for AI in T81 | **Implemented (Backend-Selection-Driven Runtime Semantics)** — AI CI validates runtime command surface (`model inspect`, `verify determinism`, `backend capabilities`, `backend select`, `inference run`, `quantization inspect`, `benchmark run`, `policy test`, `workflow run/replay/report`, `observability trace`) with fixture-backed GGUF binding, backend-selection trace linkage across UX artifacts, and replay-policy consistency checks | Promote runtime semantics from deterministic surrogate execution to direct backend execution attestations while preserving evidence determinism constraints | @t81dev | 2026-03-22 |
| P2 | RFC-00A2 | AI Benchmark Specification and Reporting Format | **Implemented (Baseline Contract)** — deterministic benchmark report contract gate + artifact active in AI CI | Replace baseline with runtime benchmark execution and trend/regression thresholds | @t81dev | 2026-03-24 |
| P2 | RFC-00A4 | Ternary Quantization Codec Contract | **Implemented (Baseline Contract)** — deterministic codec manifest contract gate + artifact active in AI CI | Add encode/decode fixture corpus validation against runtime quantization pipeline | @t81dev | 2026-03-27 |
| P3 | RFC-0026 | AI-Native Inference Opcodes | **Implemented (Phase-1 Runtime Semantics + Conformance Gate)** — `tisc::Opcode` includes `ATTN`/`QMATMUL`/`EMBED`; all three execute deterministic phase-1 runtime semantics with canonical packed-operand phase-1 encoding for `ATTN`/`QMATMUL`; AI CI runs semantic conformance for ATTN/EMBED/QMATMUL | Migrate QMATMUL from phase-1 deterministic dequantize+matmul path to finalized quantized runtime semantics and full 4-source encoding profile when ISA expands beyond 3 operands | @t81dev | 2026-03-30 |
| P3 | RFC-00A8 | AI-Native VM Opcode Exploration | **Implemented (Runtime-Bound Dispatch + Execution Evidence Report)** — RFC-00A8 exploration constrained by RFC-0026 subset contract and CI-published opcode runtime report (`ai_opcode_runtime_report.*`) plus `/experiments/ai/opcodes/IMPLEMENTATION_REPORT.md`, bound to phase-1 conformance ctest evidence (`ai_phase1_opcode_ctest.log`), per-opcode deterministic output hashes, and baseline-vs-current hash diffing via `PHASE1_BASELINE_HASHES.json` | Expand report with richer fixture/vector provenance metadata and configurable baseline promotion windows (planned multi-baseline support) | @t81dev | 2026-04-02 |

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
