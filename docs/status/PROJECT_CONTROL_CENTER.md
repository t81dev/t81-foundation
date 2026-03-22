# Project Control Center

Status: Active
Last Updated: 2026-03-22
Owner: @t81dev
Version: 1.9.0

This is the dashboard. One page. If you need detail, follow a cross-reference.

---

## Phase

**Maintenance / Governance Alignment** — v1.9.0-Stable; core release surface remains active, while RFC-0042 through RFC-0053 are open draft governance layers that define backend equivalence, validation, packed-trit substrate, memory, scheduling, JIT, deterministic-surface boundaries, arithmetic semantics, vector semantics, heterogeneous acceleration, dataflow, and distributed execution.

---

## Classification Note

- **DCP / verified deterministic surface**: explicitly inside the deterministic-core boundary and enforced through the registry, conformance evidence, and CI gates.
- **Governed non-DCP**: architecturally important and policy-bounded, but not yet entitled to full deterministic-surface claims.
- **Experimental / non-DCP**: outside release-grade deterministic guarantees unless later promoted through governance.

---

## Program Health

| Dimension | Status |
| :--- | :--- |
| Overall | Green — release branch remains healthy; determinism and structural-integrity gates remain the controlling signals |
| Release Readiness | **GO** for the current shipped core profile; broader verticals remain classified per DCP / governed non-DCP / experimental boundaries |
| Current Main | `64954188` — VM fuzz security fix: 3-wave int-narrowing OOB closed in reg_ok/mem_ok/check_mem + 5 Axion handler sites; 9 crashes fixed; 406/406 tests passing |
| Open Blockers | None |
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
| R-05 | Governed non-DCP surface growth outpacing boundary and evidence updates | Medium |
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
| RFC-0046 proof enforcement — Axion epoch scheduler/audit parity lane | @t81dev | **Closed 2026-03-19** |
| Track K — HostFloat result-rep fix (TExp/TSiLU/TSoftmax; 7 eligibility guard sites) | @t81dev | **Closed 2026-03-22** |
| Track L — HostFloat matmul fast path (`deterministic_fma` bypass + lazy result construction) | @t81dev | **Closed 2026-03-22** |

Full backlog: `HARDENING_BACKLOG.md`

Recent commit audit: `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`

---

## Experimental and Governed Non-DCP (Do Not Overclaim)

Cognitive Tiers · Hanoi VM · Distributed · Trace-JIT · llama.cpp adapter · std.io/sys/async/agent · broad Axion OS runtime claims beyond the current verified registry boundary

Full boundary: `EXTENSION_PROFILE.md`

---

## Next Decision Points

1. **2026-03-31** — C2 Month-Close runbook executed and stamped in `docs/records/audits/2026-03-governance-review.md` (**Completed 2026-03-10**)
2. **2026-05-15** — T81Lang promotion follow-on: bytecode deterministic compilation profile; full spec-section traceability audit; clarify compiler-surface promotion against RFC-0043/RFC-0048
3. **Closed** — RFC-00B5 interrupt governance: Slice 28 `UnhandledInterruptDropped` done; RFC-00B5 → `integrated`; 3214/3214 ternaryos assertions
4. **Closed** — TernaryOS QEMU x86_64 EFI boot lane: BOOTX64.EFI validated, all 5 contract files verified, `hal_main_result=0`, `kernel_boot_ready_slice=complete` (evidence: `TERNARYOS_X86_64_BOOT_EVIDENCE_2026-03-16.md`)
5. **Closed 2026-03-22** — TernaryOS ARMv8 QEMU boot lane: BOOTAA64.EFI executed under QEMU HVF (Apple Silicon); 10/10 contract files verified, `hal_main_result=0`, `kernel_boot_ready_slice=complete`, `phase=5`, `shell_mode=typed-builtins` (evidence: `TERNARYOS_ARMV8_QEMU_BOOT_EVIDENCE_2026-03-22.md`). VirtualBox headless blocked by macOS 15 Gatekeeper symlink-escape policy (VirtualBox 7.2.97 packaging issue); QEMU HVF is the authoritative ARMv8 boot evidence path on Apple Silicon.
6. **Closed 2026-03-22** — Fuzz corpus growth: 3-wave int-narrowing OOB fix in `vm/vm.cpp` + `mem_ok`; 9 crash inputs fixed + archived; 79-entry VM corpus committed; 120s clean run (113k execs); 406/406 passing
7. **Active now** — Governance closure path: connect RFC-0042..RFC-0053 to registry, threat model, promotion evidence, and CI gate inventory so public status claims remain classification-correct
8. **Closed 2026-03-22** — Axion runtime-integration evidence gap: `axion_agent_invoke_policy_test` ([AI-01..05]) proves policy engine is wired to `AgentInvoke` dispatch; 406/406 passing

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
