# Axion Partial-Coverage Alignment Plan (2026-03)

Status: Completed (Planning Milestone Closed)
Owner: @t81dev
Last Updated: 2026-02-25
Target Completion: 2026-03-19 (A2 planning milestone)

## Purpose

Define a bounded alignment plan for Axion Governance Kernel partial coverage against draft
spec scope, with dated milestones and evidence paths.

## Scope

This plan addresses status/control alignment between:

- `spec/axion-kernel.md`
- implementation surfaces under `kernel/axion/`
- status/governance evidence artifacts

This plan does not change normative semantics, freeze boundaries, determinism
scope, or DCP guarantees.

## Baseline Gap Statement

- `docs/status/IMPLEMENTATION_MATRIX.md` marks Axion Governance Kernel as Partial / Medium
  drift.
- `docs/status/SYSTEM_STATUS.md` marks Axion Governance Kernel as Alpha with partial
  implementation against draft surfaces.
- Current status notes describe gap posture but not milestone-bounded closure
  actions.

## Milestone Checklist

### M1 — Draft Scope Segmentation

- Target Date: 2026-03-05
- Objective:
  - Segment `spec/axion-kernel.md` scope into implementation buckets:
    implemented, partial, missing, deferred.
- Acceptance Criteria:
  - Segmentation table published in this document.
  - Each segment references implementation or test surface where available.
- Evidence:
  - This file
  - `docs/status/IMPLEMENTATION_MATRIX.md`

Status: Completed (2026-02-25)

### M2 — Coverage Gap Prioritization

- Target Date: 2026-03-10
- Objective:
  - Prioritize partial/missing segments into a bounded queue with owner and
    target dates.
- Acceptance Criteria:
  - Top-priority gaps recorded with rationale and constraints.
  - Deferred items explicitly marked as non-DCP and non-verified unless
    promoted through governance.
- Evidence:
  - This file
  - `docs/status/SYSTEM_STATUS.md`

Status: Completed (2026-02-25)

### M3 — Evidence Mapping

- Target Date: 2026-03-14
- Objective:
  - Map current tests/checks to prioritized Axion segments.
- Acceptance Criteria:
  - For each priority segment, evidence path is declared (test, integration
    surface, or pending evidence).
  - Uncovered segments are explicitly listed as open.
- Evidence:
  - This file
  - `tests/` (relevant Axion-related test surfaces)

Status: Completed (2026-02-25)

### M4 — Matrix and Review Synchronization

- Target Date: 2026-03-19
- Objective:
  - Synchronize Axion plan status into matrix and monthly governance review.
- Acceptance Criteria:
  - Matrix notes reference this alignment plan.
  - March governance review records A2 planning completion state.
- Evidence:
  - `docs/status/IMPLEMENTATION_MATRIX.md`
  - `docs/records/audits/2026-03-governance-review.md`

Status: Completed (2026-02-25)

## Bounded Next Actions

| Action ID | Action | Owner | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- |
| A2-N1 | Publish segmented scope table for draft Axion surfaces | @t81dev | 2026-03-05 | Completed (2026-02-25) |
| A2-N2 | Prioritize top 3 partial/missing segments with dated targets | @t81dev | 2026-03-10 | Completed (2026-02-25) |
| A2-N3 | Attach evidence paths for each prioritized segment | @t81dev | 2026-03-14 | Completed (2026-02-25) |
| A2-N4 | Sync matrix and governance review with A2 snapshot | @t81dev | 2026-03-19 | Completed (2026-02-25) |

## M1 Scope Segmentation Table

