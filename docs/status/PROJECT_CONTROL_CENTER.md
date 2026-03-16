# Project Control Center

Status: Active
Last Updated: 2026-03-16
Owner: @t81dev
Version: 3.2.5

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Maintenance** — v1.4.1-Stable; RFC program 100% complete (49/49 active RFCs accepted; 11 superseded); all drafts closed; no open blockers

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — **363/363 tests passing (100%)** |
| Release Readiness | **GO** — v1.4.1-Stable tagged; all RFC drafts closed 2026-03-16 |
| Current Main | `1d80abe8` — RFC-0015 agentic constructs (agent/behavior/AGENT_INVOKE); RFC-00A2 determinism benchmarks; RFC-0011/0015/00A2 accepted; AgentInvoke §5.16 in tisc-spec; 363/363 tests |
| Open Blockers | None |
| Frozen Core | Intact — AgentInvoke added as freeze exception (RFC-0015, §5.16) |
| Determinism Registry | All Verified surfaces clean; CanonHash81 determinism_score=1.0 confirmed |
| Structural Integrity | **Green** — 363/363 tests passing (100%); conformance suite 27/27 programs pass |

---

## Open Governance Gates

| Gate | Date | State |
| :--- | :--- | :--- |
| C2 Month-Close execution | 2026-03-31 | **Closed** — executed 2026-03-14; post-close addendum 2026-03-16; all 6 checklist items PASS; March 2026 governance window closed clean |
| T81Lang Stable promotion | 2026-03-16 | **Closed** — spec v1.3 Stable; §3.2 I/O channels defined; TG-01 waived (non-normative translations); all TG-02..TG-06 met |
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
| T81VM | **RESOLVED** | **✅ PROMOTED TO STABLE** - TV-01..TV-06 criteria satisfied; complete spec implementation (sections 0-7); deterministic execution verified (27/27 tests); runtime stability confirmed (54/54 VM tests); Axion integration stable (2/2 spec tests); performance benchmarks established (19.12s execution); production-ready deterministic virtual machine |
| T81Lang | **RESOLVED** | **✅ PROMOTED TO STABLE** - spec v1.3 Stable (2026-03-16); §3.2 VM I/O channels defined; all 11 sections complete; 363/363 tests; deterministic compilation verified (21 fixtures); feature registry 100%; translation waiver granted (non-normative, 2026-Q2). Review record: `T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md` |
| Axion Governance Kernel | **RESOLVED** | **✅ PROMOTED TO STABLE** - Beta candidacy review PASSED 2026-03-15; P4/P5 satisfied; AX-M6 verbatim reason-string concatenation implemented and verified (5/5 canonical reason tests); 54/54 tests passing; production-ready governance capabilities verified; §2.5 policy subsystem separation deferred to post-Stable |
| Axion OS Kernel | Medium | Experimental RFC-00B3 path active; kernel-owned handoff, MMU fault reporting, persistent runtime state, active device arbitration, runtime-owned scheduler/IPC execution, deterministic kernel loop, FIFO fault delivery, process-group fault policy with manual acknowledgement gate, an audit-only supervisor layer, the first service-facing runtime contract with healthy/faulted-group semantics, stable diagnostics, stable audit summaries, per-device ownership detail views, the first narrow service-facing action (supervisor fault-group acknowledgement), supervisor-facing recovery/report flows, a second narrow service-facing action for deterministic device claim/release, explicit request/action rejection semantics, and a first kernel-owned service runtime layer with deterministic service registration, deterministic service unregister, deterministic service suspend/resume, same-supervisor lifecycle control, explicit service health transitions, audit-visible service lifecycle transitions, retained supervisor inventory lifecycle metadata, per-entry supervisor inventory transition metadata, compact supervisor-status lifecycle metadata, aligned supervisor-recovery lifecycle metadata, aligned fault-summary lifecycle metadata, aligned runtime-status lifecycle metadata, aligned audit-summary lifecycle metadata, aligned device-summary lifecycle metadata, aligned service-status transition metadata, stable service detail, richer supervisor-owned inventory, explicit kernel-owned address-space ownership diagnostics, internal pager-needed fault-state diagnostics, deterministic internal pager handoff diagnostics, deterministic internal pager resolution diagnostics, and a real kernel-owned pager worker with duplicate unresolved fault coalescing, retained backlog/load diagnostics, ready-behind-active diagnostics, ready-backlog depth tracking, retained receipt identities/ordinals, active-work handoff ordinals, queued-head identities/ordinals, a bounded deterministic ready-bypass parking rule, retained ready-bypass deferral diagnostics, retained parked-cycle and parked-episode diagnostics, live parked-ready backlog diagnostics, retained parked-resumption diagnostics, retained parked-resume backlog diagnostics, retained parked-resume handoff diagnostics, retained parked-resumed-head handoff diagnostics, retained parked-resolved-head diagnostics, retained parked-resolved remaining-work diagnostics, retained parked-resolution follow-on diagnostics, retained parked-resolution successor completion diagnostics, retained blocker/blocked address-space identities, retained stall ordinals, retained blocked-side stall ordinals, retained blocked-side backlog depth, retained activation identities/ordinals, retained completion identities/ordinals, a terminal parked-head failure policy with retained terminal diagnostics, a kernel-owned boot-critical pager auto-resolution policy with retained resolution diagnostics, explicit boot-progress/fail runtime reporting, and a now-closed RFC-00B5 interrupt summary-convergence slice with kernel-owned interrupt intake, deterministic loop delivery, stable queue/accounting/audit surfaces, and record-level intake/delivery provenance are implemented; the current boot-ready kernel slice is closed, the local external boot-lane packaging phase is closed, staged ARM guest validation is in place, and a local QEMU x86_64 EFI diagnostic lane now executes the staged `BOOTX64.EFI` candidate and validates the shipped `x86_64` handoff contract. The next interrupt milestone is actual RFC-00B5 policy/handler behavior, while the external boot milestone remains actual `x86_64` VirtualBox host execution and evidence return (tracked in `experimental/ternaryos/docs/kernel_execution_plan.md`) |
| T81Graph | **RESOLVED** | **✅ PROMOTED TO BETA** - All graph operations work from language level; comprehensive test suite (6/6 passing); DCP verification complete; experimental status outdated |
| Experimental | — | Non-DCP by definition |

Full decomposition: `DRIFT_DECOMPOSITION.md`

---

## Top Risks

| ID | Risk | Severity |
| :--- | :--- | :--- |
| R-05 | AGI-facing surface growth outpacing promotion evidence updates | Medium |
| R-06 | Documentation maintenance burden after reorganization | Low |
| R-07 | Benchmark variability — false signal in `vm workload gate` guardrail | Low |

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

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · std.io/sys/async/agent

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — C2 Month-Close runbook executed and stamped in `docs/records/audits/2026-03-governance-review.md` (**Completed 2026-03-10**)
2. **2026-05-15** — T81Lang spec promotion: bytecode deterministic compilation profile; full spec-section traceability audit
3. **TBD** — RFC-00B5 interrupt policy: actual interrupt handler behavior (policy dispatch, vector table wiring)
4. **TBD** — TernaryOS bare-metal boot: x86_64 VirtualBox host execution + evidence return (see `experimental/ternaryos/docs/kernel_execution_plan.md`)
5. **Active now** — Fuzz corpus growth: run libFuzzer on `fuzz_parser` / `fuzz_vm`; commit any crash inputs

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
