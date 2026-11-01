# T81-AIOPT-001 M2 Independent Protocol Acceptance Audit

**Date:** 2026-11-01 (America/New_York)  
**Reviewer:** Codex, AI technical/scientific protocol reviewer  
**Status:** Proposed audit; no protocol acceptance or experiment authorization  
**Final disposition:** `PROTOCOL_REVISION_REQUIRED`

## 1. Executive verdict

Candidate B asks a meaningful unresolved **causal representation-selection** question, but PR #672 is not an executable preregistration and is not ready for approval. PR #671 is a defensible recommendation to review B; it does not demonstrate that B is scientifically superior to C. Neither PR is accepted or merged in the inspected snapshot. M2 execution remains unauthorized.

The present design can measure a synthetic serialization strategy. It cannot yet distinguish a useful adaptive policy from a density threshold, establish stateful conversion benefits, or attribute a gain to ternary computation. Its reduction outputs are unusually easy to obtain during input inspection, its format contract has unresolved canonicality and size limits, and its statistical rule names procedures that it does not specify. These gaps can produce a misleading positive conclusion even with correct implementations.

Require the P1 corrections below before protocol acceptance. There is no demonstrated P0 emergency or irreversible defect requiring abandonment of B. No benchmark, M2 implementation, merge, or experiment authorization was performed by this audit.

### Independence and evidence boundary

This is a fresh adversarial analysis in the visible task context, not independent human replication. I did not author #671/#672 within this visible conversation. Their model/prompt provenance and whether earlier Codex sessions share authorship cannot be authenticated; the PR account alone does not establish that relationship. I inspected their documents, prior AI reviews and source evidence before writing the findings. This shared AI/repository context is disclosed rather than treated as scientific independence. No second reviewer or delegated agent was used.

Source inspection and elementary derivations are new audit work. Historical correctness and performance runs remain **source-reported**. I did not rerun M1, H04-R02, CPU kernels, or CI. The embedded Commons review record sets `independent_execution=false`.

## 2. Verified sources and acceptance state

GitHub metadata and contents were read during this audit; these are snapshot statements, not promises of future status. `main` was `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263` (called **R** below). Every repository link below pins a revision. PR links identify discussions/status only.

