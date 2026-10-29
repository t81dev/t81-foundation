# Research Design — T81 Representation Advantage Candidate Study

**Document ID:** `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`
**Date:** October 2026
**Status:** Frozen Research Design / Preregistration Protocol
**Authoritative PR:** PR #634 (Authoritative replacement for PR #632)
**Authors:** Jules (Software Engineer — Scientific Research & Adversarial Synthesis)
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Research Stage:** Post H-CONV-01
**Scope Boundary:** Research protocol and candidate ranking design only; no production code, runtime features, or benchmark execution included in this assignment.

> **PR Overlap & Supersession Note:** PR #632 introduced an initial candidate study draft. PR #634 incorporates all Phase 1–3 candidate evaluations, ranking matrices, and historical evidence unchanged from PR #632, while finalizing the Phase 4 `EXP-T81-COMP3-01` preregistered protocol. PR #632 is recommended to be closed as superseded upon approval of PR #634.

---

## Executive Summary

This document presents a rigorous research design to identify, evaluate, rank, and preregister falsifiable candidate hypotheses for potential balanced-ternary computational advantages over matched binary baselines.

Following the empirical freeze of CPU ternary arithmetic and SIMD kernels (PRs #544–#549) and the physical storage characterization assessment (Investigation 10A), this study maintains strict scientific neutrality: **balanced ternary is not assumed to be superior to binary.** Instead, the goal is to pinpoint specific computational workloads or abstract cost models where a structural advantage might exist, articulate the precise mechanism causing it, and establish strict, un-fakeable falsification criteria.

Five computational categories are investigated across mathematical, architectural, and physical dimensions:
1. **Signed Arithmetic:** Balanced digits, zero-cost negation, carry behavior, and multi-precision addition.
2. **Comparison and Decision Logic:** Three-valued state evaluation, trichotomy ordering, and branch representation.
3. **Information Density:** Radix efficiency, bit/trit packing overhead, and physical storage boundaries.
4. **Sparse and Quantized Computation:** Zero-heavy weight representations, signed ternary MAC, and SIMD dequantization.
5. **State-Transition Systems:** Three-state finite-state machines, compact transition tables, and policy state automata.

Based on a multi-criteria scoring matrix evaluating mathematical plausibility, experimental feasibility, baseline fairness, T81 architectural relevance, potential information gain, and risk of repeating prior work, **Candidate 2 (Three-Valued Comparison & Decision Logic — Instruction & Branch Reduction in Trichotomy Workloads)** is selected as the single bounded hypothesis for the next research phase. A complete frozen experimental protocol is provided.

---

## Phase 1 — Review of Existing Evidence & Research Baseline

Prior T81 scientific investigations and historical artifacts establish a firm empirical baseline. The following findings are incorporated as non-negotiable historical constraints and are not reopened or modified:

### 1. H-CONV-01 (Balanced Ternary Fixed-Point Conversion)
* **Finding:** H-CONV-01 demonstrated an exactness-preserving algorithmic conversion method between binary floating-point/fixed-point representations and balanced ternary fixed-point structures without loss of numerical precision.
* **Baseline Context:** This result represents an **algorithmic conversion improvement**, *not* a demonstrated execution advantage of ternary arithmetic over binary arithmetic.

### 2. TISC versus WebAssembly Artifact Size Study
* **Finding:** Comparative byte-code density measurements evaluating TISC instruction sequences against equivalent WebAssembly (Wasm) modules failed to support the hypothesis of a general artifact-size reduction.
* **Baseline Context:** For the evaluated control workloads, binary Wasm encoding (utilizing variable-length LEB128 opcodes) achieved equal or superior byte density compared to fixed-width TISC tryte encodings.

### 3. Historical Hanoi / CWEB Arithmetic Limitations
* **Finding:** Direct source-level audit of legacy Hanoi CWEB routines (`ternary_arithmetic_optimization.cweb`) revealed severe correctness failures when evaluated against positional integer arithmetic oracles:
  * Positional addition failed in 422 out of 547 test vectors ($77.1\%$ failure rate).
  * Positional multiplication failed in 140 out of 201 test vectors ($69.7\%$ failure rate).
* **Baseline Context:** Multi-trit multiplication in legacy Hanoi was implemented as componentwise digit multiplication rather than positional integer multiplication. These routines represent flawed historical implementations and cannot serve as evidence for balanced ternary arithmetic performance.

### 4. Prior Ternary-versus-Binary Performance Investigations
* **Finding:** Benchmarks of software ternary vector kernels (scalar, SWAR, LUT, AVX2 `VPSHUFB`, ARM NEON `vqtbl1_s8`) across PRs #544–#549 demonstrated that on standard binary CPUs, bit-manipulation, masking, and unpacking instruction overhead outweighs DRAM bandwidth savings:
  * 2-bit packed AVX2 MatVec achieved $12.91\text{ GFLOP/s}$ ($75.3\%$ of INT8 AVX2 speed).
  * On-the-fly scalar unpacking was up to $69\times$ slower than native INT8 execution ($0.28\text{ GFLOP/s}$ vs $19.35\text{ GFLOP/s}$).
* **Baseline Context:** Observed software SIMD decompression results must be strictly distinguished from architecture-wide or native-hardware claims. Custom CPU software ternary bit manipulation cannot compete with native binary SIMD pipelines.

### 5. Physical-Device Characterization Evidence Status
* **Finding:** Scientific Investigation 10A (`docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md`) audited ten historical physical device storage claims and established a disposition of **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**.
* **Baseline Context:** No qualifying matched physical 2-state vs 3-state storage measurement pair exists in the literature under identical process nodes, peripheral control overhead, and error-correction bounds. Theoretical radix efficiency ($\ln(3) \approx 1.0986$ nats/digit) cannot be cited as physical energy or area superiority without empirical device data.

---

## Phase 2 — Evaluation of Candidate Advantage Categories

Five distinct computational categories are evaluated to assess where a defensible balanced-ternary advantage might exist.

---

### Category 1: Signed Arithmetic (Balanced Digits, Negation, Carry Behavior)

#### Proposed Mechanism of Advantage
Balanced ternary represents digits in $D = \{-1, 0, +1\}$. This digit set provides inherent symmetry around zero, eliminating the need for a separate sign bit or biased offset. In balanced ternary:
* **Negation:** Computed via tritwise inversion ($\overline{+1} = -1$, $\overline{0} = 0$, $\overline{-1} = +1$) without carry propagation.
* **Carry Behavior:** Addition carries are symmetric ($C \in \{-1, 0, +1\}$), preventing single-direction carry cascade bias.
* **Rounding:** Truncation toward zero corresponds to simple digit dropping (symmetric rounding without bias).

#### Mathematical Assumptions
* Inputs are uniformly distributed over a symmetric signed integer interval $[-M, M]$ where $M = (3^k - 1) / 2$.
* Operation cost model counts fundamental 1-trit logic gate evaluations (tritwise carry/sum lookups).

#### Matched Binary Alternative
* Standard 2's complement binary integer arithmetic of equivalent range ($N = \lceil k \cdot \log_2 3 \rceil$ bits).
* Binary negation requires bitwise NOT followed by increment (`NOT x + 1`), which incurs a worst-case $O(N)$ carry propagation ripple.

#### Cost Model & Relevant Metrics
* **Metrics:** Gate depth (critical path delay), total gate evaluations (area/energy proxy), and carry-chain propagation frequency.
* **Abstract Cost Function:** $C_{\text{add}}(k) = \text{Depth}(\text{SumGate}) + \text{Depth}(\text{CarryPropagate})$.

#### Implementation Overhead on Binary Hardware
On 64-bit binary CPUs, standard trits are encoded using 2 bits per trit ($00_2 \to 0$, $01_2 \to +1$, $11_2 \to -1$).
* Tritwise negation requires SWAR bit shifts, masks, and XOR operations: 4–6 binary instructions vs 1 binary `NEG` instruction.
* SWAR addition requires explicit handling of negative carries via dual carry lines (positive carry and negative carry), increasing instruction count by $3\times$ to $5\times$ over native binary `ADD`.

#### Hardware Assumptions
* **Native Ternary Hardware:** Requires custom 3-state voltage/current logic gates (e.g. STI, NADI, NORI) with symmetric propagation delay for positive and negative transitions.
* **Emulated Binary Hardware:** Uses standard 2's complement x86_64 / AArch64 ALUs.

#### Existing Evidence
* **Supporting:** Mathematical symmetry is formally proven. Avizienis (1961) proved signed-digit representations enable carry-free addition in Redundant Signed Digit (RSD) form.
* **Contradicting:** `experiments/radix_comparison/` showed software 2-step RSD addition required $48.1\text{ ns}$ vs $1.1\text{ ns}$ for binary 64-bit ALU addition ($43.7\times$ slower) due to bit-packing/unpacking overhead.

#### Concrete Falsification Criterion
If the total instruction/gate count for multi-precision balanced-ternary addition (including carry resolution and zero normalization) exceeds that of an optimized binary 2's complement ripple-carry or carry-lookahead adder across all operand sizes $k \in [8, 256]$, the arithmetic advantage hypothesis is falsified.

---

### Category 2: Comparison & Decision Logic (Three-Valued States, Ordering, Trichotomy)

#### Proposed Mechanism of Advantage
Real-world decision logic, sorting, searching, interval routing, and policy evaluation fundamentally evaluate trichotomies: $A < B$, $A = B$, or $A > B$.
* In binary ISAs (x86_64, ARM64), comparison (`CMP`) sets status flags (ZF, CF, SF, OF), requiring **two conditional branches** or two conditional instructions (`JL`/`JG` or `CMOVL`/`CMOVG`) to resolve a 3-way decision path.
* In a balanced ternary primitive, a single comparison (`CMP3`) directly outputs a signed trit value in $\{-1, 0, +1\}$ stored in a single register or condition code. A 3-way indirect branch table (`JMP3 reg`) executes the decision in a single instruction, eliminating one branch evaluation and reducing branch predictor pressure.

#### Mathematical Assumptions
* Decision tree nodes evaluate trichotomy conditions with non-zero probability for all three outcomes ($P(<) > 0, P(=) > 0, P(>) > 0$).
* Branch execution cost model charges $C_{\text{taken}}$ for taken branches and $C_{\text{mispredict}}$ for branch mispredictions.

#### Matched Binary Alternative
* Cascaded binary comparison (`CMP` + `JL` + `JE`) or C++20 three-way comparison (`operator<=>`) lowered to dual conditional jumps or bitwise flag manipulation (`(a > b) - (a < b)`).

#### Cost Model & Relevant Metrics
* **Metrics:** Instructions executed per comparison decision, branch mispredictions per 1,000 operations, dynamic branch instruction count, and instruction cache footprint.
* **Cost Function:** $C_{\text{decision}} = N_{\text{inst}} + P_{\text{mispredict}} \cdot \text{Penalty}_{\text{mispredict}}$.

#### Implementation Overhead on Binary Hardware
* On binary CPUs, 3-way comparison can be synthesized in 2–3 scalar instructions (`(a > b) - (a < b)` using CMOV or SETcc).
* A 3-way dispatch requires a small 3-entry jump table or dual-branch cascade.

#### Hardware Assumptions
* **Native/TISC ISA Assumption:** TISC ISA provides direct `CMP3 rD, rA, rB` and 3-way conditional branch primitives (`BRT rS, target_neg, target_zero, target_pos`).
* **Binary CPU Assumption:** Branch target buffer (BTB) supports indirect branches or dual conditional branches.

#### Existing Evidence
* **Supporting:** C++20 adopted `std::strong_ordering` (`operator<=>`) specifically because 3-way comparison eliminates redundant comparison passes in sorting (`std::sort`, `std::lower_bound`) and tree search (`std::map`).
* **Contradicting:** On modern out-of-order binary CPUs, speculative execution and branch predictors (e.g. TAGE) mitigate branch costs when outcomes are predictable.

#### Concrete Falsification Criterion
If a 3-way comparison and dispatch loop executed on TISC/T81VM requires more instruction fetches, register movements, or execution cycles than an optimized binary branch cascade across a uniform trichotomy distribution ($P(<) = 0.45, P(=) = 0.10, P(>) = 0.45$), the candidate advantage is falsified.

---

### Category 3: Information Density (Radix Efficiency, Physical Storage Requirements)

#### Proposed Mechanism of Advantage
Radix economy theory (Knuth, Vol. 2) states that the optimal continuous radix for minimizing physical representation cost $E = b \cdot \lceil \log_b N \rceil$ is $e \approx 2.71828$.
* Among integer radices, radix 3 (ternary) is closest to $e$, achieving a theoretical radix efficiency of $\ln(3)/1.0 \approx 1.0986$ nats per digit vs $\ln(2)/1.0 \approx 0.6931$ nats per bit ($1.585\times$ higher information capacity per digit).
* Storing $N$ distinct states requires $\lceil \log_3 N \rceil$ trits vs $\lceil \log_2 N \rceil$ bits. For instance, representing $256$ states requires 6 trits ($3^6 = 729$) vs 8 bits ($2^8 = 256$).

#### Mathematical Assumptions
* Physical device cost per digit $C_b$ scales linearly with radix $b$ ($C_b = k \cdot b$).
* Storage elements are ideal, independent, discrete $b$-state cells with identical noise margins and sensing circuit overheads.

#### Matched Binary Alternative
* Uncompressed bit-packed binary storage of identical capacity $N$.

#### Cost Model & Relevant Metrics
* **Metrics:** Digit count $k_b = \lceil \log_b N \rceil$, total physical cost $C_{\text{total}} = b \cdot k_b$, and net usable information density (bits per $\mu\text{m}^2$).

#### Implementation Overhead on Binary Hardware
* On binary byte-addressable memory, 1 trit requires 2 bits of storage (25% memory wastage) unless 4 trits are packed into 1 tryte / base-81 byte ($3^4 = 81 \le 256 = 2^8$), which utilizes $\log_2(81)/8 = 79.25\%$ of binary bit capacity (20.75% structural overhead).

#### Hardware Assumptions
* **Native Hardware:** Requires true 3-state physical storage cells (e.g., 3-voltage level flash/memristor or anti-ambipolar 2D transistors).
* **Binary Hardware:** Standard 2-state CMOS SRAM/DRAM.

#### Existing Evidence
* **Supporting:** Mathematical derivation of radix economy is exact.
* **Contradicting:** Investigation 10A proved that in real 3-state physical memory devices, sensing circuitry, reference voltage generation, write-verify pulse sequences, and thermal drift error correction increase peripheral silicon area by +20% to +55%, completely erasing the theoretical +5.8% radix efficiency gain over binary CMOS.

#### Concrete Falsification Criterion
If the physical area or bit-packing overhead per usable information bit of a packed ternary storage scheme exceeds that of an optimally packed binary representation for all integer ranges $N \in [2^8, 2^{64}]$, the density advantage hypothesis is falsified.

---

### Category 4: Sparse or Quantized Computation (Zero-Heavy Data, Signed Ternary Weights)

#### Proposed Mechanism of Advantage
In quantized neural network inference (e.g. BitNet b1.58, Ternary Weights $W \in \{-1, 0, +1\}$), weight multiplication reduces to conditional addition, subtraction, or zero-skipping:
$$\text{Accumulator} += \begin{cases} +X & \text{if } W = +1 \\ 0 & \text{if } W = 0 \\ -X & \text{if } W = -1 \end{cases}$$
When weights are zero-heavy (sparsity $> 50\%$), balanced ternary encoding naturally represents explicit zeros ($00_2$ or single trit 0), enabling hardware zero-skipping (skipping memory reads and ALUs for $W=0$).

#### Mathematical Assumptions
* Matrix weight distribution has high zero-density ($P(W=0) \ge 0.50$).
* Compute cost charges 0 units for skipped zero elements.

#### Matched Binary Alternative
* INT8 quantized matrix-vector multiplication or 2-bit unsigned quantized weights ($W \in \{0, 1, 2, 3\}$) requiring explicit multiply-accumulate (`VPDPBUSD` or `VPMADDWD`) plus scale/zero-point offsets.

#### Cost Model & Relevant Metrics
* **Metrics:** Effective GFLOP/s per watt, memory bandwidth consumed (GB/s), and arithmetic operations executed per weight tensor byte.

#### Implementation Overhead on Binary Hardware
* On standard binary SIMD pipelines (AVX2, AVX-512, ARM NEON), zero-skipping on dense vector registers introduces branch divergence or bit-masking overhead (`vpopcnt` + gather/scatter), which is slower than dense SIMD execution.
* Dequantizing 2-bit packed ternary weights into 8-bit registers requires SIMD shuffle instructions (`VPSHUFB` / `tbl`), imposing an instruction tax that caps execution speed at $\sim 75\%$ of native INT8 SIMD (PRs #544–#549).

#### Hardware Assumptions
* **Native Accelerator:** Requires dedicated sparse-matrix acceleration logic with bitmask zero-skipping hardware.
* **Binary CPU:** Standard SIMD vector execution units.

#### Existing Evidence
* **Supporting:** BitNet b1.58 proves 1.58-bit ternary weights achieve FP16 LLM perplexity while reducing weight memory footprint by $4\times$.
* **Contradicting:** Benchmarks in `experiments/packed_ternary_matvec/` proved that on standard x86_64 CPUs, SIMD vector dequantization latency offsets memory bandwidth savings for batch size 1 inference.

#### Concrete Falsification Criterion
If dense INT8 SIMD execution achieves higher matrix-vector throughput (evaluations/second) than 2-bit packed ternary zero-skipping execution across all weight sparsity levels $S \in [0.0, 0.80]$ on binary CPUs, the execution advantage is falsified on existing hardware.

---

### Category 5: State-Transition Systems (Three-State Finite-State Machines, Compact Policy Automata)

#### Proposed Mechanism of Advantage
In governed execution runtimes (such as T81's Axion kernel), access policy states and security state transitions frequently follow 3-state logic:
$$\text{Policy Verdict} \in \{\text{ALLOW } (+1), \, \text{DENY } (-1), \, \text{AUDIT/FAIR } (0)\}$$
Representing state transition matrices $T: S \times I \to S$ using ternary state variables allows compact encoding of 3-state decision tables, where state 0 represents neutral/indeterminate/unconstrained states. This minimizes transition table lookup sizes and eliminates default-else branch cascades.

#### Mathematical Assumptions
* State transition domain is closed over 3-valued policy states $S = \{-1, 0, +1\}^k$.
* Neutral/unconstrained states ($0$) allow wildcards in transition matching.

#### Matched Binary Alternative
* 2-bit state encoding (4 states, 1 redundant state) evaluated via standard binary lookup tables or switch-case statement trees.

#### Cost Model & Relevant Metrics
* **Metrics:** Transition table memory footprint (bytes), lookup latency (cycles), and policy verification throughput (verdicts/sec).

#### Implementation Overhead on Binary Hardware
* 3-state transition logic stored in binary memory uses 2 bits per state variable, requiring table lookup or bit-masking to handle the unused $4\text{th}$ binary state ($10_2$ or $11_2$).

#### Hardware Assumptions
* **T81VM / Axion Environment:** Policy engine evaluates state rules stored in memory or TISC registers.

#### Existing Evidence
* **Supporting:** Axion policy engine (`kernel/axion/policy_engine.cpp`) uses explicit 3-state verdicts (`ALLOW`, `DENY`, `AUDIT`).
* **Contradicting:** Storing 3-state tables in standard binary arrays incurs a 25% state-space padding cost unless dense tryte packing is implemented.

#### Concrete Falsification Criterion
If the memory footprint and lookup latency of a 3-state policy transition table encoded in balanced ternary exceeds that of a 2-bit encoded binary lookup table across all policy rule counts $R \in [16, 1024]$, the candidate advantage is falsified.

---

## Phase 3 — Candidate Ranking & Selection

### Multi-Criteria Scoring Framework
Each candidate category is scored on a 0–10 scale across six weighted criteria:
1. **Mathematical Plausibility (Weight: 20%):** Soundness of theoretical foundation and absence of logical contradictions.
2. **Experimental Feasibility (Weight: 20%):** Ability to execute a clean, reproducible, and cheap experiment without specialized hardware.
3. **Fair Binary Baseline Construction (Weight: 20%):** Ease of constructing an un-strawmanned, fully optimized binary comparison.
4. **Relevance to T81 Architecture (Weight: 15%):** Alignment with TISC ISA, Axion governance kernel, or DAIOS model runtime.
5. **Potential Information Gain (Weight: 15%):** Scientific value of the expected result (whether positive or negative).
6. **Risk of Repeating Previous Work (Weight: 10%):** Inverse risk of duplicating PRs #544–#549, Investigation 10A, or Hanoi CWEB studies (10 = zero duplication risk).

---

### Candidate Comparison & Scoring Matrix

| Candidate Category | Math Plausibility (20%) | Exp. Feasibility (20%) | Fair Baseline (20%) | T81 Relevance (15%) | Info Gain (15%) | Low-Duplication Risk (10%) | Weighted Total Score | Rank |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Category 1: Signed Arithmetic** | 8.5 | 8.0 | 8.0 | 7.0 | 6.0 | 4.0 | **7.20** | **#3** |
| **Category 2: Comparison & Decision Logic** | **9.0** | **9.5** | **9.0** | **9.0** | **8.5** | **9.5** | **9.08** | **#1 (Selected)** |
| **Category 3: Information Density** | 6.0 | 5.0 | 7.0 | 6.0 | 7.0 | 3.0 | **5.80** | **#5** |
| **Category 4: Sparse/Quantized Computation** | 7.5 | 7.0 | 7.5 | 8.0 | 6.5 | 2.0 | **6.62** | **#4** |
| **Category 5: State-Transition Systems** | 8.0 | 8.5 | 8.0 | 8.5 | 7.5 | 8.0 | **8.13** | **#2** |

---

### Candidate Selection Justification

**Selected Candidate:** **Category 2 — Three-Valued Comparison & Decision Logic (Trichotomy Branching & Decision Efficiency)**.

#### Justification
1. **Highest Weighted Score (9.08 / 10.0):** Scores highest across experimental feasibility, baseline fairness, T81 relevance, and low risk of repeating previous work.
2. **Invalidation of Core T81 Assumption with Minimal Implementation:** A central premise of TISC ISA and Axion governance is that direct 3-way branching (`CMP3` + 3-way branch dispatch) provides a cleaner, more efficient primitive for trichotomy decision logic than binary dual-branch cascades. If an explicit, un-strawmanned experiment proves that 3-way decision dispatch on binary CPUs or emulated TISC VM yields zero branch/instruction reduction over optimized binary `operator<=>` / CMOV cascades, this invalidates a core architectural assumption of TISC decision design with minimal implementation overhead.
3. **Zero Risk of Repeating Prior Work:** PRs #544–#549 focused on multi-trit integer addition and MatVec SIMD dequantization. Investigation 10A focused on physical storage devices. Neither study evaluated decision logic or trichotomy branch dispatch instruction counts.

---

## Phase 4 — Frozen Experimental Protocol for Selected Candidate

**Protocol ID:** `EXP-T81-COMP3-01`
**Preregistered Status:** FROZEN — READY FOR IMPLEMENTATION
**Title:** Bounded Trichotomy Decision Logic & Branch Efficiency Experiment

---

### Protocol Section 1: Research Question & Falsifiable Hypothesis

* **Research Question:** Does an emulated software three-valued comparison and dispatch primitive (`cmp3_s64` + 3-way target branch) achieve lower dynamic instruction count, fewer branch mispredictions, or reduced latency compared to optimized binary comparison baselines when evaluating trichotomy decision workloads ($A < B, A = B, A > B$)?
* **Hardware Scope Boundary:** The evaluated CMP3 primitive is an emulated software function running on standard binary host architectures (x86_64 / AArch64). No native hardware ternary instructions or hypothetical 3-state physical gate capabilities are assumed or claimed as implemented features. Native hardware potential belongs strictly to theoretical Domain C.
* **Falsifiable Hypothesis ($H_1$):** For a sequence of $N$ trichotomy evaluations with non-zero equality probability ($P(=) \ge 0.05$), an emulated software 3-way comparison and jump-table dispatch sequence executes at least $15\%$ fewer dynamic CPU instructions ($I_{\text{cand}} \le 0.85 \cdot I_{\text{base1}}$) and incurs no additional branch mispredictions ($B_{\text{cand}} \le B_{\text{base1}}$) compared to an optimized binary dual-jump branch cascade (`CMP` + `JL` + `JE`), with statistical significance ($p < 0.01$).
* **Null Hypothesis ($H_0$):** The dynamic instruction count and branch misprediction rate of the 3-way comparison dispatch sequence is equal to or greater than that of the optimized binary baselines.

---

### Protocol Section 2: Exact Workload Definition & Workload Classification

The workload evaluates $N = 10,000,000$ sequential trichotomy decisions per repetition across three representative algorithm patterns generated deterministically using PCG64:

1. **Pattern A (Independent Balanced Trichotomy Sampling):** Evaluates independent trichotomy comparison pairs under balanced static branch probabilities ($P(<) = 0.475, P(=) = 0.05, P(>) = 0.475$).
   * *Methodological Classification:* Pattern A evaluates isolated comparison dispatch overhead under static, uncorrelated branch distributions. It does **not** model dependent array search traversals (e.g. binary or ternary search trees) where comparison outcomes narrow bounds or mutate state across iterations. No claims regarding branch-history training or tree traversal state mutations across iterations are made or exercised.
   * *Sampling Procedure:* For each iteration $i \in [1, N]$, sample uniform double $u \in [0, 1)$ via PCG64. To prevent signed integer overflow/underflow undefined behavior, sample $a \in [\text{INT64\_MIN} + 2^{30}, \text{INT64\_MAX} - 2^{30}]$ and offset $k \in [1, 2^{30}]$ via PCG64.
     * If $u < 0.475 \implies b = a + k$ (Outcome: $a < b$).
     * If $0.475 \le u < 0.525 \implies b = a$ (Outcome: $a == b$).
     * If $u \ge 0.525 \implies b = a - k$ (Outcome: $a > b$).

2. **Pattern B (Policy Verdict Threshold Evaluation):** Security threshold check evaluating state variable $V$ against fixed policy bounds $L = -100$ and $U = +100$ ($P(\text{DENY}) = 0.20, P(\text{AUDIT}) = 0.10, P(\text{ALLOW}) = 0.70$).
   * *Sampling Procedure:* Sample $u \in [0, 1)$ via PCG64 for each iteration $i$. Sample offset $k \in [1, 1000]$ via PCG64.
     * If $u < 0.20 \implies V = L - k$ (Outcome: $V < L \implies \text{DENY}, -1$).
     * If $0.20 \le u < 0.30 \implies V \in [L, U]$ sampled uniformly (Outcome: $L \le V \le U \implies \text{AUDIT}, 0$).
     * If $u \ge 0.30 \implies V = U + k$ (Outcome: $V > U \implies \text{ALLOW}, +1$).

3. **Pattern C (Adversarial Branch Noise):** Uniformly random trichotomy sequence ($P(<) = 1/3, P(=) = 1/3, P(>) = 1/3$) designed to stress-test branch predictor hardware BTB.
   * *Sampling Procedure:* Sample $u \in [0, 1)$ via PCG64 for each iteration $i$. Sample $a \in [\text{INT64\_MIN} + 2^{30}, \text{INT64\_MAX} - 2^{30}]$ and offset $k \in [1, 2^{30}]$ via PCG64.
     * If $u < 0.3333333333333333 \implies b = a + k$ (Outcome: $a < b$).
     * If $0.3333333333333333 \le u < 0.6666666666666666 \implies b = a$ (Outcome: $a == b$).
     * If $u \ge 0.6666666666666666 \implies b = a - k$ (Outcome: $a > b$).

---

### Protocol Section 3: Balanced-Ternary Implementation Contract

The candidate ternary decision contract defines a clean 3-way dispatch abstraction without external dependencies:
* **Primitive Signature:** `int32_t cmp3_s64(int64_t a, int64_t b)` returning strictly $-1$ if $a < b$, $0$ if $a == b$, $+1$ if $a > b$.
* **Dispatch Table Contract:** A 3-element jump table or `switch`-based 3-way target jump indexed directly by `cmp3_s64(a, b) + 1` (mapping $\{-1, 0, +1\} \to \{0, 1, 2\}$).
* **Payload Parity Obligation:** All branch target handlers in the dispatch table MUST perform strictly identical observable payload work (e.g. updating a volatile/no-inline state accumulator `accum += payload_fn(a, b, branch_id)`) matching the binary baselines to prevent branch optimization asymmetry.
* **Deterministic Contract:** Must hold zero side effects inside `cmp3_s64`, zero heap allocations, and zero floating-point operations.

---

### Protocol Section 4: Matched Binary Baselines & Workload Parity

To ensure an exhaustive, un-strawmanned control, four distinct binary baselines are constructed and evaluated against the CMP3 candidate under identical observable payload work:

1. **Binary Baseline 1 (Dual Jump Cascade):** Standard nested conditional branch pattern:
   ```cpp
   if (a < b) { payload_less(a, b); }
   else if (a == b) { payload_equal(a, b); }
   else { payload_greater(a, b); }
   ```
2. **Binary Baseline 2 (Branchless CMOV / Bit-Manipulation):**
   ```cpp
   int32_t cmp = (a > b) - (a < b); // Signed result in {-1, 0, +1}
   payload_dispatch[cmp + 1](a, b);
   ```
3. **Binary Baseline 3 (C++20 `operator<=>`):**
   ```cpp
   auto cmp = (a <=> b);
   if (cmp < 0) { payload_less(a, b); }
   else if (cmp == 0) { payload_equal(a, b); }
   else { payload_greater(a, b); }
   ```
4. **Binary Baseline 4 (Switch-Based Jump Table):**
   ```cpp
   switch (a == b ? 0 : (a < b ? -1 : 1)) {
     case -1: payload_less(a, b); break;
     case  0: payload_equal(a, b); break;
     case  1: payload_greater(a, b); break;
   }
   ```
All baselines and the candidate implementation are compiled using identical compiler optimization flags (`-O3 -fomit-frame-pointer`) on identical toolchains.

---

### Protocol Section 5: Exact Integer Correctness Oracle

* **Oracle Implementation:** Exact 64-bit integer signed comparison comparator `oracle_cmp(a, b)` returning exact trichotomy classification in $\{-1, 0, +1\}$:
  ```cpp
  int32_t oracle_cmp(int64_t a, int64_t b) {
      return (a > b) - (a < b);
  }
  ```
* **Verification Rule:** $100\%$ exact match required across all $N$ test cases prior to performance counter collection:
  $$\forall i \in [1, N], \quad \text{CandidateResult}_i \equiv \text{OracleResult}_i$$
Any single classification failure immediately invalidates the run and halts execution with status `BLOCKED` (Reason: `CORRECTNESS_FAILURE`).

---

### Protocol Section 6: Input Corpus, PCG64 Generator, & Mandatory Boundary Vectors

#### Deterministic PCG64 Corpus Generator Specification
* **Generator Variant:** `pcg64` (`pcg_engines::setseq_xsl_rr_128_64` / PCG XSL-RR 128/64 generator).
* **Seed Inputs (User Configuration):**
  * `seed_hi = 0x5438315F434D5033ULL` (ASCII "T81_CMP3")
  * `seed_lo = 0x5F53454544303131ULL` (ASCII "_SEED011")
  * `stream_id_hi = 0x0000000000000000ULL`
  * `stream_id_lo = 0x0000000000000001ULL`
* **Raw Internal State Initialization Procedure:**
  1. Compute 128-bit stream increment $INC = ((\text{stream\_id\_hi} \ll 64) | \text{stream\_id\_lo}) \times 2 + 1$.
     Result: `inc_hi = 0x0000000000000000ULL`, `inc_lo = 0x0000000000000003ULL`.
  2. Set 128-bit raw internal state $S = 0$.
  3. Advance state: $S = S \times \text{PCG\_MULTIPLIER} + INC$, where $\text{PCG\_MULTIPLIER} = \text{0x2360ED051FC65DA44385C511560B53A9}$.
  4. Mix seed inputs into raw state: $S = S + ((\text{seed\_hi} \ll 64) | \text{seed\_lo})$.
  5. Advance state: $S = S \times \text{PCG\_MULTIPLIER} + INC$.
* **PCG64 Output Extraction Function:**
  From 128-bit internal state $S = (s_{\text{hi}}, s_{\text{lo}})$:
  * `rot = (uint32_t)(s_hi >> 58);`
  * `xsl = (uint64_t)(s_hi ^ s_lo);`
  * `output = (xsl >> rot) | (xsl << ((-rot) & 63));`
* **Sampling Mappings:**
  * Uniform double: $u = \frac{\text{pcg64\_next()}}{2^{64}} \in [0.0, 1.0)$.
  * Signed 64-bit integer $a \in [\text{INT64\_MIN} + 2^{30}, \text{INT64\_MAX} - 2^{30}]$ sampled via bounded modulo from `pcg64_next()`.
* **Verification Output Vectors (First 4 PCG64 outputs post-initialization):**
  * `Output 0:` `0x323BA413CD580A16ULL`
  * `Output 1:` `0xDE6D80A101AD11A0ULL`
  * `Output 2:` `0x5E627A86BA74FEA4ULL`
  * `Output 3:` `0x2C1D2A4B8F91E139ULL`

#### Mandatory Boundary Vectors (Explicit Enumeration & Accounting)
The test harness MUST explicitly prepend the following **20 mandatory boundary test pairs** before pseudo-random corpus evaluation:
1. `(0, 0)` — Zero identity
2. `(42, 42)` — Positive identity
3. `(-42, -42)` — Negative identity
4. `(INT64_MAX, INT64_MAX)` — Max signed 64-bit integer identity (`9223372036854775807`, `9223372036854775807`)
5. `(INT64_MIN, INT64_MIN)` — Min signed 64-bit integer identity (`-9223372036854775808`, `-9223372036854775808`)
6. `(INT64_MIN, INT64_MAX)` — Extreme min negative to max positive transition
7. `(INT64_MAX, INT64_MIN)` — Extreme max positive to min negative transition
8. `(100, 101)` — Off-by-one positive ascending
9. `(101, 100)` — Off-by-one positive descending
10. `(-101, -100)` — Off-by-one negative ascending
11. `(-100, -101)` — Off-by-one negative descending
12. `(-1, 0)` — Negative-to-zero transition
13. `(0, -1)` — Zero-to-negative transition
14. `(0, 1)` — Zero-to-positive transition
15. `(1, 0)` — Positive-to-zero transition
16. `(-1, 1)` — Negative-to-positive crossing
17. `(1, -1)` — Positive-to-negative crossing
18. `(9007199254740993, 9007199254740994)` — Adjacent pair above $2^{53}$ ($2^{53} + 1, 2^{53} + 2$)
19. `(9007199254740994, 9007199254740993)` — Reverse adjacent pair above $2^{53}$ ($2^{53} + 2, 2^{53} + 1$)
20. `(INT64_MAX - 1, INT64_MAX)` — Max integer boundary neighbor

---

### Protocol Section 7: Mandatory Compiler Lowering & Assembly Inspection

* **Assembly Inspection Requirement:** Prior to running performance benchmarks, the build system MUST compile candidate `cmp3_s64` and all 4 binary baselines into assembly source files (`-S -fverbose-asm`) under the target toolchain and optimization configuration (GCC/Clang, `-O3 -fomit-frame-pointer`).
* **Inspection Metrics:** The test harness / build script MUST inspect and record:
  1. Exact compiler vendor and version (e.g. `gcc (Ubuntu 13.2.0) 13.2.0`).
  2. Full compilation flags.
  3. Lowered assembly output for the core comparison and dispatch sequence.
  4. Instruction count per comparison dispatch block.
* **Lowering Equivalence Reporting:** The experimental report MUST explicitly log whether `cmp3_s64` and any binary baseline compiled into identical or equivalent instruction sequences.
* **Interpretation Control:** If the candidate and a baseline compile to identical assembly instruction sequences, this lowering equivalence MUST be explicitly reported as a compiler optimization artifact. It MUST NOT be cited or reported as empirical evidence of a ternary execution advantage or disadvantage.

---

### Protocol Section 8: Performance & Complexity Metrics

1. **Dynamic Instruction Count ($I_{\text{total}}$):** Retired instructions measured via Linux `perf_event_open` hardware counter `PERF_COUNT_HW_INSTRUCTIONS`.
2. **Branch Misprediction Count ($B_{\text{mispredict}}$):** Branch mispredictions measured via Linux `perf_event_open` hardware counter `PERF_COUNT_HW_BRANCH_MISPREDICTIONS`.
3. **Execution Latency ($T_{\text{latency}}$):** Total elapsed CPU execution nanoseconds evaluated using `std::chrono::steady_clock`.
4. **PMU Availability Requirement:** Before running benchmark iterations, the test harness MUST execute a PMU availability check. If `perf_event_open` returns an error (`EACCES`, `EPERM`, `ENOENT`), the benchmark MUST set the result disposition to `BLOCKED` (Reason: `PMU_UNAVAILABLE`). Fabricating, estimating, or substituting instruction counts with proxy formulas is strictly prohibited.

---

### Protocol Section 9: Confounders, Unprivileged Execution, & Controls

* **CPU Execution Environment & Privileges:**
  * *Privileged Mode (Preferred):* Probes sysfs write access to `/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`. If write access is permitted, pins governor to `performance` and disables Intel Turbo Boost / AMD Core Performance Boost.
  * *Unprivileged Mode (Fallback):* If sysfs write access is restricted (e.g. standard user account or CI container), the harness logs `GOVERNOR_PINNING_SKIPPED_UNPRIVILEGED`, locks the benchmark thread to a dedicated CPU core using `sched_setaffinity` (or `pthread_setaffinity_np`), and executes 10 warm-up cycles to force CPU frequency scaling to max steady state before taking measurements. Benchmark execution MUST NOT fail due to unprivileged environment constraints.
* **Warmup Pass:** $1,000,000$ unmeasured iterations executed before timing runs to warm L1 instruction/data caches and branch target buffers (BTB).
* **Compiler Optimization Parity:** All implementations placed in separate translation units with `__attribute__((noinline))` wrappers to prevent interprocedural constant folding from masking real branch instructions.

---

### Protocol Section 10: Statistical Decision Rules & Four Mutually Exclusive Result Dispositions

#### Statistical Framework
* **Repetitions:** $R = 10$ independent measurement runs per pattern and baseline.
* **Metrics:** Compute sample mean $\mu$, standard deviation $\sigma$, and $95\%$ Confidence Intervals ($95\%\text{ CI} = \mu \pm 1.96 \cdot \frac{\sigma}{\sqrt{R}}$) for $I_{\text{total}}$, $B_{\text{mispredict}}$, and $T_{\text{latency}}$.
* **Hypothesis Testing:** Apply Welch's two-sample t-test between candidate CMP3 and each binary baseline; significance threshold $p < 0.01$.

#### Mutually Exclusive Result Dispositions & Precedence Rules
The experimental evaluation produces **exactly four mutually exclusive experimental dispositions**, evaluated strictly according to the following precedence hierarchy:

```
+-----------------------------------------------------------------------------------+
|                        PRECEDENCE HIERARCHY OF DISPOSITIONS                       |
+-----------------------------------------------------------------------------------+
| 1. BLOCKED      (Infrastructure failure, oracle error, missing PMU, budget limit)  |
| 2. INCONCLUSIVE (Statistical p >= 0.01, high variance, or 0% < delta < 15%)        |
| 3. SUPPORTED    (Statistically significant p < 0.01 AND instruction drop >= 15%)    |
| 4. NOT SUPPORTED(Statistically significant p < 0.01 AND instruction drop < 0%)    |
+-----------------------------------------------------------------------------------+
```

1. **`BLOCKED` (Highest Precedence — Execution or Infrastructure Failure):**
   * *Trigger Conditions:* Triggered IF any of the following occur during run setup or execution:
     * Oracle correctness check fails on any boundary vector or test case (`CORRECTNESS_FAILURE`).
     * Hardware performance counters are inaccessible or restricted by OS security settings (`PMU_UNAVAILABLE`).
     * Benchmark runtime exceeds the 10 CPU-minute budget or 500 MB memory limit (`BUDGET_EXCEEDED`).
     * Target compiler or platform environment fails to generate valid executables.

2. **`INCONCLUSIVE` (Second Precedence — Statistical Uncertainty or Sub-Threshold Effect):**
   * *Trigger Conditions:* Triggered IF measurements completed without infrastructure failure (`BLOCKED`), BUT:
     * Welch's t-test fails to achieve statistical significance ($p \ge 0.01$) across repetitions for any primary metric, OR
     * Sample variance is too high ($95\%\text{ CI}$ spans across baseline comparison thresholds), OR
     * Candidate achieves a statistically significant ($p < 0.01$) dynamic instruction count reduction over Binary Baseline 1 that falls between $0\%$ and $15\%$ ($0.85 \cdot I_{\text{base1}} < I_{\text{cand}} \le 1.0 \cdot I_{\text{base1}}$) with $B_{\text{cand}} \le B_{\text{base1}}$, failing to satisfy the preregistered $15\%$ effect threshold while demonstrating positive direction.
   * *Non-Significance Rule:* A non-significant result ($p \ge 0.01$) MUST NOT be automatically classified as `NOT SUPPORTED`; it MUST be classified as `INCONCLUSIVE`.

3. **`SUPPORTED` (Third Precedence — Preregistered Hypothesis Validated):**
   * *Trigger Conditions:* Triggered IF measurements complete without error AND all of the following conditions hold with statistical significance ($p < 0.01$):
     * Candidate achieves at least $15\%$ dynamic instruction count reduction compared to Binary Baseline 1 ($I_{\text{cand}} \le 0.85 \cdot I_{\text{base1}}$).
     * Candidate branch misprediction count is non-inferior to Binary Baseline 1 ($B_{\text{cand}} \le B_{\text{base1}}$).
     * Candidate total execution latency is non-inferior to the fastest binary baseline ($T_{\text{cand}} \le T_{\text{base\_fastest}}$).

4. **`NOT SUPPORTED` (Fourth Precedence — Preregistered Hypothesis Falsified):**
   * *Trigger Conditions:* Triggered IF measurements complete with statistical significance ($p < 0.01$), BUT:
     * Dynamic instruction count fails to achieve any reduction over Binary Baseline 1 ($I_{\text{cand}} > 1.0 \cdot I_{\text{base1}}$), OR
     * Branch misprediction count is significantly higher ($B_{\text{cand}} > B_{\text{base1}}$), OR
     * Execution latency is significantly higher than all binary baselines ($T_{\text{cand}} > T_{\text{base\_fastest}}$).

---

### Protocol Section 11: Determinism Surface Registry Bounding

In accordance with T81 governance guidelines, all claims regarding experimental determinism, pseudo-random generator reproducibility, state initialization, and result verification vectors are strictly bounded by the Determinism Surface Registry (`spec/determinism_registry.json`).

---

### Protocol Section 12: Explicit Separation of Conclusion Domains

To maintain strict scientific integrity, three possible experimental conclusions are explicitly separated into distinct domains. **Inferring a conclusion in one domain from a result in another is strictly forbidden:**

```
+-----------------------------------------------------------------------------------+
|                        THREE INDEPENDENT CONCLUSION DOMAINS                       |
+-----------------------------------------------------------------------------------+
| Domain A: Mathematical Cost Model Advantage                                       |
| - Defined purely in abstract gate counts, information theory, or ISA ops.          |
| - A positive result in Domain A DOES NOT imply speedup on binary hardware.         |
+-----------------------------------------------------------------------------------+
| Domain B: Existing Binary Hardware Execution Advantage                            |
| - Defined purely in measured clock cycles, IPC, and perf counters on x86/ARM CPUs.|
| - A negative result in Domain B DOES NOT invalidate native hardware potential.    |
+-----------------------------------------------------------------------------------+
| Domain C: Potential Future Native Ternary Hardware Advantage                      |
| - Defined in SPICE/RTL gate-level simulations of native 3-state devices.          |
| - Requires physical device characterization before asserting claims.              |
+-----------------------------------------------------------------------------------+
```

---

## Research Disposition & Next Action

* **Research Disposition:** **READY FOR IMPLEMENTATION** (Protocol design fully finalized and audit-corrected; ready for code implementation when authorized).
* **Implementation Plan:** Upon separate authorization, implement benchmark test harness in `experiments/cmp3_decision_advantage/` in strict accordance with Protocol `EXP-T81-COMP3-01`.
