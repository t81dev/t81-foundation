# T81-AIOPT-001 M2 — Adversarial Workload & Experimental Design Review

**Document ID:** `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-WORKLOAD-REVIEW.md`
**Date:** 2026-11-01
**Role:** Independent Adversarial Scientific Reviewer
**Context:** T81-AIOPT-001 Milestone M2 Protocol Review
**Lineage & Reviewed Sources:**
- PR #669: M2 Adversarial Research Design & Premise Assessment (`docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md`)
- PR #671: M2 Candidate B versus C Decision Gate (`docs/research/aiopt/T81-AIOPT-001-M2-B-VS-C-DECISION-GATE.md`)
- PR #672: Candidate B Preregistration Draft (`docs/research/aiopt/T81-AIOPT-001-M2-B-PREREGISTRATION-DRAFT.md`)
- Experiment H04-R02 Corrected Research (`experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md`)
- CPU Ternary Closure & Next-Hypothesis Synthesis (PRs #544–#549; `docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`)
- Historical Hanoi / CWEB Lineage (`experiments/balanced_ternary_conversion/hanoi-source.cweb`)
**Status:** ADVERSARIAL REVIEW DELIVERABLE / NON-NORMATIVE CRITIQUE — NO EXECUTION, IMPLEMENTATION, OR PRODUCTION CHANGES AUTHORIZED.

---

## 1. Executive Conclusion

### Primary Question
**Can the proposed Candidate B adaptive-representation experiment establish anything more meaningful than the observation that different data representations work better for different workloads?**

### Definitive Verdict: No
The Candidate B protocol proposed in PR #672 cannot establish any new, non-trivial scientific knowledge. Under rigorous analysis, the experiment reduces to a circular, degenerate demonstration of basic compression economics: if an artificial trace is explicitly assembled by concatenating blocks of extreme sparsity ($5\%$ nonzeros) and extreme density ($95\%$ nonzeros), a switching mechanism between a sparse container and a dense container will trivially outperform a single static format forced across both extremes.

This outcome is scientifically empty for four reasons:
1. **Workload Degeneracy:** The proposed compute operation—serializing a vector, decoding it, and computing `(sum, nonzero_count)`—has virtually zero operational intensity. It does not exercise computational execution, but merely measures memory write/read bandwidth and loop dispatch.
2. **Circular Reward Function:** The evaluation trace does not reflect any natural computational or physical phenomenon. It is an engineered step-function oscillating between $5\%$ and $95\%$ density specifically configured to guarantee that fixed baselines fail on opposite ends.
3. **Absence of Ternary Specificity:** The observed advantage in Candidate B is entirely an artifact of generic sparse-versus-dense scheduling. A standard binary sparse array (e.g., bitmask or coordinate list) paired with a dense byte array exhibits the exact same dynamic. There is zero balanced-ternary algebraic or structural contribution.
4. **Selector Inutility:** A trivial static threshold heuristic (e.g., `if (nonzeros / length < 0.125) choose sparse else choose packed`) or a deterministic cost function matches or exceeds an "adaptive" policy while avoiding runtime hysteresis, state-space tracking, or learned inference overhead.

### Recommendation Summary
**We recommend Path B: Retain Candidate B’s core inquiry into representation switching under complete cost accounting, but completely replace the degenerate workload.**

Rather than executing an artificial `encode -> decode -> (sum, count)` serialization loop or returning to unvetted GEMV microbenchmarks, Candidate B must be grounded in an exact, deterministic operation exhibiting meaningful arithmetic depth and genuine representation-dependent tradeoffs: **Sparse-Trit Polynomial Evaluation with Dynamic Horner/Index Routing** (or bounded 1D convolution over non-uniform representations).

---

## 2. Assessment of Scientific Novelty

To evaluate whether Candidate B provides new scientific information, we must delineate the four levels of scientific claims:

```
+--------------------------------------------------------------------------+
| 1. MATHEMATICAL POSSIBILITY (Proven in Abstract / Formal Theory)        |
|    Balanced ternary has unique representation; capacity bounds 3^k >= 2^b|
+------------------------------------+-------------------------------------+
                                     |
                                     v
+------------------------------------+-------------------------------------+
| 2. MODELED ADVANTAGE (Proved in Symbolic Cost Models)                    |
|    H04-R02: AT=1210 < AB=1250 at r=1.60 under Tier-0 10-bit model        |
|    Assumes abstract unit costs U without memory, instruction, or CPU tax |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+------------------------------------+-------------------------------------+
| 3. IMPLEMENTATION CORRECTNESS (Verified Codec & Semantics)               |
|    PR #665 (M1): Verified SWAR/byte/int functional equivalence          |
|    EXP-H04-R03: Matched codec deterministic round-trips & random access   |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+------------------------------------+-------------------------------------+
| 4. MEASURED CPU ADVANTAGE (Empirical Reality on Physical Silicon)       |
|    PRs #544-#549: Software ternary arithmetic slower than binary ALU     |
|    Candidate B (PR #672): Currently UNPROVEN & DEGENERATE                |
+--------------------------------------------------------------------------+
```

### What Candidate B Claims vs. What Prior Art Already Established

| Prior Art / Milestone | Established Knowledge | What Candidate B Claims to Add | Actual Scientific Delta in Candidate B |
| :--- | :--- | :--- | :--- |
| **H04-R02 Corrected** | Proved that in an abstract symbolic unit model ($U$), adaptive balanced ternary achieves a conditional crossover advantage against binary ($AT=1210 < AB=1250$) under boundary oscillation at $r=1.60$. Proved that universal ternary superiority is false. | "Bridges a symbolic cost model to measured dynamic execution." | **Zero novel ternary knowledge.** Moving from symbolic slots to wall-clock time on a CPU does not validate H04-R02 because the CPU is binary; any measured difference stems from SIMD instruction shuffles and memory layout, not base-3 optimality. |
| **Standard Adaptive Compression** | Well established for over 40 years (LZ, Huffman, Run-Length, Snappy, Zstd, column-oriented database encodings like Parquet/ORC). Switching formats between blocks based on metadata or density is textbook systems engineering. | "Exposes when dynamic format switching reduces complete end-to-end latency." | **Zero algorithmic novelty.** Proving that a sparse representation saves time at $5\%$ density while a dense representation wins at $95\%$ density is a known baseline fact of computer science. |
| **T81 CPU Ternary Closure (PRs #544–#549)** | Proved that binary 64-bit ALUs are $10.6\times$ faster than Radix-81 tryte arithmetic ($1.1\text{ ns}$ vs $11.7\text{ ns}$). Proved that packed 2-bit AVX2 SIMD MatVec runs at only $75.3\%$ the speed of INT8 AVX2 due to decode instruction overhead on binary ALUs. | "Tests adaptation without claiming ternary arithmetic superiority." | **Concession of defeat disguised as a new angle.** By retreating to vector reduction `(sum, count)`, it abandons computation and measures serialization overhead. |
| **Hanoi / CWEB Lineage** | Historically posited seamless kernel-level ternary computing, AI optimization, and custom accelerators without reproducible physical verification. | "Provides a rigorous, falsifiable preregistration draft." | **Methodological improvement only.** PR #672 is disciplined in its caveats, but rigorous execution of an uninformative premise still yields uninformative data. |

### Conclusion on Novelty
The experiment as drafted in PR #672 cannot discover any new scientific principle. It conflates the validation of a software state-machine with empirical proof of a computational advantage.

---

## 3. Analysis of Experimental Degeneracy

A scientific experiment is degenerate if its design guarantees a foregone conclusion, or if the observable metric is completely detached from the underlying phenomenon under investigation. PR #672 suffers from acute experimental degeneracy in three areas:

### 3.1 Workload Triviality and Lack of Computational Depth
The PR #672 protocol defines the timed operation as:
$$\text{Input } v_t \longrightarrow \text{Encode} \longrightarrow \text{Retain Bytes} \longrightarrow \text{Decode} \longrightarrow \text{Compute } (\sum v_t, \sum [v_t \ne 0])$$

This operation is computationally vacuous:
- **No Operational Intensity:** The reduction of an integer vector `{-1, 0, +1}` into a sum and count requires exactly one scalar addition and one comparison per element. In modern CPU terms, this is completely memory-bandwidth bound.
- **Artificial Serialization Overhead:** In any realistic computational runtime (such as an inference engine, graph compiler, or numerical solver), vectors are not serialized to bytes, retained, and immediately deserialized within the same thread before performing a single addition.
- **Bypassing the Representation:** For the sparse arm, computing `sum` and `nonzero_count` does not even require full decoding! The `nonzero_count` is stored directly in the 32-bit header. The `sum` is simply the sum of the non-zero values array. The sparse arm is thus artificially favored because its format metadata happens to pre-compute half the task.

### 3.2 Artificially Tailored Regime Switching (Circular Validation)
PR #672 defines the primary confirmatory trace as:
- Regime 1: 256 vectors, length 256, density $5\%$ ($0.05$)
- Regime 2: 256 vectors, length 4,096, density $95\%$ ($0.95$)
- Regime 3: 256 vectors, length 4,096, density $5\%$ ($0.05$)
- Regime 4: 256 vectors, length 256, density $95\%$ ($0.95$)

This trace is constructed with extreme bimodal polarization:
- At $D = 0.05$, a vector of length 4,096 has only $\approx 204$ nonzeros. A sparse representation requires $\approx 204 \times (4 + 1) = 1,020\text{ bytes}$, whereas dense INT8 requires $4,096\text{ bytes}$ and packed 2-bit requires $1,024\text{ bytes}$.
- At $D = 0.95$, a vector of length 4,096 has $\approx 3,891$ nonzeros. A sparse representation requires $3,891 \times 5 = 19,455\text{ bytes}$ ($4.75\times$ larger than dense INT8!).
- A single fixed format forced across all four regimes will incur massive penalties: fixed-sparse will suffer catastrophic blowup in Regimes 2 and 4, while fixed-dense will waste memory traffic in Regimes 1 and 3.

**The Degeneracy:** The experiment constructs a scenario where fixed baselines are mathematically crippled by design, and then celebrates the fact that a switching selector achieves a $>10\%$ speedup. This does not prove that adaptation has high scientific efficacy; it proves that bimodal torture-testing of fixed representations produces bimodal inefficiency.

### 3.3 Representation Equivalence and Ordinary Compression Analogy
Does Candidate B expose anything unique to ternary representations?
Consider an ordinary binary system handling vectors in $\{0, 1\}$ or $\{-128..127\}$:
- Arm 1: Bitmask + Value array (Sparse)
- Arm 2: Dense Byte array (Dense)
- Arm 3: Bit-packed array (Packed)

If subjected to the same trace of alternating $5\%$ and $95\%$ densities, an ordinary binary adaptive compressor will switch between bitmask-sparse and dense-byte, beating any fixed format by the same or greater margin. Thus, Candidate B is not a study of ternary computing; it is a rudimentary reinvention of run-time adaptive compression heuristics.

---

## 4. Strongest Conventional Comparator & The Selector Problem

### 4.1 Challenging the "Adaptive Selector"
Candidate B proposes comparing a "deterministic adaptive policy" against fixed formats and a simple threshold heuristic. However, in the context of computer architecture and systems software, what is the legitimate state of the art?

1. **The Trivial Density Threshold:**
   If the selector is provided with or computes the non-zero density $D$, the decision rule is trivial:
   $$\text{Format} = \begin{cases} \text{Sparse}, & \text{if } D \le \theta_{\text{sparse}} \\ \text{Packed}, & \text{if } \theta_{\text{sparse}} < D \le \theta_{\text{dense}} \\ \text{Dense}, & \text{otherwise} \end{cases}$$
   On modern x86/ARM processors, the crossover points $\theta$ are static constants determined by memory bus width and vector decode costs (typically $\theta_{\text{sparse}} \approx 0.12$ to $0.15$). A static branch or lookup on this threshold executes in $<2\text{ ns}$.

2. **The Offline Hindsight Oracle:**
   An offline oracle computes the exact minimum execution time across all candidate formats for each vector:
   $$T_{\text{oracle}} = \sum_{t=1}^N \min_{m \in \{\text{dense, packed, sparse}\}} T_m(v_t)$$
   In PR #672, the oracle is relegated to an "unattainable upper bound." But in reality, on the proposed primary trace, the trivial density threshold will achieve $\ge 98\%$ of the hindsight oracle’s performance because the regimes are separated by a massive gap ($5\%$ vs $95\%$).

3. **Deterministic Cost-Model Selector:**
   A deterministic analytical model estimates cost based on input length $L$ and density $D$:
   $$C_m = \alpha_m \cdot L + \beta_m \cdot (D \cdot L) + \gamma_m$$
   Evaluating this linear cost model requires only two integer multiplications and additions.

### 4.2 The "AI" / Learned Selector Fallacy
Why is an AI-driven or learned selector completely unwarranted here?
- **Decision Complexity:** The decision space is tiny (choose 1 of 3 formats) based on two scalar inputs ($L$ and $D$).
- **Inference Tax:** Even the smallest neural network or decision forest requires thousands of CPU cycles to evaluate. When the total time to encode, decode, and reduce a 256-element vector is on the order of 200–500 nanoseconds, executing an ML inference routine (even a micro-MLP taking $1\text{ }\mu\text{s}$) will introduce an overhead larger than the entire computation being optimized!
- **Scientific Value:** Introducing a learned selector into this micro-workload adds no scientific insight; it merely serves as "AI-washing" to fit a research marketing label.

### 4.3 Summary of Comparator Ranking

```
+--------------------------------------------------------------------------+
| 1. Offline Hindsight Oracle (Theoretical Upper Bound, 0ns decision)     |
+--------------------------------------------------------------------------+
                                     |  ~99% efficiency
                                     v
+--------------------------------------------------------------------------+
| 2. Static Density Threshold Heuristic (2ns execution, zero state)       |
+--------------------------------------------------------------------------+
                                     |  ~98% efficiency
                                     v
+--------------------------------------------------------------------------+
| 3. Deterministic Linear Cost Model (10ns execution, robust to shapes)   |
+--------------------------------------------------------------------------+
                                     |  Negative efficiency (Inference Tax)
                                     v
+--------------------------------------------------------------------------+
| 4. Learned / AI Selector (1,000ns execution, fails amortization)        |
+--------------------------------------------------------------------------+
```

---

## 5. Propose a Stronger Alternative: Recommended Workload and Hypothesis

If Candidate B is to be retained, the workload must be replaced with an operation that cannot be trivialized, exhibits computational intensity, exercises representation-specific features, and runs deterministically on standard CPUs.

### 5.1 The Replacement Workload: Sparse-Trit Polynomial Evaluation with Dynamic Horner/Index Routing

Instead of trivial reduction `(sum, count)`, the operation is the exact evaluation of high-degree integer polynomials with ternary coefficients $c_i \in \{-1, 0, +1\}$ over an integer evaluation point $x \in \mathbb{Z}$:
$$P(x) = \sum_{i=0}^{L-1} c_i x^i \pmod{2^{64}}$$

#### Why This Workload is Scientifically Superior
1. **Computational Meaningfulness:**
   Evaluating $P(x)$ requires non-trivial computation:
   - For dense and packed representations, evaluation proceeds via **Horner's Rule**:
     $$P(x) = (\dots((c_{L-1} \cdot x + c_{L-2}) \cdot x + c_{L-3})\dots) \cdot x + c_0$$
     Because $c_i \in \{-1, 0, +1\}$, the multiply-accumulate $y \cdot x + c_i$ does not require a full 64-bit multiplier for $c_i$; it requires an integer multiplication $y \cdot x$ followed by an addition, subtraction, or nop depending on $c_i$.
   - For sparse representations, evaluation proceeds via **Sparse Exponentiation**:
     $$P(x) = \sum_{k: c_{i_k} \ne 0} c_{i_k} \cdot x^{i_k} \pmod{2^{64}}$$
     where powers $x^{i_k}$ are computed via binary exponentiation or modular exponentiation tables.
2. **Representation-Dependent Tradeoffs:**
   - At high density ($D \approx 1.0$), Horner's method on packed/dense formats performs $L$ multiplications and $L$ additions in a tight loop with zero indexing overhead.
   - At low density ($D \ll 0.1$), computing Horner's rule performs $L$ multiplications where most additions are $+0$, wasting immense ALU time. In contrast, sparse index evaluation performs only $K = D \cdot L$ evaluations, but requires exponentiation $x^k$.
   - **The Crossover is Non-Trivial:** The crossover between Horner's rule (dense/packed) and sparse indexed exponentiation is not a simple linear function of byte count; it depends directly on CPU multiplier throughput, dependency chains, and index cache locality.
3. **Exact Determinism and No Speculative Hardware:**
   - Both algorithms produce exact 64-bit integer outputs modulo $2^{64}$.
   - Runs directly on existing x86-64 and ARM64 binary ALUs without floating-point rounding ambiguities.
4. **Matched Conventional Binary Baselines:**
   - Evaluated against standard binary integer polynomial algorithms.
   - Packed ternary (2 bits/trit, 4 trits/byte) competes directly against binary 2-bit signed packing and standard 8-bit Horner evaluation.

### 5.2 Formal Preregistered Hypothesis for the Stronger Alternative

> **Hypothesis $H_{B\text{-alt}}$:**
> For an input stream of polynomial coefficient vectors $C_t \in \{-1, 0, +1\}^L$ and evaluation points $x_t \in \mathbb{Z}$, a deterministic causal representation selector switching between Sparse-Indexed Exponentiation and Packed-Ternary Horner Evaluation will achieve at least a **$15\%$ reduction in total wall-clock execution time** relative to the best static representation across a mixed-density evaluation suite, when all descriptor scanning, format transitions, and Horner/exponentiation computations are fully accounted for.

---

## 6. Expected Failure Modes

If the revised Candidate B experiment is executed, the following distinct failure modes must be preregistered and monitored:

| Failure Mode ID | Description | Root Mechanism | Scientific Disposition |
| :--- | :--- | :--- | :--- |
| **FM-1: Selector Overhead Dominance** | Time spent scanning vectors to determine density exceeds the time saved by selecting the optimal execution kernel. | $O(L)$ descriptor extraction on cold cache negates algorithmic speedups. | **NEGATIVE (Selector Tax Invalidation)** |
| **FM-2: Static Packed Hegemony** | Packed 2-bit representation with unrolled Horner evaluation outperforms both sparse and dense across all practical densities ($D \in [0.08, 1.0]$). | The branch-free ALU Horner loop ($y \cdot x + \text{sign}$) is so fast on modern pipelines that sparse index lookup cannot beat it except at near-zero density ($D < 0.03$). | **NEGATIVE (Static Representation Dominance)** |
| **FM-3: Binary Comparator Equivalence** | Packed 2-bit balanced ternary exhibits identical execution timing to conventional 2-bit binary signed integers. | Binary CPUs treat both representations identically after nibble masking; no ternary-specific algebraic gain exists. | **INCONCLUSIVE for Ternary / POSITIVE for General Packing** |
| **FM-4: Cache Truncation Collapse** | If vector length $L$ fits completely within L1 data cache (e.g., $L=256$, taking 64–256 bytes), memory bandwidth ceases to matter, and all formats perform identically within measurement noise. | Benchmark is execution-pipeline bound rather than representation/bandwidth bound. | **INVALID (Unsound Experimental Regime)** |
| **FM-5: Hysteresis Churn** | In alternating or near-threshold sequences, the selector frequently switches formats, incurring catastrophic allocation and re-packing penalties. | Thrashing state transitions without hysteresis dampers. | **NEGATIVE (Adaptive Instability)** |

---

## 7. Estimated Relative Implementation Effort

To inform governance decisions, the table below provides a rigorous comparative breakdown of engineering effort across Candidate B (current), Candidate B (revised alternative), and Candidate C (T-GEMV).

| Phase / Engineering Dimension | Path A: Candidate B (Current PR #672) | Path B: Candidate B (Revised Horner Workload) | Path C: Candidate C (T-GEMV from PR #669) |
| :--- | :---: | :---: | :---: |
| **1. Specification & Protocol Formalization** | 1 day (Draft exists in PR #672) | 2–3 days (Formalize Horner/exponentiation contract) | 4–5 days (Must audit prior AVX2 MatVec novelty) |
| **2. Correctness & Reference Oracles** | 2 days (Trivial sum/count oracle) | 3 days (GMP/BigInt & 64-bit modular polynomial oracle) | 4 days (FP32/INT8 GEMV reference & error bounds) |
| **3. Kernel Implementation (x86/ARM)** | 2 days (Simple loop encode/decode/reduce) | 4 days (Vectorized Horner + Sparse index exponentiation) | 8–10 days (AVX2/AVX-512 VNNI / NEON assembly kernels) |
| **4. Selector & Dispatch State Machine** | 2 days (Threshold heuristic + cost model) | 3 days (Descriptor scanner + transition cache) | N/A (Static matrix layout) |
| **5. Statistical & Measurement Harness** | 3 days (Wall-clock timer, trace runner) | 3 days (Run-block randomization, warmup, CI gates) | 5 days (PMU cycle counters, cache flush, DRAM isolation) |
| **6. Total Estimated Engineering Effort** | **10 Person-Days** | **15–18 Person-Days** | **21–27 Person-Days** |
| **Scientific Information Value (Bits/Cost)** | **Near Zero** (Degenerate result) | **High** (Resolves dynamic Horner vs Sparse tradeoffs) | **Moderate** (High risk of repeating PR #549 AVX2 results) |

Path B requires approximately 5–8 person-days of additional development over the trivial PR #672 draft, but increases the scientific value from near-zero to high by avoiding workload degeneracy.

---

## 8. Evidence Supporting the Recommendation

The recommendation to reject the current PR #672 workload and adopt Path B is supported by concrete empirical and historical records across the T81 repository:

1. **The Lesson of Milestone M1 (PR #665 / PR #666):**
   Milestone M1 evaluated wildcard matching over 4-trit vectors against 40 fixed patterns ($C_{40}$). The independent review (PR #666) proved that the workload was mathematically degenerate ($O(1)$ scalar range check) because the pattern space was artificially restricted. Executing PR #672’s vector reduction `(sum, count)` repeats the exact same methodological failure: choosing an artificially simple workload to ensure trivial pass criteria.
2. **The CPU Ternary Closure (PRs #544–#549):**
   In `docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`, extensive benchmarking proved that general-purpose binary CPUs penalize software ternary bit-manipulation. A 2-bit AVX2 MatVec kernel could only reach $75.3\%$ of INT8 AVX2 speed. This definitive finding mandates that any future experiment on binary CPUs *must* account for the instruction tax of unpacking. Candidate B’s current workload completely obscures this by using an operation so light that unpacking dominates $100\%$ of the runtime.
3. **The H04-R02 Reconstruction (`CORRECTED_RESULTS.md`):**
   The H04-R02 audit demonstrated that initial claims of universal ternary representation advantage were artifacts of flawed division arithmetic and metadata omissions. When corrected under equal 10-bit Tier-0 controls, the advantage was strictly conditional ($AT=1210$ vs $AB=1250$ at $r=1.60$) and bounded to a theoretical cost model ($U$). Attempting to confirm this symbolic model using vector `(sum, count)` on physical hardware will produce false negatives or false positives driven entirely by cache line alignment and memory allocation, not representation theory.

---

## 9. One Next Action

### Formal Recommendation: Reject PR #672 Workload; Mandate Path B Redesign
The project maintainers should adopt **Path B**: formally reject the trivial decode/reduction workload in PR #672, and draft a revised Milestone M2 protocol based on **Sparse-Trit Polynomial Evaluation with Dynamic Horner/Index Routing**.

### Single Concrete Next Action
Maintainers should update the disposition of PR #671 and PR #672 to:
```
DISPOSITION: M2_B_WORKLOAD_REVISION_REQUIRED
ACTION: Reject vector (sum, count) reduction workload as experimentally degenerate.
TASK: Author revised Candidate B protocol draft specifying Sparse-Trit Polynomial
      Evaluation (Horner vs Sparse Exponentiation) with exact modular 2^64 semantics,
      strictly maintaining NO EXECUTION and NO BENCHMARKING authorization until approved.
```
