# T81-AIOPT-001 — M2 Candidate B Polynomial Protocol Revision

**Status: REVISION PROPOSED / DESIGN ONLY / NOT PREREGISTERED / NO IMPLEMENTATION OR MEASUREMENT AUTHORIZED**  
**Date:** 2026-11-01  
**Replaces:** This PR's original `encode → decode → (sum, nonzero_count)` workload, which remains recoverable in Git history.  
**Review basis:** [Codex acceptance audit #673](https://github.com/t81dev/t81-foundation/pull/673) (seven P1 blockers), [Jules adversarial review #674](https://github.com/t81dev/t81-foundation/pull/674), [B/C decision #671](https://github.com/t81dev/t81-foundation/pull/671), [merged design assessment #669](https://github.com/t81dev/t81-foundation/pull/669), and [H04-R02 corrected results](../../../experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md).

## 1. Research question and limitations

**Proposed question:** Can a causal representation/algorithm routing policy improve *complete-trace* exact polynomial-evaluation latency over the best fixed, competent representation/algorithm strategy, after charging observation, encoding, storage, conversion, precomputation, dispatch and output costs?

This is an experiment in **adaptive software representation and algorithm selection**. It does **not** directly test native ternary hardware, balanced-radix physical optimality, energy savings, AI learning, or general-purpose inference acceleration. An adaptive win is not a ternary-specific win. H04-R02's AT=1210 U versus AB=1250 U at r=1.60 is a conditional *symbolic model* result, not a CPU prediction.

**Current status:** The polynomial workload is a *candidate*, not an accepted scientific replacement. A reviewer must determine whether its algorithm/format tradeoffs are nontrivial, fair, and worth implementing.

## 2. Exact mathematical operation

For each record `t`, input is a coefficient vector `c_t` of length `L` over `{-1,0,+1}` and an unsigned 64-bit evaluation point `x_t`. Produce exactly one unsigned 64-bit output

`y_t = Σ(i=0..L-1) c_t[i] * x_t^i (mod 2^64)`.

All additions, subtractions, and multiplications are modulo `2^64`, using unsigned 64-bit arithmetic; do not invoke signed-overflow behavior. Interpret -1 as additive inverse modulo `2^64`. The empty polynomial evaluates to zero, including at x=0. Define x^0=1 even for x=0. Evaluation at x=0 returns c[0] modulo `2^64` when L>0. Include x=0,1,2,2^64-1, random odd/even x, all-zero/all-positive/alternating-sign vectors, long zero runs, and boundary lengths in correctness tests. An independent coefficient-array oracle must not share the production evaluator's dispatch logic.

All arms receive the **same** coefficient vectors, evaluation points, ordering and outputs. Reject invalid trits, malformed encodings, noncanonical bytes, out-of-range/duplicate sparse indices, trailing bytes and length arithmetic overflow before accepting the record. Define fail-closed behavior consistently.

## 3. Canonical representation contracts — proposed, reviewable

Every stored record has a common little-endian header `L:u32le | payload_len:u32le | x:u64le` (16 bytes). The record's format is known by a fixed-arm stream type; a mixed adaptive stream adds one explicit `format_tag:u8` (0=dense, 1=packed, 2=sparse) after the header. A format-specific payload follows immediately, with no implicit padding or trailing bytes. Any shared stream envelope, allocator alignment, record index or additional tag must be separately accounted for in real memory and time.

- **Dense:** exactly L bytes, each one of `FF` (-1), `00` (0), `01` (+1). Reject other byte values.
- **Packed:** exactly `ceil(L/4)` bytes. Element i occupies bits `2*(i mod 4)` and `2*(i mod 4)+1` of byte `floor(i/4)`; `00`=-1, `01`=0, `10`=+1; `11` invalid anywhere. Unused high two-bit pairs in the last byte must be `00` and are **not semantic coefficients**. Require exact length and canonical padding.
- **Sparse:** `K:u32le`, then K strictly increasing `index:u32le`, then K signed-value bytes (`FF` or `01` only). Indices are less than L, zeros forbidden, and K<=L. Exact payload size `4+5*K`; no trailing bytes. Require `4+5*K <= 2^32-1`, hence K<=858993458, and checked wider intermediate arithmetic before allocation. Missing indices imply zero coefficients.

For all formats require `0<=L<=2^32-1` and `payload_len<=2^32-1`; reject a record if its selected format cannot encode it within these bounds. The small proposed study domain below is far narrower. Reject unknown format tags, invalid lengths, truncation, noncanonical padding and excess bytes; no partial output for a failed record. Empty streams are valid and produce no outputs. These are proposed wire contracts, not implemented/validated codecs.

**Illustrative storage ledger (no speed claims):** fixed-arm bytes per record are dense `16+L`, packed `16+ceil(L/4)`, sparse `20+5K`; mixed adaptive adds one tag byte per record. For L=4096 and K=205, sparse payload is 1029 bytes versus packed 1024 bytes; a 5% sparse record does not automatically favor sparse storage.

## 4. Algorithm controls — separate algorithm from representation

Each applicable format must be tested with a competent algorithm, not automatically assigned the most favorable one:

1. **Dense Horner:** process coefficients in descending index order, `acc = acc*x + c_i (mod 2^64)`.
2. **Packed Horner:** decode each coefficient and apply the same Horner recurrence.
3. **Sparse gap-Horner:** process nonzero coefficients in descending index order; multiply by `x^gap` to skip missing coefficients and finish with the remaining low-order power. Verify the exact exponent/gap convention against the oracle.
4. **Sparse power evaluation:** compute `Σ c_i*x^i` using a declared exponentiation strategy. Charge any power table, memoization, index traversal and reuse.
5. **Equivalent binary-coded controls:** since packed ternary is already a binary two-bit encoding of three states, include a semantically identical two-bit mapping/control where meaningful. Do not claim radix-specific benefit from an equivalent relabeling.

**Fairness requirement:** Compare *algorithm and representation* as distinct factors. Where feasible, allow both dense and sparse decoded streams to use Horner and sparse algorithms with all required conversions charged. A fixed arm may choose a preregistered representation/algorithm combination; an adaptive arm cannot claim a ternary-specific gain from switching between fundamentally different algorithms. If full crossing is infeasible, restrict conclusions to the combined routing system.

A competent **direct evaluation from the original input** is a diagnostic lower-bound control. It must not be described as an equivalent stored-representation comparator if the primary contract requires retention and later decoding.

## 5. Producer–consumer lifecycle and causal selection

**Proposed lifecycle:** A producer receives all input records, encodes each according to the selected arm and writes its complete canonical bytes into a retained buffer. The original coefficient arrays are then unavailable to the timed consumer. The consumer traverses the retained records in order, validates/decodes as needed, evaluates P(x), and writes outputs. End-to-end timing includes producer, descriptor scan, encoding, retention, consumer and output. Do not allow the producer to cache y, precompute the evaluation, or pass a hidden coefficient-array pointer to the consumer. An independent checksum/round-trip validation and a correctness oracle confirm that the consumer depends on retained bytes. Compiler optimizations are allowed only when they preserve this observable contract.

**Proposed switching semantics:** Independent records select an encoding at production time; there is **no persistent object conversion** unless a separately specified stateful-reuse variant is approved. Selector state, hysteresis and record transitions must be charged and reset per trace. Do not call per-record dispatch persistent representation migration.

**Permitted causal features:** L, x and descriptors actually computed from the current coefficients at decision time (e.g., K), with scan cost charged. No generator labels, future records, oracle timing, precomputed phase identity or validation-set outcomes. All arms get identical input semantics; fixed arms are not forced to scan for unused descriptors.

## 6. Proposed workloads and selectors — NOT FROZEN

**Proposed bounded primary study:** one platform, one deterministic trace with 1024 records; lengths from {257, 1024, 4095}, density levels from {0, .02, .10, .40, .90, 1}, and evaluation points including 0,1,2 and random odd/even values. Include stationary, alternating, near-crossover and unpredictable mixed regimes without assuming any format crossover exists. The final trace composition, PRNG algorithm, seeds, density rounding, sign distribution, x distribution, record ordering and digest **must be pinned before protocol acceptance**. This paragraph is a design space, not a frozen confirmatory sample or permission to optimize the trace against results.

**Fixed comparators:** each competent format/algorithm pair on the *entire* trace, with identical lifecycle. Report every fixed arm, and compare to the best fixed expected complete-trace time using prespecified selection or simultaneous inference; do not cherry-pick a favorable fixed arm.

**Causal selectors:** (a) predeclared density/length threshold heuristic, (b) deterministic analytical cost-model selector using only causal features and a frozen calibration set, (c) optional policy with hysteresis if it has a justified stateful benefit. Define thresholds, ties, feature scans, calibration split and resets before measurement. A hindsight oracle is noncausal diagnostic context only. No learned or AI selector is proposed or authorized.

**Holdout:** A separate, preregistered confirmation trace or strict holdout design is necessary if any thresholds, kernels or format choices are tuned. Do not report training/calibration results as confirmatory evidence.

## 7. Measurement and inference — specification still required

**Primary metric:** elapsed wall-clock time per complete trace, including producer and consumer, allocations, metadata/feature extraction, storage, precomputation, conversion, selector decisions, kernel execution and ordered output. Report storage bytes and memory residency separately; do not combine them into an invented scalar objective.

**Proposed primary effect:** `I=1 - μ_adaptive / μ_best_fixed`, where μ denotes expected complete-trace time on the prespecified platform/trace and `best_fixed` is the fastest true fixed-arm mean. A 10% practical improvement threshold in earlier PRs and 15% in Jules's review are **both unapproved**. Maintainer must justify and select a threshold before confirmation. Any incremental claim for a sophisticated selector additionally requires a preregistered comparison with the strongest simple causal heuristic.

**Decision rule:** With a prespecified multiplicity-aware confidence interval on the comparative effect, SUPPORT only if its lower bound exceeds the accepted practical threshold; BOUNDED NEGATIVE only if its upper bound falls below that threshold with adequate power and valid measurement; otherwise INCONCLUSIVE. Correctness/comparator/provenance failure is INVALID, not negative. A result conditional on one fixed trace cannot be generalized to all workload distributions.

**Unresolved preregistration requirements:** CPU model, ISA, compiler/version/flags, OS, timer and overhead, warmups, number of paired run blocks, block/session independence, balanced randomized arm order, sample size/power assumptions, noise and exclusion policy, exact estimator and CI algorithm, resampling unit and seed, multiple-comparison adjustment, fixed-arm selection treatment, raw-run schema, trace digest and stopping rule. These are **approval blockers**, not optional execution details. A variance pilot, if needed, requires separate authorization and cannot contaminate the confirmation trace.

## 8. Reconciliation of PR #673's seven P1 blockers

| Audit finding | Revision response | Gate |
| --- | --- | --- |
| P1-01 degenerate sum/count and bypassable decoder | Replace with polynomial evaluation; freeze producer/consumer and prohibit hidden precomputed outputs | **Open:** verify nondegeneracy and observable consumer dependency |
| P1-02 tailored trace/no demonstrated crossover | Broaden proposed regimes and x values; do not assert crossovers; define dispatch versus migration | **Open:** freeze trace and analytical justification |
| P1-03 wire grammar/size limits | Explicit byte grammar, padding, sparse indices, tags, checked limits | **Open:** independent formal grammar and boundary-case review |
| P1-04 best-fixed/heuristic fairness | Factor algorithm and representation; competent fixed arms, causal heuristic and cost-model controls | **Open:** pin implementations and policies |
| P1-05 no ternary-specific attribution | Explicitly narrow claim to adaptive software routing; matched binary-coded controls | **Open:** attribution and comparison design review |
| P1-06 incomplete statistics | Define estimand, valid decision categories, holdout and required statistical checklist | **Open:** numerical procedure and environment |
| P1-07 threshold/upstream authority | Treat 10%/15% as unapproved; preserve M1 and B/C acceptance dependencies | **Open:** maintainer decision and explicit commit-scoped authorization |

No P1 is marked resolved solely by authoring this document. A reviewer must validate each correction and accept the completed protocol.

## 9. Explicit gates and stopping conditions

- **G0 — Continuity:** Reconcile M1 #665, scientific review #666, and reconciliation #667. No M1 performance claims.
- **G1 — Research selection:** Maintainer formally decides B versus C and whether the polynomial redesign is worth the resource cost. PR #671 is a recommendation, not an authorization.
- **G2 — Protocol acceptance:** Freeze operation, grammar, algorithms, trace, selectors, comparator fairness, statistical method, platform and commit SHA; obtain independent technical review.
- **G3 — Explicit execution authority:** Separate written approval specifying allowed correctness implementation and whether any benchmark may run, with resource cap. A merge, CI success or AI recommendation is not approval.
- **Stop/INVALID:** Semantics mismatch, noncanonical format accepted, unobservable retained-byte dependency, comparator inequivalence, unregistered tuning, or missing authorization. Preserve negative and inconclusive evidence.

**Final disposition: `M2_B_POLYNOMIAL_PROTOCOL_REVISION_DRAFT — SEVEN_P1_GATES_OPEN — NO_EXECUTION_AUTHORIZED`.**
