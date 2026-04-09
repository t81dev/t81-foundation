# Project Control Center

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Project Control Center](#project-control-center)
  - [Phase](#phase)
  - [Classification Note](#classification-note)
  - [Program Health](#program-health)
  - [Open Governance Gates](#open-governance-gates)
  - [Drift Level](#drift-level)
  - [Top Risks](#top-risks)
  - [Active Hardening Work](#active-hardening-work)
  - [Experimental and Governed Non-DCP (Do Not Overclaim)](#experimental-and-governed-non-dcp-do-not-overclaim)
  - [Next Decision Points](#next-decision-points)
  - [Cockpit Index](#cockpit-index)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-04-01
Owner: @t81dev
Version: 1.10.0

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Handoff Hardening / RFC-00D1 Contract Clarification** — core release surface remains active; the highest-value work is currently keeping `main` boring, keeping status and handoff docs trustworthy, and preserving RFC-00D1 as a narrow finishable draft-to-code lane.

---

## Classification Note

- **DCP / verified deterministic surface**: explicitly inside the deterministic-core boundary and enforced through the registry, conformance evidence, and CI gates.
- **Governed non-DCP**: architecturally important and policy-bounded, but not yet entitled to full deterministic-surface claims.
- **Experimental / non-DCP**: outside release-grade deterministic guarantees unless later promoted through governance.

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green / Watch — core posture is healthy, with the main watch items now being portability churn and status/control-surface freshness rather than missing subsystem implementation |
| Release Readiness | **GO** for the shipped deterministic core profile; newer lanes remain governed by DCP / non-DCP / experimental boundaries and active CI evidence |
| Current Main | Rolling `main` — see GitHub for exact SHA/run state; this page is the control summary, not the per-commit audit |
| Open Blockers | No confirmed product blocker on current head; active concern is keeping portability-sensitive lanes and status/control docs boring and current |
| Frozen Core | Intact — AgentInvoke added as freeze exception (RFC-0015, §5.16) |
| Determinism Registry | Verified surfaces remain the only source of DCP-strength deterministic claims |
| Structural Integrity | Green — conformance, freeze, and determinism enforcement remain the governing release criteria |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| Deterministic-surface governance chain (RFC-0042..RFC-0053) | 2026-03-19 | **Open** — full horizontal and expansion draft layer added; backend/tritwise/epoch proof slices are now entering active CI and governance enforcement |
| C2 Month-Close execution | 2026-03-31 | **Closed** — executed 2026-03-14; post-close addendum 2026-03-16; all 6 checklist items PASS; March 2026 governance window closed clean |
| T81Lang Stable promotion | 2026-03-16 | **Closed** — language-spec stability review completed; this does not by itself promote the entire language toolchain into DCP / verified deterministic status |
| RFC-0015 agentic constructs closure | 2026-03-16 | **Closed** — agent/behavior/AGENT_INVOKE; 9/9 AC met; 16/16 assertions; tisc-spec §5.16 added |
| RFC-0011 grammar modernization closure | 2026-03-16 | **Closed** — 7/7 AC met; all features realized via RFC-0003/0007/0015/0029 |
| RFC-00A2 benchmark spec closure | 2026-03-16 | **Closed** — 6/6 AC met; BM_DeterminismValidation suite; determinism_score=1.0 |
| RFC-0031 + RFC-0032 closure | 2026-03-15 | **Closed** — both advanced to `accepted`; all 5 phases complete |
| RFC-0033 TUI closure | 2026-03-15 | **Closed** — advanced to `accepted`; all 4 phases complete; `t81 studio`/`agent`/`ui` shipped; snapshot tests + binary-size gate + user guide |
| RFC-DPE-0002 closure | 2026-03-15 | **Closed** — advanced to `accepted`; all 5 acceptance criteria met; `[DPE-02-01..05]` passing |
| RFC-0002 DEC closure | 2026-03-15 | **Closed** — advanced to `accepted`; §11 conformance tests fulfilled |
| AI A-series RFC final status | 2026-03-15 | **Closed** — 00A0/A1/A5/A8 superseded; 00A3/A4/A6 accepted |
| Axion Beta candidacy review | 2026-03-15 | **Closed** — GO stamped; P4 (§1.2) + P5 (§1.6) satisfied; AX-G01 met; 49/49 tests; review record `AXION_BETA_STABILITY_REVIEW_2026-03.md` |
| RFC sweep (0001/0003/0004/0005/0007/0009/0023/0024/0029/0030) | 2026-03-15 | **Closed** — all 10 advanced Draft → Accepted; spec/t81lang_features.md created; all remaining drafts subsequently closed 2026-03-16 |
| AX-M6 canonical reason verification | 2026-03-15 | **Closed** — 5/5 t81_test_axion_m6_canonical_reason passing; unknown fallback + action=unknown fallback fixed |
| Windows MSVC build | 2026-03-15 | **Closed** — C4996 (getenv), C4456 (local variable hiding), ir_generator vector_type shadow all resolved; /wd4996 /wd4456 added to MSVC suppressions |
| T81Graph lang-side serialization (BG-09) | 2026-05-15 | **Closed** |
| T3K spec document (T3K-S1) | 2026-04-30 | **Closed** |
| Deputy-approval policy (GOV-01) | 2026-04-30 | **Closed** |

---

## Drift Level

| Surface | Drift | Ref |
| :--- | :--- | :--- |
| TISC ISA | None | Frozen |
| Data Types | None | Frozen; audit closed 2026-02-27 |
| T81VM | Controlled | DCP / verified deterministic surface for interpreter execution and current supported-platform replay parity; broader VM-adjacent acceleration remains governed by the registry and RFC-0042..RFC-0053 |
| T81Lang | Controlled | Governed non-DCP overall; language spec is stable, but compiler/toolchain-wide deterministic promotion remains partial and evidence-bound |
| Axion Governance Kernel | Controlled | Governed non-DCP; important governance/runtime surface with active evidence, but broader kernel behavior is not automatically a verified deterministic surface |
| Axion OS | Medium | Governed non-DCP / experimental kernel track; significant implementation progress, but whole-system promotion remains evidence-gated and outside the current DCP |
| T81Graph | Medium | Governed non-DCP; useful language/VM surface, but not yet a promoted verified deterministic surface |
| Experimental | — | Experimental / non-DCP by definition |

Full decomposition: `DRIFT_DECOMPOSITION.md`

---

## Top Risks

| ID | Risk | Severity |
| :--- | :--- | :--- |
| R-05 | Governed non-DCP and draft surfaces could outpace boundary, registry, and evidence updates | Medium |
| R-06 | Documentation and handoff drift could reintroduce maintainer-memory dependence | Medium |
| R-07 | CI portability churn on Windows / ARM64 could keep `main` operationally noisy | Medium |

Full register: `ACTIVE_RISKS.md`

---

## Active Hardening Work

| Item | Owner | Target |
| :--- | :--- | :--- |
| CI-04 — Windows / ARM64 portability churn closure | @t81dev | 2026-04-15 |
| DOC-02 — handoff / status control-surface coherence refresh | @t81dev | 2026-04-05 |
| RFC-00D1-H2 — seed-contract clarification and remaining negative-path review | @t81dev | 2026-04-12 |

Full backlog: `HARDENING_BACKLOG.md`

Recent commit audit: `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`

---

## Experimental and Governed Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · std.io/sys/async/agent · broad Axion OS runtime claims beyond the current verified registry boundary

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **Now** — Keep `main` boring on the current workflow set, with particular attention to Windows, ARM64, and required-context stability
2. **2026-04-05** — Finish refreshing `/docs/status` so the control pages match the current CI and RFC posture
3. **2026-04-12** — Review any remaining RFC-00D1 negative-path or contract ambiguities without widening source kinds, target kinds, or interchange formats
4. **Next after RFC-00D1 follow-through** — Narrow RFC-00D0 into a resolver/descriptor prototype before any broader TCP/IP implementation work

---

## Cockpit Index

| Question | Document |
| :--- | :--- |
| What is frozen? | `FROZEN_CORE_PROFILE.md` |
| What is drifting? | `DRIFT_DECOMPOSITION.md` |
| What is risky? | `ACTIVE_RISKS.md` |
| What is blocked / being hardened? | `HARDENING_BACKLOG.md` |
| What is experimental / non-DCP? | `EXTENSION_PROFILE.md` |
| What is the review protocol? | `GOVERNANCE_REVIEW_CADENCE.md` |
| What do CI gates look like? | `CI_GATE_STATUS.md` |
| What decisions were made and why? | `DECISION_LOG.md` |
| What does the spec-to-impl matrix say? | `IMPLEMENTATION_MATRIX.md` |
| What is the audit history? | `DETERMINISM_AUDIT_LOG.md` |
| What are the dependency versions? | `DEPENDENCY_HEALTH.md` |
