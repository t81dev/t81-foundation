# T81 Research Continuity — START_HERE

**Latest operational view (2026-11-04):** Read the November 4 entries in this document and [`ACTIVE_RESEARCH_HANDOFF.md`](ACTIVE_RESEARCH_HANDOFF.md).
Current repository head is `d836f7edb9b70608ef7476e82e98a0a89084d74b` on `main`.
Key operational and research milestones reconciled at this head:
1. **Core Runtime & Hosted Platform:** Fully functional across Linux, macOS, and Windows. T81Lang compiles to frozen TISC v1.9.0 bytecode, executes deterministically in T81VM, enforces fail-closed Axion safety policies, and persists immutable content-addressed objects in CanonFS.
2. **CanonFS Portability Integration:** PR #700 (`63d87633`, BASE-01 verified correctness & Windows test portability) and PR #701 (`9bbc4453`, portable `h1-<hex>` storage keys & Windows Debug VM denial) are merged. Active follow-ups PR #702 (`origin/fix/canonfs-cli-list-portable-key` at `41f95d4c`, canonical reference restoration & CTest suite) and PR #703 (`origin/fix/canonfs-portable-snapshot-paths` at `991f3169`, portable snapshot directories `h1-<hex>` & CLI lifecycle) are in progress.
3. **Freestanding Axion OS Distinction:** The bare-metal UEFI kernel under `ternaryos/` boots on bare metal / hypervisors (QEMU AArch64 and x86-64) to an interactive prompt with hardware timers, PL011 UART, and virtio-blk drivers, but does **not** embed T81VM or execute TISC bytecode; it currently executes only raw native assembly stubs (`T81X`).
4. **BASE-01 & H04-R03 Protection Boundaries:** BASE-01 integration (#694 merged at `52921bd0`, #695 review merged at `4f9edb9c`) and EXP-H04-R03 Candidate A (#679 merged at `d8e1376a`, #680 review merged at `927972f8`) are integrated in **correctness-only mode**. All performance measurements and benchmarking campaigns remain strictly frozen and unauthorized without dedicated hardware qualification and human maintainer sign-off.
5. **New System Integration Audit (T81-SIA-001):** Published under `docs/reports/audit/` (reports A through H), establishing the exact boundary between demonstrated hosted execution and experimental freestanding microkernel capabilities.

**Latest BASE-01 integration and readiness view (2026-11-03):** Read [BASE01_2026-11-03.md](BASE01_2026-11-03.md). PRs #694 and #695 are merged; integration acceptance does not authorize B1 measurement. Older dated operational views below remain historical.

**Document ID:** `docs/ai-context/START_HERE.md`
**Version:** 1.1.0 · **Last Reviewed:** 2026-11-04
**Source Snapshot:** `main` at `d836f7edb9b70608ef7476e82e98a0a89084d74b` (checked 2026-11-04; refresh before use)
**Status:** Non-normative navigation and research handoff index; not an accepted contract or independent verification

---

## 1. T81 Identity in One Paragraph

T81 Foundation is an experimental computing architecture and scientific research platform investigating alternative digital representations (balanced ternary, Base-81 tryte grouping, adaptive radix tiering) and policy-governed deterministic execution. The program combines scientific research into mathematical, algorithmic, and physical substrate advantages (Track R) with foundation systems engineering for bit-exact cross-platform determinism, content-addressed artifact identity via CanonFS, pre-side-effect policy authorization via Axion, and detached accountable evidence verification (Track F). T81 is not a commercial desktop operating system or a production CPU product; it is a hypothesis-driven empirical research instrument designed to determine under what exact assumptions, workloads, cost models, and physical substrates non-binary computational representations yield defensible advantages over optimized conventional baselines.

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

1. **PROHIBITED:** Claiming software CPU ternary arithmetic or SWAR tryte packing provides an execution speedup on general-purpose binary CPUs (Falsified: software tryte addition is $10.6\times$ slower than 64-bit binary ALU addition; 2-bit packed MatVec is limited to $75.3\%$ of INT8 AVX2 speed in cache due to SIMD decode tax).
2. **PROHIBITED:** Claiming T81 is a commercial operating system running on physical ternary microprocessors (Falsified: no physical ternary silicon exists in the repository; all execution uses binary hardware emulation).
3. **PROHIBITED:** Claiming that a custom VM is strictly necessary **or** that Wasmtime + OPA has already demonstrated equivalent safety in the complete Baseline-X protocol. The [bounded precursor report](../../examples/governance/accountable-computation/REPORT.md) did not deploy Wasmtime/OPA; the [claim register](../../experiments/research_001/CLAIM_REGISTER.md) leaves that comparison under active test.
4. **PROHIBITED:** Redefining, renumbering, or silently closing canonical hypotheses H01–H08.

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

## 10. Operational Status Addendum — 2026-11-04 (T81-SIA-001 Integration Audit)

**Authoritative Repository Pin:** `main` at commit `d836f7edb9b70608ef7476e82e98a0a89084d74b`.

The T81 program has reached an inflection point where isolated subsystem achievements must be rigorously evaluated as a unified system. Under direction of the Maintainer, an exhaustive, read-only system integration audit (**T81-SIA-001**) was conducted and published under `docs/reports/audit/` across eight companion reports:
- `T81_SYSTEM_INTEGRATION_AUDIT.md`: Executive summary and subsystem maturity register.
- `T81_BOOT_EXECUTION_TRACE.md`: Firmware-to-kernel boot trace on x86-64 and AArch64.
- `T81_APPLICATION_LIFECYCLE_MATRIX.md`: Hosted vs freestanding application lifecycles and recovery.
- `T81_SUBSYSTEM_DEPENDENCY_MAP.md`: Call graph and inter-system coupling analysis across 13 subsystems.
- `T81_OS_CAPABILITY_MATRIX.md`: 18-point capability evaluation against conventional operating systems.
- `T81_INTEGRATION_GAP_REGISTER.md`: Prioritized register of engineering gaps and minimum viable repairs.
- `T81_VALIDATION_EVIDENCE.md`: Reproducible build commands, test results, logs, and empirical evidence.
- `T81_SYSTEM_READINESS_VERDICT.md`: Formal subsystem readiness dispositions, architectural identity, and milestone roadmap.

### Strongest Demonstrated Capabilities
1. **Hosted Deterministic Toolchain:** T81Lang compiles to frozen TISC v1.9.0 bytecode, executes in T81VM with 100% deterministic repeatability, and integrates register-level policy event hooks.
2. **Fail-Closed Policy Governance:** Axion policy engine intercepts storage mutations, model intakes, and runtime events before side effects take place.
3. **Immutable Content-Addressed Storage:** CanonFS stores raw blocks, capability grants, and snapshot manifests using cryptographic hashes (`sha3-256`, `sha3-512`), with active cross-platform portability integration ensuring portable `h1-<hex>` storage keys on disk.

### Critical Execution Boundary (Freestanding Disconnect)
Freestanding Axion OS (`ternaryos/`) boots independently on bare metal and hypervisors (QEMU/VirtualBox) via UEFI, exits boot services, establishes MMU paging (TTBR0), PL011 UART, and virtio-blk modern drivers. However, **Axion OS does not host T81VM or execute TISC bytecode**. It currently executes only raw position-independent native AArch64 machine-code stubs (`T81X`). Bridging this gap by embedding a zero-allocation TISC runtime into Axion OS represents Milestone 1 (P0) of the system roadmap.

---

## 11. Latest Operational Addenda (2026-10-29)

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
