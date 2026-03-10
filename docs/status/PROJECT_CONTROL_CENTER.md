# Project Control Center

Status: Active
Last Updated: 2026-03-10
Owner: @t81dev
Version: 3.2.3

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Post-Hardening** — Documentation reorganization completed; focus on maintenance and optimization

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — **336/336 tests passing** (100% success rate) |
| Release Readiness | **GO** — v1.4.0-beta tagged; v1.3.2 stable released 2026-03-08 |
| Current Main | `85a0b438` — CLI stress test + binary_io OOM hardening |
| Open Blockers | None |
| Frozen Core | Intact — no freeze exceptions |
| Determinism Registry | All Verified surfaces clean |
| Structural Integrity | **PERFECT** — 336/336 tests; CLI stress test now covers full command surface |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| C2 Month-Close execution | 2026-03-31 | Scheduled — preflight PASS 2026-03-10; re-confirmed 3× same day |
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
| T81VM | Low | FW-02 closure landed; BG-07 closure landed; 3 OOB reg-index bugs fixed (SymLoad, ReflCap, ReflJustify); binary_io OOM-on-corrupt-input hardened |
| T81Lang | **RESOLVED** | Frontend refactor complete: typed AST, unified builtin registry, IRGen to .cpp; 336/336 test coverage |
| Axion Kernel | Low | AX-M5..M7 evidence landed; awaiting Beta review cycle |
| T81Graph | Low | Lang-side serialization wired; determinism coverage in place |
| Experimental | — | Non-DCP by definition |

Full decomposition: `DRIFT_DECOMPOSITION.md`

---

## Top Risks

| ID | Risk | Severity |
| :--- | :--- | :--- |
| R-01 | Determinism overclaim — registry boundary language omitted externally | High |

Full register: `ACTIVE_RISKS.md`

---

## Active Hardening Work

| Item | Owner | Target |
| :--- | :--- | :--- |
| BG-07 — BigInt precision scope resolution (phase 2) | @t81dev | **Closed 2026-03-05** |
| FW-02 — VM dispatch concentration reduction | @t81dev | **Closed 2026-03-05** |
| FW-01 — dependency firewall waiver retirement | @t81dev | **Closed 2026-03-05** |
| SEC-01 — Fuzz infrastructure (fuzz_parser, fuzz_vm) + 3 OOB VM fixes | @t81dev | **Closed 2026-03-10** |
| SEC-02 — binary_io OOM-on-corrupt-input (read_checked_size guard) | @t81dev | **Closed 2026-03-10** |
| QA-01 — CLI stress test covering full command surface (336th test) | @t81dev | **Closed 2026-03-10** |

Full backlog: `HARDENING_BACKLOG.md`

Recent commit audit: `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`

---

## Experimental / Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · T81Graph · std.io/sys/async/agent

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — Execute C2 runbook; stamp outcome in `docs/records/audits/2026-03-governance-review.md`
2. **Pre-C2 / Active now** — Axion Beta candidacy evidence for P4 (§1.2 Safety & Ethics) and P5 (§1.6 Privileged Instruction); both explicitly mapped to existing verified surfaces (Owner: @t81dev, Date: 2026-03-10), fully satisfying AX-G01.
3. **Active now** — Fuzz corpus growth: run libFuzzer for hours on `fuzz_parser` / `fuzz_vm`; commit any crash inputs
4. **Post-C2 Release Prep** — Cut v1.4.0-Stable from v1.4.0-beta; determinism coverage for new binary_io guard path (**Completed 2026-03-10**)

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
