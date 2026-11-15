# T81-AIOPT-001 M2 — Adversarial Research Design & Premise Assessment

**Document ID:** `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md`
**Date:** 2026-11-01
**Role:** Independent Experimental-Design Critic
**Scope:** Critical evaluation of the M2 research premise, adjudication of ternary computational advantage, proposal of three competing experimental protocols, comparative ranking, and recommendation of the minimum viable experiment.
**Authority / Status:** CRITICAL DESIGN ASSESSMENT / NON-NORMATIVE RESEARCH EVALUATION (No execution, benchmarking, or code mutation authorized; Candidate C is an unaccepted candidate design; M2 execution remains blocked).

---

## 1. Executive Summary & Core Verdict

The T81-AIOPT-001 research track was initiated to investigate whether ternary representation structures provide measurable performance or efficiency benefits when processed by adaptive AI runtimes. Following the verification of Milestone M1 (PR #665), independent scientific review (PR #666), and reconciliation gating (PR #667), a draft protocol for Milestone M2 was proposed (PR #668).

This assessment delivers an adversarial critique of the foundational research question:
> **"Is wildcard ternary pattern matching actually a suitable experiment for establishing a ternary-specific computational advantage?"**

### Primary Finding: Premise Invalidation

**No.** Wildcard ternary pattern matching over linear arrays on general-purpose binary CPUs **cannot establish a ternary-specific computational advantage**.

The central vulnerability of the proposed M2 protocol is a category error: it conflates **representation density** (the physical space required to store 3-state or 4-state symbols) with **ternary computation** (algorithmic or operational properties unique to base-3 algebra). On binary architectures:
1. **Bit-Sliced Binary Baseline Competitiveness:** Any wildcard ternary matching rule over $\{-1, 0, +1, *\}$ can be mapped into two parallel binary bitplanes (e.g., `pos_mask` and `neg_mask`, or `value_bits` and `care_mask`). In bit-sliced format, $N$ patterns can be checked across $W$ vector lanes using bitwise ALU operations (`AND`, `XOR`, `TEST`) per 64-bit word or 512-bit vector register. While bit-sliced layouts require catalog transposition and output materialization overhead, packed 2-bit ternary SWAR structures require serial unpacking or complex multi-instruction bit-manipulation shuffles (`vpshufb`, shifts, masks) that face structural instruction-tax hurdles on general binary ALUs.
2. **Indexing and Preprocessing Tradeoffs:** In real-world pattern matching, static or infrequently updated catalogs are rarely scanned linearly at $O(M)$ cost. Depending on pattern correlation, wildcard density, and memory budgets, inverted indices, decision trees, or tries can narrow candidate sets significantly, although arbitrary wildcard patterns with high multi-hit rates introduce state-space and preprocessing overheads that must be explicitly accounted for.
3. **Absence of Ternary-Native Semantics:** Wildcard pattern filtering does not exercise balanced ternary algebraic properties (such as sign symmetry, carry-free addition, or exact symmetric rounding). It is fundamentally 4-state symbol equality with a "don't care" mask.

### Strategic Recommendation

Milestone M2 should be **substantially redesigned and redirected**. Rather than executing synthetic microbenchmarks on linear wildcard filtering, the project should evaluate **Candidate C: Ternary-Quantized Matrix-Vector Inner Product with Zero-Skipping (T-GEMV)** as the most scientifically decisive candidate for future maintainer consideration, while retaining **Candidate B: Adaptive Representation Selection** as a secondary algorithmic investigation for memory-constrained regimes. **Candidate C is not yet approved as an accepted successor workload; M2 execution remains formally blocked.**

---

## 2. Source and Evidence Ledger

To ground this critique in verifiable project history rather than conjecture, the table below inventories the relevant prior art across PRs #665–#668, H04-R02, the CPU ternary closure (PRs #544–#549), and the Hanoi/CWEB lineage.

| Artifact / Milestone | Pinned Identifier / Head | Evidentiary Status | Key Findings & Validated Boundaries |
| :--- | :--- | :--- | :--- |
| **M1 Protocol & Implementation** | PR #665 (`e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db`) | Verified Correctness (Tier C local evidence) | Evaluates 4-trit vectors against 40 fixed patterns ($C_{40}$). Agreement verified across 81 inputs (3,240 decisions). Proves functional isomorphism of byte, integer, and SWAR encodings. Does **not** measure performance or lane-level SIMD arithmetic. |
| **M1 Scientific Review** | PR #666 (`b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe`) | Analytical Review / Audit | Assigned disposition `M2_REDESIGN_REQUIRED`. Proved $C_{40}$ is mathematically degenerate: Lane 0 contains zero positive trits; patterns are mutually exclusive; problem reduces to an $O(1)$ scalar range check (`index < 40`). |
| **M1/M2 Reconciliation Gate** | PR #667 (`1edf70bbffc9168c27be5c6c19cf97388f6991a4`) | Governance Policy Gate | Enforced separation between M1 correctness and M2 authorization. Bounded M2 to design-only; prohibited execution without explicit maintainer preregistration. Mandated fair binary baselines and complete cost accounting. |
| **Proposed M2 Protocol Draft** | PR #668 (`86838191420ecc7ccdca02e41f4e009c6375417a`) | Proposed Protocol (Unexecuted) | Proposes grid: $L \in \{16, 64, 256, 1024\}$, $M \in \{16, 256, 4096\}$, $D \in \{0.25, 0.50, 0.90, 1.00\}$. Suggests SWAR 2-bit vs INT8/bitset comparators. Premised on finding a 10% end-to-end advantage for packed ternary. |
| **H04-R02 Adaptive Representation** | `experiments/h04_r02_adaptive_representation/` | Reclassified Audit (Classification D) | Corrected in `CORRECTED_RESULTS.md`: Original universal ternary advantage claims falsified due to metadata scaling errors. Under 10-bit Tier-0 control on `boundary_oscillation`, symbolic model yields $AT = 1210 < AB = 1250$ at $r = 1.60$ with crossover $5/3$. Proves conditional mathematical advantage under specific cost models, but does not demonstrate physical CPU speedup. |
| **CPU Ternary Closure** | PRs #544–#549 (`docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`) | Empirical Benchmark & Decision Record | Rigorous CPU kernel optimization frozen. Proved binary 64-bit ALU is $10.6\times$ faster than software Radix-81 tryte addition ($1.1\text{ ns}$ vs $11.7\text{ ns}$). Packed 2-bit AVX2 MatVec reached $75.3\%$ of INT8 AVX2 speed; decode instruction tax on binary ALUs offsets bandwidth savings. |
| **Hanoi / CWEB Lineage** | `experiments/balanced_ternary_conversion/hanoi-source.cweb` | Historical Ancestry | Original literate CWEB source claimed kernel-level ternary coprocessor, AI optimization, and GPU acceleration. Audit revealed aspirational claims without reproducible physical execution. Serves as a historical baseline of unverified narrative claims now replaced by rigorous empirical standards. |

---

## 3. Adversarial Critique of the Existing M2 Premise

The proposed M2 protocol asks: *"Under identical wildcard ternary matching semantics, can a packed ternary implementation achieve a practically meaningful end-to-end advantage over the strongest correct conventional binary implementation?"*

We challenge the scientific and technical validity of this premise across six critical dimensions.

### 3.1 Binary Bitset and Bit-Sliced Competitiveness

The proposal assumes that because ternary trits have 3 states (or 4 states with wildcard: $\{-1, 0, +1, *\}$), a 2-bit packed ternary representation is structurally suited to the problem.

This assumption must be contrasted against bit-sliced binary designs. Consider an alphabet of 4 symbols:
- $N$ (Negative / -1)
- $Z$ (Zero / 0)
- $P$ (Positive / +1)
- $W$ (Wildcard / *)

In a **bit-sliced binary representation**, a catalog of $M$ patterns of length $L$ can be stored as transposed binary bit vectors per pattern position $i \in [0, L-1]$:
1. `is_pos[i]`: Bitmask of patterns requiring $+1$ at position $i$.
2. `is_neg[i]`: Bitmask of patterns requiring $-1$ at position $i$.
3. `is_zero[i]`: Bitmask of patterns requiring $0$ at position $i$.

Because $W$ matches anything, a pattern with a wildcard at position $i$ sets all three state bits to 0 (unconstrained).

When an input vector $x$ arrives:
- For each position $i$, depending on whether $x[i]$ is $+1, -1,$ or $0$, we can filter candidate patterns using bitwise operations:
  $$\text{match\_mask} \gets \text{match\_mask} \ \& \ (\sim\text{req\_mask}[i] \ | \ \text{match\_bits}(x[i]))$$
On modern 64-bit binary ALUs, a bit-sliced binary filter evaluates up to 64 patterns in parallel per word operation. On AVX-512, it evaluates up to 512 patterns per vector instruction.

**Bounded Caveat:** Bit-sliced matching is not an unconstrained panacea. It requires upfront transposition of the catalog into bitplane format, and when multiple matches occur, reconstructing the list of matching indices from a bitmask requires bit-scan / population count loops that incur material output materialization cost. However, in terms of core comparison filtering, packed 2-bit SWAR operates *across lanes within a single vector*, requiring shifts, masks, and horizontal reductions, whereas bit-sliced binary operates *across patterns across vectors*, presenting a highly formidable conventional baseline.

### 3.2 Representation Density vs. Ternary Computation

Any potential speedup observed in a naive packed ternary scan arises strictly from **data compression / memory bandwidth**, not **ternary computation**:
- If a packed 2-bit representation outperforms a 1-byte-per-trit representation, the speedup is entirely due to reducing memory footprint from 8 bits to 2 bits (a $4\times$ cacheline density improvement).
- However, binary bitsets also achieve 2 bits per trit (or fewer when entropy/wildcard compression is applied).
- Confusing memory footprint reduction with "ternary computational advantage" repeats the methodological error identified during the CPU Ternary Research Closure (PRs #544–#549), where 2-bit packed MatVec appeared competitive only when DRAM bandwidth was saturated, while incurring severe compute instruction penalties.

### 3.3 Indexing, Preprocessing, and Algorithmic Realism

In real-world applications (network packet classification, rule-based inference, firewall filtering, symbol resolution):
- The catalog $M$ is known in advance or updated asynchronously.
- Evaluating a query does not necessarily require an exhaustive $O(M)$ linear scan over the entire catalog.
- Instead, catalogs can be preprocessed into inverted indices, decision trees, or tries to narrow candidates.
- **Complexity Boundaries:** For general wildcard catalogs with high wildcard density ($D > 0.5$) and frequent overlapping matches, tries and decision trees can suffer exponential state explosion ($O(2^L)$ or $O(3^L)$ states), making linear bit-sliced scanning preferable. Conversely, for structured or low-wildcard catalogs, indexing dramatically reduces the number of candidate evaluations.
- In either regime, testing purely unindexed linear scans over $M \in \{16, 256, 4096\}$ risks benchmarking a strawman implementation unless candidate filtering and complete preprocessing cost accounting are strictly evaluated.

### 3.4 Workload Non-Representativeness

The proposed wildcard matching problem has no documented anchor in modern production AI workloads or the T81 runtime:
- **Ternary Neural Networks (TNNs / BitNet b1.58):** Weights are ternary ($W \in \{-1, 0, +1\}$), but activations are continuous (FP16/BF16/INT8). The core workload is **multiply-accumulate (GEMM/GEMV)**, not pattern filtering.
- **Axion Policy Engine:** Authorization policies evaluate capability tokens, cryptographic signatures, and monotonic epoch counters (RFC-0028/0030), not dense ternary strings with wildcards.
- Testing wildcard matching over random uniform trits evaluates an artificial synthetic construct that does not advance T81's core architecture.

### 3.5 Negative Outcome Utility

If M2 is executed as proposed:
- **Expected Result:** Binary bit-sliced SIMD and inverted indices will outperform packed ternary SWAR across virtually all non-degenerate parameter cells ($L \ge 16$, $M \ge 256$).
- **Scientific Utility of Negative Result:** Extremely low. We already proved in PRs #544–#549 that software-emulated ternary bit manipulation on binary CPUs incurs heavy instruction penalties. Re-proving that binary ALUs excel at binary bitwise logic while struggling with emulated ternary packing yields zero novel insight.

---

## 4. Specification of Three Competing Experimental Candidates

To establish a sound path forward, we define three bounded, falsifiable candidate experimental protocols.

```
                              ┌─────────────────────────────────────────────────────────────┐
                              │           T81-AIOPT RESEARCH CANDIDATES                     │
                              └─────────────────────────────────────────────────────────────┘
                                                             │
                  ┌──────────────────────────────────────────┼──────────────────────────────────────────┐
                  ▼                                          ▼                                          ▼
   ┌─────────────────────────────┐            ┌─────────────────────────────┐            ┌─────────────────────────────┐
   │         Candidate A         │            │         Candidate B         │            │         Candidate C         │
   │    Parameterized Wildcard   │            │   Adaptive Representation   │            │    Ternary-Native Workload  │
   │      Pattern Filtering      │            │          Selection          │            │  (Signed GEMV Zero-Skipping)│
   └─────────────────────────────┘            └─────────────────────────────┘            └─────────────────────────────┘
   • Redesigned M2 proposal                   • Workload-driven representation           • TNN BitNet b1.58 linear layer
   • Bit-sliced binary baselines              • Non-AI heuristic selector                • Branchless sign accumulation
   • Inverted-index control                   • Evaluates selection overhead             • Tests algebraic ternary symmetry
```

---

### Candidate A: Parameterized Wildcard Pattern Filtering (Redesigned M2)

#### 1. Scientific Hypothesis
On general-purpose x86_64/ARM64 binary processors, a word-parallel 2-bit packed ternary SWAR comparator cannot achieve higher throughput or lower latency than an optimized bit-sliced binary comparator when matching arbitrary ternary query vectors against a wildcard catalog under equal semantics.

#### 2. Appropriate Binary Baselines
1. **Scalar Byte Baseline:** Reference loop comparing `int8_t` values with explicit branch checks.
2. **Bit-Sliced Binary Baseline:** Three per-position requirement bitplanes (`is_neg`, `is_zero`, `is_pos`) with one bit per catalog pattern. All three zero represents wildcard. For input state q, the mismatch bits are `(is_neg | is_zero | is_pos) & ~is_q`; intersect their complement with the current candidate bitset. Include three-plane storage, transposition and multi-match output costs. Any two-plane alternative requires an explicit four-state encoding and verified equivalent predicate.
3. **Inverted Index / Pre-Filtered Baseline:** Per-position sparse index filtering candidates before full evaluation.
4. **SIMD Byte Vectorized Baseline:** AVX2/NEON vector compare using `_mm256_cmpeq_epi8` with wildcard blend.

#### 3. Expected Sources of Overhead
- In packed 2-bit SWAR: Shifting, masking, invalid code checks ($10_2$), and horizontal reduction to extract boolean matches.
- In bit-sliced binary: Transposition overhead during catalog initialization and output bit-index extraction (must be accounted for separately).
- Memory bandwidth: Memory bus saturation at $M = 4096, L = 1024$.

#### 4. Required Correctness Evidence
- 100% agreement across all comparators against an independent scalar oracle for all match sets $\mathcal{M}(x) \subseteq \{0, \dots, M-1\}$.
- Explicit validation tests rejecting invalid trit codes and malformed vector lengths.
- Zero false positives and zero false negatives across empty, single-hit, multi-hit, and all-wildcard catalogs.

#### 5. Minimum Meaningful Experimental Scope
- Vector lengths: $L \in \{16, 64\}$.
- Catalog sizes: $M \in \{64, 512\}$.
- Wildcard densities: $D \in \{0.10, 0.50\}$.
- Batch sizes: $B \in \{1, 64\}$.
- Total parameter space: 16 core cells (narrowed from PR #668's 576 cells).

#### 6. Failure and Inconclusive Criteria
- **Falsification of H0 (Ternary Advantage Observed):** Packed SWAR achieves $>10\%$ lower end-to-end latency than the fastest binary baseline across $\ge 75\%$ of tested cells with $99\%$ bootstrap confidence.
- **Validation of H0 (Binary Superiority):** Bit-sliced binary outperforms packed SWAR by $\ge 20\%$ across all non-degenerate cells ($L \ge 16$).
- **Inconclusive:** Measurement variance exceeds effect size, or results reverse between x86_64 and ARM64 architectures.

#### 7. Resource Requirements
- Pure software C++20 implementation.
- Standard GitHub Actions Linux/macOS runners.
- Execution time: $< 2$ hours. Agent development effort: Low.

#### 8. Relationship to T81 Long-Term Objectives
Directly addresses PR #668; provides rigorous closure to the wildcard pattern filtering question, preventing ongoing investment in degenerate microbenchmarks.

---

### Candidate B: Adaptive Representation Selection (Workload-Driven Representation)

#### 1. Scientific Hypothesis
In dynamic, heterogeneous workloads exhibiting time-varying sparsity, vector length, and query batch size, a lightweight deterministic representation selector can achieve lower total latency (selection cost + execution cost) than any single static representation (Sparse Coordinate, Dense Byte, or Packed 2-bit).

#### 2. Appropriate Binary Baselines
1. **Best Static Baseline (Dense Byte):** Unconditionally execute all queries using standard byte arrays.
2. **Best Static Baseline (Packed 2-Bit):** Unconditionally execute all queries using packed 2-bit SWAR.
3. **Best Static Baseline (Sparse CSR/Index):** Unconditionally execute all queries using sparse non-zero index lists.
4. **Static Majority Oracle (Clairvoyant):** Offline optimal choice selected with zero runtime overhead.
5. **Non-AI Heuristic Selector:** Simple decision tree or lookup table based on batch size $B$ and measured non-zero sparsity $S = \text{nnz} / L$.

#### 3. Expected Sources of Overhead
- Metadata extraction: Counting non-zero trits or inspecting vector headers.
- Dispatch cost: Indirect function calls or branch mispredictions in the selector.
- Format conversion: Transcoding between representations when mismatch occurs.

#### 4. Required Correctness Evidence
- Complete functional equivalence: Every selected path produces bit-identical mathematical results.
- Selector determinism: Identical workload descriptors yield identical format selection.
- Comprehensive cost ledger: Selection decision time must be included in total execution latency.

#### 5. Minimum Meaningful Experimental Scope
- Workload dimension: $L \in \{32, 256\}$.
- Sparsity levels: $S \in \{0.05 \text{ (extreme sparse)}, 0.33 \text{ (balanced)}, 0.80 \text{ (dense)}\}$.
- Batch variability: Interleaved batches alternating between sparse and dense distributions.
- 6 discrete evaluation phases testing both static regimes and dynamic shifts.

#### 6. Failure and Inconclusive Criteria
- **Failure (Selector Unviable):** Total latency of the adaptive pipeline exceeds the best static baseline due to metadata inspection and dispatch overhead.
- **Success:** Adaptive selection achieves $\ge 15\%$ lower end-to-end latency across heterogeneous regimes than any single static representation, while beating simple static heuristics.
- **Inconclusive:** Adaptive selector wins only when format conversion cost is artificially excluded from the accounting model.

#### 7. Resource Requirements
- Software test harness in C++20.
- Synthetic workload generator with controllable Markovian distribution shifts.
- Standard CI runners. Development effort: Medium.

#### 8. Relationship to T81 Long-Term Objectives
Directly investigates the "AI-driven adaptive representation" premise of T81 without relying on opaque neural network selectors. Connects H04-R02 theoretical models to real software runtime dispatch.

---

### Candidate C: Ternary-Native Structural Workload (Ternary MatVec with Zero-Skipping)

#### 1. Scientific Hypothesis
In quantized matrix-vector inner products where matrix weights are ternary ($W \in \{-1, 0, +1\}$) and activation vectors are 8-bit integers ($X \in \mathbb{Z}^L$), exploiting the algebraic symmetry of balanced ternary via branchless sign-accumulation and zero-skipping provides higher arithmetic efficiency per byte of memory bandwidth than standard INT8 multiplication on general-purpose binary CPUs under memory-constrained or sparse conditions.

#### 2. Specific Delta Versus Prior CPU Experiments (PRs #544–#549)
Prior experiments in PRs #544–#549 established that:
- Radix-81 stack addition is $10.6\times$ slower than 64-bit binary ALU addition.
- AVX2 2-bit MatVec (`matvec_ternary_2bit_avx2`) reaches $75.3\%$ of INT8 AVX2 speed when using `vpshufb` nibble-parallel lookup tables on dense matrices, because decode instructions negate bandwidth savings.

Candidate C introduces three distinct, uninvestigated scientific deltas:
1. **Dynamic Zero-Skipping:** Unlike PR #549's dense `vpshufb` table lookups which decoded all trits unconditionally, Candidate C evaluates whether high weight sparsity ($Z \ge 0.50$, typical of BitNet b1.58) allows branchless zero-bypassing, skipping both computation and memory traffic.
2. **Branchless Sign Accumulation:** PR #544 tested software tryte arithmetic with redundant digit canonicalization. Candidate C tests whether ternary signs ($+1, -1$) can be accumulated directly into INT32 accumulators using sign-extension masks (`P` adds, `N` subtracts, `Z` no-ops) without full multiplication instructions.
3. **Modern Microarchitectural Comparators:** Prior tests compared generic AVX2 against scalar loops. Candidate C explicitly benchmarks against modern AVX-512 VNNI (`vpdpbusd`) and ARM NEON dot-product (`sdot`) instructions with explicit memory-bandwidth saturation tracking.

#### 3. Appropriate Binary Baselines
1. **INT8 GEMV (BLAS / MKL / Eigen):** Native `gemv` using 8-bit integer multiply-accumulate (`vpdpbusd` / VNNI / NEON `sdot`).
2. **Binary BitNet GEMV:** Standard 2-bit unpacked dequantization into INT8 registers followed by vector MAC.
3. **Sparse CSR Binary GEMV:** Traditional sparse matrix encoding storing indices of non-zero entries.

#### 4. Expected Sources of Overhead
- Branch mispredictions if zero-skipping uses conditional jumps (requires branchless bitmasking).
- Register unpacking and lane alignment when extracting non-zero signs.
- Format conversion and transpose costs if matrix is not pre-packaged.

#### 5. Required Correctness Evidence
- Exact integer arithmetic equivalence: $\sum W_{ij} X_j$ must match the 64-bit scalar reference for every row across all tested inputs.
- Symmetric roundoff / truncation tracking: Validate that balanced ternary symmetric accumulation maintains bounded numerical error.
- Known-answer test vectors derived from real open-weights BitNet b1.58 models.

#### 6. Minimum Meaningful Experimental Scope
- Matrix dimensions: $M \times K \in \{256 \times 256, 1024 \times 1024, 2048 \times 2048\}$.
- Weight sparsity (fraction of zeros): $Z \in \{0.33 \text{ (uniform)}, 0.60 \text{ (high-sparsity BitNet)}\}$.
- Single-vector latency ($B = 1$) and batched throughput ($B = 32$).

#### 7. Bounded Failure and Inconclusive Criteria
- **Bounded Negative Finding:** The confidence interval for paired relative end-to-end improvement must exclude the preregistered practical-improvement threshold from above, with adequate power and measurement validity. A point estimate below threshold, failure to reject a null or non-overlap of separate latency intervals is insufficient. Conclusions remain restricted to the tested workload, implementation, comparator and microarchitecture.
- **Supported in Tested Scope:** The paired comparative-effect interval establishes the preregistered end-to-end improvement threshold with correct outputs and full cost accounting, on the tested shapes and verified memory-residency regime.
- **Inconclusive:** The comparative-effect interval spans the practical-improvement threshold, precision or power is inadequate, or platform differences lack a preregistered interpretation.

#### 8. Resource Requirements
- C++20 implementation with SIMD intrinsics (AVX2, AVX-512 VNNI, ARM NEON).
- Dedicated bare-metal x86_64 and ARM64 machines with CPU frequency pinning and core isolation.
- Development effort: Medium-High.

#### 9. Relationship to T81 Long-Term Objectives
Directly aligns with modern neural network inference (LLM quantization, BitNet b1.58). Connects T81's ternary foundations to real-world AI efficiency, providing decisive evidence on whether ternary representations have a legitimate software role in neural computation.

---

## 5. Comparative Decision Matrix & Candidate Ranking

Each candidate is evaluated across seven objective criteria. The weights assigned to each dimension represent subjective research governance priorities reflecting the project's emphasis on scientific rigor and actionable knowledge.

### Evaluation Criteria Definitions
1. **Scientific Information Gained (20%):** Degree to which the experiment produces generalizable, non-trivial knowledge about computation.
2. **Distinguishing Ternary-Specific Effects (20%):** Capability to isolate base-3 / balanced algebraic properties from generic bit-packing.
3. **Fairness of Comparison (15%):** Rigor with which binary baselines are allowed their optimal algorithmic formulations.
4. **Implementation Complexity (10%):** Architectural and engineering effort required to implement and verify the test harness.
5. **Computational & Agent Cost (10%):** Wall-clock time, CI resource consumption, and engineering cycles required.
6. **Reproducibility (15%):** Determinism, portability across platforms, and robustness against microarchitectural noise.
7. **Risk of Repeating Prior Art (10%):** Probability of duplicating answered questions from PRs #544–#549 or H04-R02.

### Decision Matrix

| Dimension (Weight) | Candidate A: Wildcard Pattern Filtering | Candidate B: Adaptive Representation Selection | Candidate C: Ternary MatVec with Zero-Skipping |
| :--- | :--- | :--- | :--- |
| **1. Information Gained (20%)** | **Poor (2/10):** Proves minor variations of bit-packing in linear scans. | **Moderate (6/10):** Produces useful data on dynamic dispatch overheads. | **High (9/10):** Resolves whether ternary structure accelerates modern quantized AI inference. |
| **2. Distinguish Ternary Effects (20%)** | **Very Poor (1/10):** Conflates 4-state symbol packing with ternary logic. | **Moderate (5/10):** Measures representation switching, mostly orthogonal to radix. | **High (9/10):** Directly exercises balanced ternary zero-neutrality ($\times 0$) and sign inversion. |
| **3. Fairness of Comparison (15%)** | **Moderate (5/10):** Hard to avoid penalizing binary with forced linear scans. | **Good (8/10):** Compares against best static and non-AI rule-based selectors. | **High (9/10):** Compares directly against production-grade INT8 VNNI / BLAS pipelines. |
| **4. Implementation Complexity (10%)** | **Low (8/10):** Simple bit manipulation loops in C++. | **Medium (6/10):** Requires multi-representation codec and selector engine. | **Medium (6/10):** Requires SIMD kernels and cacheline-aligned buffers. |
| **5. Resource & Agent Cost (10%)** | **Low (9/10):** Runs in minutes on standard CI. | **Moderate (7/10):** Requires multi-phase synthetic workloads. | **Moderate (7/10):** Requires dedicated hardware for PMU measurements. |
| **6. Reproducibility (15%)** | **High (8/10):** Deterministic random seeds and known-answer vectors. | **High (8/10):** Fully deterministic state-machine workloads. | **High (9/10):** Standardized matrix multiplication oracles. |
| **7. Prior Art Duplication Risk (10%)** | **Severe (2/10):** Duplicates PR #549 CPU freeze conclusions on SWAR tax. | **Low (7/10):** Extends H04-R02 from abstract cost models to execution code. | **Low (8/10):** Breaks past synthetic trytes to real-world quantized AI tensor ops. |
| **Weighted Score (100%)** | **4.25 / 10** | **6.65 / 10** | **8.45 / 10** |
| **Rank** | **Rank 3 (Reject)** | **Rank 2 (Viable Secondary)** | **Rank 1 (Recommended Candidate)** |

*Note: Dimension weights represent explicit research governance choices prioritizing architectural relevance and scientific discriminative power.*

---

## 6. Recommended Minimum Meaningful Experiment

### Recommendation: Pivot Away from Wildcard Pattern Filtering

We recommend **abandoning the wildcard pattern filtering track (M2 as currently drafted in PR #668)**. If maintainers authorize further research into software ternary representations, we recommend evaluating **Candidate C: Ternary Matrix-Vector Inner Product with Zero-Skipping**.

**Formal Research Governance Status:** Candidate C is a proposed alternative design submitted for independent review. **It is not an approved or accepted successor workload, and no benchmark or execution of Candidate C is authorized by this assessment.**

### Minimum Viable Experimental Specification for Candidate C

To ensure methodological fairness and prevent the errors of past benchmarks, any future protocol for Candidate C must adhere to the following controls:

1. **Workload Bounding:**
   - Single precision dot product: Matrix $W \in \{-1, 0, +1\}^{M \times K}$, Vector $X \in \{-128, \dots, +127\}^K$, Accumulator $Y \in \mathbb{Z}^{M}$.
   - Matrix shapes: One nominal cache-resident control ($M = 256, K = 256$) and one larger candidate ($M = 2048, K = 2048$). Do not presume the larger case is DRAM-bound: INT8 weights occupy 4 MiB and 2-bit packed weights about 1 MiB, potentially fitting in last-level cache. Inspect the target hardware cache hierarchy and verify memory traffic; if DRAM behavior is essential, preregister a larger hardware-relative working set or controlled streaming/cold-cache protocol. Otherwise report the actual cache-residency regime.
   - Weight sparsity: Exactly two fixed levels: $Z = 0.33$ (uniform random trits) and $Z = 0.60$ (sparse BitNet distribution).
2. **Comparators:**
   - **Baseline 1:** Reference unrolled scalar loop in C++.
   - **Baseline 2:** Optimized INT8 dot product (AVX2 / AVX-512 VNNI / ARM NEON) treating ternary weights as unpacked `int8_t` values.
   - **Ternary Candidate:** Packed 2-bit storage with branchless zero-skipping and sign-selection.
3. **Primary Metric & Full Cost Accounting:**
   - **Primary Metric:** Absolute end-to-end wall-clock latency per batch and amortized query throughput (queries/sec), measured with nanosecond precision under core isolation.
   - **Mandatory Cost Accounting:** Must explicitly include input quantization/conversion time, matrix transposition/repacking overhead amortized over a predeclared reuse count $R$, memory footprint in bytes, and output writeback.
   - **Secondary Diagnostic Metrics:** Hardware PMU counters (`instructions`, `cycles`, `L1-dcache-load-misses`, `LLC-load-misses`) via `perf_event_open`. 'Cycles per non-zero MAC' is strictly a diagnostic ratio, not the primary evaluation metric.
4. **Scope-Bounded Falsification Rule:**
   - Define paired relative end-to-end improvement as `1 - (ternary latency / best fair binary latency)`. Preregister the practical threshold (proposed 10%), sampling, confidence level and multiplicity correction. Support requires the effect interval's lower bound to exceed the threshold; a bounded negative requires its upper bound below the threshold with adequate power and valid measurement; otherwise report inconclusive. Separate latency intervals or failure to reject a null are insufficient. Restrict findings to the measured implementations, workloads, hardware and verified cache regime.
   - Under no circumstances shall a negative outcome on this bounded configuration be extrapolated to claim universal impossibility across all potential hardware architectures or untested algorithmic formulations.

---

## 7. Explicit Unresolved Assumptions & Threats to Validity

To preserve scientific rigor, we document the unresolved assumptions underlying this assessment:

1. **Hardware Emulation Asymmetry:** All evaluations assume execution on conventional binary microprocessors (x86_64 and ARM64). This assessment does **not** evaluate hypothetical native ternary silicon (e.g., carbon nanotube FETs or memristor ternary crossbars). Results must be explicitly scoped to *software-emulated ternary execution on binary ALUs*.
2. **Compiler Auto-Vectorization Artifacts:** Compilers (GCC 14/15, Clang 18) may auto-vectorize byte-level scalar loops into efficient SIMD sequences while failing to vectorize custom bit-manipulation loops. Benchmark measurements must inspect generated assembly to ensure that observed differences reflect data representation rather than compiler optimization limitations.
3. **PMU Noise on Virtualized Runners:** GitHub Actions and virtualized cloud VMs do not provide deterministic PMU hardware counter access. Confirmatory benchmarks require bare-metal execution with pinned CPU frequencies.
4. **Cacheline Boundary Striding:** In packed representations, accessing unaligned bits across byte or word boundaries introduces masking overhead. Memory layouts must enforce 64-byte cacheline alignment.
5. **Multi-Hit Output Materialization:** In wildcard matching problems with high hit counts, the cost of allocating and writing match lists can dwarf comparison time. Any benchmarking must separate matching predicate latency from output collection.

---

## 8. Separation of Evidence, Analysis, and Speculation

In compliance with T81 research standards, the statements in this report are categorized by epistemological status:

### Verified Empirical Evidence
- M1 functional equivalence across 81 four-trit inputs against catalog $C_{40}$ (PR #665).
- Mathematical degeneracy of catalog $C_{40}$ and reduction to $k < 40$ (PR #666).
- Software Radix-81 addition is $10.6\times$ slower than binary 64-bit ALU addition (PR #544).
- Packed 2-bit MatVec suffers decode instruction penalties that offset memory savings on binary CPUs (PR #549).
- H04-R02 original universal advantage claims were invalid due to scaling defects; conditional symbolic model results survive under specific parameters (Classification D audit).

### Analytical Findings
- Bit-sliced binary pattern matching evaluates up to 64 patterns simultaneously per 64-bit word, presenting a highly formidable baseline against packed 2-bit SWAR lane filtering.
- Linear pattern filtering on static catalogs is an unrepresentative benchmark that conflates representation efficiency with naive algorithmic choice.
- Evaluating $M \in \{16, \dots, 4096\}$ wildcard patterns does not exercise balanced ternary algebraic properties.

### Speculative Projections
- Whether dedicated hardware memristor crossbars could execute wildcard ternary matching faster than binary TCAMs (unsupported by physical empirical evidence; see Investigation 10A).
- Whether dynamic representation selection overhead can be fully amortized in real-time inference workloads.
- The precise break-even sparsity threshold where ternary zero-skipping outperforms INT8 VNNI on modern AVX-512 processors.

---

## 9. Conclusion

The proposed M2 experiment (PR #668) asks the wrong research question on the wrong workload. Attempting to demonstrate a ternary computational advantage via wildcard pattern filtering over linear arrays on binary CPUs will produce an easily anticipated negative result, repeating lessons already established in PRs #544–#549.

The project should formally close Milestone M1 as a successful functional verification exercise, reject the linear pattern-matching premise of M2, and submit **Candidate C: Ternary-Quantized Matrix-Vector Inner Product with Zero-Skipping** for independent maintainer review as the only candidate offering a genuine scientific delta. All execution of M2 remains formally blocked pending governance authorization.