| Spec Area | Coverage Bucket | Current Surface | Evidence |
| :--- | :--- | :--- | :--- |
| 1.6 Privileged Instruction Arbitration (`AXREAD`/`AXSET`/`AXVERIFY`) | Implemented (bounded) | Axion policy evaluation path and opcode-level arbitration hooks are present in policy engine and API plumbing | `kernel/axion/policy_engine.cpp`, `kernel/axion/axion_api.cpp`, `tests/cpp/test_axion_opcodes.cpp`, `tests/cpp/axion_policy_bytecode_test.cpp` |
| 1.9 Axion API & Policy Enforcement | Implemented (bounded) | Policy parser, bytecode serialization, and deterministic verdict path are implemented for guard and trace requirements | `include/t81/axion/policy.hpp`, `kernel/axion/policy_serialization.cpp`, `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/axion_policy_segment_event_test.cpp` |
| 1.10 CanonFS Observability | Partial | CanonFS hook and trace reason capture exist; broader persistence/audit lifecycle remains tied to downstream integration surfaces | `kernel/axion/canonfs_hook.cpp`, `tests/cpp/canonfs_axion_trace_test.cpp`, `tests/cpp/axion_segment_trace_test.cpp` |
| 1.3 Complexity Measurement | Partial | Instruction/recursion-bound checks are implemented; full call-graph/path-divergence instrumentation is not yet mapped in this cycle | `kernel/axion/policy_engine.cpp`, `kernel/axion/engine.cpp`, `tests/cpp/axion_instruction_counter_test.cpp`, `tests/cpp/test_axion_recursion_limit_ext.cpp` |
| 1.4 Tier Supervision | Partial | Tier ceiling checks are implemented in policy path; broader promotion/demotion orchestration remains outside mapped Axion kernel evidence in this cycle | `kernel/axion/policy_engine.cpp`, `tests/cpp/axion_stub_test.cpp` |
| 1.2 Safety & Ethics Enforcement | Partial | Ethics principle checks and guardrail verdicts exist; full policy-surface closure remains in-progress under M2/M3 evidence mapping | `kernel/axion/ethics.cpp`, `include/t81/axion/ethics.hpp`, `tests/cpp/axion_recursion_guardrails_test.cpp` |
| 1.1 Determinism Stewardship (end-to-end canonical trace stewardship) | Missing (mapped evidence gap) | Deterministic trace evidence exists for selected events, but complete stewardship coverage and canonical-memory enforcement evidence are not yet fully mapped in this plan cycle | `tests/cpp/axion_log_determinism_test.cpp`, `tests/cpp/axion_heap_compaction_trace_test.cpp`, `tests/cpp/vm_trace_test.cpp` |
| 2.5 Tier Transition Subsystem (full transitions across cognitive tiers) | Deferred | Tier-transition orchestration beyond current policy ceilings remains outside this plan cycle and is governed alongside experimental cognitive-tier maturation | `spec/cognitive-tiers.md`, `docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md` |

## M2 Prioritized Gap Queue (Top 3)

| Priority | Segment | Coverage Bucket | Rationale | Constraints | Owner | Target Date |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| P1 | 1.1 Determinism Stewardship (canonical trace stewardship and deterministic memory/canonicalization evidence closure) | Missing (mapped evidence gap) | Highest governance risk because determinism overclaim risk increases if stewardship evidence stays partial in status artifacts | Evidence-only closure; no runtime semantics, CI policy, freeze, or DCP scope changes | @t81dev | 2026-03-10 |
| P2 | 1.10 CanonFS Observability (persistence/audit lifecycle traceability) | Partial | High audit value: CanonFS persistence path is a primary observability surface for Axion verdict reasons | Keep claims bounded to current verified surfaces; map evidence without expanding guarantees | @t81dev | 2026-03-12 |
| P3 | 1.3 Complexity Measurement (call-path/branching coverage traceability) | Partial | Needed to justify tier/safety posture statements and reduce medium drift in Axion status row | Prioritize existing tests/surfaces; defer new instrumentation outside current planning cycle | @t81dev | 2026-03-14 |

Deferred note:

- 2.5 Tier Transition Subsystem remains deferred and non-DCP/non-verified unless
  promoted through governance and determinism-registry update.

## M3 Evidence Mapping for Prioritized Segments

| Priority | Segment | Evidence Path | Coverage Signal | Open/Uncovered Scope |
| :--- | :--- | :--- | :--- | :--- |
| P1 | 1.1 Determinism Stewardship | `tests/cpp/axion_log_determinism_test.cpp`, `tests/cpp/axion_heap_compaction_trace_test.cpp`, `tests/cpp/vm_trace_test.cpp` | Deterministic trace behavior and selected canonical trace events are covered in existing test surfaces | Full canonical-memory post-GC/compaction enforcement evidence mapping across all Axion-visible transitions remains open |
| P2 | 1.10 CanonFS Observability | `kernel/axion/canonfs_hook.cpp`, `tests/cpp/canonfs_axion_trace_test.cpp`, `tests/cpp/axion_segment_trace_test.cpp`, `tests/cpp/axion_policy_segment_event_test.cpp` | CanonFS policy hook integration and deterministic segment-event trace coverage are present | End-to-end persistence lifecycle audit closure (beyond current hook + trace checks) remains open |
| P3 | 1.3 Complexity Measurement | `kernel/axion/policy_engine.cpp`, `kernel/axion/engine.cpp`, `tests/cpp/axion_instruction_counter_test.cpp`, `tests/cpp/test_axion_recursion_limit_ext.cpp` | Instruction and recursion limit controls are covered with deterministic verdict behavior | Call-graph complexity and branch/path-divergence evidence remain open in this cycle |

Uncovered segments explicitly open:

1. 1.1 Determinism Stewardship:
   complete canonical-memory enforcement traceability across all Axion-visible
   transitions.
2. 1.10 CanonFS Observability:
   persistence lifecycle closure evidence beyond current policy-hook and
   segment-event trace checks.
3. 1.3 Complexity Measurement:
   call-graph/path-divergence measurement evidence mapping.

## Cross-References

- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/SYSTEM_STATUS.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

## Versioning Statement

This artifact is a status/control plan and does not override `/spec` or freeze
policy.
