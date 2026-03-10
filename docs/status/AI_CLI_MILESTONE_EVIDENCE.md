# AI CLI Milestone Evidence

Last Updated: 2026-03-07
Owner: @t81dev
Scope: Build and runtime evidence for AI-facing CLI milestones

---

## Purpose

This document records build and runtime evidence for AI CLI milestones gated in
the `AI Experiments CI` workflow. It tracks the operational status of the `t81 ai`
command surface and governed inference lane, binding CI artifact evidence to the
RFC implementation milestones in `AI_RFC_BACKLOG.md`.

Evidence here is CI-verifiable. Claims without a corresponding CI artifact or
ctest are not accepted as milestone closures.

---

## Build Evidence

### Candidate: `b566bff8` (2026-03-07)

| Milestone | RFC | CI Gate | Artifact | Status |
| :--- | :--- | :--- | :--- | :--- |
| Sandbox boundary guard active | RFC-00A0 | `check_ai_experiment_boundary.py` | `ai_boundary_check.*` | Pass ✅ |
| Deterministic evidence bundle | RFC-00A1 | `ai_cross_lane_evidence.*` | `ai_evidence_manifest.*` | Pass ✅ |
| Model provenance gate | RFC-00A3, RFC-0025 | Provenance chain gate | `ai_provenance_chain.*` | Pass ✅ |
| Axion policy event contract | RFC-00A6 | Policy-event reason-code gate | `ai_axion_policy_ledger_snapshot.*` | Pass ✅ |
| Backend adapter selection | RFC-00A5 | Backend-selection manifest gate | `runtime_backend_selection_manifest.*` | Pass ✅ |
| Ternary quantization codec | RFC-00A4 | Codec corpus roundtrip gate | `ai_quantization_profile.*` | Pass ✅ |
| Benchmark reporting format | RFC-00A2 | Benchmark threshold gate | `ai_benchmark_thresholds.*` | Pass ✅ |
| UX command surface | RFC-00A7 | Direct backend execution attestation | `ai_direct_backend_execution_attestation.*` | Pass ✅ |
| AI-native opcode phase-1 semantics | RFC-0026 | Phase-1 conformance ctest | `ai_rfc0026_readiness.*` | Pass ✅ (phase-1 subset) |
| Opcode exploration baseline | RFC-00A8 | Opcode runtime report gate | `ai_opcode_runtime_report.*` | Pass ✅ |

### VM Trace Evidence

| Test | Status | Notes |
| :--- | :--- | :--- |
| `t81_vm_trace_test` | Pass ✅ | VM-level execution trace bound to AI CI lane |
| `canonfs_axion_trace_test` | Pass ✅ | CanonFS + Axion policy trace binding |
| `t81_determinism_containers_test` | Pass ✅ | `T81String::serialize_canonical()` fixes CanonHash non-determinism |

Test count: **325/325** (100% pass rate as of `b566bff8`)

---

## Governed Replay Evidence

The governed inference lane uses the reproducible fixture at
`tests/fixtures/llama_cpp_repro/model.gguf` with deterministic multi-seed
replay attestation. Strict-deterministic probes for `gguf` and `t3k` backends
are satisfied through the `t81_reference_vm` capability lane.

| Lane | Mode | Status |
| :--- | :--- | :--- |
| `t81 llama-run` (governed) | Fixture GGUF, multi-seed replay | Pass ✅ |
| `t81_reference_vm` | Strict-deterministic reference lane | Pass ✅ |
| Host-float backend | Bounded (non-strict) | Informational ⚠️ |

---

## Key Material Status

Key material referenced in this document uses `material_env` secret injection
with CI secret-env wiring. Production KMS-backed rotation automation is **Implemented** 
via `rotate_ai_kms_keys.py` and GitHub Actions.

| Keyring | KMS-Backed | Target |
| :--- | :--- | :--- |
| `ai_governed_replay_keyring.json` | Active | 2026-03-31 |
| `ai_benchmark_threshold_approval_keyring.json` | Active | 2026-03-31 |
| `ai_quantization_profile_approval_keyring.json` | Active | 2026-03-31 |
| `ai_opcode_baseline_approval_keyring.json` | Active | 2026-03-31 |

---

## Cross-References

- `docs/status/AI_RFC_BACKLOG.md`
- `docs/status/CI_WORKFLOW_CONFIRMATION.md`
- `docs/status/CI_GATE_STATUS.md`
- `.github/workflows/ai-experiments-ci.yml`
- `scripts/ci/ai_status_doc_freshness_expectations.json`
- `tests/fixtures/llama_cpp_repro/`
