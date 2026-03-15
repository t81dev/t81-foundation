# Project Control Center

Status: Active
Last Updated: 2026-03-15
Owner: @t81dev
Version: 3.2.3

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Maintenance** — v1.4.0-Stable cut complete; RFC-0031/0032/0033 accepted; AI subsystem fully promoted; Axion Beta candidacy confirmed 2026-03-15; DPE-0002 accepted; no open blockers

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — **344/344 tests passing (100%)** |
| Release Readiness | **GO** — v1.4.0-Stable tagged; v1.3.2 stable released 2026-03-08 |
| Current Main | `50fff89c` — RFC-0031/0032 accepted; AI A-series RFC final status; axion-event-registry; 3 AI conformance programs; build system fixed (stale subbuild caches cleared, Ninja reconfigure) |
| Open Blockers | None |
| Frozen Core | Intact — no freeze exceptions |
| Determinism Registry | All Verified surfaces clean |
| Structural Integrity | **Green** — 344/344 tests passing (100%); conformance suite 27/27 programs pass |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| C2 Month-Close execution | 2026-03-31 | Scheduled — preflight PASS 2026-03-10; re-confirmed 3× same day |
| RFC-0031 + RFC-0032 closure | 2026-03-15 | **Closed** — both advanced to `accepted`; all 5 phases complete |
| RFC-0033 TUI closure | 2026-03-15 | **Closed** — advanced to `accepted`; all 4 phases complete; `t81 studio`/`agent`/`ui` shipped; snapshot tests + binary-size gate + user guide |
| RFC-DPE-0002 closure | 2026-03-15 | **Closed** — advanced to `accepted`; all 5 acceptance criteria met; `[DPE-02-01..05]` passing |
| RFC-0002 DEC closure | 2026-03-15 | **Closed** — advanced to `accepted`; §11 conformance tests fulfilled |
| AI A-series RFC final status | 2026-03-15 | **Closed** — 00A0/A1/A5/A8 superseded; 00A3/A4/A6 accepted |
| Axion Beta candidacy review | 2026-03-15 | **Closed** — GO stamped; P4 (§1.2) + P5 (§1.6) satisfied; AX-G01 met; 49/49 tests; review record `AXION_BETA_STABILITY_REVIEW_2026-03.md` |
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
| T81Lang | **RESOLVED** | Frontend refactor complete: typed AST, unified builtin registry, IRGen to .cpp; 344/344 tests passing (100%) |
| Axion Governance Kernel | Low | AX-M5..M7 evidence landed; **Beta candidacy review PASSED 2026-03-15** — P4/P5 satisfied, next review 2026-06-30 |
| Axion OS Kernel | Medium | Experimental RFC-00B3 path active; kernel-owned handoff, MMU fault reporting, persistent runtime state, active device arbitration, runtime-owned scheduler/IPC execution, deterministic kernel loop, FIFO fault delivery, process-group fault policy with manual acknowledgement gate, an audit-only supervisor layer, the first service-facing runtime contract with healthy/faulted-group semantics, stable diagnostics, stable audit summaries, per-device ownership detail views, the first narrow service-facing action (supervisor fault-group acknowledgement), supervisor-facing recovery/report flows, a second narrow service-facing action for deterministic device claim/release, explicit request/action rejection semantics, and a first kernel-owned service runtime layer with deterministic service registration, deterministic service unregister, deterministic service suspend/resume, same-supervisor lifecycle control, explicit service health transitions, audit-visible service lifecycle transitions, retained supervisor inventory lifecycle metadata, per-entry supervisor inventory transition metadata, compact supervisor-status lifecycle metadata, aligned supervisor-recovery lifecycle metadata, aligned fault-summary lifecycle metadata, aligned runtime-status lifecycle metadata, aligned audit-summary lifecycle metadata, aligned device-summary lifecycle metadata, aligned service-status transition metadata, stable service detail, richer supervisor-owned inventory, explicit kernel-owned address-space ownership diagnostics, internal pager-needed fault-state diagnostics, deterministic internal pager handoff diagnostics, deterministic internal pager resolution diagnostics, and a real kernel-owned pager worker with duplicate unresolved fault coalescing, retained backlog/load diagnostics, ready-behind-active diagnostics, ready-backlog depth tracking, retained receipt identities/ordinals, active-work handoff ordinals, queued-head identities/ordinals, a bounded deterministic ready-bypass parking rule, retained ready-bypass deferral diagnostics, retained parked-cycle and parked-episode diagnostics, live parked-ready backlog diagnostics, retained parked-resumption diagnostics, retained parked-resume backlog diagnostics, retained parked-resume handoff diagnostics, retained parked-resumed-head handoff diagnostics, retained parked-resolved-head diagnostics, retained parked-resolved remaining-work diagnostics, retained parked-resolution follow-on diagnostics, retained parked-resolution successor completion diagnostics, retained blocker/blocked address-space identities, retained stall ordinals, retained blocked-side stall ordinals, retained blocked-side backlog depth, retained activation identities/ordinals, retained completion identities/ordinals, a terminal parked-head failure policy with retained terminal diagnostics, a kernel-owned boot-critical pager auto-resolution policy with retained resolution diagnostics, explicit boot-progress/fail runtime reporting, and a now-closed RFC-00B5 interrupt summary-convergence slice with kernel-owned interrupt intake, deterministic loop delivery, stable queue/accounting/audit surfaces, and record-level intake/delivery provenance are implemented; the current boot-ready kernel slice is closed, the local external boot-lane packaging phase is closed, staged ARM guest validation is in place, and a local QEMU x86_64 EFI diagnostic lane now executes the staged `BOOTX64.EFI` candidate and validates the shipped `x86_64` handoff contract. The next interrupt milestone is actual RFC-00B5 policy/handler behavior, while the external boot milestone remains actual `x86_64` VirtualBox host execution and evidence return (tracked in `experimental/ternaryos/docs/kernel_execution_plan.md`) |
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
| QA-01 — CLI stress test covering full command surface (338th test) | @t81dev | **Closed 2026-03-10** |

Full backlog: `HARDENING_BACKLOG.md`

Recent commit audit: `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`

---

## Experimental / Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · T81Graph · std.io/sys/async/agent

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — C2 Month-Close runbook executed and stamped in `docs/records/audits/2026-03-governance-review.md` (**Completed 2026-03-10**)
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
