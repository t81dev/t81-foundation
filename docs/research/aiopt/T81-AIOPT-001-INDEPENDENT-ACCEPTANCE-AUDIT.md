# T81-AIOPT-001 — Acceptance and M2 protocol audit

Status: proposed audit; no maintainer acceptance, merge, or M2 execution authorization.
Snapshot: 2026-11-01 07:10:15 UTC. Author: Codex (GPT-6), AI-assisted source assessment and fresh local correctness reproduction.

| Area | Disposition |
|---|---|
| Bounded M1 correctness | **M1_ACCEPTANCE_RECOMMENDED** |
| Scientific assessment | **SCIENTIFIC_REVIEW_CORRECTION_REQUIRED** |
| Reconciliation gate | **RECONCILIATION_ACCEPTABLE** |
| M2 proposal | **M2_PROTOCOL_REVISION_REQUIRED** |

There are **0 P0, 4 P1, and 2 P2 findings**. M1's finite-domain correctness evidence supports a maintainer's bounded acceptance decision; it establishes no performance advantage. M2 needs a focused protocol revision before approval. This report does not grant either decision.

## Authority, provenance, and inspected sources

The requesting human authorized this audit, existing-harness correctness reproduction, and one report-only draft PR. The attached task is the audit specification; earlier implementation missions are context, not permission to implement M2.

**Independence limitation:** this same agent previously authored the corrections and verifier in #665 and participated in the earlier, different M1 design. Re-running its existing harness is fresh execution, not independent-person replication or independent scientific validation. The requested audit role does not erase that relationship. Mathematical checks below were derived separately from the matching implementation, but no new independent reviewer or experimental implementation was introduced. Jules's identity, execution environment, and initial M1 source revision cannot be authenticated from the scientific review alone.

The audit branch starts from current main `005714caced20b7ce54c37c68808d0492c7567b7`. All four actual remote PR heads were fetched and checked again through GitHub metadata immediately before preparing this report. They equal the task's reference SHAs; all remain open and unmerged.

| Source | Exact inspected revision | Primary evidence |
|---|---|---|
| #665 | `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db` | [protocol](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md), [implementation, verifier, report and evidence](https://github.com/t81dev/t81-foundation/tree/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog) |
| #666 | `b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe` | [scientific review](https://github.com/t81dev/t81-foundation/blob/b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe/docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md) |
| #667 | `1edf70bbffc9168c27be5c6c19cf97388f6991a4` | [reconciliation](https://github.com/t81dev/t81-foundation/blob/1edf70bbffc9168c27be5c6c19cf97388f6991a4/docs/research/aiopt/T81-AIOPT-001-M1-M2-RECONCILIATION.md) |
| #668 | `86838191420ecc7ccdca02e41f4e009c6375417a` | [M2 draft](https://github.com/t81dev/t81-foundation/blob/86838191420ecc7ccdca02e41f4e009c6375417a/docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md) |

Main advanced from `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` during the audit. The only intervening change was README.md; its revised research/evidence boundaries were inspected and the audit branch was rebased onto the current main above. The reviewed PR heads and all cited research evidence were unchanged.

Main-revision context inspected: AGENTS.md; README and handoff/roadmap/buildable-next-steps/project-profile entry documents; the [continuity charter](../continuity/MASTER_CONTINUITY_CHARTER_v1.0.md), [research charter](../RESEARCH_CHARTER.md), [canonical hypothesis register](../continuity/HYPOTHESIS_REGISTER_v1.json), [authority model](../../governance/SPEC_AUTHORITY_MODEL.md), and Commons evidence/schema surfaces. Canonical H01–H08 and historical Hanoi/CWEB continuity remain unchanged.

