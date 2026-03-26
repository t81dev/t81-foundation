# AI RFC Backlog

Last Updated: 2026-03-26
Owner: @t81dev
Scope: AI integration RFC implementation sequencing and compliance tracking

This backlog tracks active AI integration RFC work only. It is separate from structural hardening in `HARDENING_BACKLOG.md`.

---

## Priority Order

| Priority | RFC | Title | Current State | Next Deliverable | Owner | Target |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| P0 | RFC-0000 | T81 Base-81 Ternary Computing Stack | **Implemented (100% Complete 2026-03-10)** — All core deliverables closed. | | @t81dev | 2026-03-10 |
| P0 | RFC-00A0 | AI Experiment Sandbox and Repository Boundaries | **Implemented (Guard Active)** — CI boundary guard landed via `scripts/ci/check_ai_experiment_boundary.py` and workflow wiring | Monitor for violations and keep guard roots aligned with repository topology | @t81dev | 2026-03-08 |
| P0 | RFC-00A1 | Deterministic Evidence and Reproducibility Protocol for AI Workloads | **Implemented (Key-Backed Signed Multi-Lane Evidence Manifest + Promotion Window Attestation)** — canonical evidence bundle binds fixture GGUF replay vectors; AI CI includes VM trace-level ctest evidence (`t81_vm_trace_test`, `canonfs_axion_trace_test`), cross-lane continuity gate (`ai_cross_lane_evidence.*`), signed multi-lane manifest artifact (`ai_evidence_manifest.*`), `material_env` secret wiring, automated key-expiry alert gate (`check_ai_keyring_expiry.py`), and KMS metadata contract enforcement across AI keyrings (`check_ai_keyring_kms_contract.py`) | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-10 |
| P0 | RFC-00A3 | Model Artifact Identity and Provenance (GGUF/Safetensors Policy) | **Implemented (CanonFS Identity + Signed Multi-Entry Provenance Chain Gate + Lineage Policy Constraints)** — CI provenance gate binds model hash to CanonFS object identity, verifies keyring-signed chain integrity, enforces minimum multi-entry lineage (`--min-lineage-entries 2`), validates required promotion boundary event sequence (`--required-lineage-events artifact_ingest,artifact_promotion_candidate`), and runs deterministic deny-on-mismatch self-test; signing keys support `material_env` secret injection with CI secret-env wiring | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-12 |
| P1 | RFC-0025 | Policy-Gated Tensor Loading via CanonFS | **Implemented (Governed Runtime Lane + Signed Multi-Seed Replay Attestation + Escalation Mapping)** — AI CI runs VM `TLOADHASH` conformance + `t81 canonize-*` toolchain artifact, requires governed `llama-run` evidence using sanctioned fixture `tests/fixtures/llama_cpp_repro/model.gguf`, and enforces deterministic multi-seed replay attestations with failure-taxonomy artifact (`governed_llama_replay_attestation.*`) now signed via governed replay keyring (`ai_governed_replay_keyring.json`) with machine-readable escalation mapping for keyring/signature/determinism failures | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-15 |
| P1 | RFC-00A6 | Axion Policy Hooks for Inference and Tooling Events | **Implemented (Signed Multi-Event Axion Ledger + Replay Verification + Escalation Mapping)** — deterministic policy event contract gate with reason-code coverage validates runtime observability trace binding (`ai_runtime_trace.json`), emits signed Axion ledger snapshot/replay artifacts (`ai_axion_policy_ledger_snapshot.*`, `ai_axion_policy_ledger_replay_verification.*`), and now publishes machine-readable escalation mapping for keyring/signature/runtime-trace failures; key material supports `material_env` secret injection with CI secret-env wiring | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-18 |
| P1 | RFC-00A5 | LLM Backend Adapter Interface (Engine-Agnostic) | **Implemented (Signed Backend-Selection Manifest Bound to Policy/Runtime Evidence + Escalation Mapping)** — deterministic adapter-contract gate probes `t81_ai` (`--help`, `backend capabilities`, `backend select`, `model inspect`, `verify determinism`), emits selection trace + deterministic replay artifacts, signs `runtime_backend_selection_manifest.*` bound to policy/runtime evidence, and now publishes machine-readable escalation mapping for keyring/signature/policy-ledger failures; key material supports `material_env` with CI secret-env wiring | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-20 |
| P2 | RFC-00A7 | UX Integration for AI in T81 | **Implemented (Signed Direct Backend Execution Attestations + Escalation Mapping + Inference Capability Matrix Contract)** — AI CI validates runtime command surface (`model inspect`, `verify determinism`, `backend capabilities`, `backend select`, `inference run`, `quantization inspect`, `benchmark run`, `policy test`, `workflow run/replay/report`, `observability trace`) with fixture-backed GGUF binding, backend-selection trace linkage, replay-policy consistency checks, and signed direct backend execution attestation artifact (`ai_direct_backend_execution_attestation.*`) using governed `t81 llama-run` deterministic replay plus machine-readable escalation mapping for keyring/signature/replay failures; supported UX smoke lanes use the reproducible GGUF backend path, while governed `gguf` + `t3k` strict-deterministic probes are now satisfied by the `t81_reference_vm` capability lane exposed through the inference capability matrix and direct backend-selection contract | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-22 |
| P2 | RFC-00A2 | AI Benchmark Specification and Reporting Format | **Implemented (Runtime Benchmark Execution + Governed Threshold History + Rolling Trends + Signed Approval Policy Gate + Capability Matrix Contract)** — AI CI executes runtime benchmark lane (`t81_ai benchmark run`) with deterministic replay attestation, threshold gating against baseline (`ai_benchmark_thresholds.json`), active-window selection from governed history (`ai_benchmark_thresholds_history.json`), rolling multi-window trend analytics (`--trend-window-count`), benchmark threshold window approval-policy validation using deterministic approval attestations plus keyring-verified signatures (`check_ai_benchmark_threshold_approvals.py`, `ai_benchmark_threshold_approval_keyring.json`) with `material_env` secret wiring, and benchmark format/mode capability-matrix reporting (`check_ai_benchmark_capability_matrix.py`) enforced against governed expectation contract (`ai_benchmark_capability_expectations.json`); governed strict-deterministic `gguf` + `t3k` probes are now reported as supported through the `t81_reference_vm` reference backend, while host-float backends remain bounded/non-strict | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-24 |
| P2 | RFC-00A4 | Ternary Quantization Codec Contract | **Implemented (Runtime Fixture Corpus Validation + Governed Codec Profile History Windows + Signed Approval Policy Gate + Rolling Trends)** — AI CI runs runtime quantization inspect replay attestation, enforces deterministic encode/decode fixture corpus roundtrip validation, validates runtime codec output against active governed profile windows (`ai_quantization_codec_profile_history.json`), enforces promotion approval metadata + deterministic approval attestation hashes with keyring-verified signatures per history window (`check_ai_quantization_profile_approvals.py`, `ai_quantization_profile_approval_keyring.json`) with `material_env` secret wiring, and now publishes rolling multi-window profile trend analytics (`--trend-window-count`) in quantization contract artifacts | KMS rotation automation active; monitor for expiry | @t81dev | 2026-03-27 |
| P3 | RFC-0026 | AI-Native Inference Opcodes | **Operationally Closed at Phase-1 Surface** — All six phase-1 opcodes implemented and spec-documented. Deterministic kernel tightening landed for the active AI/tensor math surface while stable conformance hashes remained unchanged. AI-M4 remains intentionally scoped to audited `WLOAD` materialization rather than full policy-gated CanonFS-backed loading. Follow-on work now centers on `WLOAD` promotion review and RFC-0030 float-domain promotion policy, not broad host-float cleanup. | Review `WLOAD` promotion boundary and keep RFC-0030 focused on non-fixed tensor promotion/parity rules | @t81dev | 2026-03-30 |
| P3 | RFC-00A8 | AI-Native VM Opcode Exploration | **Implemented (Runtime-Bound Dispatch + Execution Evidence Report + Baseline History Windows + Signed Baseline Approval Policy Gate)** — RFC-00A8 exploration constrained by RFC-0026 subset contract and CI-published opcode runtime report (`ai_opcode_runtime_report.*`) plus `/experiments/ai/opcodes/IMPLEMENTATION_REPORT.md`, bound to phase-1 conformance ctest evidence (`ai_phase1_opcode_ctest.log`), per-opcode deterministic output hashes, baseline-vs-current hash diffing with governed baseline history-window selection (`PHASE1_BASELINE_HASHES_HISTORY.json`), and signed opcode baseline window promotion approval verification (`check_ai_opcode_baseline_history_approvals.py`, `ai_opcode_baseline_approval_keyring.json`) enforced against provenance-reference expectations (`ai_opcode_baseline_provenance_expectations.json`) with baseline-window provenance surfaced directly in opcode runtime evidence artifacts, policy-checked for safe/path-bounded references, and now cross-validated by runtime provenance consistency gate (`check_ai_opcode_runtime_provenance.py`) | Maintain provenance references as baseline windows rotate and keep runtime provenance consistency gate aligned with expectations contract changes | @t81dev | 2026-04-02 |

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

