# Research Design — T81 Representation Advantage Candidate Study

**Document ID:** `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`
**Date:** October 2026
**Status:** Frozen Research Design / Preregistration
**Authors:** Jules (Software Engineer — Scientific Research & Adversarial Synthesis)
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Research Stage:** Post H-CONV-01
**Scope Boundary:** Research protocol and candidate ranking design only; no production code, runtime features, or benchmark execution included in this assignment.

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
**Preregistered Status:** FROZEN — READY FOR AUTHORIZATION
**Title:** Bounded Trichotomy Decision Logic & Branch Efficiency Experiment

---

### Protocol Section 1: Research Question & Falsifiable Hypothesis

* **Research Question:** Does a three-valued comparison and dispatch primitive (`CMP3` + 3-way target branch) achieve lower instruction count, fewer branch mispredictions, or reduced latency compared to an optimized binary branch cascade or CMOV sequence when evaluating trichotomy decision workloads ($A < B, A = B, A > B$)?
* **Falsifiable Hypothesis ($H_1$):** For a sequence of $N$ trichotomy evaluations with non-zero equality probability ($P(=) \ge 0.05$), a native 3-way comparison and indirect branch dispatch sequence executes at least $15\%$ fewer CPU instructions and incurs no additional branch mispredictions compared to an optimized binary dual-jump branch cascade (`CMP` + `JL` + `JE`).
* **Null Hypothesis ($H_0$):** The instruction count and branch misprediction rate of 3-way comparison dispatch is equal to or greater than that of the optimized binary baseline.

---

### Protocol Section 2: Exact Workload Definition

The workload evaluates $N = 10,000,000$ sequential trichotomy decisions across three representative algorithm patterns:
1. **Pattern A (Balanced Trichotomy Search):** Binary search / 3-way search comparison loop over a sorted array ($P(<) = 0.475, P(=) = 0.05, P(>) = 0.475$).
2. **Pattern B (Policy Verdict Evaluation):** Security threshold check evaluating state variable $V$ against lower bound $L$ and upper bound $U$ ($P(\text{DENY}) = 0.20, P(\text{AUDIT}) = 0.10, P(\text{ALLOW}) = 0.70$).
3. **Pattern C (Adversarial Branch Noise):** Uniformly random trichotomy sequence ($P(<) = 0.333, P(=) = 0.333, P(>) = 0.333$) designed to stress-test branch predictor hardware.

---

### Protocol Section 3: Balanced-Ternary Implementation Contract

The candidate ternary decision contract defines a clean 3-way dispatch abstraction without external dependencies:
* **Primitive Signature:** `int32_t cmp3_s64(int64_t a, int64_t b)` returning strictly $-1$ if $a < b$, $0$ if $a == b$, $+1$ if $a > b$.
* **Dispatch Table Contract:** A 3-element jump table or `switch`-based 3-way target jump indexed directly by `cmp3_s64(a, b) + 1` (mapping $\{-1, 0, +1\} \to \{0, 1, 2\}$).
* **Deterministic Contract:** Must hold zero side effects, zero heap allocations, and zero floating-point operations.

---

### Protocol Section 4: Matched Binary Baseline

To ensure a strict, un-strawmanned control, two binary baselines are constructed:
1. **Binary Baseline 1 (Dual Branch Cascade):** Standard C/C++ nested conditional branch pattern:
   ```cpp
   if (a < b) { action_less(); }
   else if (a == b) { action_equal(); }
   else { action_greater(); }
   ```
2. **Binary Baseline 2 (Branchless CMOV / C++20 `operator<=>`):**
   ```cpp
   auto res = (a > b) - (a < b); // Signed result in {-1, 0, +1}
   // Followed by direct indexed array dispatch
   ```
Both baselines are compiled with identical compiler flags (`-O3 -fomit-frame-pointer`) on identical toolchains.

---

### Protocol Section 5: Correctness Oracle

* **Oracle Implementation:** Double-precision floating-point reference comparator `oracle_cmp(a, b)` returning exact trichotomy classification.
* **Verification Rule:** $100\%$ exact match required across all $N$ test cases:
  $$\forall i \in [1, N], \quad \text{CandidateResult}_i \equiv \text{OracleResult}_i$$