| ID | Inspected source | Verified role and limits |
| --- | --- | --- |
| S1 | [#671 decision gate](https://github.com/t81dev/t81-foundation/blob/e58910b1cbc760c2685e8eb13a298e78c0eabc0d/docs/research/aiopt/T81-AIOPT-001-M2-B-VS-C-DECISION-GATE.md), §§Hypothesis, Minimal design, Explicit gates | Head `e58910b1cbc760c2685e8eb13a298e78c0eabc0d`; open/draft/unmerged. B recommendation, proposed 10%, heuristic secondary, no execution permission. |
| S2 | [#672 preregistration draft](https://github.com/t81dev/t81-foundation/blob/b67b79ea53b327388e324be8302f4d59427e3185/docs/research/aiopt/T81-AIOPT-001-M2-B-PREREGISTRATION-DRAFT.md), §§1–6 | Head `b67b79ea53b327388e324be8302f4d59427e3185`; open/draft/unmerged. Exact reductions proposed; generator, policies and measurement details pending. |
| S3 | [Merged #669 assessment at R](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md), §§4 Candidate B/C, 5–8 | [#669](https://github.com/t81dev/t81-foundation/pull/669) merged; head `30bef63a433f95c8be7356a75d8f6971255b569a`, reported merge R. B suggests ≥15% and beating heuristics; C recommended but explicitly unaccepted. Merged commentary is not experiment approval. |
| S4 | [Corrected H04 results](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md), [mathematics](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/MATHEMATICS.md), [model-gap assessment](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/H04_MODEL_GAP_ASSESSMENT.md) | Conditional abstract U costs; no CPU timing. 10-bit boundary control AB=1250, AT=1210 at r=1.60; crossover 5/3. Six trits and 10 bits have unequal native capacity. |
| S5 | [H04 evaluator](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/adaptive_experiment.hpp), `Evaluator::evaluate_trace`; [suffix oracle](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/oracle.py); [audit tests](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/tests/ci/h04_r02_audit_regression_test.py); [packing](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/h04_r02_adaptive_representation/packing.py) | Prefix DP/backtracking uses full trace; suffix/exhaustive-rational checks exist. `test_blocks_metadata_and_comparator` asserts 1250/1210; codec tests reject bad padding. Source existence is not a newly executed pass. |
| S6 | [CPU closure](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md), [MatVec report](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md) | Historical reported CPU negatives and frozen kernel work; no universal impossibility follows. Native ARM performance is not established by QEMU. |
| S7 | [MatVec kernels](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/packed_ternary_matvec/matvec_experiment.hpp), `matvec_int8_avx2`, `matvec_ternary_2bit_avx2`; [harness](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/packed_ternary_matvec/main.cpp), `run_benchmarks`, `main` | FP32 activation/accumulation paths, LUT unpacking, fixed shapes and repeated runs; workload labels do not prove physical cache regimes. Default execution runs benchmarks, so it was deliberately not invoked. |
| S8 | [M1 verification report at #665 head](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md); [#666 corrected review](https://github.com/t81dev/t81-foundation/blob/b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe/docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md); [#667 reconciliation](https://github.com/t81dev/t81-foundation/pull/667) at `1edf70bbffc9168c27be5c6c19cf97388f6991a4` | #665 open/draft, #666 open/non-draft, #667 open/draft; all unmerged. M1 local correctness report recommends independent acceptance; it does not grant it. #668 also remains open/draft at `86838191420ecc7ccdca02e41f4e009c6375417a`. |
| S9 | [AGENTS](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/AGENTS.md), [continuity charter](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md), [research charter](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/docs/research/RESEARCH_CHARTER.md), [canonical register](https://github.com/t81dev/t81-foundation/blob/3d395cb604ddd49da3354d17f0505f2ed033757e/docs/research/continuity/HYPOTHESIS_REGISTER_v1.json), H04/H08 | H04 requires matched fixed/adaptive binary/ternary contrasts. Preserve Hanoi/CWEB lineage and H01–H08; AI role labels do not establish independent validation. |

SHA-256 of inspected raw UTF-8 files (content identity only):

| Source | SHA-256 |
| --- | --- |
| S1 decision document | `8bffcf895309198bae98669f7cc00355d43bce91e7224db8a0a2d596eacaae36` |
| S2 preregistration document | `c0ed06a124779591455dec4aab2b2d878f1ace3578d308c5b60f33ca8154cc47` |
| S3 merged assessment at R | `a620a0494d6272fe6f0ce3287b1c9997aad77765a226caa2b8563bdcb48523eb` |
| S4 corrected-results document | `194c8af0d73cd1f0997a94ec87fba2806058b8c3dc423efbae25b6330e46895f` |

## 3. Prioritized findings and required corrections

Severity here concerns protocol acceptance: P0 means an immediate critical integrity/authority breach; P1 blocks scientific acceptance; P2 improves precision or transferability. A documented draft placeholder can be a P1 acceptance blocker without being misconduct.

### P0 — None established

Both target documents preserve the design-only boundary. No source inspected demonstrates that they executed M2, accepted themselves, or activated governance. Absence of such evidence is not a certification of all repository activity.

### P1-01 — Operation and observation can collapse the scientific question

**Sources:** S1 minimal design item 1; S2 §§2–4.

The selector may scan the whole input to obtain `nnz`; one required output is precisely `nnz`. The same pass can compute the sum. With positives P and negatives M, outputs are `(P-M, P+M)`. In sparse form the count is the stored K and the sum needs only K signs; indices do not contribute to these outputs. A round-trip correctness check does not prove that the timed decoder reads indices, reconstructs the vector, or even rereads retained payloads. Fused encode/reduce or compiler elimination can legitimately optimize away the intended read stage if only the pair is observable.

This does not prove the task is useless or that an implementation will cheat. It shows that its observable contract does not enforce its claimed storage/read lifecycle. A win might merely reflect a favorable serialization loop or redundant scans imposed on controls.

**Required:** Freeze whether timed work is actual reconstruction or direct encoded reduction; define the producer/consumer boundary, retention lifetime, release of original input and permitted fusion. Require evidence that the consumer depends on retained bytes, with identical validation and observable-output obligations for every arm. Include a competent direct one-pass reduction as a diagnostic lower-cost alternative, clearly excluded from the forced-serialization primary claim if it lacks that obligation. Either justify serialization as the synthetic research purpose or revise the operation before approval. Do not silently substitute GEMV or wildcard matching.

### P1-02 — Primary trace does not establish an adaptation opportunity

**Sources:** S2 §3; S3 Candidate B hypothesis and scope.

Four 256-record regimes with only lengths 256/4096 and densities .05/.95 permit a simple density threshold to mimic regime-aware selection. Long stationary blocks underrepresent frequent transitions. Conversely, one format might be fastest everywhere after single-use encoding costs. Heterogeneity of input does not imply a crossover in complete costs. These are unresolved possibilities, not measured outcomes.

The largest dense record is 4096 payload bytes. Processing records immediately may keep source/output hot; retaining all records instead yields a different working set and cache history. Neither lifecycle is fixed. Encoding every new vector from its original input is per-record dispatch, not necessarily conversion of a persistent object between formats. Hysteresis across unrelated records may add cost without preserving useful state.

**Required:** Specify streaming versus whole-trace retention, initial state, reuse=1, allocation/reuse/reset rules, and what actually changes on a switch. Keep constant/alternating/near-threshold controls, and freeze generator, PRNG, seeds, rounding, sign distribution, index sampling, order and trace identity before execution. Distinguish per-record selection from persistent representation conversion. Justify the trace analytically without asserting unmeasured crossovers; any later feasibility pilot needs separate authorization and held-out confirmation, not post-hoc replacement of a losing primary trace.

### P1-03 — Canonical byte language and framing limits are incomplete

**Source:** S2 §2, with the explicit unresolved sparse-layout blocker in §6.

The proposed contracts can be repaired narrowly:

| Format/boundary | Ambiguity or failure | Required specification |
| --- | --- | --- |
| Dense | Signed byte interpretation is not a complete portable wire rule | Explicit two's-complement bytes FF/00/01, reject all others; exact payload length L. Avoid host `char`/endianness assumptions. |
| Packed | Symbol map is explicit; pair order within a byte is not. `00` padding denotes -1 as a symbol, not semantic zero | Fix element-to-bit order and final unused pair positions. Require exact `ceil(L/4)` bytes and unused pairs exactly `00`; never reduce padding. Decide whether `11` in padding is a canonicality error (recommended: yes), independently of ignored logical positions. |
| Sparse | Count/indices/values order needs an exact byte grammar; no explicit prohibition of zero-valued entries | For example, K:u32le, then K index:u32le, then K signed bytes; require K≤L, strictly increasing indices <L, values only -1/+1, payload exactly `4+5K`, no trailing bytes. Zero entries produce multiple encodings of the same vector unless rejected. |
| Shared framing | L≤2^32−1 does not ensure payload fits a u32 length | Dense fits; sparse requires `4+5K≤2^32−1`, hence K≤858,993,458. Specify narrower supported domains or wider/versioned framing; never truncate. Perform length arithmetic before allocation using checked sufficiently wide types. |
| Tags/output/errors | Tag values/placement, unknown tags, count width, failure precedence and empty-stream rules absent | Fix these and L=0/K=0, truncation, surplus bytes, malformed lengths, invalid values, partial-output policy and allocation failure behavior. Count must cover L; sum is signed 64-bit. |

For allowed L, `abs(sum)≤L≤2^32−1`; signed 64-bit sum overflow is impossible for valid input. Allocation arithmetic, counters, intermediate multiplication, signed conversion and indexing remain hazards, especially on 32-bit hosts. Primary lengths being small does not repair the advertised boundary contract. Future correctness cases must include L mod 4 = 1,2,3; both proposed primary lengths are divisible by four and cannot test padding.

### P1-04 — Best-fixed and heuristic comparisons can support different claims

**Sources:** S1 hypothesis/secondary comparison; S2 §§3,5; S3 Candidate B success rule.

Best fixed means the fastest complete-trace fixed arm, not a per-record minimum. That definition in #672 is sound. But fixed kernels are unspecified: dense vectorized copy/reduction, efficient packed decode and sparse validation must receive competent comparable optimization, compiler settings and implementation budgets. Fixed arms should avoid selector scans they do not need. A selector may reuse genuinely computed descriptors only under an explicit, equally available fusion rule; generator labels, precomputed true density, phase number, future vectors and timing-derived lookups are unavailable deployment information.

#669's B success criterion included beating heuristics. #671/#672 demote that to a secondary contrast; an adaptive policy can therefore be labeled supported while losing to a simple threshold. This is acceptable only for a narrowly worded **selection versus fixed** claim, not incremental policy sophistication or AI value. Neither policy is presently defined, so they may even be identical.

**Required:** Freeze both algorithms, thresholds, hysteresis, ties, allowed causal information and state resets; document equal tuning budgets and train/validation/confirmation separation. Report each fixed arm and heuristic. Require an appropriate confirmed heuristic contrast for any claim of incremental adaptive-policy benefit, or explicitly limit the primary claim to generic selection. This is not evidence for H08 or a learned selector.

### P1-05 — Binary control cannot identify ternary-specific benefit as written

**Sources:** S2 binary-attribution control; S4 model-gap §5; S9 H04.

Three semantic states stored in binary bytes using two-bit fields already constitute a binary codec. A relabeling can use exactly the same bytes and instructions. A second symbol permutation tests code mapping/kernel quality, not physical radix. The shared control does not supply matched fixed/adaptive binary families needed for canonical H04's attribution contrast. At most this is an adaptation subquestion under H04, with a narrower experimental target.

**Required:** Define the control's exact byte language, operation, access/validation rules and optimized algorithm; admit identity equivalence where applicable. Claim only format/adaptation effects unless an independently specified mechanism and matched fixed/adaptive contrast separate them. Do not expand the experiment merely to manufacture a radix distinction. A permitted generic selection finding can still be useful.

### P1-06 — CI wording is not a reproducible statistical procedure

**Sources:** S1 measurement gate; S2 §§3,5,6.

Neither draft fixes confidence level, estimator, resampling unit/method, run count, power, warmups, arm order, stopping, exclusions or fastest-baseline selection treatment. The 1024 correlated records are not 1024 independent experiments. One deterministic trace permits conditional timing inference on that trace; it cannot estimate a workload-population effect. Repetition does not create new workload draws.

**Required:** Fill the statistical acceptance checklist in §5. Treat correctness/fairness failures as invalid evidence, not a statistical negative. Preserve noise-limited or underpowered outcomes as inconclusive. A procedure must be pinned before authorized measurement; reviewers must be able to reproduce its calculation from raw run rows.

### P1-07 — Practical threshold and upstream gates need an explicit decision

**Sources:** S3 Candidate B ≥15%; S1/S2 proposed 10%; S8 acceptance state.

No examined document derives 10% from deployment value, measurement precision, or a resource objective. It is a proposed practical margin, not a scientific constant. The change from #669's 15% and the weaker heuristic requirement need a reasoned maintainer decision before confirmation, independent of observed results. A lower margin is not automatically invalid; inheriting it as already approved would be.

**Required:** Record acceptance/rejection of the margin and resulting claim, reconcile M1 and #666/#667, explicitly select B versus C, and obtain separate written authorization tied to the final protocol commit and permitted implementation/measurement resources. Accepting this audit, merging a document, or passing CI grants none of these rights.

### P2-01 — Byte ledger exposes deliberate format choices

**Source:** S2 framing. **Audit derivation, not a measurement:** assuming the proposed sparse grammar, fixed record sizes are dense `8+L`, packed `8+ceil(L/4)`, sparse `12+5K`; adaptive records add a tag byte. Stream headers, alignment, capacity slack, allocator metadata, retained original inputs and selector state are additional actual-memory costs.

Sparse is smaller than packed only if `4+5K<ceil(L/4)`: K≤11 at L=256 and K≤203 at L=4096. Rounding .05 density to K=13 and K=205 makes sparse slightly **larger** than packed in both cells. Floor/ceiling choices should be pinned; storage crossing alone does not imply latency crossing. The synthetic sparse layout also charges four-byte indices despite 8/12-bit index domains at these lengths. This can be a valid fixed-format contract but must be disclosed, not called the best possible conventional compression.

### P2-02 — Historical reports contain unresolved measurement scope limits

**Sources:** S6/S7 and [tracked radix results](https://github.com/t81dev/t81-foundation/blob/1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263/experiments/radix_comparison/results.json).

The closure reports 11.7 ns versus 1.1 ns and 12.91 versus 17.15 GFLOP/s; the MatVec report has 12.97, other baseline values and a 69.4%–75.3% range. The tracked radix JSON instead records about 105.337 versus 1.99995 ns from another artifact. These do not supply a single reconciled raw run for a new effect estimate. The MatVec report names 45 MB L3 while its 4096² INT8 and packed matrices occupy about 16 MiB/4 MiB; a DRAM label alone does not establish DRAM saturation. Preserve the reports and their negative results; do not transplant their ratios or categorical claims into B. No reconciliation or retiming of frozen experiments is authorized here.

## 4. Experimental validity and historical consistency

**Meaningful question:** Can a fully charged causal policy exploit format cost differences on a specified stream? H04's corrected source expressly leaves causal adaptation and real conversion costs unresolved (S4/S5). Testing that question adds information beyond its offline symbolic DP. B nevertheless changes the domain from variable-width int64 tiers to fixed-alphabet vectors and density-driven layouts. It is a declared refinement, not a reproduction of H04-R02 or the original Hanoi scheduler.

**Validity of proposed workload:** Decode/reduce is an acceptable small synthetic codec probe if its observable lifecycle and claims are narrowed. It has no demonstrated production-AI representativeness. Sign symmetry, positional ternary arithmetic and native three-state hardware are not exercised. Sparse K and sign counts nearly solve its requested output; forcing redundant scans/reconstruction only on controls would favor a representation artificially. Equal mandatory serialization may instead suppress a useful compression effect at reuse=1. Neither possibility establishes an outcome before measurement.

**Does the primary trace discriminate adaptation?** Not yet established. Its best-fixed contrast can reveal value of per-record routing if cost crossings exist. It cannot by itself show benefit over a simple causal rule, persistent conversion benefit, generalization beyond four regimes, or ternary-specific advantage. Constant and adversarial traces are valuable diagnostics but cannot become confirmatory successes after the primary trace fails.

**History/governance:** S1/S2 correctly preserve H04's conditional AT=1210<AB=1250 model result; equal metadata does not erase it. It is not a CPU speedup prediction and does not imply a 10% target. Historical CPU losses remain relevant evidence of decode/conversion overhead, not universal ternary impossibility. C's zero-skipping/INT8-activation proposal has potential differences from the older FP32-activation kernels, but novelty, comparator competence and actual cache behavior still need review. B preference is a human research-priority choice, not a proven ranking. M1's finite exact-equality verification provides no M2 performance or authorization evidence. Preserve canonical H01–H08 and Hanoi/CWEB history; modify no frozen kernel, accepted finding, protected artifact or governance activation.

This document-only audit reduces ambiguity in research contracts and maintainer handoff. It adds no runtime surface and does not broaden the stable CLI/VM/CanonFS maintenance lanes into a new implementation program.

## 5. Comparator and statistical acceptance checklist

These are required design corrections, **not** an approved measurement recipe or permission to execute it.

| Item | Required reproducible decision |
| --- | --- |
| Best fixed | Define population target `min_f mu_f` over complete-trace costs. Report all three fixed arms; freeze selection on independent calibration or use simultaneous inference against every fixed arm. A holdout-selected arm supports a claim against that selected arm, not automatically the true best. Taking a noisy minimum is often conservative for a positive gain but still invalidates naive CI coverage and can overstate a negative. |
| Pairing | Each run block processes the identical complete trace with every arm; retain block/arm/order IDs and elapsed times. Randomize or balance **arm** order with a committed seed; preserve the primary trace's internal order. Reset policy state and lifetime rules equally. |
| Experimental units | Resample whole paired blocks or independent sessions at the level justified by noise dependence. Preserve within-block pairing across arms and any session clustering. Records, inner iterations and timer ticks are not independent units. State that uncertainty is conditional on one fixed trace/platform. |
| Estimator and CI | Choose ratio of mean complete-trace times, mean paired ratios or another precise target; these are not interchangeable. Pin confidence level, one/two-sided bounds, interval algorithm, resample count/seed, clustering and adjustment. For a best-fixed claim, simultaneous per-fixed improvement intervals can bound `min_f(1-mu_A/mu_f)`; freeze the exact construction. |
| Power/sample size | Fix block/session count and maximum stopping rule using declared variance assumptions, a target effect **above** delta, desired power and alpha. Report minimum detectable effect or precision. An effect exactly at delta is the support boundary, not a sensible high-power alternative. Any variance pilot must be separately authorized and excluded from confirmation/tuning leakage. |
| Multiplicity | Account for best-of-three selection, confirmatory heuristic claims, additional traces/platforms and any policy choices. Diagnostic comparisons stay labeled diagnostic. Do not choose the reported primary from favorable cells. |
| Timer/environment | Pin CPU/ISA, OS, compiler/version/flags, binary/source hashes, affinity/frequency policy, warmup, timer, repetitions, allocations, alignment, working sets and cold/warm procedure. Record realized conditions and noise rules; inspect optimized code/observable payload dependence. PMU diagnostics need separate permission if outside the final approved scope. |
| Complete cost | Start from a declared common raw-input boundary. Include validation, density inspection, encoding/framing/tagging, allocations, retained storage, dispatch, actual conversion, decoding, reductions and ordered output; define cleanup timing. No free prepacked inputs or generator density hints for one arm. Avoid summing separately timed stages as a substitute for total time. |
| Oracle | A hindsight oracle has future information and is diagnostic only. With real transition costs it requires a feasible whole-path schedule, not independent per-record minima; if switches are stateless, say so. Noisy observed minima are optimistically biased and not a certified latency bound. A cost-model oracle is only a bound within that model. |
| Supported | Correctness, observability and fairness pass; adjusted lower bound **strictly exceeds approved delta** for the declared contrast. Heuristic superiority requires its own predeclared contrast. |
| Bounded negative | Adjusted upper bound is **strictly below delta**, with valid precision/power and costs. This rules out the practical margin in scope; it can still allow smaller positive benefits and does not prove universal inferiority. |
| Inconclusive / invalid | Equality to delta or an interval spanning it is inconclusive, as are unresolved precision/noise issues. Correctness, replay, preregistration or comparator violations invalidate the confirmatory claim; preserve raw evidence and distinguish invalid from not tested. No result may be rescued by selective stopping or silently omitted arms. |

## 6. Audit procedure, checks and limits

Environment: local Darwin ARM64, Python 3.14.3. Initial workspace HEAD was `42b9f6240b136b19c52476727f9f78d2c2c01c08`, on an unrelated research branch with pre-existing untracked work. That checkout was not treated as current main and its unrelated files were not changed. Remote publication uses a dedicated branch from R through the GitHub connector.

Read-only evidence operations: GitHub `fetch_pr` for #665–#669/#671/#672, `fetch_pr_comments` for those PRs, changed-file listings for #671/#672, `fetch_file` at pinned commits, and GET of `branches/main` and historical experiment directories. Metadata verified #669 merged and the other listed PRs unmerged; no reviewed status is promoted to scientific acceptance. A guessed `test_audit.py` fetch returned 404; the actual inspected regression file is S5's `tests/ci/h04_r02_audit_regression_test.py`.

Local commands used include `git status --short`, `git log -1 --format='%H %s'`, `git show REV:PATH`, `git ls-tree -r --name-only R docs/research experiments tests`, `git cat-file -e REV^{commit}`, `uname -sm`, and `python3 --version`. SHA-256 entries were computed from `subprocess.check_output(['git','show',rev+':'+path])` with `hashlib.sha256(data).hexdigest()`. Analytical size/overflow bounds above were derived from the proposed grammar, not benchmarked.

The initial command `git fetch origin main refs/pull/671/head refs/pull/672/head` failed before fetching: `.git/FETCH_HEAD: Operation not permitted`. No escalation was needed: pinned objects were already readable locally and current sources/status were fetched read-only through GitHub. The first Commons command under `python3` failed with missing `jsonschema`; `/usr/bin/python3` lacked it too. An existing environment supplied the dependency; these read-only requests then succeeded:

```sh
/private/tmp/t81-pr656-venv/bin/python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H04"}}'
/private/tmp/t81-pr656-venv/bin/python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_evidence","arguments":{"evidence_id":"h04-limitations"}}'
```

Commons reported historical baseline `3d395cb604ddd49da3354d17f0505f2ed033757e`, manifest digest `a1fa8d17672d35364cd630ba8deba3498d18432095f3bf079ac7ad800aa0d6da`. Its old H04 limitations are historical discovery, superseded where applicable by corrected evidence at R; discovery does not refresh findings or grant execution rights. Review-record schema validation is a document check only. No experimental correctness tests, timing, build, hardware counters or benchmark scripts were run. Cross-platform behavior and implementation correctness therefore remain untested here.

Document checks passed using the existing environment's `jsonschema.Draft202012Validator` against the schema at R: embedded record shape and Commons source IDs, 21 distinct pinned file targets via `git cat-file -e REV:PATH`, and the sparse-size arithmetic. An initial whitespace assertion rejected intentional Markdown hard breaks; the corrected document check allows exactly two trailing spaces and passes. No research result depends on that formatting check.

### Embedded Commons independent-review record

Embedded to retain the required review disclosure in the single requested document. `source_ref` values below use existing Commons IDs; newer exact audit targets are pinned in `target` and S1–S8. This record is proposed and is not an update to the Commons manifest. `human_authorization` concerns experimentation; the user explicitly authorized this audit and its draft PR, not experiments.

```json
{
  "schema": "t81.research-commons.independent-review.v1",
  "id": "T81-AIOPT-001-M2-INDEPENDENT-PROTOCOL-AUDIT",
  "status": "proposed",
  "hypothesis_id": "H04",
  "attribution": {
    "name": "Codex",
    "kind": "agent",
    "model": null,
    "role": "Adversarial technical and scientific protocol reviewer"
  },
  "repository_revision": "1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263",
  "evidence": [
    {"source_ref": "register", "relationship": "context", "scope": "Canonical H04 identity and matched adaptation/radix requirements"},
    {"source_ref": "h04-r02", "relationship": "context", "scope": "Historical model snapshot only; current corrected evidence pinned in S4/S5"},
    {"source_ref": "h04-limitations", "relationship": "context", "scope": "Historical limitations, not current measurements"}
  ],
  "uncertainty": "Source inspection and analytical derivation only; historical executions, CI and performance not reproduced; independence of prior agent authorship cannot be authenticated.",
  "human_authorization": {"status": "not_requested", "human": null, "record": null},
  "target": "PR671 e58910b1cbc760c2685e8eb13a298e78c0eabc0d and PR672 b67b79ea53b327388e324be8302f4d59427e3185; adaptation subquestion under canonical H04",
  "independence": {
    "relationship_to_author": "No authorship of these drafts in visible task history; same AI ecosystem and repository context; prior authorship identity unknown.",
    "prior_context": "User task and repository instructions; inspected earlier AI reviews, target drafts and corrected evidence before analysis.",
    "independent_execution": false,
    "limitations": "No human replication, second reviewer, experiment execution, benchmark, CI rerun or independent hardware observation."
  },
  "adversarial_checks": [
    "Examined reduction bypass and causal descriptor information",
    "Derived sparse size crossover and u32 payload limit",
    "Examined packed padding, sparse canonicality and exact round-trip obligations",
    "Compared heuristic, fixed, adaptive and hindsight claims",
    "Checked threshold change and missing statistical unit/procedure",
    "Inspected corrected H04 source/test paths and historical CPU evidence limits",
    "Verified PR status without treating merge or AI review as authorization"
  ],
  "findings": [
    "No P0 established",
    "P1-01 through P1-07 require protocol revision before acceptance",
    "P2-01 and P2-02 bound format and historical measurement claims"
  ],
  "verdict": "changes_requested"
}
```

## 7. Final disposition and one recommended next action

**`PROTOCOL_REVISION_REQUIRED`**. B remains a plausible bounded representation-selection investigation. Neither draft is protocol-ready; this finding does not authorize implementation or replace human research direction. Experiment execution remains blocked by unresolved design and authorization gates, while the protocol itself is repairable.

**One next action:** Submit one consolidated, design-only revision of the existing #671/#672 proposal for maintainer review, resolving P1-01–P1-07 with an explicit claim, complete byte/lifecycle contract, frozen causal policies, reproducible statistical plan and threshold rationale; stop before implementation or measurement pending the separate human decision tied to its final commit.