## Compliance Snapshot (2026-03-15)

- RFC-0031 (Deterministic AI Execution Contract): `proposed` → **`accepted`**
- RFC-0032 (AI Subsystem Promotion Pathway): `proposed` → **`accepted`** — all 5 phases complete
- RFC-0002 (Deterministic Execution Contract): `draft` → **`accepted`** — §11 fulfilled
- RFC-00A0: `draft` → **`superseded`** by RFC-0032 (3-stage lifecycle executed)
- RFC-00A1: `draft` → **`superseded`** by RFC-0032 §5 (FNV-1a/key=value replaces SHA-256/JSON/wall-clock)
- RFC-00A3: `draft` → **`accepted`** (TLOADHASH + `allowed_tensor_hashes` + `model_load` event)
- RFC-00A4: `draft` → **`accepted`** (`quantize_threshold` + `pack_ternary_to_base81` promoted Phase 1)
- RFC-00A5: `draft` → **`superseded`** by RFC-0032 Phase 4 (T81VmBackend replaces llama.cpp/ONNX)
- RFC-00A6: `draft` → **`accepted`** (AIHookEngine + PolicyEngine + axion-event-registry live)
- RFC-00A8: `draft` → **`superseded`** by RFC-0026 (ATTN/QMATMUL/EMBED/WLOAD in TISC)
- ai-opcode-phase1-conformance.md: `phase_status` → `spec_conformant` (was `runtime_bound`)
- Conformance suite: 27 programs (3 new AI programs: attn-determinism, qmatmul-scale-order, embed-bounds-check)
- New deliverables: `spec/supplemental/axion-event-registry.md`, `core/vm/ai_backend/backend_adapter.cpp`, `tests/determinism/evidence_collector.cpp`, `tooling/cli/ai/t81_ai_cli.cpp`

