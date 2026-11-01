# T81-AIOPT-001 — M2 Candidate B versus C Decision Gate

**Status:** PROPOSED / DESIGN-ONLY — NOT ACCEPTED; NO IMPLEMENTATION OR BENCHMARK AUTHORIZATION  
**Date:** 2026-11-01  
**Decision requested:** Select one discriminating successor protocol after acceptance review of M1 and its scientific assessment.  
**Sources:** [M1 PR #665](https://github.com/t81dev/t81-foundation/pull/665), [scientific review #666](https://github.com/t81dev/t81-foundation/pull/666), [reconciliation #667](https://github.com/t81dev/t81-foundation/pull/667), [M2 draft #668](https://github.com/t81dev/t81-foundation/pull/668), [adversarial assessment #669](https://github.com/t81dev/t81-foundation/pull/669), [H04-R02 results](../../../experiments/h04_r02_adaptive_representation/RESULTS.md). These references identify evidence and proposals; a merged design assessment is not approval to run an experiment.

## Decision in one paragraph

**Recommend Candidate B, adaptive representation selection, for the next *design review*, not for execution.** B asks a still-open question connected to H04-R02's corrected conditional *modeled* advantages: can deterministic workload-aware switching between equivalent representations beat the strongest fixed representation and a simple heuristic **after** observation, conversion, dispatch, and data movement are charged? Candidate C, ternary GEMV with zero-skipping, is relevant to quantized inference but overlaps historical packed-ternary MatVec work. C is retained as an alternative only if a pre-experiment novelty and hardware-regime audit identifies a genuinely new, testable distinction. Neither candidate has demonstrated an empirical advantage.

## Comparative decision

| Criterion | B — Adaptive representation selection | C — Ternary GEMV with zero-skipping |
| --- | --- | --- |
| Distinguishing question | When does workload heterogeneity justify changing representations? | Does signed/zero ternary structure improve a specified quantized dot-product workload? |
| Prior evidence | H04-R02 has **conditional symbolic** cost advantages, not CPU results | Earlier packed ternary AVX2 MatVec underperformed INT8 on its tested workload |
| Main confound | Selector can win from generic sparse/dense scheduling, not radix-specific algebra | Savings can come from generic sparsity/packing, not ternary-specific operations |
| Essential controls | Best fixed format; simple deterministic heuristic; oracle ceiling; same input/output semantics | Optimized INT8/VNNI or NEON; sparse binary; same weight values and output; cache-regime controls |
| Cost accounting | Descriptor extraction, selector, conversions, cache effects, execution, outputs, switching hysteresis | Packing amortization, index/sign storage, decoding, matrix reuse, output, memory traffic |
| Novelty over past T81 experiments | Bridges a symbolic cost model to measured *dynamic* execution | Must first show a clear difference from historical packed-ternary MatVec |
| Practical resource exposure | Bounded software-only pilot possible after approval | Credible DRAM claims require measured hardware and possibly bare-metal access |
| Primary risk | A trivial heuristic or best fixed baseline wins | Repeats prior negative CPU result without isolating a new mechanism |

**Preference rationale:** B has the clearer unresolved experimental question at lower initial resource cost. This is a research-priority judgment, not a measured ranking or claim that B will succeed. An observed B win over a fixed baseline would establish **adaptive strategy benefit**, not automatically a *ternary-specific* benefit.

## Proposed single M2 protocol — B, design for approval

### Hypothesis and estimand

For a preregistered sequence of heterogeneous vector workloads, a **deterministic, causal selector** of dense-byte, packed-two-bit and sparse-index formats achieves at least **10% lower total end-to-end latency** than the best *single fixed* format, while producing identical outputs. The 10% margin is **proposed**, not accepted.

The primary estimand is paired relative improvement `1 - (adaptive total latency / best fixed total latency)` on the complete sequence, including setup and all format conversions. A secondary comparison against the **best simple causal heuristic** determines whether the more elaborate selector provides incremental value. An offline oracle is an unattainable upper-bound reference, not a fair deployment baseline.

### Minimal discriminating design

1. **Freeze semantics:** Select one exact, nondegenerate operation already supported by the representation paths. All candidates must produce the same ordered outputs and error behavior. If an operation or its independent oracle is not yet specified, **protocol approval is blocked**; do not silently substitute wildcard matching or GEMV.
2. **Workloads:** Preregister short and long vectors, low and high nonzero density, and both stationary and alternating regimes. Include constant-regime controls in which switching should not help. Fix seeds, trace lengths, order, update frequency, and representative batch sizes *before* timing.
3. **Formats:** Dense INT8, packed 2-bit ternary, and sparse index/value encoding. Include at least one equally capable conventional binary-coded alternative wherever representation-specific attribution is claimed. No hypothetical ternary hardware assumptions.
4. **Selectors:** (a) one fixed format for the whole trace, each tested; (b) a simple fixed-threshold heuristic; (c) a deterministic adaptive policy using only information available **at decision time**; (d) an offline oracle, labeled noncausal and excluded from success claims. No AI-learned selector is implied or authorized.
5. **Accounting:** Time descriptor collection, conversions, storage changes, selector decisions, dispatch, computation, and output materialization. Specify cold/warm states and amortization over actual reuse; avoid free metadata or clairvoyant format choices.
6. **Correctness gate first:** Independent scalar oracle, full output equivalence, malformed input rejection, deterministic trace replay, and cross-format transition tests. No performance claims until correctness acceptance.
7. **Measurement gate:** Fix hardware, compiler, CPU features, timing method, run order/randomization, warmups, sample size, and primary trace aggregation in advance. Estimate a paired confidence interval for the primary improvement; adjust for multiple confirmatory traces if applicable. **Supported** only if the lower bound exceeds the preregistered practical threshold; **bounded negative** if the upper bound is below it with adequate power; otherwise **inconclusive**. Do not interpret non-significance as proof of no advantage.
8. **Attribution:** Report separate deltas versus the best fixed format and simple heuristic. Compare ternary-packed against an equivalent binary-coded representation; if only adaptive scheduling helps, label the finding representation-selection rather than ternary-specific.
9. **Stopping:** Complete a design review and preregistration before code or benchmarking. Stop on correctness failure, invalid comparator equivalence, unmeasurable cost, or missing authorization; preserve negative and inconclusive evidence.

### Explicit decision gates

- **Gate 0 — M1:** Independently reconcile the current PR #665 correctness record and PR #666 critique; record disposition without treating AI review as independent human replication.
- **Gate 1 — Selection:** Maintainer accepts or rejects B versus C in a separately recorded decision. This document is only a recommendation.
- **Gate 2 — Protocol:** Specify exact operation, generator, workload traces, baseline implementations, practical threshold, statistical procedure, hardware, and reproducibility commands. Obtain protocol review and explicit authorization.
- **Gate 3 — Execution:** Only after Gate 2 may implementation or measurements begin. No production runtime change, merge, or activation is implied.

## Why C is not discarded

C may be preferable if a scoped audit shows a novel **sparsity/zero-skipping mechanism** beyond prior AVX2 MatVec work, and a fair optimized binary/sparse comparator can be measured on hardware with verified cache/DRAM behavior. A 2048×2048 INT8 matrix is only 4 MiB; it must **not** be presumed DRAM-bound. A loss on one matrix shape cannot close all CPU ternary inference hypotheses. These cautions are preserved from PR #669.

## Disposition

**B_DESIGN_REVIEW_RECOMMENDED — EXECUTION_NOT_AUTHORIZED.**  
**C_RETAINED_AS_ALTERNATIVE — NOVELTY_AUDIT_REQUIRED.**  
**M2_NOT_APPROVED.**

Next authorized action proposed: one maintainer review of this decision gate and one tightly specified B protocol, with no implementation or benchmarking until explicit approval.


---

## Post-review reconciliation (2026-11-01; proposal only)

Codex's [seven-P1 protocol audit #673](https://github.com/t81dev/t81-foundation/pull/673) and Jules's [adversarial workload review #674](https://github.com/t81dev/t81-foundation/pull/674) agree that the original decode/reduce workload in PR #672 is not acceptable as a confirmatory M2 protocol. The original B preference was **for design review**, not a finding of scientific superiority. The prior minimal design above is superseded **as a proposed workload** by the revised polynomial-evaluation draft in [PR #672](https://github.com/t81dev/t81-foundation/pull/672), not by an accepted protocol.

The proposed polynomial operation is exact evaluation of coefficients in {-1,0,+1} modulo 2^64. Horner and sparse indexed evaluation must be treated as different algorithms; their performance contrast does not isolate ternary representation. The revised protocol therefore requires crossed/competent algorithm-and-format baselines, canonical wire contracts, observable producer/consumer lifecycle, causal selectors, held-out trace design, complete cost accounting, and a pinned statistical procedure. Neither the 10% nor the 15% improvement margin is approved.

**Updated decision status:** `B_POLYNOMIAL_DESIGN_REVIEW_RECOMMENDED`; `C_RETAINED_PENDING_NOVELTY_AUDIT`; `SEVEN_P1_GATES_OPEN`; `M2_NOT_APPROVED`. This does not merge, activate, implement or authorize measurements. Acceptance of #671, #672, #673 or #674 as documentation cannot substitute for separate written experimental authorization.


## Research-question separation decision proposal (2026-11-01)

**Disposition: `M2_RESEARCH_QUESTION_REFINEMENT_REQUIRED`.** The recommendation for Candidate B remains a recommendation for **design review only**, not an approval of the polynomial workload, a ternary-specific hypothesis, or implementation.

### Separate claims and evidence requirements

| Lane | Falsifiable question | Required comparator | Permitted conclusion |
| --- | --- | --- | --- |
| **S — adaptive software representation** | Does causal format selection improve complete end-to-end cost on a prespecified workload? | Best competent fixed format, simple causal heuristic, matched lifecycle, all selection/conversion costs | Bounded adaptive software benefit on the measured platform/trace; **not** radix superiority |
| **A — algorithm routing** | Does choosing among exact evaluation algorithms improve end-to-end cost? | Same canonical input/representation with fixed Horner, gap-Horner, sparse-power controls where applicable | Bounded algorithm-selection benefit; **not** format or ternary attribution |
| **T — ternary-specific attribution** | Does a genuinely ternary-specific encoding or arithmetic mechanism beat an equivalently optimized binary-coded mechanism at matched semantics/capacity? | Matched binary controls, explicit non-equivalent mechanism, identical algorithm and lifecycle | Only the narrowly isolated mechanism's advantage; two-bit trit storage alone cannot establish it |

**Primary long-horizon scientific objective:** T, but no currently proposed Candidate B protocol isolates T. S and A are supporting systems studies and must not be promoted to T evidence. No invented ternary-specific mechanism is authorized by this decision note.

### Decision before protocol freeze

1. Retain the polynomial workload in #672 as an **unaccepted candidate for S/A**, not as a mandatory M2 workload.
2. Require a written workload justification grounded in a plausible use case. If the producer must serialize, retain and later consume bytes, identify the real storage/transport boundary and reuse count; otherwise compare direct evaluation and do not impose artificial serialization to make adaptation attractive.
3. Require factorial or explicitly restricted comparisons separating algorithm from format. Where crossing is infeasible, label the result combined routing only.
4. A T-specific experiment requires a separate preregistered mechanism and attribution design; neither positive S nor A results close H04/H08.
5. The maintainer must explicitly choose whether M2 is a bounded S/A investigation or is deferred pending a defensible T-specific hypothesis. Until then, do not freeze trace/threshold or authorize correctness implementation or measurement.

**Authorization:** None. The seven P1 gates in #673 remain open; the corrective reviews #673/#674 are evidence, not approval. Preserve all earlier versions in Git history.
