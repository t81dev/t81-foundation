# T81 Research Continuity — ACTIVE_RESEARCH_HANDOFF

**Document ID:** `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`
**Version:** 1.0.1 · **Original Snapshot:** 2026-10-29T07:15:00Z · **Correction reviewed:** 2026-10-29
**Repository:** `t81dev/t81-foundation`
**Verified `main` Reference:** `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (2026-10-29; PR heads may change)
**Status:** Non-normative, dated research handoff; not an experiment authorization or live monitor

---

## 1. Operational Snapshot

### Current Research Boundary
T81 is studying representation advantage (Track R), bounded deterministic/policy execution (Track F), and auditable AI research method (Track M). The components and reported tests have different maturity levels. Software ternary emulation results do not establish universal hardware conclusions.

### Investigations and PR Status (checked 2026-10-29)

| Investigation / PR | Evidence status | Next gate |
| :--- | :--- | :--- |
| **H04-R02 adaptive representation** | **Completed packet reported** in [HANDOFF](../../experiments/h04_r02_adaptive_representation/HANDOFF.md), [RESULTS](../../experiments/h04_r02_adaptive_representation/RESULTS.md), and code/oracle under `experiments/h04_r02_adaptive_representation/`. The packet gives a conditional negative result when binary-encoded trit costs and adaptive binary controls are considered; **not independently rerun in this update**. | Verify the existing packet and exact assumptions; do **not** rebuild the completed four-arm experiment. |
| **CMP3 #632–#637** | **Open/unmerged** overlapping protocol and harness PRs. #634 head `21095d65cc0c849f5a6f2599a962cbce032b5cea`; #635 `b37005ceda6a3f4349ec9027b2a9be84ea176aa9`; #637 `1a71733cdd266a34a174d13185c78e5b6a07c68e`. Divergent protocol blobs and incomplete preflight remain. | Reconcile protocol ancestry, code parity, statistics and withdrawn 10A claims before an acceptance decision or confirmatory execution. |
| **Accountable computation / Baseline-X** | A [local precursor](../../examples/governance/accountable-computation/REPORT.md) was tested, but **no complete Wasmtime/OPA comparison**. Draft RFC-00D4, protocol/CI assumptions and [CLM-03/04](../../experiments/research_001/CLAIM_REGISTER.md) remain unresolved. | Separate actual local results from planned conventional baseline and authorization claims. |
| **Track F determinism tax** | A useful possible cost study, **not confirmed active/approved** by this handoff. | Check owner priority, exact comparators and accepted scope before scheduling. |
| **PRs #565/#566** | Historical governance records require fresh head/CI reconciliation. | No activation or merge inference from old status badges. |

---
## 2. Current Blockers

### Scientific Blockers
1. **Missing Matched Semiconductor Characterization Data (H01 / Inv 10A):** Physical evaluation remains blocked due to unavailable cell area, switching energy, and noise margin data for 3-state silicon devices.
2. **H04 Verification & Interpretation:** The existing H04-R02 packet reports four-arm metadata, transition and conversion accounting. Check its independent oracle and cost-unit choices before treating its reported conditional result as a general H04 closure or initiating new traces.

### Engineering Blockers
1. **Windows PATH Quote Parsing:** CLI tools executing subprocesses on Windows must trim surrounding quotes from environment PATH entries before passing strings to `std::filesystem::path`.
2. **Compiler Reference Warnings:** Dangling reference warnings in `examples/research/validated-descriptors/runner.cpp` line 144 under Apple Clang require cleanup.

---

## 3. Unresolved Architectural Contradictions

1. **Software Compression vs CPU Execution Speed:** On-disk 2-bit tryte packing saves $75\%$ memory footprint, but SIMD vector decode instruction tax limits CPU MatVec execution speed to $75.3\%$ of INT8 AVX2 in cache.
   *Resolution:* Retain 2-bit packing strictly for storage/interchange while treating CPU software arithmetic speedup as falsified.
2. **Custom VM vs Runtime-Agnostic Governance:** [Research Protocol 001](../../experiments/research_001/RESEARCH_PROPOSAL.md) specifies a Wasmtime/OPA comparator, but the [precursor report](../../examples/governance/accountable-computation/REPORT.md) tested different local A/B/C arms. The observed B-arm authorization failures (90/150 negative requests) must remain visible.
   *Unresolved:* Equivalence or necessity cannot be concluded until the actual conventional comparator, identical threat model and [draft RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) conformance obligations are tested and accepted.

---

## 4. Primary Next Research Action — Read-Only Reconciliation

### Action
**Reconcile the completed H04-R02 evidence and the competing CMP3 protocol/harness revisions before selecting another experiment.**

### Scope and Inputs
1. Pin the current `main` SHA and exact PR heads; check acceptance records, not PR-body words such as FROZEN or READY.
2. Inspect H04-R02 [HANDOFF](../../experiments/h04_r02_adaptive_representation/HANDOFF.md), [RESULTS](../../experiments/h04_r02_adaptive_representation/RESULTS.md), oracle and raw outputs. Report tested conditions, modeling assumptions, independent-run status and any contradictions; do not reconstruct H04-R02.
3. For CMP3, compare the protocol blobs in #634/#635/#636/#637 with the actual harness code, independent oracle, equivalent binary controls, PMU counters and decision thresholds. Audit the candidate design for reintroduced 10A numerical claims withdrawn by [corrected 10A](../research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md).
4. Return one evidence-pinned recommendation to the maintainer: accept a precise bounded review task, leave the experiment blocked, or select a distinct new falsifier.

### Acceptance and Stopping Criteria
- **Deliverable:** One source/commit-pinned discrepancy table separating reported, independently inspected and independently executed evidence, plus a single human decision gate.
- **Stop:** Protocol ancestry or source identity unresolved; missing equality of binary controls; no trustworthy raw artifacts; invalid threshold semantics; unavailable required PMU; or missing maintainer authorization.
- **Not authorized by this index:** New benchmark execution, branch protection changes, RFC acceptance, protocol edits after seeing measurements, production activation, or merge.

---
## 5. Explicitly Deferred Work

- **Bare-Metal Kernel / TernaryOS:** General-purpose bare-metal OS development is deferred until Layers 1–4 of DAIOS ([RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)) are fully stabilized.
- **Base-81 TCP/IP Stack ([RFC-00D0](../../spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)):** Deferred to design-led status; restricted to narrow resolver prototypes.
- **Fourth Bounded AI OS Composition:** Expanding the protected bounded family beyond `assess-fixed`, `route-fixed`, and `classify-fixed` is deferred.

---

## 6. Template for Next Session Update (Append-Only Log)

When completing a research session, append a new entry using this standardized template:

```text
================================================================================
DATE / SESSION: YYYY-MM-DD / Session Name
REPOSITORIES AND PINNED COMMITS: t81-foundation@<commit-sha>
QUESTION OR HYPOTHESIS TESTED: [H01-H08 identifier and formal question]
EVIDENCE EXAMINED: [file paths, CTest IDs, log outputs, datasets]
WHAT WAS REPRODUCED: [Tier A reproduced benchmark or test outputs]
WHAT WAS ONLY REPORTED OR MODELED: [Tier B/C/D claims inspected or modeled]
RESULT / NEGATIVE RESULT: [Explicit scientific outcome and verdict]
KNOWN DEFECTS / CONFOUNDERS: [Identified confounders or test failures]
OPEN PRS AND CI STATUS: [Branch heads and PR review status verified via git]
NEXT SINGLE ACTION: [Concrete next falsifiable task]
STOPPING CRITERION: [Explicit halt condition]
================================================================================
```

---

## 7. Canonical Session Log (Append-Only)

```text
================================================================================
DATE / SESSION: 2026-10-29 / AI Research Continuity System Construction
REPOSITORIES AND PINNED COMMITS: t81-foundation@f6d4d680709c5c6c98b305628e04346f5ef61620
QUESTION OR HYPOTHESIS TESTED: Construction of durable 6-document research continuity package in docs/ai-context/ covering H01-H08, TISC ISA, Axion, CanonFS, and DAIOS (RFC-00D2).
EVIDENCE EXAMINED: docs/HANDOFF.md, HYPOTHESIS_REGISTER_v1.json, LINEAGE.md, MASTER_CONTINUITY_CHARTER_v1.0.md, RFC-00D2, T81_FULL_REASSESSMENT_2026-10-25.md.
WHAT WAS REPRODUCED: Created docs/ai-context/ (START_HERE.md, RESEARCH_IDENTITY.md, SYSTEM_ARCHITECTURE.md, EVIDENCE_AND_FINDINGS.md, DECISIONS_AND_GOVERNANCE.md, ACTIVE_RESEARCH_HANDOFF.md). Verified relative links, H01-H08 exact statements, and bash governance checks.
WHAT WAS ONLY REPORTED OR MODELED: Historical Hanoi CWEB probes (422/547 add, 140/201 mult failures) and Inv 10A storage characterization block inspected.
RESULT / NEGATIVE RESULT: Continuity package established. Software CPU ternary arithmetic re-confirmed as FALSIFIED and FROZEN for execution speed.
KNOWN DEFECTS / CONFOUNDERS: MatVec 2-bit packing saves 75% memory footprint but incur SIMD VPSHUFB decode instruction tax in L1/L2 cache.
OPEN PRS AND CI STATUS: Main branch clean. PR #565/#566 under review (CodeQL pass, NOT READY). PR #634 merged.
NEXT SINGLE ACTION: Execute Experiment H04-R02 (Four-Arm Exact Trace Ledger & Crossover Analysis).
STOPPING CRITERION: Halt if adaptive ternary yields higher total cost than adaptive binary across parameter sweep.
================================================================================
```

### Correction appended 2026-10-29 (documentation review; no experiment rerun)

The preceding session entry is preserved verbatim as historical author-reported text, **not independently verified**. Its assertions that PR #634 was merged and that H04-R02 still needed construction are contradicted by the checked GitHub head and the existing H04-R02 packet. Likewise, the Baseline-X claim of completed Wasmtime/OPA equivalence is unsupported by the cited precursor.

```text
DATE / SESSION: 2026-10-29 / PR #638 continuity correction
REPOSITORIES AND PINS: t81-foundation main f8ac77159ded2c4b93f4cc83abd48f6f35655468; PR #638 initial head 020af6c2c808e0280b238719abbfc6dd8be1ab61
QUESTION: Correct continuity authority, draft-RFC status, Baseline-X evidence, H04-R02 handoff and reproduction paths.
EVIDENCE INSPECTED: RFC-00D2; RFC-00D4; experiments/research_001/CLAIM_REGISTER.md; examples/governance/accountable-computation/REPORT.md; experiments/h04_r02_adaptive_representation/{HANDOFF,RESULTS}.md; exact PR metadata and experiment CMakeLists.
INDEPENDENTLY RAN: No code build, benchmark, or experimental verifier in this documentation correction.
RESULT: H04-R02 packet already exists (conditional source-reported outcome); CMP3 #634-#637 remain open/unmerged; full Wasmtime/OPA deployment comparator has not been established by the precursor.
CORRECTIONS: RFC-00D2 and RFC-00D4 draft; source-observed behavior does not override accepted normative contracts; experiment executable paths corrected.
NEXT SINGLE ACTION: Read-only H04/CMP3 evidence and protocol reconciliation, then obtain maintainer decision.
NO-AUTHORITY ACTIONS: No experimental measurement, security/policy activation, RFC acceptance or merge.
```