Any single classification failure immediately invalidates the run.

---

### Protocol Section 6: Input Corpus & Boundary Cases

The test corpus comprises $10,000,000$ 64-bit signed integer pairs per pattern, including mandatory boundary cases:
* Identical values ($a == b$, including `INT64_MAX == INT64_MAX`, `INT64_MIN == INT64_MIN`, $0 == 0$).
* Extreme opposite bounds ($a = \text{INT64\_MIN}, b = \text{INT64\_MAX}$).
* Off-by-one boundary pairs ($a = X, b = X + 1$).
* Pseudo-random inputs generated via deterministic PCG64 seed (`0xT81_CMP3_SEED`).

---

### Protocol Section 7: Performance & Complexity Metrics

1. **Dynamic Instruction Count ($I_{\text{total}}$):** Retired instructions measured via Linux `perf_event_open` hardware counters (`PERF_COUNT_HW_INSTRUCTIONS`).
2. **Branch Misprediction Count ($B_{\text{mispredict}}$):** Branch mispredictions measured via `PERF_COUNT_HW_BRANCH_MISPREDICTIONS`.
3. **Execution Latency ($T_{\text{latency}}$):** Nanoseconds per comparison decision evaluated using monotonic CPU clock (`std::chrono::steady_clock`).
4. **Instruction Fetch Efficiency ($I_{\text{fetch}}$):** Retired instructions per clock cycle (IPC).

---

### Protocol Section 8: Confounders & Controls

* **CPU Frequency Scaling:** CPU governor pinned to `performance` mode; Turbo Boost disabled.
* **Warmup Pass:** 1,000,000 unmeasured iterations executed before timing runs to warm L1 instruction/data caches and branch target buffers.
* **Compiler Optimization Parity:** All implementations placed in separate translation units with `__attribute__((noinline))` wrappers to prevent interprocedural constant folding from masking real branch instructions.
* **Determinism Language Bounding:** Any claims regarding execution determinism are strictly bounded by the Determinism Surface Registry (`spec/determinism_registry.json`) in accordance with T81 governance guidelines.

---

### Protocol Section 9: Success & Falsification Thresholds

* **Success Threshold (Candidate Advantage Confirmed):**
  $$I_{\text{candidate}} \le 0.85 \cdot I_{\text{binary\_baseline1}} \quad \text{AND} \quad B_{\text{mispredict\_candidate}} \le B_{\text{mispredict\_binary\_baseline1}}$$
  (At least 15% fewer retired instructions with equal or lower branch mispredictions).

* **Falsification Threshold (Candidate Advantage Rejected):**
  $$I_{\text{candidate}} > 0.95 \cdot I_{\text{binary\_baseline1}} \quad \text{OR} \quad T_{\text{latency\_candidate}} \ge T_{\text{latency\_binary\_baseline2}}$$
  (Candidate fails to achieve $>5\%$ instruction reduction over dual-branch cascade, or executes slower than branchless binary `CMOV`).

---

### Protocol Section 10: Execution Budget & Stopping Criteria

* **Execution Budget:** Maximum 10 CPU minutes total benchmark runtime; maximum 500 MB memory footprint.
* **Hard Stopping Criteria:**
  1. **Correctness Failure:** Any single failure against the correctness oracle halts execution immediately (**DISPOSITION: REJECTED**).
  2. **Instruction Tax Threshold:** If 3-way dispatch requires $>1.20\times$ the instruction count of branchless binary `CMOV` on Pattern A, benchmark terminates immediately (**DISPOSITION: FALSIFIED**).
  3. **Target Threshold Met:** If success thresholds are satisfied with statistical significance ($p < 0.001$, 5 independent runs), experiment completes successfully (**DISPOSITION: VALIDATED**).

---

## Explicit Separation of Conclusion Domains

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

* **Research Disposition:** **READY FOR AUTHORIZATION** (Design complete; awaiting formal user authorization before code implementation).
* **Implementation Plan:** Upon separate authorization, implement benchmark test harness in `experiments/cmp3_decision_advantage/` in strict accordance with Protocol `EXP-T81-COMP3-01`.
