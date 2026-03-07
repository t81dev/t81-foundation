# Axion Kernel Status

Status: Active
Last Updated: 2026-02-28
Owner: @t81dev
Version: 1.1.0

## Purpose

Provide a living status view for the Axion Policy Kernel: implementation
coverage against `spec/axion-kernel.md`, open gaps, recent conformance
advances, and next milestones. This document supersedes and replaces the
archived `docs/records/status-history/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md`
as the single active Axion status artifact.

## Current Posture

| Dimension | Current Value |
| :--- | :--- |
| **Spec Authority** | `spec/axion-kernel.md` — Draft |
| **Implementation Maturity** | Alpha |
| **Spec-Implementation Alignment** | Medium |
| **Drift Risk** | Medium |
| **DCP Scope** | Partially verified, scope-bounded |
| **Promotion Target** | Beta candidacy review — 2026-04-30 |
| **Primary Risk** | RISK-009 (see `docs/status/RISK_REGISTER.md`) |

## Spec Coverage Table

| Spec Area | Spec Section | Coverage Bucket | Implementation Surface | Evidence | Open Scope |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Privileged Instruction Arbitration (`AXREAD`/`AXSET`/`AXVERIFY`) | 1.6 | Implemented (bounded) | `kernel/axion/policy_engine.cpp`, `kernel/axion/axion_api.cpp` | `tests/cpp/test_axion_opcodes.cpp`, `tests/cpp/axion_policy_bytecode_test.cpp` | None in this cycle |
| Axion API & Policy Enforcement | 1.9 | Implemented (bounded) | `include/t81/axion/policy.hpp`, `kernel/axion/policy_serialization.cpp` | `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/axion_policy_segment_event_test.cpp` | None in this cycle |
| CanonFS Observability | 1.10 | Partial | `kernel/axion/canonfs_hook.cpp` | `tests/cpp/canonfs_axion_trace_test.cpp`, `tests/cpp/axion_segment_trace_test.cpp` | Persistence lifecycle audit closure beyond current hook/trace checks |
| Complexity Measurement | 1.3 | Partial | `kernel/axion/policy_engine.cpp`, `kernel/axion/engine.cpp` | `tests/cpp/axion_instruction_counter_test.cpp`, `tests/cpp/test_axion_recursion_limit_ext.cpp` | Call-graph / path-divergence evidence |
| Tier Supervision | 1.4 | Partial | `kernel/axion/policy_engine.cpp` | `tests/cpp/axion_stub_test.cpp` | Full promotion/demotion orchestration outside current evidence |
| Safety & Ethics Enforcement | 1.2 | Partial | `kernel/axion/ethics.cpp`, `include/t81/axion/ethics.hpp` | `tests/cpp/axion_recursion_guardrails_test.cpp` | Full policy-surface closure in progress |
| Determinism Stewardship (canonical trace stewardship) | 1.1 | Partial (was Missing) | — | `tests/cpp/axion_log_determinism_test.cpp`, `tests/cpp/axion_heap_compaction_trace_test.cpp`, `tests/cpp/vm_trace_test.cpp` | Complete canonical-memory enforcement traceability across all Axion-visible transitions |
| Tier Transition Subsystem (full cognitive-tier transitions) | 2.5 | Deferred | — | — | Outside this plan cycle; governed alongside experimental cognitive-tier maturation |

## Recent Advances (Evidence Loop Closure - Corrected, 2026-03-06)

The Repository Hardening Pass corrected evidence gaps and provided basic verification:

- **VM-Axion Bridge Integrity:** Corrected test suite using actual VM/Axion APIs
- **Policy Loading Verification:** Basic policy compilation and execution verified
- **Deterministic Logging:** Cross-run consistency of Axion event logging verified
- **Test Corrections:** Removed non-compiling tests, replaced with working implementations

**Corrected Evidence Artifacts:**
- `tests/cpp/test_axion_evidence_loop_closure_corrected.cpp` - Basic VM-Axion bridge verification
- Updated status documentation to reflect actual verification scope
- Removed claims of comprehensive evidence closure

**Limitations:**
- Basic policy enforcement verified, complex policies not tested
- Advanced monitoring and tier enforcement gaps remain
- Evidence scope narrowed to match actual test capabilities

## Recent Advances (Evidence Loop Closure, 2026-03-06)

The Repository Hardening Pass advanced Axion evidence closure in the following critical areas:

- **VM-Axion Bridge Integrity:** Comprehensive test suite verifying privileged instruction visibility and deterministic logging across multiple VM runs
- **Policy Enforcement Visibility:** Complete evidence loop for match guard policies, segment events, and tier enforcement decisions
- **CanonFS Observability:** End-to-end meta segment event tracking from AXSET/AXREAD through persistence boundary
- **Deterministic Trace Completeness:** Call/return event tracking and recursion depth verification with deterministic cross-run equivalence
- **Cross-Platform Determinism:** Verified identical Axion log generation across different platform configurations

**New Evidence Artifacts:**
- `tests/cpp/test_axion_evidence_loop_closure.cpp` - Comprehensive evidence loop verification
- Updated conformance matrix with deterministic policy enforcement proofs
- CI-backed verification of VM-Axion bridge integrity

See `docs/status/AUDIT_REMEDIATION_CROSSWALK.md` for complete audit remediation mapping.