## Compliance Snapshot (2026-03-08)

- Baseline CI-gated RFCs: `RFC-0000`, `RFC-00A0`, `RFC-00A1`, `RFC-00A2`, `RFC-00A3`, `RFC-00A4`, `RFC-00A5`, `RFC-00A6`, `RFC-00A7`, `RFC-0025`, `RFC-0026`, `RFC-00A8`
- Partial RFCs: `RFC-0026` (phase-1 opcode surface complete; `WLOAD` promotion review and RFC-0030 policy follow-on remain)
- Current gating principle: no AI feature promotion claims without deterministic evidence + provenance + policy enforcement
- 2026-03-08: RFC-0000 — 7 core implementation items delivered: `EthicsViolation`/`CapabilityDenied` traps, `CanonBlock`, 81-slot Hanoi scheduler + `SchedulerFull` error, ethics-first `boot()`, `t81 axion` CLI surface (5 subcommands), T6561 Tier 6 distributed monads + Θ₇ containment, CanonHash-81 reference vector test suite
- 2026-03-08: Hanoi kernel — `Kernel::boot()` interface added; ethics-first spawn gate active; `Error::SchedulerFull` added to error enum
- 2026-03-08: T6561 — `TierId::Tier6`, `MeshReflector`, `MonadState` types; `promotion.cpp` Tier5→Tier6 path; `experimental/tiers/cog/tier6/distributed_monad.cpp`

## Compliance Snapshot (2026-03-07)

- Baseline CI-gated RFCs: `RFC-00A0`, `RFC-00A1`, `RFC-00A2`, `RFC-00A3`, `RFC-00A4`, `RFC-00A5`, `RFC-00A6`, `RFC-00A7`, `RFC-0025`, `RFC-0026`, `RFC-00A8`
- Partial RFCs: `RFC-0026` (phase-1 opcode surface complete; promotion/policy follow-on remains)
- Current gating principle: no AI feature promotion claims without deterministic evidence + provenance + policy enforcement
- 2026-03-07: RFC-0026 phase-1 extended — `WLOAD`, `GATHER`, `SCATTER` opcodes added to TISC, VM, tensor kernel, and conformance suite
- 2026-03-07: RFC-0026 AI-M4 — WLOAD CanonFS audit gate implemented; `action=WeightLoad` event verified
- 2026-03-07: RFC-0026 AI-M5 — multi-axis GATHER (axis=0/1) and SCATTER aliasing detection implemented and tested
- 2026-03-07: RFC-0026 spec docs — `opcode-registry.md` §2.18 and `opcode-unified-reference.md` §5.18 updated with full AI opcode table (0xBB–0xC1)
- 2026-03-07: RFC-0026 AI-M6 — T81Lang `@attention`/`@qmatmul` annotation lowering + `Tensor.attention`/`Tensor.qmatmul` builtins; SA Tier 2+ enforcement; `lang_ai_m6_annotation_lowering_test` added

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
