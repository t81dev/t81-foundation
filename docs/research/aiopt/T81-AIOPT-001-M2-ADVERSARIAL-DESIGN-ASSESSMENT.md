# T81-AIOPT-001 M2 — Adversarial Research Design & Premise Assessment

**Document ID:** `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md`
**Date:** 2026-11-01
**Role:** Independent Experimental-Design Critic
**Scope:** Critical evaluation of the M2 research premise, adjudication of ternary computational advantage, proposal of three competing experimental protocols, comparative ranking, and recommendation of the minimum viable experiment.
**Authority / Status:** CRITICAL DESIGN ASSESSMENT / NON-NORMATIVE RESEARCH EVALUATION (No execution, benchmarking, or code mutation authorized).

---

## 1. Executive Summary & Core Verdict

The T81-AIOPT-001 research track was initiated to investigate whether ternary representation structures provide measurable performance or efficiency benefits when processed by adaptive AI runtimes. Following the verification of Milestone M1 (PR #665), independent scientific review (PR #666), and reconciliation gating (PR #667), a draft protocol for Milestone M2 was proposed (PR #668).

This assessment delivers an adversarial critique of the foundational research question:
> **"Is wildcard ternary pattern matching actually a suitable experiment for establishing a ternary-specific computational advantage?"**

### Primary Finding: Premise Invalidation

**No.** Wildcard ternary pattern matching over linear arrays on general-purpose binary CPUs **cannot establish a ternary-specific computational advantage**.

The central vulnerability of the proposed M2 protocol is a category error: it conflates **representation density** (the physical space required to store 3-state or 4-state symbols) with **ternary computation** (algorithmic or operational properties unique to base-3 algebra). On binary architectures:
1. **Bit-Sliced Binary Dominance:** Any wildcard ternary matching rule over $\{-1, 0, +1, *\}$ can be mapped into two parallel binary bitplanes (e.g., `pos_mask` and `neg_mask`, or `value_bits` and `care_mask`). In bit-sliced format, $N$ patterns can be checked across $W$ vector lanes using 2–3 bitwise ALU operations (`AND`, `XOR`, `TEST`) per 64-bit word or 512-bit vector register. Packed 2-bit ternary SWAR structures require serial unpacking or complex multi-instruction bit-manipulation shuffles (`vpshufb`, shifts, masks) that consistently lag behind native bit-parallel binary logic.
2. **Algorithmic Collapse via Preprocessing:** In real-world pattern matching, static or infrequently updated catalogs are never scanned linearly at $O(M)$ cost. Trie indexing, Aho-Corasick automata, hash filtering, or inverted indices collapse matching latency to $O(L)$ or $O(1)$ amortized lookups, entirely eliminating the loop overhead that M2 seeks to measure.
3. **Absence of Ternary-Native Semantics:** Wildcard pattern filtering does not exercise balanced ternary algebraic properties (such as sign symmetry, carry-free addition, or exact symmetric rounding). It is merely 4-state symbol equality with a "don't care" mask.

### Strategic Recommendation

Milestone M2 must be **substantially redesigned and redirected**. Rather than executing synthetic microbenchmarks on linear wildcard filtering, the project should pursue **Candidate C: Ternary-Quantized Matrix-Vector Inner Product with Zero-Skipping (T-GEMV)** as the smallest scientifically decisive experiment, while retaining **Candidate B: Adaptive Representation Selection** as a secondary algorithmic investigation for memory-constrained regimes.

---

## 2. Source and Evidence Ledger

To ground this critique in verifiable project history rather than conjecture, the table below inventories the relevant prior art across PRs #665–#668, H04-R02, the CPU ternary closure (PRs #544–#549), and the Hanoi/CWEB lineage.

| Artifact / Milestone | Pinned Identifier / Head | Evidentiary Status | Key Findings & Validated Boundaries |
| :--- | :--- | :--- | :--- |
| **M1 Protocol & Implementation** | PR #665 (`e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db`) | Verified Correctness (Tier C local evidence) | Evaluates 4-trit vectors against 40 fixed patterns ($C_{40}$). Agreement verified across 81 inputs (3,240 decisions). Proves functional isomorphism of byte, integer, and SWAR encodings. Does **not** measure performance or lane-level SIMD arithmetic. |
| **M1 Scientific Review** | PR #666 (`b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe`) | Analytical Review / Audit | Assigned disposition `M2_REDESIGN_REQUIRED`. Proved $C_{40}$ is mathematically degenerate: Lane 0 contains zero positive trits; patterns are mutually exclusive; problem reduces to an $O(1)$ scalar range check (`index < 40`). |
| **M1/M2 Reconciliation Gate** | PR #667 (`1edf70bbffc9168c27be5c6c19cf97388f6991a4`) | Governance Policy Gate | Enforced separation between M1 correctness and M2 authorization. Bounded M2 to design-only; prohibited execution without explicit maintainer preregistration. Mandated fair binary baselines and complete cost accounting. |
| **Proposed M2 Protocol Draft** | PR #668 (`8683819docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md`) | Proposed Protocol (Unexecuted) | Proposes grid: $L \in \{16, 64, 256, 1024\}$, $M \in \{16, 256, 4096\}$, $D \in \{0.25, 0.50, 0.90, 1.00\}$. Suggests SWAR 2-bit vs INT8/bitset comparators. Premised on finding a 10% end-to-end advantage for packed ternary. |
| **H04-R02 Adaptive Representation** | `experiments/h04_r02_adaptive_representation/` | Reclassified Audit (Classification D) | Corrected in `CORRECTED_RESULTS.md`: Original universal ternary advantage claims falsified due to metadata scaling errors. Under 10-bit Tier-0 control on `boundary_oscillation`, symbolic model yields $AT = 1210 < AB = 1250$ at $r = 1.60$ with crossover $5/3$. Proves conditional mathematical advantage under specific cost models, but does not demonstrate physical CPU speedup. |
| **CPU Ternary Closure** | PRs #544–#549 (`docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`) | Empirical Benchmark & Decision Record | Rigorous CPU kernel optimization frozen. Proved binary 64-bit ALU is $10.6\times$ faster than software Radix-81 tryte addition ($1.1\text{ ns}$ vs $11.7\text{ ns}$). Packed 2-bit AVX2 MatVec reached $75.3\%$ of INT8 AVX2 speed; decode instruction tax on binary ALUs offsets bandwidth savings. |
| **Hanoi / CWEB Lineage** | `experiments/balanced_ternary_conversion/hanoi-source.cweb` | Historical Ancestry | Original literate CWEB source claimed kernel-level ternary coprocessor, AI optimization, and GPU acceleration. Audit revealed aspirational claims without reproducible physical execution. Serves as a historical baseline of unverified narrative claims now replaced by rigorous empirical standards. |

---

## 3. Adversarial Critique of the Existing M2 Premise

The proposed M2 protocol asks: *"Under identical wildcard ternary matching semantics, can a packed ternary implementation achieve a practically meaningful end-to-end advantage over the strongest correct conventional binary implementation?"*

We challenge the scientific and technical validity of this premise across six critical dimensions.

### 3.1 Binary Bitset and Bit-Sliced Superiority

The proposal assumes that because ternary trits have 3 states (or 4 states with wildcard: $\{-1, 0, +1, *\}$), a 2-bit packed ternary representation is structurally suited to the problem.

This assumption fails under basic bitwise algorithmic analysis. Consider an alphabet of 4 symbols:
- $N$ (Negative / -1)
- $Z$ (Zero / 0)
- $P$ (Positive / +1)
- $W$ (Wildcard / *)

In a **bit-sliced binary representation**, a catalog of $M$ patterns of length $L$ is transposed into two binary bit vectors per pattern position $i \in [0, L-1]$:
1. `is_pos[i]`: Bitmask of patterns requiring $+1$ at position $i$.
2. `is_neg[i]`: Bitmask of patterns requiring $-1$ at position $i$.
3. `is_zero[i]`: Bitmask of patterns requiring $0$ at position $i$.

Because $W$ matches anything, a pattern with a wildcard at position $i$ simply sets all three masks to 0.

When an input vector $x$ arrives:
- For each position $i$, depending on whether $x[i]$ is $+1, -1,$ or $0$, we select the corresponding mask and accumulate candidates with a single bitwise `AND`:
  $$\text{match\_mask} \gets \text{match\_mask} \ \& \ (\sim\text{req\_mask}[i] \ | \ \text{match\_bits}(x[i]))$$
- Alternatively, using standard 2-bit mask encoding:
  $$\text{mismatch} = (\text{input\_bits} \oplus \text{pattern\_bits}) \ \& \ \text{care\_mask}$$
On modern 64-bit binary ALUs, a bit-sliced binary filter evaluates **64 patterns simultaneously per instruction**. On AVX-512, it evaluates **512 patterns per instruction**.

In contrast, a packed 2-bit ternary format packs 4 trits per byte (or 32 trits per 64-bit word). Comparing a packed 2-bit vector requires shifting, masked equality tests, and horizontal folding to detect whether any lane mismatched. Packed ternary operates **across lanes within a single vector**, whereas bit-sliced binary operates **across patterns across vectors**. Bit-sliced binary matching achieves $O(L \cdot \lceil M/64 \rceil)$ instructions with zero branching and minimal memory accesses, completely outclassing packed ternary SWAR scans.

### 3.2 Representation Density vs. Ternary Computation

Any potential speedup observed in a naive packed ternary scan arises strictly from **data compression / memory bandwidth**, not **ternary computation**:
- If a packed 2-bit representation outperforms a 1-byte-per-trit representation, the speedup is entirely due to reducing memory footprint from 8 bits to 2 bits (a $4\times$ cacheline density improvement).
- However, binary bitsets also achieve 2 bits per trit (or fewer when entropy/wildcard compression is applied).
- Confusing memory footprint reduction with "ternary computational advantage" repeats the methodological error identified during the CPU Ternary Research Closure (PRs #544–#549), where 2-bit packed MatVec appeared competitive only when DRAM bandwidth was saturated, while incurring severe compute instruction penalties.

### 3.3 Algorithmic Collapse via Indexing and Preprocessing

In real-world applications (network packet classification, rule-based inference, firewall filtering, symbol resolution):
- The catalog $M$ is known in advance or updated asynchronously.
- Evaluating a query does **not** perform an $O(M)$ linear scan over the entire catalog.
- Instead, the catalog is preprocessed into:
  - **Inverted Indices:** Subsetting candidate patterns by checking non-wildcard positions.
  - **Decision Trees / Tries:** Branching on salient trit positions to terminate search in $O(L)$ or $O(\text{depth})$ steps.
  - **Lookup Tables (LUTs):** For small or dense prefixes, resolving matches in $O(1)$.

Measuring linear scan throughput over $M \in \{16, 256, 4096\}$ benchmarks an algorithm that no competent software engineer would deploy for static rule sets. Once an inverted index or trie is introduced, the inner loop is dominated by pointer chasing or sparse bitmask operations, where ternary packing provides no structural advantage.

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
2. **Bit-Sliced Binary Baseline:** Two 64-bit words per position (`care_mask`, `val_mask`), checking 64 patterns in parallel via bitwise operations.
3. **Inverted Index / Pre-Filtered Baseline:** Per-position sparse index filtering candidates before full evaluation.
4. **SIMD Byte Vectorized Baseline:** AVX2/NEON vector compare using `_mm256_cmpeq_epi8` with wildcard blend.

#### 3. Expected Sources of Overhead
- In packed 2-bit SWAR: Shifting, masking, invalid code checks ($10_2$), and horizontal reduction to extract boolean matches.
- In bit-sliced binary: Transposition overhead during catalog initialization (must be accounted for separately).
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
In quantized matrix-vector inner products where matrix weights are ternary ($W \in \{-1, 0, +1\}$) and activation vectors are 8-bit integers ($X \in \mathbb{Z}^L$), exploiting the algebraic symmetry of balanced ternary via branchless sign-accumulation and zero-skipping provides higher arithmetic efficiency per byte of memory bandwidth than standard INT8 multiplication on general-purpose binary CPUs.

#### 2. Appropriate Binary Baselines
1. **INT8 GEMV (BLAS / MKL / Eigen):** Native `gemv` using 8-bit integer multiply-accumulate (`vpdpbusd` / VNNI / NEON `sdot`).
2. **Binary BitNet GEMV:** Standard 2-bit unpacked dequantization into INT8 registers followed by vector MAC.
3. **Sparse CSR Binary GEMV:** Traditional sparse matrix encoding storing indices of non-zero entries.

#### 3. Expected Sources of Overhead
- Branch prediction penalties if zero-skipping uses conditional branches (must mandate branchless masking).
- Register unpacking and lane alignment when extracting non-zero signs.
- Memory bandwidth saturation on large matrices ($> 4096 \times 4096$).

#### 4. Required Correctness Evidence
- Exact integer arithmetic equivalence: $\sum W_{ij} X_j$ must match the 64-bit scalar reference for every row.
- Zero-drift guarantee: Balanced ternary accumulation guarantees zero bias when weights are mean-zero.
- Known-answer test vectors derived from real open-weights BitNet b1.58 models.

#### 5. Minimum Meaningful Experimental Scope
- Matrix dimensions: $M \times K \in \{256 \times 256, 1024 \times 1024, 4096 \times 4096\}$.
- Weight sparsity (fraction of zeros): $Z \in \{0.33 \text{ (uniform)}, 0.60 \text{ (high-sparsity BitNet)}\}$.
- Single-vector latency ($B = 1$) and batched throughput ($B = 32$).

#### 6. Failure and Inconclusive Criteria
- **Falsification (Binary VNNI Superiority):** Hardware INT8 dot-product instructions (`vpdpbusd` / `sdot`) process 4–16 multiply-accumulates in 1 cycle, making them faster than ternary zero-skipping even with $60\%$ zeros.
- **Ternary Advantage Supported:** Ternary zero-skipping achieves higher throughput than INT8 VNNI on memory-bound or sparse matrices by saving $>50\%$ memory bandwidth without compute decode stalls.
- **Inconclusive:** Results diverge between architectures supporting VNNI/DOTPROD and legacy architectures lacking integer dot-product extensions.

#### 7. Resource Requirements
- C++20 implementation with optional SIMD intrinsics (AVX2, AVX-512 VNNI, ARM NEON).
- Dedicated physical x86_64 and ARM64 machines with cycle-level PMU counters (`perf_event_open`).
- Development effort: Medium-High.

#### 8. Relationship to T81 Long-Term Objectives
Directly aligns with modern neural network inference (LLM quantization, BitNet b1.58). Connects T81's ternary foundations to real-world AI efficiency, providing decisive evidence on whether ternary representations have a legitimate software role in neural computation.

---

## 5. Comparative Decision Matrix & Candidate Ranking

Each candidate is evaluated across seven objective criteria established in Section 4 of the task directive.

### Evaluation Criteria Definitions
1. **Scientific Information Gained:** Degree to which the experiment produces generalizable, non-trivial knowledge about computation.
2. **Distinguishing Ternary-Specific Effects:** Capability to isolate base-3 / balanced algebraic properties from generic bit-packing.
3. **Fairness of Comparison:** Rigor with which binary baselines are allowed their optimal algorithmic formulations.
4. **Implementation Complexity:** Architectural and engineering effort required to implement and verify the test harness.
5. **Computational & Agent Cost:** Wall-clock time, CI resource consumption, and engineering cycles required.
6. **Reproducibility:** Determinism, portability across platforms, and robustness against microarchitectural noise.
7. **Risk of Repeating Prior Art:** Probability of duplicating answered questions from PRs #544–#549 or H04-R02.

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
| **Rank** | **Rank 3 (Reject)** | **Rank 2 (Viable Secondary)** | **Rank 1 (Recommended)** |

---

## 6. Recommended Minimum Meaningful Experiment

### Formal Recommendation: Adopt Candidate C (T-GEMV Zero-Skipping) as the Successor to M1

We recommend **abandoning the wildcard pattern filtering track (M2 as currently drafted in PR #668)** and replacing it with **Candidate C: Ternary Matrix-Vector Inner Product with Zero-Skipping**.

### Minimum Viable Experimental Specification

To prevent scope creep, the successor experiment must be bounded to the smallest scientifically decisive configuration:

1. **Workload Bounding:**
   - Single precision dot product: Matrix $W \in \{-1, 0, +1\}^{M \times K}$, Vector $X \in \{-128, \dots, +127\}^K$, Accumulator $Y \in \mathbb{Z}^{M}$.
   - Matrix dimension: Exactly one small cache-resident shape ($M = 256, K = 256$) and one memory-bound shape ($M = 2048, K = 2048$).
   - Weight distribution: Fixed ternary distribution with $50\%$ zeros, $25\%$ positive ones, $25\%$ negative ones.
2. **Comparators:**
   - **Baseline 1:** Reference unrolled scalar loop in C++.
   - **Baseline 2:** Native INT8 dot product (AVX2 / NEON) treating ternary weights as regular `int8_t` values.
   - **Ternary Candidate:** Packed 2-bit storage with branchless zero-skipping and sign-selection.
3. **Primary Metric:**
   - Execution cycles per non-zero multiply-accumulate (MAC), captured via hardware performance counters (`perf_event_open`).
4. **Falsification Threshold:**
   - If Packed Ternary fails to beat Native INT8 on the memory-bound matrix ($2048 \times 2048$), the hypothesis that software ternary representation provides computational advantages in CPU inference is formally falsified across the project.

---

## 7. Explicit Unresolved Assumptions & Threats to Validity

To preserve scientific rigor, we document the unresolved assumptions underlying this assessment:

1. **Hardware Emulation Asymmetry:** All evaluations assume execution on conventional binary microprocessors (x86_64 and ARM64). This assessment does **not** evaluate hypothetical native ternary silicon (e.g., carbon nanotube FETs or memristor ternary crossbars). Results must be explicitly scoped to *software-emulated ternary execution on binary ALUs*.
2. **Compiler Auto-Vectorization Artifacts:** Compilers (GCC 14/15, Clang 18) may auto-vectorize byte-level scalar loops into efficient SIMD sequences while failing to vectorize custom bit-manipulation loops. Benchmark measurements must inspect generated assembly to ensure that observed differences reflect data representation rather than compiler optimization limitations.
3. **PMU Noise on Virtualized Runners:** GitHub Actions and virtualized cloud VMs do not provide deterministic PMU hardware counter access. Confirmatory benchmarks require bare-metal execution with pinned CPU frequencies.
4. **Cacheline Boundary Striding:** In packed representations, accessing unaligned bits across byte or word boundaries introduces masking overhead. Memory layouts must enforce 64-byte cacheline alignment.

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
- Bit-sliced binary pattern matching evaluates 64 patterns simultaneously per word, structurally outperforming packed 2-bit SWAR lane filtering.
- Linear pattern filtering on static catalogs is an unrepresentative benchmark that conflates representation efficiency with naive algorithmic choice.
- Evaluating $M \in \{16, \dots, 4096\}$ wildcard patterns does not exercise balanced ternary algebraic properties.

### Speculative Projections
- Whether dedicated hardware memristor crossbars could execute wildcard ternary matching faster than binary TCAMs (unsupported by physical empirical evidence; see Investigation 10A).
- Whether an adaptive representation selector could amortize its dispatch overhead in real-time inference workloads.

---

## 9. Conclusion

The proposed M2 experiment (PR #668) asks the wrong research question on the wrong workload. Attempting to demonstrate a ternary computational advantage via wildcard pattern filtering over linear arrays on binary CPUs will produce an easily anticipated negative result, repeating lessons already established in PRs #544–#549.

The project should formally close Milestone M1 as a successful functional verification exercise, reject the linear pattern-matching premise of M2, and pivot research resources toward **Candidate C: Ternary-Quantized Matrix-Vector Inner Product with Zero-Skipping**, which directly engages the modern architectural frontier of efficient AI inference.