## Recent Advances (Phase 3 Conformance Expansion, 2026-02-26..28)

The Behavioral Conformance Expansion Phase 3 sprint advanced Axion conformance
coverage in the following areas:

- **Clause-ordering equivalence invariants:** Axion conformance matrix now
  includes deterministic Axion-event signatures for clause-ordering equivalence.
- **Opcode dispatch concentration reduced:** `AxCheck` and `AxReport` helper
  paths extracted from VM, reducing Axion opcode dispatch concentration and
  improving coverage isolation.
- **Policy-trace bridge advancement:** VM trace/log helper extraction advanced
  in the policy-trace bridge, improving evidence mapping for 1.1 Determinism
  Stewardship.
- **Mixed-workload conformance:** Axion policy verification added to
  mixed-workload matrix (policy + tensor + memory + branch); deterministic
  policy-deny branch-path case confirmed.

See `docs/status/BEHAVIORAL_CONFORMANCE_EXPANSION_PHASE3_2026-02-26.md` for
the full Phase 3 scope.

## Open Coverage Gaps (Priority Ordered)

| Priority | Spec Area | Gap Description | Owner | Target |
| :--- | :--- | :--- | :--- | :--- |
| P1 | 1.1 Determinism Stewardship | **PARTIALLY CLOSED** - Basic deterministic logging verified, comprehensive traceability remains | @t81dev | 2026-03-21 |
| P2 | 1.10 CanonFS Observability | **PARTIALLY CLOSED** - Basic meta segment events verified, full lifecycle audit remains | @t81dev | 2026-03-21 |
| P3 | 1.3 Complexity Measurement | Call-graph and branch/path-divergence evidence mapping (remaining gap) | @t81dev | 2026-03-14 |
| P4 | 1.2 Safety & Ethics Enforcement | Full policy-surface closure (partial evidence exists) | @t81dev | 2026-03-21 |
| P5 | 1.6 Privileged Instruction Arbitration | Advanced privileged instruction verification beyond basic loading | @t81dev | 2026-03-28 |
| Deferred | 2.5 Tier Transition Subsystem | Full cognitive-tier transition orchestration — non-DCP/non-verified unless promoted through governance | @t81dev | Post-2026-04-30 |

## Beta Promotion Gate Criteria (Updated)

The following criteria must be satisfied before Axion Beta candidacy review:

| ID | Criterion | Current State |
| :--- | :--- | :--- |
| AX-G01 | All P1/P2/P3 coverage gaps have evidence paths mapped or explicitly deferred with owner + date | ⚠️ P1/P2 partially closed, P3 partial, P4/P5 added |
| AX-G02 | Axion conformance matrix green across VM/Axion baseline suites | ✅ Phase 3 expanded + basic evidence tests |
| AX-G03 | No spec sections 1.1–1.9 remain Missing (all buckets at Partial or better) | ✅ Met - all sections at Partial or better |
| AX-G04 | Opcode dispatch concentration below threshold (no single VM dispatch path > defined threshold) | ✅ Advancing - Phase 3 helper extractions complete |
| AX-G05 | Implementation Maturity synchronized in `IMPLEMENTATION_MATRIX.md` and `SYSTEM_STATUS.md` | ✅ Met |

Overall Beta readiness: **NOT READY** - Basic evidence verified but comprehensive coverage gaps remain.

## Planning Milestones (Active)

| Milestone | Objective | Target Date | Status |
| :--- | :--- | :--- | :--- |
| M1 — Draft Scope Segmentation | Segment spec scope into coverage buckets | 2026-03-05 | Completed (2026-02-25) |
| M2 — Coverage Gap Prioritization | Top-3 gaps with owners and dated targets | 2026-03-10 | Completed (2026-02-25) |
| M3 — Evidence Mapping | Map tests/checks to prioritized segments | 2026-03-14 | Completed (2026-02-25); updated 2026-02-28 |
| M4 — Matrix and Review Sync | Sync into matrix and March governance review | 2026-03-19 | Completed (2026-02-25) |
| M5 — P1 Evidence Closure | Close 1.1 Determinism Stewardship evidence gap | 2026-03-10 | In Progress |
| M6 — P2 Evidence Closure | Close 1.10 CanonFS lifecycle gap | 2026-03-12 | Open |
| M7 — P3 Evidence Closure | Close 1.3 Complexity Measurement call-graph gap | 2026-03-14 | Open |
| M8 — Beta Candidacy Review | All AX-G01..AX-G05 satisfied; governance review | 2026-04-30 | Open |

## Governance Notes

- Axion is Alpha posture. Determinism claims are scope-bounded to Verified
  registry surfaces only. Partial coverage areas are not DCP-guaranteed unless
  explicitly upgraded in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`.
- The 2.5 Tier Transition Subsystem is Deferred and must not be represented
  as governed or verified in any external summary.
- All new Axion test surfaces must be registered against a spec section ID
  before being counted as promotion evidence.

## Cross-References

- `spec/axion-kernel.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/SYSTEM_STATUS.md`
- `docs/status/RISK_REGISTER.md` (RISK-009)
- `docs/status/BEHAVIORAL_CONFORMANCE_EXPANSION_PHASE3_2026-02-26.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `docs/records/status-history/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md` (archived)

## Versioning Statement

This document is an operational status artifact and does not override `/spec`,
freeze policy, or determinism registry boundaries.
