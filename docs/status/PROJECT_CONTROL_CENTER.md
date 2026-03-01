# Project Control Center

Status: Active
Last Updated: 2026-02-28
Owner: @t81dev
Version: 3.0.0

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Hardening** — March governance close + stdlib Sprint 2 + collection/type determinism tightening (BG-06..09)

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — one scheduled gate pending (C2 close 2026-03-31) |
| Release Readiness | **GO** — candidate `1ec312e3`, stamped 2026-02-28 |
| Open Blockers | None — required contexts satisfied on `1ec312e3` |
| Frozen Core | Intact — no freeze exceptions |
| Determinism Registry | All Verified surfaces clean |
| Structural Integrity | Pass — 247/247 tests |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| C2 Month-Close execution | 2026-03-31 | Scheduled |
| Axion Beta candidacy review | 2026-04-30 | Open |
| T81Graph lang-side serialization (BG-09) | 2026-05-15 | Open |
| T3K spec document (T3K-S1) | 2026-04-30 | Open |
| Deputy-approval policy (GOV-01) | 2026-04-30 | Open |

---

## Drift Level

| Surface | Drift | Ref |
| :--- | :--- | :--- |
| TISC ISA | None | Frozen |
| Data Types | None | Frozen; audit closed 2026-02-27 |
| T81VM | Low | Policy-bridge dispatch concentration (FW-02) |
| T81Lang | Medium | BG-06..09 open; fixture-bounded determinism only |
| Axion Kernel | Medium | §1.1/1.3/1.10 evidence gaps (AX-M5..M7) |
| T81Graph | High | No determinism tests; lang serialization not wired |
| Experimental | — | Non-DCP by definition |

Full decomposition: `DRIFT_DECOMPOSITION.md`

---

## Top Risks

| ID | Risk | Severity |
| :--- | :--- | :--- |
| R-01 | Determinism overclaim — registry boundary language omitted externally | High |
| R-02 | Axion Alpha posture delays Beta promotion | High |
| R-03 | Single-owner concentration — all GO/HOLD gated on @t81dev | Medium |

Full register: `ACTIVE_RISKS.md`

---

## Active Hardening Work

| Item | Owner | Target |
| :--- | :--- | :--- |
| AX-M5/M6/M7 — Axion evidence closure | @t81dev | 2026-03-14 |
| BG-06 — Collection determinism tests | @t81dev | 2026-05-15 |
| BG-09 — Lang-side serialization wiring | @t81dev | 2026-05-15 |
| FW-02 — VM dispatch concentration reduction | @t81dev | 2026-04-15 |
| std.io/sys/async/agent fixture suites | @t81dev | Ongoing |

Full backlog: `HARDENING_BACKLOG.md`

---

## Experimental / Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · T81Graph · std.io/sys/async/agent

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — Execute C2 runbook; stamp outcome in `docs/records/audits/2026-03-governance-review.md`
2. **2026-04-30** — Axion Beta candidacy review; GOV-01 deputy-approval policy deadline
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
