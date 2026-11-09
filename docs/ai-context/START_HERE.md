> **Latest verified program status (2026-11-09):** Read [T81_RESEARCH_STATUS_2026-11-09.md](T81_RESEARCH_STATUS_2026-11-09.md) **before** the 2026-11-08 snapshot and dated operational views below. PR #741 (SCG-07 Phase 6) is merged into main at `8dec43ac2a9a702c9ae54229cf6891ee63f285c0`; F1–F7 are **QUALIFIED WITH LIMITATIONS**, not universally proven. SCG-08 Phase 0 is a proposed documentation-only research charter pending Jules; no new experiment or authorization is established. Older statements that F1–F7 are still awaiting repair are historical, not current.

> **Latest verified program status (2026-11-08):** Read [T81_COMPLETE_STATUS_2026-11-08.md](T81_COMPLETE_STATUS_2026-11-08.md) **before** older operational and research-gate summaries. Main observed at `e45c5435aa47d4ccb8f343798214eb917352c871`. SCG-03 (#729) and SCG-04 (#731) are merged; SCG-05 (#732) and SCG-06 (#733) remain open/unmerged. SCG-06 reports F4 fixed and F6 changed from silent corruption to termination; F1–F3/F5/F7 remain unresolved. PR #733 needs evidence corrections, including F2/F3 oracle coverage and the nine-check CI attribution. This is a non-normative dated status; it supersedes older *operational next-step* suggestions, not historical experiment evidence or authorizations.

# T81 Research Continuity — START_HERE

**Latest Cross-Profile Architecture & Identity View (2026-11-05):** T81 has demonstrated a functioning bounded computational architecture. Bounded cross-profile TISC execution is verified across hosted T81VM and freestanding Axion OS (AArch64 QEMU) against an independent mathematical oracle across all 25 observation boundaries (CPA-004/CPA-005). Incoming research agents should **not** re-verify whether the bounded cross-profile implementation works; focus on open research hypotheses, remaining ISA discrepancies, and disciplined comparative advantage benchmarks.

**Latest BASE-01 integration and readiness view (2026-11-03):** Read [BASE01_2026-11-03.md](BASE01_2026-11-03.md) first. PRs #694 and #695 are merged; integration acceptance does not authorize B1 measurement. Older dated operational views below remain historical.

**Latest operational view (2026-11-02):** Read the selected view in
[`CURRENT_STATE.json`](CURRENT_STATE.json), backed by the bounded
[PR/source receipt](receipts/2026-11-02-pr678-observations.json), before older status tables.
Observed main is `8e44c0f667c62e0d4143bb93169fc3235d5ebd66`: #675 is merged with
arithmetic validation accepted within its declared scope; #676 is merged historical
backlog-audit evidence; #677 is merged H04-R03 Candidate A preregistration, with
**Phase C still unauthorized as of 2026-11-02**. At the 2026-11-02 snapshot, H04-R03 Candidate A implementation and correctness validation were reported and PRs #679/#680 were open; a subsequent 2026-11-06 repository check found both merged, without implying Phase C authorization. #656 remains open pending integration and #678
remains open with corrections pending acceptance. Local cleanup's audit is reported
completed; its execution outcome is **not independently verified** by this index.
The earlier views and snapshots below remain dated history. This update grants no
merge, measurement, M2 execution or governance authority.

**Document ID:** `docs/ai-context/START_HERE.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
**Status:** Non-normative navigation and research handoff index; not an accepted contract or independent verification

---

## 1. T81 Identity in One Paragraph

T81 Foundation is an implemented experimental computing architecture and scientific research platform investigating alternative digital representations (balanced ternary, Base-81 tryte grouping, adaptive radix tiering) and policy-governed deterministic execution. T81 defines TISC (Ternary Instruction Set Computer) as its core computational instruction contract, with bounded cross-profile execution demonstrated across both a hosted virtual machine (T81VM) and a bare-metal freestanding microkernel environment (Axion OS on AArch64 QEMU). The program combines scientific research into mathematical, algorithmic, and physical substrate advantages (Track R) with foundation systems engineering for bit-exact cross-platform determinism, content-addressed artifact identity via CanonFS, pre-side-effect policy authorization via Axion, and detached accountable evidence verification (Track F). T81 is not a commercial desktop operating system or a production CPU product; it is a functioning experimental computational architecture and hypothesis-driven empirical research instrument designed to determine under what exact assumptions, workloads, cost models, and physical substrates non-binary computational representations yield defensible advantages over optimized conventional baselines.

---

## 2. Research Mission and Boundaries

The central scientific question of T81 is:

> **Under what assumptions, workloads, cost models, and physical substrates can ternary or alternative computational representations demonstrate reproducible, practically meaningful advantages over appropriately optimized conventional alternatives?**

### Core Boundaries
- **Exploratory Science vs. Productization:** The researcher's objective is scientific discovery. Do not confuse working software prototypes with validated physical or computational advantages.
- **Track R (Scientific Computing & Alternative Representations):** Focuses on radix theory, balanced ternary arithmetic, 3-state algebra, adaptive tiering, and low-bit weight formats.
- **Track F (Foundation Systems & Governance Engineering):** Studies scoped deterministic execution (DCP), math surfaces covered by accepted [RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md), CanonFS identity and the draft [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) interchange, and Axion pre-side-effect policy paths. Draft [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) is a target-architecture map, not an Axion enforcement contract. Track F supports Track R; it does not prove ternary superiority.
- **Track M (AI-Assisted Research Methodology):** Tracks prompt-to-code provenance, AI error rates, and research iteration trustworthiness.

---

## 3. Document Loading Order

When initializing a new research session, load the six continuity documents in this precise order:

1. [`docs/ai-context/START_HERE.md`](START_HERE.md) — Operational entry point, rules of engagement, and authority rules (this document).
2. [`docs/ai-context/RESEARCH_IDENTITY.md`](RESEARCH_IDENTITY.md) — Fundamental scientific mission, Knuth/CWEB lineage, and canonical H01–H08 hypothesis register.
3. [`docs/ai-context/SYSTEM_ARCHITECTURE.md`](SYSTEM_ARCHITECTURE.md) — Architecture map, component responsibilities, Mermaid diagram, and DAIOS target layers ([RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)).
4. [`docs/ai-context/EVIDENCE_AND_FINDINGS.md`](EVIDENCE_AND_FINDINGS.md) — Scientific evidence index, negative findings, commit pins, and reproduction commands.
5. [`docs/ai-context/DECISIONS_AND_GOVERNANCE.md`](DECISIONS_AND_GOVERNANCE.md) — Accepted decisions, rejected alternatives, PR lineage, RFC status, and safety boundaries.
6. [`docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`](ACTIVE_RESEARCH_HANDOFF.md) — Operational snapshot, active blockers, primary next research action, and session handoff template.

---

## 4. Authority Depends on the Question

1. **Scientific identifiers and historical lineage:** The [H01–H08 register](../research/continuity/HYPOTHESIS_REGISTER_v1.json) and [Master Continuity Charter](../research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md) govern canonical hypothesis identities, source lineage and historical scope.
2. **Normative implementation requirements:** Accepted specifications and explicitly adopted contracts define required behavior for their declared surfaces. Draft RFCs—including [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) and [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md)—remain proposals. Implementation code and passing tests cannot override an accepted contract.
3. **Empirical observations:** Pinned source, independent oracle, raw experimental artifacts, executed assertions, corrective reports and actual CI receipts establish what was observed in the exact tested scope. If observed behavior conflicts with a contract, document the discrepancy and seek correction or authorization; never redefine the contract by inference.
4. **Current operational decisions:** Dated maintainer approvals and freshly checked GitHub branch/PR state determine what is currently authorized or integrated. A PR description, AI report, or green check alone does not grant permission.
5. **This continuity package:** A non-normative index to primary records. Its summaries never supersede canonical sources, recorded negative evidence, or acceptance decisions.

**Conflict rule:** Separate normative requirements, actual implementation behavior and scientific evidence. Cite both conflicting sources and stop short of declaring either an unauthorized amendment or a new scientific conclusion.
---

## 5. Evidence Classification Rules

Every claim or finding in T81 research must be explicitly tagged with its evidence tier:

| Tier | Definition | Standard of Proof Required |
| :--- | :--- | :--- |
| **Tier A** | **Reproduced on Pinned Source:** Executed and verified locally with pinned commit SHA and reproducible artifacts. | Runnable test/experiment script passing cleanly under default toolchain. |
| **Tier B** | **Inspected Source Code:** Verified by direct inspection of implementation code, but without executable integration test. | Code path and line references in active repository files. |
| **Tier C** | **Historical / External Report:** Unverified AI synthesis, user report, or historical document. | Document reference; marked explicitly as UNVERIFIED. |
| **Tier D** | **Mathematical / Model Derivation:** Theoretical derivation or software model under explicit stated assumptions. | Formal proof or Python script model with explicit parameter boundaries. |
| **Tier E** | **Proposal / Speculation:** Unimplemented design proposal, hypothesis, or future hardware aspiration. | Explicitly labeled as PROPOSAL or SPECULATION. |

---

## 6. Current-State Verification Requirements

Before taking any operational or research action:
- **Do NOT assume code works because a README describes it.** Inspect the source files and run the corresponding CTest target or verification script.
- **Do NOT assume a PR is merged or CI is green.** Verify git branch heads using `git log` or `git status`.
- **Do NOT re-explain or re-open resolved decisions.** Check [`docs/ai-context/DECISIONS_AND_GOVERNANCE.md`](DECISIONS_AND_GOVERNANCE.md) before proposing architectural shifts.

---

## 7. Prohibited Assumptions and Overclaims

Research agents are strictly forbidden from repeating or generating the following invalid claims:

1. **PROHIBITED:** Claiming software CPU ternary arithmetic or SWAR tryte packing provides an execution speedup on general-purpose binary CPUs (Falsified in the tested workloads: software tryte addition was $10.6\times$ slower than 64-bit binary ALU addition; historical cache-resident packed 2-bit ternary AVX2 matrix-vector multiplication measured **12.91 GFLOP/s** versus **17.15 GFLOP/s** for native INT8 AVX2, or **75.3%** of the binary baseline throughput, consistent with a SIMD decode tax). These are workload-specific negatives, not universal radix claims.
2. **PROHIBITED:** Claiming T81 is a commercial operating system running on physical ternary microprocessors (Falsified: no physical ternary silicon exists in the repository; all execution uses binary hardware emulation).
3. **PROHIBITED:** Claiming that a custom VM is strictly necessary **or** that Wasmtime + OPA has already demonstrated equivalent safety in the complete Baseline-X protocol. The [bounded precursor report](../../examples/governance/accountable-computation/REPORT.md) did not deploy Wasmtime/OPA; the [claim register](../../experiments/research_001/CLAIM_REGISTER.md) leaves that comparison under active test.
4. **PROHIBITED:** Redefining, renumbering, or silently closing canonical hypotheses H01–H08.
5. **PROHIBITED:** Treating bounded cross-profile execution correctness (CPA-004/CPA-005) as proof of ternary speed, energy, or hardware performance superiority.
6. **PROHIBITED:** Asking incoming AI agents to re-establish whether bounded cross-profile TISC execution works. The cross-profile agreement between hosted T81VM and freestanding Axion OS across 25 observation boundaries is established evidence.

---

## 8. Research Continuation Protocol & Contradiction Resolution

### Continuation Protocol
1. Load all six `docs/ai-context/` documents in sequence.
2. Read [`docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`](ACTIVE_RESEARCH_HANDOFF.md) to identify the primary next research action.
3. Formulate a falsifiable experiment or bounded implementation task.
4. Run only task-authorized, available verification commands; log exact commands and failures, or mark them NOT RUN.
5. Inspect exact source, test output, artifact identities and live PR status with available read-only tools.
6. Propose a dated handoff entry, and append it only within the authorization scope of the current task. Do not rewrite canonical append-only history.

### Resolving Contradictions
If repository files contain conflicting statements:
1. Do NOT silently choose the convenient version.
2. Report the explicit contradiction, citing exact file paths and commit SHAs.
3. Apply Section 4 to distinguish normative contract, empirical observation and current approval; do not promote observed defects into contract changes.
4. Record the contradiction in [`docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`](ACTIVE_RESEARCH_HANDOFF.md) under "Unresolved Contradictions."

---

## 9. Rules for Updating the Continuity Package

- The six files in `docs/ai-context/` are a living, non-normative research index.
- `ACTIVE_RESEARCH_HANDOFF.md` is updated at the conclusion of every research session using the append-only session handoff template.
- `START_HERE.md`, `RESEARCH_IDENTITY.md`, `SYSTEM_ARCHITECTURE.md`, `EVIDENCE_AND_FINDINGS.md`, and `DECISIONS_AND_GOVERNANCE.md` are modified only when a new empirical finding, accepted RFC, or governance decision supersedes previous state.
- Never erase historical evidence or negative experimental results when updating documents.


---

## 10. Latest Operational Addenda (2026-10-29)

For any new ChatGPT, Codex or Jules session, **read the latest dated sections before relying on earlier PR tables or next-action instructions**:

1. [ACTIVE_RESEARCH_HANDOFF.md — Section 8](ACTIVE_RESEARCH_HANDOFF.md#8-operational-update--2026-10-29-pr-reconciliation-and-governance-validation) records the current CMP3 cleanup, #637 file-identity audit, governance validation blockers and next authorized action.
2. [DECISIONS_AND_GOVERNANCE.md — Section 7](DECISIONS_AND_GOVERNANCE.md#7-dated-governance-and-pr-disposition-addendum--2026-10-29) records checked PR dispositions and explicit maintainer decision boundaries.

These are **non-normative operational overlays**, not amendments to the H01–H08 register, RFC acceptance records, or archived experimental results. The snapshot SHA in this document's header is historical; refresh the current `main` SHA, relevant PR heads and hosted check results before acting. Do not use stale PR status statements from earlier sections as current truth.

---

## 11. Operational reading order — 2026-10-31

**Verified repository reference for this documentation update:** `main@f775680aba7db200691c0f4e2a3174b0e7e51581`. This section is a non-normative overlay, not an alteration of accepted RFCs or historical research.

1. Read [BOOTSTRAP.md](BOOTSTRAP.md#2026-10-31-fast-current-state-overlay) for the short current-state overlay after the foundational six-file orientation.
2. Read [ACTIVE_RESEARCH_HANDOFF.md](ACTIVE_RESEARCH_HANDOFF.md#9-operational-reconciliation--2026-10-31) for newer PR and task status; older 2026-10-29 next actions are dated history, not automatic instructions.
3. Source-check every assertion against the pinned experiment or PR head. **Do not treat path-based archaeology Tier A tags, PR body summaries, or an unmerged implementation as independently reproduced evidence.**
4. **H04-R02:** original methodology received Classification D; the corrected conditional abstract-cost observation `boundary_oscillation`, 10-bit Tier-0 control, `AT=1210 < AB=1250` at `r=1.60`, crossover `5/3`, remains distinct from native-capacity parity or physical hardware superiority. Do not assert a universal negative.
5. **Tier A standard for new claims:** exact source revision, executed command/configuration, environment, input identities, output/exit receipt, and precisely bounded scope; independent replay status must be disclosed. A file path or a test's existence is not execution proof. Historical Tier A entries in this package are source-reported until their receipts are independently checked.

The PR head and authorization status can change after this snapshot; refresh before use.

---

## 2026-10-31 continuity reconciliation

The `Source Snapshot` above is a historical observation, not the live repository head. `BOOTSTRAP.md` and `ACTIVE_RESEARCH_HANDOFF.md` have independently dated observations; their differing commit pins must not be silently combined into one state. Refresh `main`, open PR heads, CI receipts, and authorization records before action. The ADM-2 local candidate report described below is not merged evidence.

---

## 2026-10-31 research synthesis and publication boundary

**Current publication posture:** A provisional comprehensive research assessment has been prepared in the research conversation; it is **not** a canonical, independently audited repository publication. Do not cite an unpublished local PDF/Markdown draft as primary evidence. The source hierarchy in §4 still governs every claim.

**New-instance reading rule:** Read the canonical [hypothesis register](../research/continuity/HYPOTHESIS_REGISTER_v1.json), [continuity charter](../research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md), and [evidence index](EVIDENCE_AND_FINDINGS.md) before synthesizing T81's achievements. Read [active handoff](ACTIVE_RESEARCH_HANDOFF.md) for dated PR dispositions, then refresh GitHub. **Scientific acceptance, repository integration, and production readiness are distinct states.**

**Executive synthesis:** T81 is an experimental computing and governed-execution research framework, not proof of generalized ternary superiority. It has working, scoped software and correctness evidence, important negative results, and unresolved physical and comparative hypotheses. Its next publication should explain both demonstrated engineering and what remains unproven. PR #660's benchmark/RFC classifications are not an accepted denominator until corrected and independently checked.

---

## 2026-11-01 active AIOPT M2 research boundary

**Read first for current M2:** [active handoff](ACTIVE_RESEARCH_HANDOFF.md), [decision gate #671](https://github.com/t81dev/t81-foundation/pull/671), [candidate protocol #672](https://github.com/t81dev/t81-foundation/pull/672), [Codex audit #673](https://github.com/t81dev/t81-foundation/pull/673), and [Jules review #674](https://github.com/t81dev/t81-foundation/pull/674). These PRs were observed open and unmerged on 2026-11-01; refresh before relying on status.

**Disposition:** `M2_RESEARCH_QUESTION_REFINEMENT_REQUIRED — EXECUTION_BLOCKED`. Distinguish S (adaptive software representation), A (algorithm routing), and T (ternary-specific attribution). T is the long-horizon scientific objective; positive S/A findings do not establish T. Polynomial evaluation in #672 is a proposed, **unaccepted** S/A workload, not an approved benchmark. The seven P1 blockers in #673 remain open. No experimental execution, CI bypass, governance activation or merge is authorized by these notes.

---

## 12. Cross-Profile Architecture (CPA-001 through CPA-005) Summary — 2026-11-05

**Verified repository state:** Snapshot base `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`; current `main` at `ea6d031a8117341fe03013cc9d5473de1b07727d`; PR #709 is an **open draft PR (unmerged)** at head `b803bf916a1b1e44129f3a07abf96fa6103c51fc` on branch `origin/experiment/cpa004-freestanding`.

The Cross-Profile Architecture (CPA) program achieved a major milestone:
1. **Implemented Bounded Cross-Profile Architecture:** T81 now has proven cross-profile execution of TISC bytecode between user-space (hosted T81VM) and bare-metal (freestanding Axion OS microkernel on AArch64 QEMU).
2. **Deterministic Oracle Alignment:** A 351-byte canonical program (SHA-256 `b0649777...`, 17 opcodes) executes across 24 instructions and reaches identical state at all 25 observation boundaries (R0–R80, logical memory, flags, system registers R75–R80).
3. **Execution Correctness $\neq$ Ternary Superiority:** Proving the engine works across profiles resolves the question of whether T81 is a functioning architecture. It does not establish ternary performance, density, or energy superiority over binary baselines.
4. **Preserved Discrepancies:** Four normative discrepancies remain documented without silent resolution:
   - Memory addressing: Direct logical addressing (runtime/semantics) vs register-indirect (`spec/tisc-spec.md`).
   - Branch target & condition: Direct immediate target + R[B] zero-test (runtime/semantics) vs register-indirect + FLAGS-based conditional branches (`spec/tisc-spec.md`).
   - TXor truth table: Balanced wrapped difference (`(lhs - rhs + 1) % 3 - 1`) vs ambiguous "Sum" label.
   - Opcode inventory: 174 capability-frozen vs 209 registry entries vs 244 named C++ header opcodes vs 247 numerical interval with 221–223 undefined.

---

## 13. Current research gate — 2026-11-05, after CPA-006 and ISA-001

**Current main:** `7afa4f1a424be6c65fab28f8e5bcf4fddc3ef762` (observed 2026-11-05). This addendum supersedes earlier *live status and next-action* text, not historical source identities or approvals. PR #711 merged the preceding CPA-001–005 continuity refresh to main; its seven changed files were README.md and the six canonical AI-context files.

**Established, bounded:** CPA-006 reports 19/19 frozen-oracle cases, three hosted and three fresh AArch64 QEMU executions per case (57+57), and 1,190 validated state records. Outcomes include normal Halt, runtime traps, admission rejection, and instruction-budget exhaustion. The experiment does not establish full ISA conformance, physical ternary performance, or independent-person replication. See [CPA-006 acceptance report on its merged integration branch](https://github.com/t81dev/t81-foundation/blob/514fa17cf5629a7aea9ebb1b035f01f3ef7687f3/experiments/cpa006/README.md).

**Actual integration:** PR #710 is merged into `experiment/cpa005-integration-base` (merge result `514fa17cf5629a7aea9ebb1b035f01f3ef7687f3`), **not main**. PR #709 (CPA-004 evaluator) remains open draft and PR #707 (CPA-002 matrix correction) remains open. Main describes the CPA findings but does not carry these experimental evaluator paths. Do not equate identical file content, a merged PR, and main-branch ancestry.

**ISA-001 outcome:** Read-only assessment, **READY FOR MAINTAINER DECISION**; no normative decision, RFC acceptance, code change, or new execution test occurred. Proposed direction: version an authoritative TISC contract retaining normative register-indirect memory and flag-based/register-target branches; explicitly define scalar TXor as wrapped balanced difference; preserve frozen 0–173 and reconcile accepted extension assignments; pin the current implemented CPA semantics as a *historical experimental profile*. These are recommendations, **not accepted requirements**. Raw 13-byte instructions have no profile identifier; any semantic transition needs explicit artifact-bound profile selection and the formal breaking-change process.

**Next authorized action:** Draft/review the precise versioned TISC transition-contract RFC and maintainer decision matrix **without changing implementation or historical CPA artifacts**. After approval, separately authorize discriminating tests and one cumulative integration candidate; then preregister an H02 signed-rounding comparison against optimized binary and encoding-cost controls. Do not restart CPA execution-feasibility research.

**Evidence hierarchy:** Normative specification and governance controls decide required semantics; pinned executed artifacts decide observed historical behavior; this non-normative continuity index cannot confer acceptance, merge, or performance authority.