The [H04-R02 corrected results and model-gap assessment](https://github.com/t81dev/t81-foundation/tree/005714caced20b7ce54c37c68808d0492c7567b7/experiments/h04_r02_adaptive_representation) supersede the original broad negative interpretation. Under the matched 10-bit symbolic control, the conditional crossover is `r < 5/3`; at `r=1.60`, AT is 1210 U versus AB 1250 U. These are model units, not measured processor time or energy. The historical 96% is one ratio-1 trace, not a measured distribution or uncertainty estimate. Actual codec byte counts are a separate kind of evidence. H04 remains unresolved; binary implementations can use the same byte language. The [CPU research closure](../TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md) remains a program freeze for the tested kernel lane, not a universal impossibility theorem or a license for this audit to restart that lane.

Content-identity ledger (SHA-256; identity does not authenticate truth or provenance):

| Artifact at the revision above | SHA-256 |
|---|---|
| #665 `m1.py` | `4e7a07c3f52421e0df6725a4675b00cf9af2186260af545d72be8680ef6ac07e` |
| #665 `verify_m1.py` | `399471c92238577a528dd2628a60a2a2e143841e44526d40960adfd481a93549` |
| #665 `M1_VERIFICATION_REPORT.md` | `7da36bc3e9f7ded05174e71af9b97d459ec7ecbf848dfa81f0c0ab31d19949ce` |
| #666 scientific review | `0d3634987b25fae5ce417b045a3d1bbf909e851504c018daacc35dfbe479b4dc` |
| #667 reconciliation | `60e28827a8491a71bef2aed4bcbaa61502d9231c766e50db6b7bb66ada94ef31` |
| #668 M2 draft | `874a0e9457c4ddd9cf3e3e4a79b43db6e8b13763bdd71a4f93aae14e6bfdb165` |

## M1: observations, reproduction, and acceptance limit

The proposal is newly reconstructed, not a recovered historical specification. It defines exact equality over `{-1,0,+1}^4`, with the first 40 of 81 lexicographic vectors as the catalog. Byte encoding is trit+1; SWAR uses two-bit fields, least-significant pair first, rejecting 11. The ordered output is empty or a singleton catalog index. This is not the earlier min/max design, the unverified historical 22-record/1,890-case corpus, a native ternary machine, or a performance experiment.

The independently expressed base-3 ranking relation makes the bounded problem transparent: rank is the four mapped digits interpreted in base 3; rank below 40 yields that index, otherwise no match. This checks catalog membership without copying the production equality predicate. The verifier exercises 81 inputs, 40 patterns, all 3,240 semantic decisions for each of three paths, exact ordered outputs, encoding uniqueness and round trips, all 256 SWAR payloads (81 valid, 175 invalid), malformed byte values/types, catalog shape/order/content, lengths, error precedence, and unexpected Python types. Its deliberate wrong-output and wrong-error-category probes check that failures are observable.

The four corrected defect categories are supported by the preserved before/after evidence: consumed generator catalogs, bool/float aliases, unsupported-type/error-precedence behavior, and assertion elision under `-O`. The original 604 malformed count was accurate as a count of expected-error calls, including repeated payloads. It was not proof that error-category assertions executed: baseline normal mode executed those category assertions; optimized mode executed zero. Final code uses always-active checks: 4,171 per standalone run, comprising 3,567 positive checks and 604 error calls; 81 byte and 81 SWAR filter calls remain active in both modes. Do not conflate these subject checks with the verifier's 13,274-check total.

Fresh audit execution used a clean #665 worktree at e113e75, macOS 26.5.2 ARM64, Python 3.14.3, built with Clang 17.0.0. Commands below were each executed twice, with separate output files for repetitions 1 and 2:

```sh
cd /private/tmp/t81-aiopt-m1-verify
/opt/homebrew/opt/python@3.14/bin/python3.14 -B experiments/aiopt_001_fixed_catalog/m1.py
/opt/homebrew/opt/python@3.14/bin/python3.14 -B experiments/aiopt_001_fixed_catalog/verify_m1.py --output /private/tmp/t81-aiopt-acceptance-evidence/normal-1.verification.json
/opt/homebrew/opt/python@3.14/bin/python3.14 -B -O experiments/aiopt_001_fixed_catalog/m1.py
/opt/homebrew/opt/python@3.14/bin/python3.14 -B -O experiments/aiopt_001_fixed_catalog/verify_m1.py --output /private/tmp/t81-aiopt-acceptance-evidence/optimized-1.verification.json
```

The executed argv used absolute script paths under that same worktree; `summary.json` in the scratch evidence directory retains exact argv, cwd, exit codes and per-output hashes. No subject files were changed.

| Audit execution | Verifier checks | Failed / skipped | Subject exit |
|---|---:|---:|---:|
| Normal, repetition 1 | 13,274 | 0 / 0 | 0 |
| Normal, repetition 2 | 13,274 | 0 / 0 | 0 |
| Optimized, repetition 1 | 13,274 | 0 / 0 | 0 |
| Optimized, repetition 2 | 13,274 | 0 / 0 | 0 |
| Total | **53,096** | **0 / 0** | All four passed |

All eight subprocesses exited zero. Same-mode subject stdout and verification JSON were byte-identical. Across modes the canonical result digest was `21bd3c7f69e35d32cd5d5b847f34e0e1f2c5218e76d1ffb0db7327bde47966c9`. Normal verifier JSON SHA-256 was `70eed1b0e9f6e600189e973c7d2c948af8de34638e927d5eeefdb01a12ba0b03`; optimized JSON was `cb3894a5ae08f2b017857d3a8b73533814dde86d2e0415d58b8810cb1655b0c3`. Repeated checks are not independent statistical samples.

The committed artifact registry pins the corrected executable source to `32d7f9a885d527772227d0d13d108f5de3483497`; e113e75 adds its report/evidence. The fresh runs test e113e75 directly. Preserved original logs and machine-readable outputs remain in #665; fresh raw outputs remain in the named local scratch directory, not an additional committed artifact. This one-file audit retains commands, environment, totals and identities. No sanitizer, additional platform, native compiled adapter, or wildcard implementation was tested here.

**Assessment:** recommend acceptance solely of the reconstructed finite-domain Python correctness claim, subject to the maintainer's decision. Same-author reproduction cannot satisfy a separately required independent-person review. Pending CI must be resolved before merge; this recommendation is neither merge clearance nor evidence for M2 correctness.

### CI snapshot

GitHub workflow-run API results below are observations at the snapshot, not predictions. Empty legacy combined-status lists were not interpreted as success. No failed conclusion was observed, but required checks were incomplete. The API lists first-page pull-request-triggered runs, not an exhaustive branch-protection determination.

| PR | Format | T81 CI | CodeQL | Cross-platform |
|---|---|---|---|---|
| #665 | [success](https://github.com/t81dev/t81-foundation/actions/runs/41282007855) | [queued](https://github.com/t81dev/t81-foundation/actions/runs/41282007834) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282007838) | [success](https://github.com/t81dev/t81-foundation/actions/runs/41282007835) |
| #666 | [success](https://github.com/t81dev/t81-foundation/actions/runs/41282417106) | [queued](https://github.com/t81dev/t81-foundation/actions/runs/41282417120) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282417130) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282417094) |
| #667 | [success](https://github.com/t81dev/t81-foundation/actions/runs/41282347977) | [queued](https://github.com/t81dev/t81-foundation/actions/runs/41282347932) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282347905) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282347921) |
| #668 | [queued](https://github.com/t81dev/t81-foundation/actions/runs/41282496771) | [queued](https://github.com/t81dev/t81-foundation/actions/runs/41282497125) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282496896) | [running](https://github.com/t81dev/t81-foundation/actions/runs/41282496726) |

Main's workflow text has no `aiopt`, `verify_m1`, or `aiopt_001_fixed_catalog` invocation (`rg` exited 1). Generic workflow success alone would not prove execution of this isolated Python harness.

## Scientific review: original corrections and remaining defects

The three original P1 review threads are outdated but still unresolved in GitHub's thread metadata. Content corrections and UI resolution are different facts.

| Original finding | Current evidence | Audit assessment |
|---|---|---|
| H04-R02 interpretation | #666 §3.2 now retains conditional modeled advantages, corrected crossover, one-trace qualification and unresolved H04 | Substantive historical correction present; retain it |
| Independence / execution provenance | Header and provenance section now identify AI-assisted assessment, no independent experimental execution, and unverified initial source/logs | Unsupported independence claim qualified; original execution provenance remains unknown, not retroactively established |
| Invalid falsification / inconclusive outcomes | §6.7 distinguishes bounded negatives, insufficient precision, NOT TESTED, and limits of early stops | Substantive correction present; failure to reject is no longer equated with equivalence |

Those corrections do not make the entire review acceptable. **P1-01** covers the following residual errors at the pinned #666 revision:

- §§1, 8.1 call a scalar comparison a one-cycle globally optimal solution without measuring validation, conversion, dispatch or output. §3.2 predicts switching overhead orders of magnitude larger; §5.2 predicts microsecond resolution and million-fold AI overhead; §7.1 predicts tens of nanoseconds. These are unsupported forecasts, not measurements of this Python implementation.
- §3.3's “fundamental limits” language overstates the scoped CPU freeze. §3.4's claim that trivial selection violates H08 treats a hypothesis as a prohibition. §5.2's high-dimensionality requirement is not a necessary condition for useful learned selection.
- §2.5's `log2(81)=6.340` describes input information, not an intrinsic computation lower bound. Under uniform inputs the output has 40 singleton outcomes of probability 1/81 and an empty outcome of probability 41/81: entropy approximately 3.628 bits. Neither quantity proves instruction cost.
- §4.1's suggested unguarded `1ULL << k` membership expression is invalid for 64–80 in a 64-bit C++ word; a guarded rank<40 branch is safe. This is a flaw in a suggested alternative, not an observed defect in M1 Python.
- §4.2 confuses omitted Python semantic validation with necessarily violating memory safety or introducing undefined behavior. The latter requires an actual unsafe call path. Its 81/256-byte table claim also understates storage: at least 2/4 aligned 64-byte cache lines, potentially more when misaligned.
- §5.2's inference inequality uses the minimum execution cost across baselines. If the baselines include every available selector action, this is a clairvoyant per-input lower bound: nonnegative inference overhead cannot beat it. It is a useful bound, not the deployed conventional control a learned policy must outperform.
- §§6–7 call a proposed redesign preregistered, treat L=16 as exceeding LUT capacity without a memory budget, and make PMU counters/kernel isolation categorically necessary for statistical validity. A calibrated, controlled batched wall-clock study can be valid; counters are diagnostics. Projected-support tables remain possible even when a full-domain table is impractical.

Preserve the valid negative observations: the C40 domain is tiny and biased, packed equality is conventional binary logic, storage density alone is not computation speed, and no AI-specific gain has been demonstrated. Correct the unsupported extensions without reversing those observations.

## Reconciliation and cross-PR register

#667 accurately separates bounded correctness, scientific assessment, and human acceptance. It identifies the reconstructed protocol, retains unresolved H04, requires current-head/CI checks, and leaves M2 execution unauthorized. Its acceptance conditions are proposed gates, not evidence that they have been satisfied. **RECONCILIATION_ACCEPTABLE** applies to that gate structure; the stale citation below is a minor correction.

| Register item | Difference | Required handling |
|---|---|---|
| C1 | #665 PR description says M1 remains unexecuted; its current source/report and fresh runs show execution | P2-02: update metadata later; “unaccepted” remains accurate |
| C2 | #667 pins #666 at `a59f97d39ae9a20b954a76ba98ed6213b69f3c90`, not current b2b9d3c | P2-01: refresh citation; its instruction to recheck heads remains sound |
| C3 | #666 §6 proposes 15%; #668 proposes 10% | P2-01: choose one prospective threshold with a utility rationale; neither is an accepted historical standard |
| C4 | #666 density D denotes wildcards; #668 D denotes specified positions | P2-01: use one definition, exact rounding and generator contract |
| C5 | #666 demands PMU/isolation and uses universal timing language; #668 makes counters optional and bounds conclusions | P1-01: correct #666; do not weaken #668's appropriate qualifications |
| C6 | #666 “preregistered” wording versus #667/#668 pending approval and design-only status | P2-01: call it a proposal until a frozen record and authorization exist |

No reviewed document establishes that M2 execution is already authorized. No M1 result validates wildcard matching. Reconciliation does not override the earlier CPU freeze.

## M2: workload, controls, and decision validity

The wildcard semantics can produce multiple matches, but merely allowing wildcards does not make the proposed random workload discriminating. With iid uniform input trits and each pattern position specified independently with probability d, a row matches with probability `(1 - 2d/3)^L`; expected hits are `M(1 - 2d/3)^L`. For exactly s specified positions, the probability is instead `3^-s`. The generator must choose and document one model, including integer rounding. Skew, correlations, and planted matches change these calculations.

These are analytical expectations, **not M2 executions or measured hit rates**:

| Illustrative proposal point | Expected matches under the iid-position model |
|---|---:|
| #666: L=16, M=100, wildcard density .20 | 0.000506 |
| #668: L=64, M=4096, specified density .25 | 0.0351 |
| #668: L=64, M=4096, specified density .50 | 2.20 × 10^-8 |
| #668: L=256, M=4096, specified density .25 | 2.20 × 10^-17 |
| #668: L=1024, M=4096, specified density .25 | 3.39 × 10^-78 |

The probability of any hit is at most the expected count. Many candidate cells therefore tend toward empty output, not meaningful multi-hit filtering. Shared specified positions can also collapse a long vector into a small projected key. Length alone does not defeat indexing. Duplicate rows and all-wildcard rows are necessary edge cases, but neither alone supplies a realistic discriminating primary workload. **P1-02** requires actual hit strata, support structure, seeds, immutable identities, partition rules and a narrowed justified primary set. The 576-cell grid is explicitly a candidate grid, not an execution plan.

#668 lists appropriate comparator families and says not to force scans. It does not yet freeze executable algorithms, cost boundaries, catalog reuse, memory limits, or selection of the strongest control. **P1-03** requires a competent optimized byte/SIMD control, packed two-bit conventional control, and inverted-index/bitset algorithm, with projected-key/hash/LUT controls where feasible. A binary implementation can use exactly the same two-bit representation and predicate as “packed ternary”; an identical implementation should be labeled accordingly rather than counted as independent evidence of a radix benefit. Include wildcard masks, index metadata, preprocessing, validation, conversion, allocation and ordered output in the comparison. Do not charge one side full setup while granting another prevalidated resident data.

The proposed 10% threshold is a prospective utility choice, not a physical constant. It needs a reason tied to the intended cost and complexity, plus an exact primary estimand. **P1-04** remains open: sample units and size, pilot/confirmatory split, power alternative, interval method, multiplicity, baseline selection, seed partitions and stopping/resource limits are not fixed. Repeated inner timing loops reduce timer error but are not independent workload samples. Choosing the fastest noisy baseline after observing final data also needs explicit handling.

The draft correctly separates supported, bounded-negative, inconclusive and untested outcomes. Retain this. A result can distinguish algorithm/packing engineering on the chosen binary host; it cannot establish a ternary-exclusive advantage, native hardware superiority, or AI optimization. Relevance to T81 is bounded representation-and-cost methodology, not a demonstration of adaptive selection.

## Ranked corrections and acceptance conditions

Findings below are the complete counted register; related examples above are not additional counted findings. All remain open in their source PRs because this audit changes only its report.

| ID | Priority | Evidence and consequence | Smallest adequate correction |
|---|---|---|---|
| P1-01 | P1 | #666 sections identified above contain unsupported performance universals, wrong cost/memory claims and proposal/measurement confusion | Correct those claims and equations in place; retain valid finite-domain and historical negatives |
| P1-02 | P1 | #668 workload section lacks a frozen nondegenerate corpus; analytical hit rates expose mostly-empty cases | Specify a small hit-stratified primary workload with exact support structure, generator, seeds, partitions, digests and edge cases |
| P1-03 | P1 | #668 comparator/cost sections leave strongest controls and fair setup/reuse accounting unresolved | Freeze competent controls, conventional packed identity, trust boundary, reuse, output and memory accounting |
| P1-04 | P1 | #668 hypotheses/statistics leave primary estimand, sample size/power, intervals, multiplicity and stop rules pending | Freeze a pilot-to-confirmatory decision plan and finite resource limits before implementation/measurement approval |
| P2-01 | P2 | C2–C4 and C6: stale scientific pin, inconsistent proposed threshold/density and preregistration wording | Reconcile one explicitly prospective version and update citations |
| P2-02 | P2 | C1: stale #665 execution status in PR description | Align metadata with executed evidence while preserving pending human acceptance |

No P0 defect or new bounded-M1 correctness failure was observed. CI incompleteness and missing independent-person acceptance remain explicit operational/governance gates, not invented failed tests.

## Minimum scientifically adequate next design

This is a recommendation for a revision, **not a frozen experiment or authorization**. Reduce the design before adding implementations.

1. Use one synthetic primary family initially: L=64, M=256, B=64, catalog reuse R=1. As a concrete proposed hit balance, each batch contains 32 zero-hit, 16 one-hit and 16 four-hit inputs. Use heterogeneous, overlapping specified-position supports and distinct matching rows, so matches are not merely duplicate indices or a single shared short projection. Freeze support counts, correlations, generation/rejection rules and immutable seed sets. An independent oracle must verify the claimed hit counts. Label the family synthetic; these sizes and strata are planning choices, not demonstrated application representativeness.
2. Include a small shared-support/index-friendly diagnostic control and malformed/all-wildcard/no-wildcard/duplicate boundary cases. Do not promote diagnostics to extra confirmatory hypotheses. Freeze separate development/pilot and final corpus identities; prevent tuning or comparator selection using the sealed final seed/material. State who can unseal it and record every failure without silent exclusion.
3. Establish new wildcard correctness first. A separately derived scalar oracle should cover small exhaustive domains: at L=4 there are 81 semantic inputs and 256 wildcard patterns, hence 20,736 predicate decisions. This cardinality is a proposed future test, not a run performed here. Add ordered duplicate outputs, invalid encodings, tails, word boundaries and error precedence for larger adapters. M1's equality oracle cannot substitute.
4. Require competent scalar byte, optimized byte/SIMD, conventional packed two-bit, and inverted-index/bitset controls. Include projected-key lookup where the selected supports permit it. Allow algorithm choice rather than forcing every representation into a scan. Explain when packed ternary and conventional packed binary are the same implementation.
5. Use common canonical inputs and ordered 32-bit catalog-index output; charge validation, conversion, catalog/index construction, dispatch, matching and materialization once per R=1 batch. Report resident/index bytes and setup components separately. A resident-data timing may be diagnostic only. Freeze memory cap, physical host, compiler/features, calibration, thread/thermal policy and wall-time/resource ceilings before approval. One physical host is enough for the first explicitly platform-bounded claim.
6. Define the primary ratio as packed end-to-end batch time divided by each eligible conventional control time, with an explicitly chosen aggregation across independent catalog/batch units. If 10% is retained, require simultaneous confidence bounds below 0.90 against all competent eligible controls for a practical-positive claim. A lower bound above 0.90 excludes a 10% benefit only in that tested scope; a crossing interval is inconclusive. Missing or invalid controls prevent the claim. Do not silently compare the packed implementation against an identical packed control and then infer a radix distinction.
7. Plan randomized paired measurement blocks. Predeclare the interval method, its assumptions/coverage, familywise error handling, and comparator-selection policy. Use a disjoint pilot only after separate permission to estimate variance and fix confirmatory sample size; for example, power at a proposed ratio 0.80 against the 0.90 practical boundary, not “power at the boundary.” Freeze sample count, final seeds, exclusions limited to specified invalid instrumentation, raw-output preservation and resource stopping rules before confirmation. Unrun work stays NOT TESTED; do not stop when a favorable p-value appears.
8. Obtain explicit maintainer authorization for the narrowed study, including any exception to the existing CPU-lane freeze, resource budget, and acceptance prerequisites. No learned selector is needed in this baseline study. Any later AI-selection proposal needs separate authorization, disjoint training/final data and competent nonlearned selectors with full training/inference/feature costs.

This smaller design can test whether a particular implementation is useful under a stated cost contract. A positive packing result alone would not justify reopening general ternary-performance claims or establish H08.

## Validation, remaining dependencies, and next action

Validation performed: actual-head/source inspection, preserved evidence inspection, fresh normal/optimized correctness executions, analytic catalog/hit-rate checks, current CI API inspection, report provenance/schema checks, link-path checks and report-only diff checks. No M2 generator, matcher, benchmark, optimizer or timing experiment was implemented or run. No historical results were edited.

Report-validation results: all three embedded JSON records passed their Draft 2020-12 Commons schemas using `jsonschema` 4.26.0; all relative file links and pinned GitHub blob paths resolved against local Git objects; all 46 preserved #665 artifact hashes matched; the four stored final verifier records summed to 53,096 checks with no failures. These checks ran through `/private/tmp/t81-pr656-venv/bin/python -B` with `Draft202012Validator`, `git cat-file -e REV:PATH`, and `hashlib.sha256` comparisons. The report-only whitespace and inventory checks are reproducible with:

```sh
git diff --cached --check
git diff --cached --name-only
git diff --name-only 005714caced20b7ce54c37c68808d0492c7567b7 HEAD
```

Expected inventory: only this report. No CI success is claimed for the audit PR at publication.

Remaining dependencies are the four P1 corrections, the two minor consistency fixes, completed applicable CI, a maintainer's bounded M1 decision (and independent-person review if required), an approved frozen M2 protocol and explicit resource/execution authorization. Publication of this report satisfies none of those approvals.

**Single most important next action:** make one focused document revision resolving P1-01 through P1-04 and narrowing M2 to a frozen, fair, statistically decidable study; seek the maintainer's decision on that concrete design. Do not start another implementation or a new corrective PR automatically.

## Structured proposed evidence records

The following records use the Commons independent-review, reproducibility-report and evidence-discrepancy schema shapes within this single report. Their status remains proposed; schema validity authenticates neither independence nor acceptance. H08 is methodological context, not a newly tested hypothesis. The reproduction record describes fresh execution explicitly qualified as same-author. These are not additions to the Commons manifest and activate no governance protocol.

```json
{
  "status": "proposed",
  "hypothesis_id": "H08",
  "attribution": {
    "name": "Codex",
    "kind": "agent",
    "model": "GPT-6",
    "role": "Same-author acceptance audit; no independent-person acceptance"
  },
  "repository_revision": "005714caced20b7ce54c37c68808d0492c7567b7",
  "evidence": [
    {
      "source_ref": "https://github.com/t81dev/t81-foundation/tree/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog",
      "relationship": "supporting",
      "scope": "Bounded four-trit Python correctness, not performance or independent-person validation"
    }
  ],
  "uncertainty": "Fresh same-author execution on one macOS ARM64 Python environment; CI incomplete; M2 unimplemented and unmeasured.",
  "human_authorization": {
    "status": "authorized",
    "human": "Requesting user",
    "record": "Current Independent Acceptance & M2 Protocol Audit task: report, existing-harness reproduction, one draft PR only; no M2 authorization"
  },
  "schema": "t81.research-commons.independent-review.v1",
  "id": "aiopt-001-acceptance-audit-review",
  "target": "PRs #665–#668 at the source-ledger SHAs",
  "independence": {
    "relationship_to_author": "Same agent authored #665 corrections and verifier",
    "prior_context": "Prior M1 implementation/verification tasks and earlier different M1 design",
    "independent_execution": false,
    "limitations": "Fresh execution occurred, but no independent-person or independent scientific replication is claimed."
  },
  "adversarial_checks": [
    "Re-execute existing harness normally and with -O",
    "Check H04 corrections against pinned main evidence",
    "Derive wildcard hit probabilities and inspect comparator/statistical gaps"
  ],
  "findings": [
    "P1-01 scientific claims need correction",
    "P1-02 nondegenerate corpus not frozen",
    "P1-03 comparator and cost boundaries not frozen",
    "P1-04 confirmatory statistical plan not frozen",
    "P2-01 cross-document consistency",
    "P2-02 stale PR execution metadata"
  ],
  "verdict": "changes_requested"
}
```

```json
{
  "status": "proposed",
  "hypothesis_id": "H08",
  "attribution": {
    "name": "Codex",
    "kind": "agent",
    "model": "GPT-6",
    "role": "Same-author acceptance audit; no independent-person acceptance"
  },
  "repository_revision": "005714caced20b7ce54c37c68808d0492c7567b7",
  "evidence": [
    {
      "source_ref": "https://github.com/t81dev/t81-foundation/tree/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog",
      "relationship": "supporting",
      "scope": "Bounded four-trit Python correctness, not performance or independent-person validation"
    }
  ],
  "uncertainty": "Fresh same-author execution on one macOS ARM64 Python environment; CI incomplete; M2 unimplemented and unmeasured.",
  "human_authorization": {
    "status": "authorized",
    "human": "Requesting user",
    "record": "Current Independent Acceptance & M2 Protocol Audit task: report, existing-harness reproduction, one draft PR only; no M2 authorization"
  },
  "schema": "t81.research-commons.reproducibility-report.v1",
  "id": "aiopt-001-acceptance-audit-reproduction",
  "target": "e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db",
  "environment": "macOS 26.5.2 ARM64; Python 3.14.3 built with Clang 17.0.0; clean /private/tmp/t81-aiopt-m1-verify worktree",
  "commands": [
    "python3.14 -B experiments/aiopt_001_fixed_catalog/m1.py (twice)",
    "python3.14 -B -O experiments/aiopt_001_fixed_catalog/m1.py (twice)",
    "python3.14 -B experiments/aiopt_001_fixed_catalog/verify_m1.py --output /private/tmp/t81-aiopt-acceptance-evidence/normal-{1,2}.verification.json (one execution per output)",
    "python3.14 -B -O experiments/aiopt_001_fixed_catalog/verify_m1.py --output /private/tmp/t81-aiopt-acceptance-evidence/optimized-{1,2}.verification.json (one execution per output)"
  ],
  "artifact_identities": [
    {
      "path": "/private/tmp/t81-aiopt-acceptance-evidence/normal-1.verification.json",
      "sha256": "70eed1b0e9f6e600189e973c7d2c948af8de34638e927d5eeefdb01a12ba0b03"
    },
    {
      "path": "/private/tmp/t81-aiopt-acceptance-evidence/optimized-1.verification.json",
      "sha256": "cb3894a5ae08f2b017857d3a8b73533814dde86d2e0415d58b8810cb1655b0c3"
    }
  ],
  "outcome": "reproduced_in_scope",
  "failures": [],
  "scope": "53096 passing verifier checks, zero failed/skipped, four standalone passes; repeated same-author correctness execution only"
}
```

```json
{
  "status": "proposed",
  "hypothesis_id": "H08",
  "attribution": {
    "name": "Codex",
    "kind": "agent",
    "model": "GPT-6",
    "role": "Same-author acceptance audit; no independent-person acceptance"
  },
  "repository_revision": "005714caced20b7ce54c37c68808d0492c7567b7",
  "evidence": [
    {
      "source_ref": "https://github.com/t81dev/t81-foundation/tree/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog",
      "relationship": "supporting",
      "scope": "Bounded four-trit Python correctness, not performance or independent-person validation"
    }
  ],
  "uncertainty": "Fresh same-author execution on one macOS ARM64 Python environment; CI incomplete; M2 unimplemented and unmeasured.",
  "human_authorization": {
    "status": "authorized",
    "human": "Requesting user",
    "record": "Current Independent Acceptance & M2 Protocol Audit task: report, existing-harness reproduction, one draft PR only; no M2 authorization"
  },
  "schema": "t81.research-commons.evidence-discrepancy.v1",
  "id": "aiopt-001-acceptance-audit-discrepancy",
  "conflicting_claims": [
    "PR #666 asserts categorical timing and PMU necessities",
    "PR #668 treats performance as unmeasured and PMU counters as optional diagnostics"
  ],
  "affected_sources": [
    "https://github.com/t81dev/t81-foundation/blob/b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe/docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md",
    "https://github.com/t81dev/t81-foundation/blob/86838191420ecc7ccdca02e41f4e009c6375417a/docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md"
  ],
  "observed_difference": "Scientific review retains unsupported quantitative/universal statements despite corrected scoped historical findings.",
  "proposed_resolution": "Correct P1-01 in the existing document; preserve historical negatives and pending authorization; no new performance result inferred."
}
```
