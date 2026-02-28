# Project Control Center

Status: Active Program Management
Last Updated: 2026-02-28
Owner: Project Management / Governance
Version: 2.1.0

## 1. At-a-Glance

| Item | Status |
| :--- | :--- |
| Sprint Focus | March governance close + stdlib Sprint 2 + BG-06..10 collection/type determinism hardening |
| Overall Program Health | Green with one scheduled governance gate pending |
| Release Readiness Decision | **GO** (stamped 2026-02-28, candidate `fa343e1f`) |
| Primary Blocker | None — required contexts satisfied on `fa343e1f`; non-required Pages/Jekyll failure deferred |
| Next Hard Date | C2 month-close execution on 2026-03-31 (UTC) |
| Open Task Count (`docs/roadmaps-plans/TASKS.md`) | 1 (`C2 Month-Close`) |

## 2. Purpose

Provide the control-plane view for delivery status, governance posture, active
risks, and near-term execution priorities.

## 3. Scope

This document is a roll-up, not a submodule ledger. It tracks:

- governance and release controls
- determinism and policy boundaries
- execution status and blockers
- escalation-level risks

Authority remains:
`/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

## 4. Current Snapshot

### 4.1 Governance and Release

- March packet re-evaluated on 2026-02-28: GO recommended on candidate `fa343e1f`.
  See `docs/status/RELEASE_READINESS_PACKET_2026-03.md` for full verification snapshot.
- Required-context verification results for `fa343e1f`:
  - `quality gate / required`: completed / success ✅
  - `Analyze (cpp)`: completed / success ✅
- Non-required failure: `build` (GitHub Pages / Jekyll) — classified as deferred;
  root cause is Liquid rendering of `third_party/llama.cpp` modelcard templates.
- CodeQL push-trigger remediation (`ad6c2777`) confirmed effective on post-fix main commit.
- Final GO/HOLD decision stamp pending @t81dev approval.

### 4.2 Delivery Progress

- T81Lang posture is synchronized as:
  - spec authority: Draft (`spec/t81lang-spec.md`)
  - implementation maturity: Beta (`docs/status/T81LANG_PROMOTION_GATE.md`)
- A1 through A1G closure tracks are complete and synchronized.
- Governed llama.cpp integration backlog items are complete.
- Behavioral Conformance Expansion Sprint (2026-02-26) is complete:
  - VM/Axion conformance matrix suites expanded.
  - CanonFS integrity matrix expanded (truncate/append/default-read-verify).
  - Workload determinism tiers expanded (policy-heavy + tensor-access).
  - Translation semantic gate upgraded with required section-heading parity checks.
  - CI benchmark gate expanded with VM workload dispatch/native ratio guardrail.
- Behavioral Conformance Expansion Phase 3 is in progress:
  - deterministic VM trap-family matrix coverage added
  - CanonFS read-verify env-contract matrix coverage added
  - deterministic `TLOADHASH` decode-fault matrix coverage added
  - deterministic `TLOADHASH` status classification matrix expanded (`InvalidHash`/`CanonFsMiss`/`DecodeFault`)
  - mixed workload conformance matrix (policy+tensor+memory+branch) added
  - mixed workload matrix now includes deterministic policy-deny branch-path case
  - Axion conformance matrix now includes clause-ordering equivalence invariants with deterministic Axion-event signatures
  - Axion opcode dispatch concentration reduced in VM via extracted `AxCheck`/`AxReport` helper paths
  - blocked-neural and bitwise opcode-family dispatch concentration reduced via extracted helper paths
  - VM trace/log helper extraction advanced in policy-trace bridge
- T81Lang stdlib stabilization track is active:
  - Sprint 1 stabilization package is complete:
    - governed stabilization plan published (`docs/status/STDLIB_STABILIZATION_PLAN_2026-03.md`)
    - stdlib surface baseline CI/governance gate added (`scripts/governance/check_stdlib_surface_baseline.py`)
    - stdlib promotion snapshot governance gate added (`scripts/governance/check_stdlib_promotion_snapshot.py`)
    - `std.math`/`std.core` fixture conformance suites added and wired in CMake/CTest
    - stdlib change policy published (`docs/governance/STDLIB_CHANGE_POLICY.md`)
- Experimental backlog implementation items are complete:
  - Cognitive Tier 1..5 closures
  - Runtime JIT prototype backend uplift
  - CanonFS persistent write-path optimization
  - CLI `t81 trace export` (JSON/CSV)
  - Debugger cognitive tier inspection (`t [1|2|3|4|5|all]`)
- 2026-02-26..28 implementation sprint (183 commits, 6 workstreams):
  - Collections (`List`, `Map`, `Set`, `Tree`) exposed as first-class T81Lang frontend types with canonical serialization; IR generator lowering and conformance tests added (PRs #401, #419).
  - Language-surface completeness hardening: `T81Quaternion`, `T81Prob`, `Cell` exposed in Lexer/Parser/Semantic Analyzer with deterministic lowering stubs; `serialize_canonical` added to 10 type headers; `t81lang_surface_gate_test` and updated AST/IR repro hash baseline committed (PR #420).
  - Core data types determinism audit and remediation: `Cell` signed-overflow UB fixed; `T81Float` signed-zero canonicalization enforced; `T81Map`/`T81Set` frontend type enforcement hardened; report published in `docs/reports/determinism_types_audit.md` (PRs #414, #415).
  - T81Lang stress test suite launched: 7 modules in `tests/stress/` (numeric, symbolic, container, tensor, monad, agent, composition) — PR #404.
  - T81Graph lowered to VM native opcodes; lang-side canonical serialization gap identified (BG-09) and tracked in engineering backlog (PR #424).
  - Stdlib fixture suites extended beyond `std.core`/`std.math`: `std.polynomial`, `std.symbolic`, `std.symbol`, and additional `std.collections` (Map/Set ops) fixture conformance suites added.
  - Surface gap inventory published: `docs/status/T81LANG_SURFACE_INVENTORY.md` documents all 36 exposed types with backend/determinism/serialization status; 5 new engineering gaps captured as BG-06..BG-10 in `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md`.
- Test suite deduplication and condensation complete (`7724578e`, 2026-02-28):
  - 16 files removed, −1,780 LOC, 285/285 tests passing.
  - 10 per-module `cli_std_*_fixtures_test.cpp` files consolidated into single parameterized `cli_stdlib_fixtures_test.cpp`; new stdlib modules require a one-line STDLIB_MODULES entry.
  - Shared utilities extracted: `test_sig_util.hpp` (signature mixing), `test_fixture_util.hpp` (fixture I/O).
  - Audit: `docs/status/TEST_SUITE_DEDUP_AUDIT_2026-02-28.md`.

### 4.3 C2 Month-Close Readiness

- Final C2 execution remains scheduled for 2026-03-31.
- Runbook exists:
  `docs/status/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`.
- Consolidated execution helper exists:
  `scripts/governance/c2_month_close_check.py`.
- One-command preflight helper exists:
  `scripts/governance/c2_month_close_preflight.py`.
- Latest prep report:
  `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md` (latest run: PASS).
- Latest preflight report:
  `docs/status/C2_MONTH_CLOSE_PREFLIGHT_2026-03-31.md` (latest run: PASS).

## 5. Active Workstreams

1. C2 month-close finalization
   Date: 2026-03-31
   Exit criteria: final checklist re-run and final stamp in
   `docs/records/audits/2026-03-governance-review.md`.
2. Release candidate required-context closure
   Exit criteria: required contexts satisfied on selected candidate SHA and
   packet decision re-evaluated.
3. Governance continuity maintenance
   Exit criteria: hygiene/link integrity and artifact synchronization sustained
   through close window.

## 6. Risks

### 6.1 High

- Required-context mismatch risk can keep March packet in HOLD.
- Determinism overclaim risk if external summaries omit registry boundaries.

### 6.2 Medium

- Single-owner concentration for governance/release approvals.
- AGI-facing surface growth outpacing promotion evidence updates.

### 6.3 Monitoring

- Experimental scope creep without explicit promotion/retirement actions.
- Benchmark variability and environment-sensitive signal noise.

## 7. Next Decisions

1. 2026-03-31: Execute C2 runbook and stamp final outcome fields.
2. Post-required-context pass: Re-evaluate GO/HOLD in the March packet.

## 8. Governance Directives

1. Spec-first policy remains mandatory for normative behavior changes.
2. Freeze boundaries remain unchanged.
3. Determinism claims remain limited to Verified registry surfaces.
4. Public API contract remains `include/t81/**` only.
5. Root-level documentation additions require explicit governance approval.

## 9. Cross-References

- `docs/roadmaps-plans/TASKS.md`
- `docs/records/status-history/BEHAVIORAL_CONFORMANCE_EXPANSION_SPRINT_2026-02-26.md`
- `docs/status/BEHAVIORAL_CONFORMANCE_EXPANSION_PHASE3_2026-02-26.md`
- `docs/status/STDLIB_STABILIZATION_PLAN_2026-03.md`
- `docs/status/T81LANG_SURFACE_INVENTORY.md`
- `docs/reports/determinism_types_audit.md`
- `docs/status/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/status/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`
- `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md`
- `docs/status/C2_MONTH_CLOSE_PREFLIGHT_2026-03-31.md`
- `docs/records/status-history/COGNITIVE_TIERS_SPEC_COMPLIANCE_2026-02-26.md`
- `docs/records/audits/2026-03-governance-review.md`
- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`

## 10. Versioning Statement

This control document is operational governance. Updates must preserve
authority hierarchy and may not weaken freeze or determinism boundaries.
