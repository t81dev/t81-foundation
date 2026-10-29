# T81 Research Continuity — START_HERE

**Document ID:** `docs/ai-context/START_HERE.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
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
