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

1. **Software Compression vs CPU Execution Speed:** In the historical cache-resident packed 2-bit ternary AVX2 matrix-vector benchmark, **12.91 GFLOP/s** versus **17.15 GFLOP/s** for native INT8 AVX2 means ternary reached **75.3%** of binary throughput (binary approximately **32.8%** faster); 2-bit packing reported a $75\%$ footprint reduction. These are bounded workload results, not general radix claims.
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


---

## 8. Operational update — 2026-10-29, PR reconciliation and governance validation

**Authority:** Non-normative, append-only operational correction. This section supersedes *current-status descriptions* in Sections 1 and 4 where they conflict, but does not erase historical entries, change canonical H01–H08, accept RFCs, or authorize experiments. GitHub PR states were rechecked on 2026-10-29; recheck before acting.

### CMP3 reconciliation

- **Closed without merge:** [#632](https://github.com/t81dev/t81-foundation/pull/632) (same single-file Git blob as #633), [#635](https://github.com/t81dev/t81-foundation/pull/635) (sole document preserved byte-identically at `experiments/cmp3_decision_advantage/protocol/ORIGINAL_PR635_PROTOCOL.md` on `main`), [#636](https://github.com/t81dev/t81-foundation/pull/636) (obsolete, unexecuted prototype), and [#637](https://github.com/t81dev/t81-foundation/pull/637) (integrated/superseded implementation). Closure comments retain rationale and source identities.
- **#637 full changed-path identity comparison:** 83 paths inspected against `main`: 80 identical Git blobs; three differing paths: `experiments/cmp3_decision_advantage/benchmark.cpp`, `experiments/cmp3_decision_advantage/inspect_assembly.py`, and `experiments/cmp3_decision_advantage/evidence-r1-publication/provisional/PROVISIONAL.patch`. All 83 paths exist on `main`. The three differences require preserving newer integrated corrections rather than merging the old PR head. Git blob identity is not scientific validation.
- **Still open:** #633 and #634 retain distinct historical protocol revisions; do not close as duplicates without content-level preservation evidence.
- **Integrated corrective lineage:** #642, #643 and #644 are merged. CMP3/r1 **measurement remains unauthorized**; the historical trusted-runner and PMU restrictions must be checked in the authoritative protocol and evidence before any execution. Do not claim a ternary advantage.

### Governance stack and acceptance

- **Open dependency chain:** #574 → #575 → #576 → #577; #578 → #579 → #580 is a separate investigation/preparation chain stacked above #577. #617 is a read-only acceptance report based on #577; #622 is a separate V01 regression PR, currently nonmergeable at review. None is authorized for merge or policy activation by this handoff.
- **#574:** retained reproduction reports 24/24 agreement for Bash `pipefail` propagation, exposing 13 masked validator failures; its hosted T81 Foundation CI failed even though CodeQL and several other workflows passed.
- **#577 / #617:** author-generated receipts report **18/27 pass, 9/27 fail**; these are *not* independent re-execution. Seven primary unresolved areas are root-layout authority, translations, docs layout, public API lock, spec/code mappings, README component-status rows and RFC lifecycle metadata. Aggregate docs-governance and a second translation check account for the other failures. The only head-associated hosted workflow found for #577 was QEMU x86 success; this is not a complete quality-gate or CodeQL acceptance.
- **#580:** typed root contract, draft RFC-0057 and validator prototype remain inactive. Do not auto-approve root entries, update API locks, change RFC statuses, weaken validators, or modify branch protection to force green CI.
- **#614:** dedicated skill-policy boundary workflow reported failure; Ubuntu fake-backend job passed while macOS job was cancelled. Neither a cross-platform pass nor a Linux assertion failure may be inferred. Review comment was posted.

### Next single authorized action and stop condition

**Next action:** Independently validate the exact #577 cumulative head `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` against a freshly pinned `main`, using the *original 27-command inventory* and complete regression suites. Capture each command, exit code, stdout/stderr, environment, Git SHA and auditor attribution; obtain complete trusted hosted Linux CI and CodeQL results. Report deviations from the 18/9 historical receipt. Keep failures and skipped checks visible.

**STOP / HUMAN DECISION:** Any proposed fix that redefines root authority, changes public API lock, edits accepted RFC or RFC status, relaxes required checks, rewrites historical evidence, or grants performance/measurement authority requires separate explicit maintainer approval. No new PR, CI automation, merge, measurement, or branch-protection change is authorized by this documentation update.

**Evidence tier:** PR status, changed-file lists and Git blob comparisons were inspected through the GitHub connector (Tier B for source identity). Reported validation outcomes and historical tests are Tier C until independently executed on pinned source. This update performed no build, benchmark, PMU run or 27-command validation.

---

## 9. Operational reconciliation — 2026-10-31

**Authority:** Newer non-normative snapshot appended without removing the historical Section 7 log or October 29 instructions. Checked GitHub `main@f775680aba7db200691c0f4e2a3174b0e7e51581`; future readers **must refresh** the PR heads, checks and maintainer authorization before acting. No build, code benchmark, scientific oracle or sanitizer was executed in this documentation update.

### Current parallel workstreams

**A. Research archaeology / evidence classification.** [PR #651](https://github.com/t81dev/t81-foundation/pull/651) is open/unmerged at `caf9b3fcf1d39d65d51af5311c729f705581f56f`. [PR #653](https://github.com/t81dev/t81-foundation/pull/653) is open/unmerged at `2229abf23b5f8618eca7a532e71966b32e79eb98`. #653 aims to correct #651; do not merge both overlapping inventories independently. This session directly inspected #653 `CLAIMS.jsonl`: `CLM-003` still says there is no AT advantage under symmetric metadata scaling despite the corrected H04-R02 conditional `boundary_oscillation` model result `AT=1210 < AB=1250`, `r=1.60`, reported crossover `5/3`, and non-equivalent native capacities. Classification D methodological limits remain; hardware/general advantage is not proven. Independently inspect `HISTORY.jsonl`, `tools/archaeology_classifier.py`, `FILES.jsonl`, generated summaries and regression tests before accepting any Tier A claims. **Status: SEMANTIC ACCEPTANCE BLOCKED** until provenance and conditional-result tests pass.

**B. ADM-2 engineering recovery.** [PR #652](https://github.com/t81dev/t81-foundation/pull/652) is open/unmerged at `e2e5a25b17ab37198e5fe2d53b68792963fd088f`. Codex reports unpublished local repair branches: A `repair/adm2-baseline@0c22ac323484e37cc85f95201bd4dc01729dedb4`, direct child of main; B `repair/adm2-m1@826887b40c62de9c6c903f319bbfbe905baa9911`, stacked after A. Codex-authored report `ADM2_BASELINE_RECOVERY.md` records focused host macOS ARM64 CTests, full C++20/C++23 engine TU compilation, queue TSan/ASan/UBSan and `SnapshotRef` source-compatibility checks; not independently replayed here. Local baseline/A/B API-lock and architecture-discovery failures were reported as inherited; platform checks Linux/Windows remain unexecuted on the candidates. The inherited worker remains functionally blocked (`start()` / `stop()` lifecycle state; double dequeue; single-request fallback re-enqueue), so do not claim inference runtime execution or authorize activation. **Status: LOCAL ENGINEERING ACCEPTANCE REPORTED; PUBLICATION/INTEGRATION HOLD.**

**C. Governance / SDK status.** [#649](https://github.com/t81dev/t81-foundation/pull/649) draft/unmerged at `3a28eae9c11bbd18b09b71509fa7f74e0584bc24`; [#650](https://github.com/t81dev/t81-foundation/pull/650) draft/unmerged at `137faac2830f9f21b66667a3cb103df79b550e0d`. No RFC-00E9 acceptance, public API-lock revision, required-check relaxation, or merge is granted.

**D. Scientific boundaries.** H01 Investigation 10A: **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**. H04-R02: conditional abstract-cost AT result survives corrected accounting at a specific parameter; original methodological verdict Classification D. CMP3/r1: no new trustworthy PMU/trusted-runner authorization or measurement is established by this update. Preserve all historical negative results and scope labels.

### Bounded next actions and stopping conditions

1. **Jules / research archaeology:** Fix #653's CLM-003/HISTORY inconsistency; eliminate any path-based Tier A promotion, require pinned execution receipts, regenerate records deterministically, add numerical and provenance regression tests. Stop before merge; obtain independent review of the actual final head.
2. **Codex / ADM-2:** Audit exact A/B parent-to-child diffs, test registration and evidence receipts; run approved Linux and Windows checks and attribute failures against pinned baseline. Prepare stacked draft publication only after maintainer authorization; preserve #652 history. Worker lifecycle repair must be a separate bounded task with a failing test first.
3. **Maintainer:** Decide independent PR disposition and whether to approve separate publication. No automatic RFC acceptance, governance activation, API-lock regeneration, performance measurement, or protected branch operation.

### Session receipt (non-experimental)

```text
DATE / SESSION: 2026-10-31 / AI context operational reconciliation
PINNED MAIN: f775680aba7db200691c0f4e2a3174b0e7e51581
PR HEADS CHECKED: #651 caf9b3fc; #652 e2e5a25b; #653 2229abf2; #649 3a28eae9; #650 137faac2
SOURCE INSPECTED: docs/ai-context/*; #653 CLAIMS.jsonl; Codex-authored ADM2_BASELINE_RECOVERY.md supplied in chat
EXECUTED BUILD / ORACLE / BENCHMARK: none
RESULT: Updated non-normative status and evidence boundary; no change to scientific or governance authority
NEXT ACTION: independent source-level #653 acceptance review, separate A/B portability and publication gates
STOP: no merge, no RFC acceptance, no protected check changes, no measurement authorization
```

---

## 2026-10-31 handoff delta — ADM-2 candidate report

**Source:** subsequent operator-provided local development report; not independently fetched from GitHub. Two candidates were prepared but neither pushed nor merged:

| Candidate | Local branch | Reported final commit | Reported validation |
| --- | --- | --- | --- |
| A: baseline maintenance | `repair/adm2-baseline` | `0c22ac323484e37cc85f95201bd4dc01729dedb4` | 2 selected macOS ARM64 CTests |
| B: ADM-2 M1 | `repair/adm2-m1` | `826887b40c62de9c6c903f319bbfbe905baa9911` | 5 selected macOS ARM64 CTests |

A follows pinned baseline `f775680aba7db200691c0f4e2a3174b0e7e51581`; B reportedly has two commits after A. The engine translation unit reportedly compiles. **No independent inspection, cross-platform validation, push, merge, activation, or performance measurement is established.**

**Next action:** obtain the local branch diffs and test receipts; compare against fresh `main` and accepted contracts. Keep CMP3/H04 research reconciliation distinct from ADM-2 engineering, and do not relaunch completed experiments. Earlier sections retain their explicitly dated October 29 status, not a claim about current live PR state.

---

## 2026-10-31 research assessment and integration handoff (dated; refresh before action)

**Primary research deliverable:** Complete the comprehensive T81 scientific/architectural report with commit-pinned evidence, historical lineage, hypothesis-by-hypothesis results, negative findings, comparative limitations, AI-assisted research methodology, and a prioritized decisive-experiment agenda. A provisional assessment exists outside the repository, but is **not** an independently audited publication. Do not delay drafting for unrelated CI maintenance; do not publish unverified counts as final.

**PR #658 — H04-R03 Phase B:** Deterministic correctness accepted at `983e258ed4b57b4cd6e010f036c1eb8656856e88`; current head observed `0a48054510e79e753d5152ccb3f9f130c17bbafd`. The reported integration gate is **BLOCKED** by broader required CI failures, attributed in an operator report to baseline fixture, Windows CMake, GCC GoogleBenchmark, macOS toolchain, and sanitizer issues. See [integration-only directive](https://github.com/t81dev/t81-foundation/pull/658#issuecomment-6552858759) and [separate CI maintenance directive](https://github.com/t81dev/t81-foundation/pull/658#issuecomment-6552913472). Those comments authorize neither a merge nor Phase C. Do not reopen accepted scientific correctness to resolve unrelated CI.

**PR #660 — baseline research inventory:** Head observed `114a5ef82960366d2326dc8d404ef4fcbe3da4ff`, open/unmerged. Independent Codex review of the earlier `aa8fe207ceacfc08dd0364cd48c6bd8a612d44a7` returned substantive findings; [six-item correction directive](https://github.com/t81dev/t81-foundation/pull/660#issuecomment-6552761518) is outstanding as of this snapshot. Treat RFC/capability/benchmark counts and evidence tiers as provisional pending final-SHA review.

**H04-R03 historical status (as of 2026-11-02):** Candidate A implementation and correctness validation were reported; PRs #679/#680 were open and Phase C was not authorized. A 2026-11-06 repository check found both PRs merged, which does not by itself authorize performance measurement. Refresh live authorization before relying on this dated snapshot.

**Workstream separation:** (1) scientific report and primary-source citation audit; (2) PR #660 evidence-inventory corrections; (3) separate baseline CI maintenance to unblock #658. None automatically authorizes the others. Preserve negative results and existing research scope. **No merge, benchmark, Phase C, protection bypass, or production claim is implied.**

**Next AI-instance action:** Refresh main/PR heads and CI, read the canonical register and primary experiment packets, then advance the comprehensive report without inventing independent reproduction or re-running completed investigations by default.

---

## 2026-11-01 AIOPT-001 M2 handoff — latest inspected GitHub snapshot

| PR | Role | Inspected head | Disposition |
| --- | --- | --- | --- |
| [#665](https://github.com/t81dev/t81-foundation/pull/665) | M1 correctness candidate | `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db` (earlier observation) | Local correctness evidence; independent acceptance not established |
| [#669](https://github.com/t81dev/t81-foundation/pull/669) | M2 adversarial design assessment | `30bef63a433f95c8be7356a75d8f6971255b569a` (earlier observation) | Merged assessment, **not** experiment authorization |
| [#671](https://github.com/t81dev/t81-foundation/pull/671) | B/C decision and S/A/T separation | `8892d094b6ec62728eb783cd7fb735cf3d57de5f` | Open/draft; recommendation only |
| [#672](https://github.com/t81dev/t81-foundation/pull/672) | Revised polynomial protocol | `13e1828cc4f6cbeba1b37c18345eb68a5da98147` | Open/draft; workload **unaccepted** |
| [#673](https://github.com/t81dev/t81-foundation/pull/673) | Codex independent protocol audit | `b8ab59ccb8183db9476d47259bd43de1ea34522a` | Open/draft; `PROTOCOL_REVISION_REQUIRED`, seven P1 blockers |
| [#674](https://github.com/t81dev/t81-foundation/pull/674) | Jules adversarial workload critique, qualified | `4ab1fca95a8747d33d414d3e59fe8ed014c105f7` | Open/non-draft; polynomial redesign proposed, not validated |

**What changed:** Original #672 encode/store/decode/reduce `(sum, nonzero_count)` task was challenged because input inspection can compute outputs and bypass the intended consumer. The revised proposal evaluates exact signed-trit coefficient polynomials modulo `2^64`, but sparse exponentiation versus Horner introduces an algorithm/representation confound. The draft now separates S (causal representation choice), A (algorithm routing), and T (genuinely ternary-specific mechanism). The latter has **no approved experiment**. Even at 5% density, proposed sparse payload can exceed two-bit packed payload for the earlier lengths, so no format crossover or speedup is guaranteed.

**Open gates:** Choose whether M2 is bounded S/A research or deferred pending a T hypothesis; justify a real producer/consumer lifecycle; separate format from algorithm comparisons; freeze a trace, selectors, byte grammar, hardware, statistical method and practical threshold; independently clear seven P1 findings; reconcile M1; obtain explicit commit-scoped authorization. Do not implement, benchmark, merge, or claim radix superiority from these draft documents. These observations are dated; refresh actual GitHub heads and CI.

---

## 2026-11-05 CPA-001 through CPA-005 Research Handoff & Architecture Baseline

### 1. Established Reality: Bounded Cross-Profile Implementation Works
**Future AI research agents must NOT be asked to re-establish or re-verify whether the bounded cross-profile implementation works.**
The CPA-004 experimental campaign established that:
- The 351-byte canonical program (SHA-256 `b0649777...`) executes across 24 instructions and halts deterministically (PC=27, SP=283).
- Hosted T81VM and freestanding Axion OS (AArch64 QEMU post-`ExitBootServices`) match the separately derived mathematical oracle at **all 25 observation boundaries** across registers, memory words, flags, and system registers.
- All checks pass cleanly under `NDEBUG` on the tested snapshot base `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4` (committed in `origin/experiment/cpa004-freestanding` at `b803bf916a1b1e44129f3a07abf96fa6103c51fc`).

### 2. What Remains Unproven & Active Research Questions
1. **Full ISA Conformance:** CPA-004 covers an admitted subset of 17 instructions on signed 64-bit integer values and scalar trits. It does not validate arbitrary big integers, floating point, fraction, vector/tensor operations, or the remaining 157+ opcodes.
2. **Four Preserved Normative Discrepancies:**
   - *Memory addressing:* Direct immediate address in implementation/semantics vs register-indirect `R[RS]` in `spec/tisc-spec.md`.
   - *Branch target & condition:* Direct target + R[B] zero-testing in implementation/semantics vs register-indirect + FLAGS in `spec/tisc-spec.md`.
   - *TXor interpretation:* Wrapped balanced difference `((lhs - rhs + 1) % 3) - 1` vs ambiguous "Sum" label.
   - *Opcode inventories:* 174 capability-frozen opcodes vs 209 registry entries vs 244 named C++ header opcodes (with undefined opcodes 221–223 and 247–255).
   *Directive:* Preserve these discrepancies; do not silently normalize or resolve them without explicit RFC approval.
3. **Comparative Advantage:** Execution correctness across dual profiles is an engineering milestone (Track F). It does **not** prove ternary superiority (Track R). H01–H08 remain open research questions requiring matched, rigorous binary controls.

### 3. Current Git & PR Status
- Pinned test base: `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- Current `main`: `ea6d031a8117341fe03013cc9d5473de1b07727d`.
- PR #709: **Open draft PR (unmerged)**; head `b803bf916a1b1e44129f3a07abf96fa6103c51fc` (`experiment/cpa004-freestanding`), base `ea6d031a8117341fe03013cc9d5473de1b07727d`.
- CPA-005 baseline: pinned on `origin/experiment/cpa005-integration-base` (commit `f7e089590020508077607580da271b6570fa8f60`, integrated tree `70734fd5bcfe06978cea13b4d2d595d4dda71a12`).
- PR #707: Open unmerged PR (CPA-002 opcode conformance repairs).
- Working branch: Must preserve tested boundaries and avoid unauthorized merge operations.

### 4. Primary Next Actions for Incoming Agents
1. Treat bounded dual-profile TISC execution as an established foundation; build on it rather than re-litigating it.
2. Advance the resolution of the four normative ISA discrepancies via formal specification drafts and maintainer review.
3. Advance Track R comparative hypotheses (e.g., H04-R03 Candidate A Phase C gates, BASE-01 physical Linux prerequisites) without conflating software emulation correctness with physical hardware advantage.

---

## 2026-11-05 CURRENT HANDOFF — CPA-006 complete; ISA-001 awaiting maintainer decision

**This is the newest operational handoff.** It supersedes earlier *current-status and next-action* sections, while preserving their historical reports and source pins.

### Established and bounded
- T81's implemented historical TISC profile has a working hosted T81VM and separate freestanding AArch64 QEMU evaluator after `ExitBootServices`. CPA-004/005: canonical 351-byte program, 24 executed instructions, 25 matching observation boundaries.
- CPA-006: **PASS — BOUNDED CONFORMANCE** for 19/19 frozen-oracle cases; 57 hosted runs and 57 fresh QEMU boots; 1,190 validated state records; Halt, runtime trap, admission rejection and budget exhaustion. See [committed acceptance report](https://github.com/t81dev/t81-foundation/blob/514fa17cf5629a7aea9ebb1b035f01f3ef7687f3/experiments/cpa006/README.md). Do not claim full ISA conformance, independent-person replication, a complete OS or ternary-vs-binary performance advantage.
- ISA-001: read-only source/lineage and normative-vs-implemented audit, **READY FOR MAINTAINER DECISION**. No implementation changes or new conformance executions. Its recommended normative interpretation is *not approved*.

### Pinned GitHub integration snapshot (2026-11-05)
| Surface | Observed identity and status |
|---|---|
| `main` | `7afa4f1a424be6c65fab28f8e5bcf4fddc3ef762` (PR #711 continuity refresh merged) |
| PR #707 | Open, independent opcode matrix test repair; head `55223cbd3ee29b5391e151197fe6eee1b8889452` |
| PR #709 | Open draft CPA-004 evaluator; head `b803bf916a1b1e44129f3a07abf96fa6103c51fc` |
| CPA-005 integration snapshot | `f7e089590020508077607580da271b6570fa8f60`, tree `70734fd5bcfe06978cea13b4d2d595d4dda71a12` |
| PR #710 | Merged into **experimental integration branch, not main**; merged result `514fa17cf5629a7aea9ebb1b035f01f3ef7687f3`, tree `f8f01f58901bfd9a57446a97df98bb8d481da022` |

### Outstanding semantic decisions
1. Direct-address implemented memory vs normative register-indirect memory.
2. Immediate target / register-tested JZ/JNZ vs normative register-target / FLAGS branch semantics; LLVM/MLIR lowering diverges as well.
3. Scalar TXor implemented wrapped difference vs ambiguous documentary 'XOR / Sum'.
4. Core freeze 0–173, registry/header extension assignments and TVecSub/SWAR collisions.
**No recommendation is accepted by implication.** Preserve historical CPA binaries and evidence; do not reinterpret raw 13-byte records without artifact-bound version selection.

### Primary next action — T81-ISA-002 (documentation and maintainer review only)
Draft a **versioned authoritative TISC state-transition RFC** with exact operands, tags, FLAGS, PC, traps, side-effect ordering, opcode manifests, profile selection and migration consequences. Present decision points and obtain explicit maintainer approval. Do not change production VM, freestanding evaluator, compiler lowering, tests, frozen CPA artifacts, or branches as part of the RFC drafting task.

**After approval only:** authorize discriminating successor conformance tests and one cumulative main-integration candidate (CPA-004 + CPA-006; handle #707 separately). Then consider a separately preregistered H02 signed-rounding comparison with optimized binary and representation-conversion controls.

### Handoff rules
Do not restart feasibility experiments, silently resolve the spec, claim merged-to-main status from PR #710, overwrite negative research, or confuse verified content hashes with independently replicated execution. Refresh GitHub PR tips and main before any subsequent action.
