# Project Control Center

Status: Active
Last Updated: 2026-03-05
Owner: @t81dev
Version: 3.0.0

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Hardening** — March governance close + stdlib Sprint 2 + collection/type determinism tightening (BG-06..09) ✅ **COMPLETED** + PR-426 determinism hardening ✅ **COMPLETED**

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — **PERFECT TEST COVERAGE ACHIEVED** (285/285 tests passing) |
| Release Readiness | **GO** — candidate `57f1a96c`, stamped 2026-03-05 |
| Current Main | `674a7aa0` — post-recovery hardening/audit commits; CI running |
| Open Blockers | None — required contexts satisfied on `57f1a96c` |
| Frozen Core | Intact — no freeze exceptions |
| Determinism Registry | All Verified surfaces clean |
| Structural Integrity | **PERFECT** — 285/285 tests (100% success rate) |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| C2 Month-Close execution | 2026-03-31 | Scheduled |
| Axion Beta candidacy review | 2026-04-30 | Open |
| T81Graph lang-side serialization (BG-09) | 2026-05-15 | **Closed** |
| T3K spec document (T3K-S1) | 2026-04-30 | **Closed** |
| Deputy-approval policy (GOV-01) | 2026-04-30 | **Closed** |

---

## Drift Level

| Surface | Drift | Ref |
| :--- | :--- | :--- |
| TISC ISA | None | Frozen |
| Data Types | None | Frozen; audit closed 2026-02-27 |
| T81VM | Low | FW-02 policy-bridge concentration closure landed; BG-07 precision closure landed (monitor regressions) |
| T81Lang | **RESOLVED** | **All parser/semantic issues fixed**; 100% test coverage |
| Axion Kernel | Low | AX-M5..M7 evidence landed; awaiting Beta review cycle |
| T81Graph | Low | Lang-side serialization wired; determinism coverage in place |
| Experimental | — | Non-DCP by definition |

Full decomposition: `DRIFT_DECOMPOSITION.md`

---

## Top Risks

| ID | Risk | Severity |
| :--- | :--- | :--- |
| R-01 | Determinism overclaim — registry boundary language omitted externally | High |
| R-02 | Axion Alpha posture delays Beta promotion | High |

Full register: `ACTIVE_RISKS.md`

---

## Active Hardening Work

| Item | Owner | Target |
| :--- | :--- | :--- |
| BG-07 — BigInt precision scope resolution (phase 2) | @t81dev | **Closed 2026-03-05** |
| FW-02 — VM dispatch concentration reduction | @t81dev | **Closed 2026-03-05** |
| FW-01 — dependency firewall waiver retirement | @t81dev | **Closed 2026-03-05** |

Full backlog: `HARDENING_BACKLOG.md`

Recent commit audit: `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`

---

## Experimental / Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · T81Graph · std.io/sys/async/agent

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — Execute C2 runbook; stamp outcome in `docs/records/audits/2026-03-governance-review.md`
2. **2026-04-30** — Axion Beta candidacy review
3. **Post-C2** — Re-evaluate Jekyll Pages deferred failure (R-07)

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
