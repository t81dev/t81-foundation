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

## 2026-11-01 arithmetic decoder acceptance gate — PR #675 (source-inspected; NOT RUN)

**Checked GitHub PR:** [#675](https://github.com/t81dev/t81-foundation/pull/675), open and unmerged at head `d4c053c9405a078cb588e7227507c61449c7587e`, base `1737a7ff5a267590338c0e7b513c95e95ccd6177`. Only `include/t81/ternary/arith.hpp` and `tests/cpp/ternary_arith_test.cpp` changed. The PR was reported mergeable; this is **not** a merge recommendation or acceptance receipt.

**Confirmed source-level contradiction (Tier B + Tier D, not an executed test):** The test file now contains always-active `ALWAYS_ASSERT` checks for 41-trit `+1/-1`, signed-64 extrema, saturation, and positive/negative cancellation vectors. However, the decoder still evaluates Horner prefixes with early `INT64_MAX/MIN` returns when `result > INT64_MAX/3` or `result < -3074457345618258603LL`. This cannot generally establish correct *final* saturation for arbitrary balanced-ternary vectors with subsequent lower-order cancellation. For the 41-trit positive vector (`ds[40]=Pos`, `ds[0..39]=Neg`), the exact mathematical value is `(3^40+1)/2 = 6078832729528464401`, within signed-64 range; the current prefix guard can return `INT64_MAX` prematurely. The sign-reversed vector should decode to `-6078832729528464401`. The newly added regression tests require these exact values, but the production decoder remains unchanged with respect to this defect.

**Hosted checks at inspected head:** Format Check completed successfully; T81 Foundation CI completed with failure; CodeQL and Cross-Platform Determinism Verification were still in progress when checked. No CTest, sanitizer, or independent oracle was executed by this handoff. Jules' earlier report of three passing CTest targets, deterministic/randomized sweeps and clean sanitizers refers to a **prior** head `f57c532af3c7df7e44238f7661313a9c55337d31`; it does not validate the new cancellation assertions at `d4c053c`. The earlier report attributed governance-metrics and spec/docs failures to the pinned baseline; recheck exact job logs before asserting the latest failure has the same cause.

**Disposition:** **CORRECTION INCOMPLETE — NOT READY TO MERGE.** Do not treat PR #675, the prior Jules READY FOR REVIEW report, or the added tests as proof that the algorithm is fixed. Preserve existing negative and inconclusive evidence.

**Single bounded next action:** On the **existing PR #675**, run `t81_ternary_arith_test` with assertions active at the exact current head; report actual-versus-expected cancellation outputs and independently verify using arbitrary precision. If the mismatch is confirmed, correct `decode_i64` so final saturation accounts for cancellation, without `__int128` or signed-overflow UB. Restrict changes to the two already modified files; rerun arithmetic CTests, deterministic/randomized boundary comparisons, and ASan/UBSan. Report exact SHA, commands, environment, counts, logs, baseline-vs-regression CI attribution and remaining limitations. **No new PR, merge, governance edits, CI relaxation, or unrelated refactoring.**

**Phase 14 dependency:** The source audit of arithmetic and packed-tritwise backend equivalence found that numeric agreement via `to_int64()` is weaker than canonical representation identity, and that AVX2/NEON execution needs architecture-specific receipts. A **test-only** backend-equivalence expansion is proposed, not implemented or authorized as completed. Do not start it as a substitute for correcting #675; after #675 is independently reconciled, confirm scope and current test APIs before proceeding. TXor remains noncommutative ternary difference, not binary XOR.

**Handoff receipt:** source-inspected GitHub PR diff, decoder and test excerpts, and workflow conclusions at `d4c053c`; no local build, CTest, sanitizer, benchmark, independent replay, merge, or approval performed in this documentation update.
