# T81 — Ternary Advantage Decision Matrix

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 — Ternary Advantage Decision Matrix](#t81-—-ternary-advantage-decision-matrix)
  - [1. Decision](#1-decision)
  - [2. Evidence and interpretation rules](#2-evidence-and-interpretation-rules)
    - [Primary repository sources inspected](#primary-repository-sources-inspected)
  - [3. H01–H08 decision register](#3-h01–h08-decision-register)
  - [4. Candidate-level comparison](#4-candidate-level-comparison)
  - [5. H04 next-step specification: model audit only](#5-h04-next-step-specification-model-audit-only)
    - [Question](#question)
    - [Required arms](#required-arms)
    - [Accounting](#accounting)
    - [Adversarial workload families](#adversarial-workload-families)
    - [Sensitivity and falsification](#sensitivity-and-falsification)
    - [Avoid redundant work](#avoid-redundant-work)
  - [6. Experiment authorization decision](#6-experiment-authorization-decision)
    - [Minimal deliverable](#minimal-deliverable)
    - [Stopping criteria](#stopping-criteria)
  - [7. Parallel engineering status (not scientific evidence)](#7-parallel-engineering-status-not-scientific-evidence)
  - [8. Review checklist](#8-review-checklist)

<!-- T81-TOC:END -->


**Draft:** 2026-10-29  
**Proposed repository destination:** `docs/research/TERNARY_ADVANTAGE_DECISION_MATRIX.md`  
**Status:** Research decision draft; not a protocol amendment, measurement authorization, or completed experiment.

## 1. Decision

**Recommended next scientific task: an H04 adaptive-binary versus adaptive-ternary model audit and discriminating counterexample search, without new benchmark implementation.** CMP3 (comparison and decision logic) is already an active, separately governed experiment and remains measurement-blocked. The next task should test whether H04's modeled gains, if any, are due to radix three rather than adaptive tier selection or a favorable accounting convention.

**Do not authorize a new physical measurement or performance campaign from this document.** If the existing H04-R02 correction packet already contains all proposed comparisons, conduct a read-only gap analysis and stop rather than duplicate it.

## 2. Evidence and interpretation rules

Evidence labels: **MATH** = derivation under stated assumptions; **SOURCE** = inspected repository source or frozen protocol; **REPORTED** = historical test or author assertion not independently rerun in this drafting session; **MODEL** = computed cost under stipulated weights, not hardware performance; **OPEN** = untested or insufficiently specified.

A three-valued output is not automatically cheaper than binary encoding. Four trits fit within seven information bits but a byte-oriented base-81 symbol ordinarily consumes eight bits. The ideal storage inequality is a conditional mathematical statement, not a measured energy or area advantage. Hardware-specific performance claims require matched devices, complete peripheral overhead, reliability and error correction.

### Primary repository sources inspected

1. [`docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md`](https://github.com/t81dev/t81-foundation/blob/main/docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md) — H01–H08 definitions, lineage, evidence tiers, mission and baseline requirements; Git blob `c7932c0465610a2ec29b1ba8ca9cd35b96bca60a` at inspection.
2. [`docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`](https://github.com/t81dev/t81-foundation/blob/main/docs/research/TERNARY_ADVANTAGE_CANDIDATES.md) — five-category candidate selection and frozen CMP3 design. Treat its numeric historical performance claims as reported, and its rankings as a design choice, not a scientific finding.
3. [`docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md`](https://github.com/t81dev/t81-foundation/blob/main/docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md) — physical-device evidence assessment. No matched physical superiority claim is adopted here.
4. CMP3 post-merge audit supplied in this session — `CORRECTION REQUIRED; MEASUREMENT BLOCKED`; preservation defect and T1–T8 documentation overclaims. This is an audit report, not a new benchmark result.

All status statements below are **bounded assessments**, not a new complete independent audit of every H01–H08 artifact. Source claims and measurements must be promoted only by a separately reproducible audit.

## 3. H01–H08 decision register

| ID | Claim / proposed mechanism | Supporting basis | Contrary or limiting evidence | Status for next decision | Next discriminating step |
|---|---|---|---|---|---|
| **H01** | Native three-state storage or logic reduces normalized physical cost | **MATH:** each trit encodes log₂3 bits; conditional cell-cost threshold | **REPORTED:** Investigation 10A lacks qualifying matched characterization; sensing, noise and ECC may dominate | **OPEN / physical evaluation blocked** | Obtain matched binary/ternary device data including peripheral area, energy, latency and error rates; otherwise parameterize sensitivity only |
| **H02** | Balanced signed digits improve arithmetic/rounding cost | **MATH:** symmetric digits, sign inversion; signed-digit circuit design is possible | **REPORTED:** software emulation overhead and legacy Hanoi arithmetic correctness defects; binary native ALUs are highly optimized | **OPEN for native circuits; software claims constrained** | Compare exact semantics and equal-range optimized adders, including normalization and carry; do not infer native benefit from software |
| **H03** | Base-81 grouping improves storage, interfaces or execution | **MATH:** 81 = 3⁴ | Byte packing, conversion and unpacking overhead; TISC/Wasm density evidence reported unfavorable in a scoped workload | **OPEN / scoped negative evidence** | Compare four-trit grouping against equally compact binary limbs under same payload and operations |
| **H04** | Adaptive T81/T243/T729 tiers outperform fixed representations | **SOURCE/MODEL:** historical Hanoi scheduler; later four-arm H04-R02 corrected model | Adaptive binary can capture selection benefit; metadata, conversion and switching can reverse wins; hardware units uncalibrated | **PRIORITY: MODEL AUDIT, not new benchmark** | Counterexample and sensitivity analysis across fixed binary, fixed ternary, adaptive binary, adaptive ternary |
| **H05** | Three-state algebra enables cheaper primitive families | **MATH:** three-valued operations can be specified | Equivalent Boolean synthesis may be equally cheap or better; physical gate library unspecified | **OPEN** | Select one complete operation family, synthesize equivalent Boolean and ternary circuits with matched cost rules |
| **H06** | Ternary weights/activations improve inference | **SOURCE/REPORTED:** quantization/sparsity motivation and packed SIMD studies | Binary CPU unpacking and SIMD costs; accuracy, calibration and memory layout confounders | **OPEN for dedicated hardware; scoped software negatives** | Compare matched accuracy and sparsity with binary low-bit baselines, full conversion and memory overhead |
| **H07** | Representation-independent governed execution improves auditability | **SOURCE:** identity, provenance, receipt and authorization research | Does not imply radix superiority; independent clean-room reproduction incomplete | **Separate systems track** | Tamper/replay/cross-platform conformance audit with exact artifacts |
| **H08** | AI-assisted iterative research can generate reliable novel results | **SOURCE:** extensive research and correction history | Model agreement is not independent reproduction; repeated validation defects consume effort | **Methodology track** | Track claim provenance, independent reproduction, defect recurrence and cost per resolved uncertainty |

## 4. Candidate-level comparison

| Candidate | Mechanism | Best known comparator | Critical overhead | Cheapest useful next work | Stop / reject condition |
|---|---|---|---|---|---|
| Signed arithmetic | Balanced digits, symmetric carry/negation | Optimized binary integer / signed-digit adder at same numeric range | Encoding, carry resolution, normalization | Formal circuit-cost model | No advantage under any plausible matched gate library assumptions |
| Three-way decision (CMP3) | Direct trichotomy and dispatch | Optimized binary comparison/branch, branchless, jump-table variants | Branch prediction, dispatch, payload and instrumentation | **Finish existing CMP3 correction and gate review** | Do not run second CMP3 experiment while measurement authorization is blocked |
| Information density | Fewer symbols for same state count | Binary bit-packing plus complete periphery | Cell sensing, ECC, writes, conversion | H01 parameter sweep / source search | No matched device data or benefit disappears over plausible overhead ranges |
| Sparse/quantized computation | Zero-aware ternary weights | Binary 1/2/4-bit quantization and optimized INT8 at matched accuracy | Packing, decompression, zero detection, memory movement | Review prior SIMD results and specify hardware crossover | Claimed benefit depends on omitted costs or unmatched accuracy |
| State-transition logic | Native 3-state transition primitive | Binary two-bit state, tables, bitsets, branchless automata | Encoding and table access | Formal transition-table minimization | Equivalent Boolean synthesis matches or beats modeled cost |
| **Adaptive representation (H04)** | Workload-specific tiers and switching | **Adaptive binary**, plus fixed binary and fixed ternary | Metadata, classifier, conversion, switching | **Read-only corrected-model audit and adversarial workloads** | Ternary-specific delta disappears once adaptive binary and all overheads are included |

## 5. H04 next-step specification: model audit only

### Question

Does adaptive ternary have a strictly positive **radix-specific** net benefit relative to an equally adaptive binary implementation over a declared workload family and a transparent range of cost parameters?

### Required arms

- **FB:** fixed binary, best permissible fixed tier for the complete sequence.
- **FT:** fixed ternary, best permissible fixed tier for the complete sequence.
- **AB:** adaptive binary, globally optimal tier schedule with all transition costs.
- **AT:** adaptive ternary, globally optimal tier schedule with all transition costs.

Use the same logical workload, numeric domain, exact output semantics and comparable tier freedoms. If any arm is structurally infeasible, declare the restriction and assess whether the comparison still isolates radix.

### Accounting

For representation `r`, workload sequence `w₁…wₙ`, and tier schedule `s₁…sₙ`, define a fully disclosed model:

`C(r,s,w) = initial(r,s₁) + Σᵢ [compute(r,sᵢ,wᵢ) + metadata(r,sᵢ,wᵢ) + select(r,sᵢ,wᵢ)] + Σᵢ₌₂ⁿ transition(r,sᵢ₋₁,sᵢ,wᵢ)`.

Where relevant, explicitly include terminal conversion, state materialization, allocation, and amortization assumptions. Avoid double-counting metadata and selection. Solve the adaptive arms with global dynamic programming rather than greedy local choices. Record tie-breaking and exact rational or integer arithmetic wherever possible.

**Adaptation gain:** `C(FB) − C(AB)` (binary) and `C(FT) − C(AT)` (ternary).  
**Radix-specific comparison:** `C(AB) − C(AT)`. A positive value is a *modeled ternary advantage under the assumed cost model*, not a measured speedup.  
**Interaction diagnostic:** `[C(FT) − C(AT)] − [C(FB) − C(AB)]`, which asks whether ternary gains more from adaptation; this alone does not prove ternary is the better total design.

### Adversarial workload families

1. Constant small values (adaptation should have little value).
2. Constant large values (fixed optimal tier should be competitive).
3. Long phases (switches can amortize).
4. Rapid alternation (switching can dominate).
5. Values near tier boundaries (metadata/transition sensitivity).
6. Signed extremes and range boundaries (exactness and capacity).
7. Sparse large values (representation-size vs decoding trade-off).
8. Adversarial transitions constructed to make greedy scheduling suboptimal.

### Sensitivity and falsification

Sweep explicit ratios for metadata cost, selection cost, transition cost, conversion cost and ternary primitive cost relative to binary. Publish the region in parameter space where `AT < AB`, where the ordering reverses, and where they tie. Do not cherry-pick one favorable point. Identify counterexamples and include fixed/adaptive controls. A claim of general advantage is rejected if it relies on unrealistic zero-overhead assumptions or a disadvantaged binary baseline. A conditional advantage can remain a legitimate model finding, with its assumptions prominently displayed.

### Avoid redundant work

First inspect the corrected H04-R02 material associated with PR #572. Its reported four-arm dynamic-programming corrections, metadata and transition accounting, and eight crossover cases may already satisfy some or all of this specification. Produce a **gap table** before creating new calculations. Do not rerun existing experiments merely to change report wording.

## 6. Experiment authorization decision

**Decision: NO NEW BENCHMARK AUTHORIZED.** Authorize a read-only H04-R02 evidence/gap audit only. CMP3 continues on its separate corrective path. If the H04-R02 packet already provides a complete fair four-arm comparison and sensitivity analysis, the next scientific step is to determine whether its parameter ranges can be tied to plausible native-ternary hardware or whether the result should remain explicitly conditional.

### Minimal deliverable

`H04_MODEL_GAP_ASSESSMENT.md` with (1) exact commit and file identities, (2) verified four-arm definitions, (3) cost terms and units, (4) workload coverage, (5) crossovers and counterexamples, (6) gaps ranked by expected information gain, and (7) one decision: **MODEL SUFFICIENT / MODEL CORRECTION REQUIRED / HARDWARE PARAMETERS UNAVAILABLE**.

### Stopping criteria

Stop immediately if the relevant corrected model cannot be pinned, if baseline semantics are not equivalent, if cost parameters cannot be reconstructed, or if the work would duplicate the already-validated packet. Preserve negative findings. No code changes, new runtime features, new governance layer, or production measurement activation are justified by this draft.

## 7. Parallel engineering status (not scientific evidence)

- CMP3 PR #642 was merged, but its subsequent post-merge audit reported **CORRECTION REQUIRED — MEASUREMENT BLOCKED**. Jules is assigned a bounded correction for preserved patch bytes, T1–T8 readiness documentation and non-performance validation evidence. Do not treat a passing compiler test as a measured ternary advantage.
- Existing CI/PR work should be tracked independently and checked live before merge decisions. This document does not assert current checks for unrelated PRs.
- The proposed dashboard should index hypothesis IDs, evidence tier, source SHA, tested platform, model assumptions, experiment status, governance gates and next discriminating action. It should not compute a single 'percent scientifically proven' score.

## 8. Review checklist

- [ ] Exact H04-R02 source revision and documents pinned.
- [ ] All four arms use equal semantics and transparent tier freedoms.
- [ ] Adaptation benefit distinguished from radix-specific benefit.
- [ ] Metadata, conversion, selection and transitions accounted for.
- [ ] Globally optimal adaptive scheduling checked independently.
- [ ] Counterexamples and reversal regions included.
- [ ] Source claims, model outputs and physical measurements labeled separately.
- [ ] No measurement authorization implied.

**Bottom line:** Preserve the current CMP3 correction effort. Spend the next research increment on determining whether H04 contains a ternary-specific result that survives adaptive binary controls, and stop if the corrected model already resolves that question.
