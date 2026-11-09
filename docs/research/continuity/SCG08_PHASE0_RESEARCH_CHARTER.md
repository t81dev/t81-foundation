# SCG-08 Phase 0: Architectural Advantage Research Charter

**Document ID:** `T81-SCG-08-PHASE0-CHARTER-001-R2`
**Classification:** RESEARCH CHARTER & EVIDENCE RECONCILIATION
**Task Classification:** Research Design, Evidence Harmonization & Experiment Preregistration
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Pinned Baseline Commit SHA:** `cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3` (`origin/main`)
**Preceding Milestones:** SCG-07 Phase 6 Consolidated Arithmetic Qualification (`8dec43ac2a9a702c9ae54229cf6891ee63f285c0`, PR #741); AI Context Continuity Refresh (`cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3`, PR #742)
**Status:** COMPLETE — READY FOR MAINTAINER DECISION

---

## 1. Executive Research Objective

The primary objective of SCG-08 Phase 0 is to establish a rigorous, falsifiable, and evidence-grounded research charter investigating whether balanced-ternary representation and arithmetic can offer measurable computational, architectural, or structural advantages over conventional binary computation.

Following the successful completion of the SCG-07 arithmetic qualification program (Phases 1 through 6, culminating in PR #741), the T81 core numeric foundation (`T81Int`, `T81Limb27`, `T81Limb54`, `T81BigInt`, and `T81Fraction`) has been verified correct under documented contracts. However, arithmetic correctness does not establish architectural superiority. This charter establishes:
1. An adversarial reconciliation of all prior experimental findings and negative dispositions across the T81 project lineage.
2. A representation-neutral mathematical cost model that cleanly separates mathematical identities (such as radix economy $\log_2 3 \approx 1.58496$) from binary CPU instruction overhead and physical semiconductor scaling realities.
3. Explicit derivation of the complete 3-state balanced-ternary carry Markov chain, distinguishing carry generation, occurrence, continuation, cancellation, conditional run length, and stationary carry distribution from binary addition dynamics.
4. An accurate implementation cost model distinguishing actual C++ class memory layouts (`alignas(16) int8_t trytes_[TRYTES]`) from logical digit capacity and temporary accumulators.
5. Explicit mapping to the authoritative `H01`–`H08` central hypothesis register without introducing conflicting renumbering.
6. A small set of prioritized, genuinely unresolved research questions with matched binary controls and preregistered falsification criteria.
7. Recommendation and full preregistration specification for one narrowly scoped next experiment (`EXP-SCG08-01`) executable with available repository resources.

This charter operates strictly within research-design boundaries: no production arithmetic modifications, no ISA changes, no unapproved benchmark executions, and no speculative hardware claims are authorized.

---

## 2. Pinned Repository Baseline

### 2.1 Pinned Commit SHA
- **Baseline Git Commit SHA:** `cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3` (`origin/main` as of 2026-11-09).
- **Integrated Pull Requests on Baseline:**
  - **PR #741:** Commit `8dec43ac2a9a702c9ae54229cf6891ee63f285c0` (SCG-07 Phase 6 consolidated arithmetic qualification).
  - **PR #742:** Commit `cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3` (AI Context status and handoff refresh for 2026-11-09).

### 2.2 Preceding SCG-07 Remediation & Qualification Sequence
The foundation baseline integrates the full sequence of SCG-03 and SCG-07 arithmetic defect repairs and verifications:
- **PR #729 (`2c3501e2`):** SCG-03 C05 `T81Int<N>::operator*` 2N-trit convolution with balanced-ternary carry normalization and `std::overflow_error` detection; C11 fail-closed test assertions.
- **PR #736 (`e24eba72`):** SCG-07 Phase 1 F6 `T81Fraction` exception safety (removing invalid `noexcept` boundaries) and F7 `to_double()` accumulator loop eliminating termination.
- **PR #737 (`1604fd20`):** SCG-07 Phase 2 F5 `T81BigInt` signed subtraction repair enforcing non-negative limb storage invariant under single-limb borrow.
- **PR #738 (`5cc8d945`):** SCG-07 Phase 3 F1 `T81Limb27::operator*` Booth table repair replacing tryte-scaled partial products with exact balanced-ternary multiply-accumulate and carry normalization into 54 trits.
- **PR #739 (`8497ffb8`):** SCG-07 Phase 4 F2 `T81Limb54::reference_mul` carry normalization repair eliminating non-canonical trit emissions.
- **PR #740 (`ecb1e32b`):** SCG-07 Phase 5 F3 `T81Limb54::karatsuba` verification proving intermediate carry discard invariance modulo $3^{54}$.
- **PR #741 (`8dec43ac`):** SCG-07 Phase 6 consolidated arithmetic qualification (`t81_phase6_qualification_test`) against decoupled `ExactBigIntOracle` (base $10^9$).

### 2.3 Audited Arithmetic Limitations
The qualification disposition is explicitly **QUALIFIED WITH LIMITATIONS** under Document ID `T81-SCG-07-PHASE6-QUALIFICATION-001-R1`:
1. **F3 Causal Attribution Boundary:** Finding F3 (`T81Limb54::karatsuba`) is mathematically and empirically verified correct under the 54-trit modular residue contract ($A \times B \pmod{3^{54}}$). Intermediate 28th-trit carry discard produces zero numerical error modulo $3^{54}$ because intermediate carries scale by $3^{27}$ in $Z_1$, which when shifted left by 27 trits (9 trytes) have positional weight $\ge 3^{54} \equiv 0 \pmod{3^{54}}$. However, historical causal attribution of the 299/300 SCG-06 failures remains incomplete; F1 Booth table scaling is the leading explanation, but exclusive attribution cannot be asserted without an isolated before/after experiment.
2. **F7 Floating-Point Precision Boundary:** Finding F7 (`T81Fraction::to_double() const noexcept`) is verified exception-safe and numerically bounded across evaluated fraction widths (13, 40, 41, 81, 324 trits) within binary64 floating-point precision (~53 bits mantissa, relative error tolerance $10^{-12}$). Correctly-rounded binary64 conversion remains deferred to RFC-00D5 / SCG-04 D2 and is not claimed under the existing contract.

---

## 3. Historical Evidence Matrix Across Eight Domains

This section records prior experimental findings across eight mandatory technical domains, documenting the source, tested workload, hardware/model, measurement methodology, limitations, and scientific disposition. In accordance with continuity governance, negative results are preserved and not reinterpreted as positive evidence.

| Domain | Source & Commit | Tested Workload | Hardware / Platform | Measurement Methodology | Documented Limitations | Scientific Disposition |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1. Balanced Ternary Arithmetic** | `legacy/hanoivm/`, `src/ternary_arithmetic_optimization.cweb` (`59205d76`); `experiments/radix_comparison/` | CWEB arithmetic routines; software tryte addition loop ($10^7$ iterations) | Intel Xeon @ 2.30 GHz; Linux x86_64 | Isolated compilation against exact integer oracle; cycle-timed microbenchmarks vs 64-bit binary ALU | Hanoi routines failed balanced positional semantics (422/547 add, 140/201 mul); componentwise math; tryte add took 11.7 ns vs 1.1 ns binary ALU | **REJECTED (Hanoi CWEB)** for positional math; **FALSIFIED & FROZEN (STOP)** for software CPU ternary arithmetic speedup ($10.6\times$ slower). |
| **2. Binary vs Ternary Representation** | `docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md`; `probes/storage_boundary.py` | Theoretical $2 \times 2$ factorial capacity model (FB, AB, FT, AT) | Mathematical model / Python script | Analytic derivation of capacity steps $\lceil \log_b M \rceil$ and cost ratio thresholds $C_3/C_2$ | Assumes uniform element cost; finite capacity quantization creates counterexamples (e.g. $M=256$, $C_3/C_2=1.5$ requires cost 9 vs binary 8) | **CONDITIONAL MODEL SUPPORT ONLY**; theoretical radix density ($\log_2 3 \approx 1.585$) does not guarantee physical or software advantage. |
| **3. Packed Ternary Memory Efficiency** | `experiments/packed_ternary_matvec/` (`f6d4d680`); `PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md` | $4096 \times 4096$ matrix storage; 2-bit packing (4 trits/byte) vs INT8 / FP32 | Intel Xeon @ 2.30 GHz (4 cores, 45MB L3, AVX2) | Exact byte accounting of allocated weight tensors | Validated memory reduction ($4.19\text{ MB}$ packed vs $16.78\text{ MB}$ INT8, $67.10\text{ MB}$ FP32); decoding cost separated from storage | **CONFIRMED FOR COMPRESSION (75% footprint reduction)**; retained strictly as a compact memory/interchange format. |
| **4. Vector & Matrix Computation** | `experiments/packed_ternary_matvec/`; `TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md` | Matrix-Vector multiplication across Cache-Resident ($64 \times 256$), Medium ($512^2$), DRAM ($4096^2$) | Intel Xeon @ 2.30 GHz, AVX2, FMA; QEMU AArch64 emulation | VPSHUFB SIMD nibble LUT decoding vs INT8 AVX2 (`_mm256_maddubs_epi16`) and FP32 AVX2 | Packed ternary reached $12.91\text{ GFLOP/s}$ on $4096^2$, but capped at $75.3\%$ of INT8 AVX2 ($17.15\text{ GFLOP/s}$); QEMU ARM64 measured translation, not silicon | **FALSIFIED FOR CPU SPEEDUP**; bit-shuffle decode instruction ratio prevents 2-bit packed ternary from beating native INT8 SIMD on binary ALUs. |
| **5. TISC vs Wasm Artifact Efficiency** | `spec/tisc-spec.md` (`bf6dd858`); Investigation 09-E1; `EVIDENCE_AND_FINDINGS.md` | Standardized control algorithm compiled to TISC bytecode vs WebAssembly MVP bytecode | Bytecode structural analysis / disk footprint | Exact byte measurement of conforming binary artifacts | Fixed 13-byte / 81-opcode instruction records vs variable-length LEB128 encoding in Wasm; TISC was 1,240 bytes vs Wasm 157 bytes | **NOT SUPPORTED (Ratio 7.898x larger)**; WebAssembly variable-length binary encoding is significantly more compact than fixed TISC layouts. |
| **6. CPU Instruction & SIMD Overhead** | `experiments/packed_ternary_matvec/`; `experiments/radix_comparison/` | Scalar on-the-fly, SWAR, LUT, and SIMD VPSHUFB decoders | Intel Xeon @ 2.30 GHz, Linux x86_64, GCC 13.3 `-O3` | Hardware performance counters; microbenchmarks separating packing, unpacking, and dot products | Scalar on-the-fly decoding is up to $69\times$ slower than INT8 ($0.24\text{ GFLOP/s}$); pre-packing takes $102.7\text{ ms}$; decode tax dominates ALU cycles | **CONFIRMED OVERHEAD TAX**; software bit manipulation of 2-bit trits on binary microarchitectures imposes an insurmountable instruction tax. |
| **7. Physical Hardware Qualification** | `t81-hardware` (`rtl/core/t81_top.sv` at `834521d4`); Investigation 10A | Verilog RTL synthesis for ternary ALU; semiconductor device survey | ModelSim / Quartus synthesis; physical literature review | Gate synthesis inspection; literature characterization search | Verilog RTL implements trits as 2 binary bits per trit; no native 3-state semiconductor device data (cell area, switching energy, noise margins) | **PHYSICAL EVALUATION BLOCKED**; missing matched 3-state semiconductor characterization; binary RTL provides no ternary device advantage. |
| **8. Hardware-Native Ternary Hypotheses** | `Ternary_IEEE_Draft.md`; Setun historical literature; BitNet b1.58 survey | Theoretical 3-state logic gates (STI, NADI, NORI); ternary DRAM / RRAM | SPICE / theoretical analytical models | Radix economy derivation ($E = e \approx 2.718$; $R=3$ near-optimal); literature comparison | Multiple threshold voltages degrade noise margins; 3-state voltage detection requires complex ADC/sensing circuits; Setun replaced by binary CMOS | **THEORETICAL MERIT UNPROVEN IN MODERN SILICON**; radix economy does not compensate for binary CMOS scaling, lithography, and noise-margin degradation. |

---

## 4. Existing Hypothesis Register Mapping

This charter adheres strictly to the central hypothesis register (`HYPOTHESIS_REGISTER_v1.json`, schema `t81.research-hypothesis-register.v1`). All research activities map to the existing definitions without creating conflicting identifiers.

```
+---------------------------------------------------------------------------------------------------+
|                                  AUTHORITATIVE HYPOTHESIS MAPPING                                 |
+------+--------------------------------------------------------------------+-----------------------+
| ID   | Canonical Register Claim Summary                                    | Current Research Tier |
+------+--------------------------------------------------------------------+-----------------------+
| H01  | Physical storage cost reduction under stated device/overhead models | Tier B / Tier D       |
| H02  | Balanced ternary signed arithmetic or rounding improvement          | Tier A / Tier B       |
| H03  | 4-trit grouping (Base-81) interface or execution advantage         | Tier B / Tier D       |
| H04  | Adaptive ternary representation policy vs fixed/binary policies     | Tier B (H04-R01/R03)  |
| H05  | Three-state primitive circuit or system cost reduction              | Tier B                |
| H06  | Ternary weights/activations in accuracy-constrained inference       | Tier B / Tier C       |
| H07  | Representation-independent governed execution & auditability        | Tier A / Tier B (CPA) |
| H08  | AI-assisted research trustworthiness and discovery under audit      | Tier C / Tier E       |
+------+--------------------------------------------------------------------+-----------------------+
```

### Hypotheses Dispositions in SCG-08 Context:
- **H01 (Physical Storage):** BLOCKED from physical claims due to missing matched 3-state device data (Inv 10A). Retained as an analytical boundary model only.
- **H02 (Arithmetic Advantage):** Core focus of SCG-08. Prior software tryte benchmarks were FALSIFIED on single-word ALU additions. However, multi-precision carry-propagation dynamics across balanced-ternary digit chains vs binary carry chains remain mathematically distinct and require precise qualification.
- **H03 (Grouping & Code Compactness):** FALSIFIED for general ISA code density vs Wasm (Inv 09-E1). Retained strictly for memory compression (75% savings in 2-bit packing) and structured mathematical limb groupings ($3^{27}$ and $3^{54}$).
- **H04 (Adaptive Representation):** Formally refined to H04-R03 (matched codec and retention enforcement). Phase B deterministic correctness verified; Phase C performance measurement remains frozen.
- **H05 (Three-State Primitives):** Theoretical and RTL inspection only; CMP3 instruction protocol frozen pending PMU hardware.
- **H06 (Ternary Inference):** MatVec CPU kernel optimization formally FROZEN (STOP); inference standardized on byte-aligned INT8 SIMD / GGML dequantization pathways.
- **H07 (Governed Execution & CPA):** Proven bounded dual-profile cross-execution in CPA-004/CPA-006; decoupled from ternary arithmetic advantage claims.
- **H08 (AI Methodology):** Methodological process tracking prompt provenance and continuity receipts.

---

## 5. Selected Unresolved Hypotheses

To avoid repeating unpromising CPU kernel micro-optimizations, SCG-08 selects three narrowly focused, genuinely unresolved research questions grounded in mathematical foundations and qualified SCG-07 implementations.

---

### Hypothesis H02-A: Balanced-Ternary Multi-Precision Carry-Chain Length and Propagation Density

- **Formal Mathematical Claim:** In balanced-ternary integer addition $A + B$ where digit positions $a_i, b_i \in \{-1, 0, +1\}$ are independent and uniformly distributed:
  1. The carry generation probability from a quiescent state ($c_{\text{in}} = 0$) is $P(c_{\text{out}} \neq 0 \mid c_{\text{in}} = 0) = \frac{2}{9} \approx 0.2222$.
  2. The carry continuation probability given an active carry ($c_{\text{in}} = \pm 1$) is $P(c_{\text{out}} = \pm 1 \mid c_{\text{in}} = \pm 1) = \frac{1}{3} \approx 0.3333$.
  3. The stationary carry occurrence probability under the complete 3-state Markov transition matrix is $\pi(\text{carry} \neq 0) = \frac{1}{4} = 0.2500$, compared to a stationary carry occurrence probability of $\frac{1}{2} = 0.5000$ in binary addition.
  4. Conditioned on carry generation at digit position $i$, the expected length of a continuous non-zero carry run before absorption ($c_{\text{out}} = 0$) is geometrically distributed with parameter $p = 1 - \frac{1}{3} = \frac{2}{3}$, yielding expected run length:
     $$E[L_{\text{ternary\_run}}] = \sum_{k=1}^\infty k \left(\frac{1}{3}\right)^{k-1} \left(\frac{2}{3}\right) = \frac{1}{1 - 1/3} = \frac{3}{2} = 1.500 \text{ trits}$$
     In contrast, in binary addition, an active carry continues whenever $a_i + b_i \ge 1$, which occurs with probability $P(c_{\text{out}} = 1 \mid c_{\text{in}} = 1) = \frac{3}{4} = 0.7500$, yielding an expected continuous carry run length of:
     $$E[L_{\text{binary\_run}}] = \frac{1}{1 - 3/4} = 4.000 \text{ bits}$$
  5. The total expected number of active non-zero carry events $E[N_{\text{active}}]$ over an $N$-digit addition sequence initialized with quiescent carry $c_0 = 0$ is:
     $$E[N_{\text{active, ternary}}] = \sum_{k=1}^N P(c_k \neq 0 \mid c_0 = 0) = \frac{1}{4} N - \frac{1}{32} \left(1 - \left(\frac{1}{9}\right)^N\right)$$
     In contrast, for an $N$-bit binary addition initialized with $c_0 = 0$:
     $$E[N_{\text{active, binary}}] = \sum_{k=1}^N P(c_k = 1 \mid c_0 = 0) = \frac{1}{2} N - \frac{1}{2} \left(1 - \left(\frac{1}{2}\right)^N\right)$$
- **Independent Binary Baseline:** Standard multi-precision binary addition operating on 64-bit limbs and bit-level carry-propagation chains.
- **Balanced-Ternary Treatment:** Multi-precision addition evaluated on balanced-ternary digit sequences, utilizing the qualified `T81Limb27` and `T81Limb54` digit normalization logic.
- **Controlled Variables:** Total information capacity in bits ($M = 2^k$, for $k \in \{64, 128, 256, 512, 1024\}$), operand distribution (uniform random over range $[-(M-1)/2, (M-1)/2]$), and deterministic PRNG seeds.
- **Primary Outcome Metrics & Exact Experimental Counters:**
  1. **Conditional Carry Generation Probability ($P_{\text{gen}|0}$):** Measured as $f_{\text{gen}|0} = \frac{N(\text{quiescent-to-active transitions})}{N(\text{digits with incoming carry } c_{\text{in}} = 0)}$. Evaluates the conditional transition probability $P(c_{\text{out}} \neq 0 \mid c_{\text{in}} = 0)$.
  2. **Stationary Quiescent-to-Active Transition Frequency ($f_{\text{trans}}$):** Measured as $f_{\text{trans}} = \frac{N(\text{quiescent-to-active transitions})}{N(\text{total digits})}$. Theoretical expectation is $\pi(0) \cdot P_{\text{gen}|0} = \frac{3}{4} \cdot \frac{2}{9} = \frac{1}{6} \approx 0.1667$ for ternary, compared to $\pi(0) \cdot P_{\text{gen}|0} = \frac{1}{2} \cdot \frac{1}{4} = \frac{1}{8} = 0.1250$ for binary.
  3. **Stationary Active-Carry Occurrence Frequency ($f_{\text{occ}}$):** Measured as $f_{\text{occ}} = \frac{N(\text{digits with active outgoing carry } |c_{\text{out}}| = 1)}{N(\text{total digits})}$. Theoretical expectation is $\pi(\text{carry} \neq 0) = \frac{1}{4} = 0.2500$ for ternary, compared to $\frac{1}{2} = 0.5000$ for binary.
  4. **Conditional Carry Continuation Probability ($P_{\text{cont}|\text{act}}$):** Measured as $f_{\text{cont}|\text{act}} = \frac{N(\text{active-to-active continuations } c_{\text{out}} = c_{\text{in}})}{N(\text{digits with incoming active carry } |c_{\text{in}}| = 1)}$. Theoretical expectation is $\frac{1}{3} \approx 0.3333$ for ternary, compared to $\frac{3}{4} = 0.7500$ for binary.
  5. **Average Conditional Carry Run Length ($\bar{L}_{\text{run}}$):** Measured as $\bar{L}_{\text{run}} = \frac{\sum \text{run lengths}}{N(\text{carry runs})}$. Theoretical expectation is $1.500$ trits for ternary, compared to $4.000$ bits for binary.
- **Explicit Success Thresholds:**
  - $f_{\text{gen}|0} = \frac{2}{9} \pm 0.005$ ($[0.217, 0.227]$).
  - $f_{\text{trans}} = \frac{1}{6} \pm 0.005$ ($[0.161, 0.172]$).
  - $f_{\text{occ}} = \frac{1}{4} \pm 0.005$ ($[0.245, 0.255]$).
  - $f_{\text{cont}|\text{act}} = \frac{1}{3} \pm 0.005$ ($[0.328, 0.338]$).
  - $\bar{L}_{\text{run}} = 1.500 \pm 0.05$ trits ($[1.450, 1.550]$).
- **Explicit Failure Thresholds:**
  - Stationary carry occurrence frequency $f_{\text{occ}}$ exceeds $0.300$.
  - Carry occurrence per bit of information ($\frac{f_{\text{occ}}}{\log_2 3} = \frac{0.250}{1.585} \approx 0.1577$) is found to exceed or match binary carry occurrence per bit ($0.500$).
- **Confounding Factors:**
  - Non-uniform operand distributions (e.g., dense zero trytes).
  - Tryte-level composition vs digit-level scalar carry transitions.
- **Required Instrumentation:** Hardware-independent exact carry-event counter tracking digit-level transition states.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Rigorously validates the theoretical Markov carry-reduction bound for balanced ternary.
  - *Negative Outcome:* Demonstrates that carry-cancellation dynamics in practical multi-precision implementations fail to deliver the modeled reduction.

---

### Hypothesis H03-A: Tryte Limb Memory Organization vs Binary 64-Bit Host Efficiency

- **Formal Mathematical Claim:** In multi-precision balanced-ternary arithmetic implemented via tryte-based limbs (`T81Limb27` with 9 trytes, and `T81Limb54` with 18 trytes stored as `alignas(16) int8_t trytes_[]`), the host memory storage footprint and memory-bus traffic exceed those of equivalent-capacity binary multi-precision limbs on 64-bit microprocessors, creating a structural host impedance mismatch that cannot be eliminated by algorithmic optimizations alone.
- **Independent Binary Baseline:** Standard multi-precision integer limbs stored as packed arrays of 64-bit unsigned words (`std::uint64_t`).
- **Balanced-Ternary Treatment:** The qualified SCG-07 `T81Limb27` (16 bytes footprint for 27 trits $\approx 42.79$ bits) and `T81Limb54` (32 bytes footprint for 54 trits $\approx 85.59$ bits).
- **Controlled Variables:** Equivalent mathematical value domains ($N \in \{128, 256, 512, 1024\}$ bits equivalent), cache line boundary alignments, memory access patterns.
- **Primary Outcome Metric:**
  1. Storage expansion ratio: $\rho_{\text{storage}} = \frac{\text{Bytes}_{\text{T81Limb}}}{\text{Bytes}_{\text{Binary64}}}$.
  2. Measured cache misses and memory traffic per multi-precision arithmetic pass.
- **Explicit Success Threshold:** $\rho_{\text{storage}}$ matches the theoretical analytical expectation:
  $$\rho_{\text{storage}}(27\text{ trits}) = \frac{16\text{ bytes}}{8\text{ bytes}} = 2.00\times \text{ (relative to 64-bit capacity)}$$
  $$\text{Effective bit density: } \frac{42.79\text{ bits}}{128\text{ bits allocated}} \approx 33.43\%$$
- **Explicit Failure Threshold:** If software packing schemes claim memory density advantages without accounting for 16-byte SIMD structure alignment padding.
- **Confounding Factors:** SIMD register loading alignment; compiler padding.
- **Required Instrumentation:** Binary struct sizeof audit and memory access instrumentation.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Documents the exact structural cost of tryte-array storage on byte-addressable microprocessors.
  - *Negative Outcome:* Shows whether alternative scalar-packed limb designs could mitigate storage overhead.

---

### Hypothesis H01/H05-A: Device Parameterized Energy-Delay Product Sensitivity Model

- **Formal Mathematical Claim:** The Energy-Delay Product ($\text{EDP} = E_{\text{cycle}} \times \tau_{\text{delay}}$) of a hypothetical native 3-state logic cell operating at voltage levels $\{0, V_{dd}/2, V_{dd}\}$ is governed by the multi-parameter system:
  $$\text{EDP}_3 = \left( \alpha_3 C_{\text{eff},3} V_{dd,3}^2 + I_{\text{leak},3} V_{dd,3} \tau_3 + E_{\text{periph},3} \right) \times \left( \frac{C_{\text{load},3} V_{dd,3}}{I_{\text{on},3}} + \tau_{\text{sensing},3} \right)$$
  compared to the binary CMOS baseline:
  $$\text{EDP}_2 = \left( \alpha_2 C_{\text{eff},2} V_{dd,2}^2 + I_{\text{leak},2} V_{dd,2} \tau_2 \right) \times \left( \frac{C_{\text{load},2} V_{dd,2}}{I_{\text{on},2}} \right)$$
  A native ternary advantage ($\text{EDP}_3 < \text{EDP}_2$) cannot be asserted from radix economy alone, because maintaining identical static noise margin ($\text{SNM} \ge V_{\text{noise}}$) in a 3-state system requires dividing the available supply rail into multiple transition windows $\Delta V = V_{dd}/2$, which degrades switching overdrive $(V_{gs} - V_{\text{th}})$ and increases sensing delay.
- **Independent Binary Baseline:** Standard binary CMOS inverter and full-adder cells at established technology nodes (e.g., 7nm / 14nm FinFET) with standard static noise margins ($\text{SNM}_2 \approx V_{dd}/5$).
- **Balanced-Ternary Treatment:** Parameterized multi-threshold ternary logic models reported in device literature (e.g., CNTFET or multi-threshold CMOS).
- **Controlled Variables:** Technology feature size, operating temperature ($300\text{ K}$), target bit-error-rate (BER).
- **Primary Outcome Metric:** Ratio $\frac{\text{EDP}_3}{\text{EDP}_2}$ across a multi-dimensional parameter sweep over device capacitance ratio $\kappa_C = C_3/C_2$, intermediate threshold degradation factor $\beta = \Delta V_{\text{th}}/V_{dd}$, and peripheral sensing overhead $E_{\text{periph}}$.
- **Explicit Falsification Threshold:** If maintaining equal noise margins requires supply voltage scaling or sensing delays such that $\text{EDP}_3 > \text{EDP}_2$ across the entire physically realizable parameter space ($\kappa_C \ge 1.0, \beta \ge 0.3$), the claim of native ternary circuit advantage is formally **UNSUPPORTED**.
- **Confounding Factors & Assumptions:**
  - Assumption 1: Planar/FinFET CMOS scaling behaviors apply to multi-threshold devices.
  - Assumption 2: Interconnect parasitics scale with cell area.
  - Unsupported Parameter: Exact 3-state device switching energy is unmeasured in T81 repositories.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Identifies the boundary conditions required for non-silicon devices (e.g., CNTFETs or memristors) to compete with CMOS.
  - *Negative Outcome:* Formally closes speculative hardware advantage claims within the project.

---

## 6. Representation-Neutral Mathematical Cost Model

To evaluate balanced ternary objectively, we construct a representation-neutral cost model that separates mathematically invariant properties from software emulation overhead and speculative hardware parameters.

### 6.1 Fundamental Mathematical Properties

Let $M \in \mathbb{N}$ be the number of distinct representable states.
1. **Information Content:**
   $$I = \log_2 M \quad \text{[bits]}$$
2. **Required Digit Counts:**
   $$\text{Binary Bits: } B = \lceil \log_2 M \rceil$$
   $$\text{Ternary Trits: } T = \lceil \log_3 M \rceil$$
   $$\text{Base-81 Trytes (4 trits): } Y = \lceil T / 4 \rceil$$
3. **Theoretical Radix Economy:**
   The complexity of an $R$-state digit system is modeled as $C(R, M) = k \cdot R \cdot \lceil \log_R M \rceil$.
   Evaluating the continuous function $f(R) = R / \ln R$:
   $$f(2) = \frac{2}{\ln 2} \approx 2.8854$$
   $$f(3) = \frac{3}{\ln 3} \approx 2.7307 \quad (\approx 5.36\% \text{ lower than binary})$$
   $$f(4) = \frac{4}{\ln 4} \approx 2.8854$$
   *Mathematical Invariant:* Base 3 is the integer radix with the highest theoretical radix economy. However, this optimum assumes that an $R$-state physical device has cost strictly proportional to $R$, which does not hold in planar binary CMOS.

4. **Balanced-Ternary Value Representation:**
   For digits $d_i \in \{-1, 0, +1\}$:
   $$V = \sum_{i=0}^{T-1} d_i \cdot 3^i, \quad V \in \left[ -\frac{3^T - 1}{2}, \, +\frac{3^T - 1}{2} \right]$$
   - *Inherent Sign Representation:* Negation is strictly componentwise with zero carry propagation: $-V = \sum_{i=0}^{T-1} (-d_i) \cdot 3^i$.
   - *Absence of Sign Asymmetry:* Range is perfectly symmetric around zero; no counterpart to binary two's complement asymmetry ($-\text{INT\_MIN}$ overflow).

### 6.2 Complete Three-State Carry Markov Chain Derivation

In single-digit balanced-ternary addition:
$$s_i = a_i + b_i + c_{\text{in}}$$
where inputs $a_i, b_i \in \{-1, 0, +1\}$ are independent, identically distributed uniform random variables with $P(a_i = k) = \frac{1}{3}$ and $P(b_i = k) = \frac{1}{3}$ for $k \in \{-1, 0, +1\}$, and $c_{\text{in}} \in \{-1, 0, +1\}$.
The balanced-ternary carry $c_{\text{out}}$ and canonical sum digit $r_i$ are uniquely defined by:
$$s_i = 3 \cdot c_{\text{out}} + r_i, \quad r_i \in \{-1, 0, +1\}, \, c_{\text{out}} \in \{-1, 0, +1\}$$

#### 1. Transition Probabilities by Incoming State

Let $\Sigma_{a,b} = a_i + b_i$. The 9 equiprobable input pairs $(a_i, b_i)$ have the sum distribution:
- $\Sigma_{a,b} = -2$: 1 pair $(-1, -1)$ (prob $1/9$)
- $\Sigma_{a,b} = -1$: 2 pairs $(-1, 0), (0, -1)$ (prob $2/9$)
- $\Sigma_{a,b} = 0$: 3 pairs $(-1, 1), (0, 0), (1, -1)$ (prob $3/9$)
- $\Sigma_{a,b} = +1$: 2 pairs $(0, 1), (1, 0)$ (prob $2/9$)
- $\Sigma_{a,b} = +2$: 1 pair $(1, 1)$ (prob $1/9$)

**Transition Dynamics Summary Table:**
| Metric | Balanced Ternary | Binary Unsigned | Analytical Interpretation |
| :--- | :---: | :---: | :--- |
| **Conditional Generation: $P(c_{\text{out}} \neq 0 \mid c_{\text{in}} = 0)$** | $\mathbf{2/9} \approx 0.2222$ | $\mathbf{1/4} = 0.2500$ | Probability of triggering a new carry from a quiescent digit |
| **Stationary Zero-to-Active Transition Frequency** | $\mathbf{1/6} \approx 0.1667$ | $\mathbf{1/8} = 0.1250$ | Unconditional per-digit frequency of quiescent $\to$ active transitions: $\pi(0) \cdot P_{\text{gen}|0}$ |
| **Stationary Active Carry Occurrence: $\pi(c \neq 0)$** | $\mathbf{1/4} = 0.2500$ | $\mathbf{1/2} = 0.5000$ | Steady-state fraction of digits emitting an active carry |
| **Conditional Continuation: $P(c_{\text{out}} = c_{\text{in}} \mid c_{\text{in}} \neq 0)$** | $\mathbf{1/3} \approx 0.3333$ | $\mathbf{3/4} = 0.7500$ | Probability that an existing carry propagates past current digit |
| **Conditional Carry Cancellation / Absorption** | $\mathbf{2/3} \approx 0.6667$ | $\mathbf{1/4} = 0.2500$ | Probability that an existing carry is absorbed: $1 - P_{\text{cont}}$ |
| **Expected Conditional Carry Run Length** | $\mathbf{1.500} \text{ trits}$ | $\mathbf{4.000} \text{ bits}$ | Mean run length once generated: $1 / (1 - P_{\text{cont}})$ |

**Case 1: Quiescent State ($c_{\text{in}} = 0$)**
- $s_i = \Sigma_{a,b} \in \{-2, -1, 0, 1, 2\}$.
- $s_i = -2 \implies c_{\text{out}} = -1, r_i = +1$ (prob $1/9$).
- $s_i \in \{-1, 0, 1\} \implies c_{\text{out}} = 0, r_i = s_i$ (prob $7/9$).
- $s_i = +2 \implies c_{\text{out}} = +1, r_i = -1$ (prob $1/9$).
- **Carry Generation Probability:**
  $$P(c_{\text{out}} \neq 0 \mid c_{\text{in}} = 0) = P(c_{\text{out}} = -1) + P(c_{\text{out}} = +1) = \frac{1}{9} + \frac{1}{9} = \frac{2}{9} \approx 0.2222$$

**Case 2: Positive Active Carry ($c_{\text{in}} = +1$)**
- $s_i = \Sigma_{a,b} + 1 \in \{-1, 0, 1, 2, 3\}$.
- $s_i \in \{-1, 0, 1\} \implies c_{\text{out}} = 0$ (occurs when $\Sigma_{a,b} \in \{-2, -1, 0\}$, prob $\frac{1 + 2 + 3}{9} = \frac{6}{9} = \frac{2}{3}$).
- $s_i \in \{2, 3\} \implies c_{\text{out}} = +1$ (occurs when $\Sigma_{a,b} \in \{+1, +2\}$, prob $\frac{2 + 1}{9} = \frac{3}{9} = \frac{1}{3}$).
- $c_{\text{out}} = -1$ is impossible (prob $0$).
- **Carry Continuation Probability:**
  $$P(c_{\text{out}} = +1 \mid c_{\text{in}} = +1) = \frac{1}{3} \approx 0.3333$$
- **Carry Cancellation / Absorption Probability:**
  $$P(c_{\text{out}} = 0 \mid c_{\text{in}} = +1) = \frac{2}{3} \approx 0.6667$$

**Case 3: Negative Active Carry ($c_{\text{in}} = -1$)**
- By sign symmetry:
  $$P(c_{\text{out}} = -1 \mid c_{\text{in}} = -1) = \frac{1}{3}, \quad P(c_{\text{out}} = 0 \mid c_{\text{in}} = -1) = \frac{2}{3}, \quad P(c_{\text{out}} = +1 \mid c_{\text{in}} = -1) = 0$$

#### 2. Complete Transition Probability Matrix
Mapping states to ordered index $\{-1, 0, +1\}$:
$$\mathbf{P} = \begin{pmatrix}
P(-1 \mid -1) & P(0 \mid -1) & P(+1 \mid -1) \\
P(-1 \mid 0)  & P(0 \mid 0)  & P(+1 \mid 0) \\
P(-1 \mid +1) & P(0 \mid +1) & P(+1 \mid +1)
\end{pmatrix} = \begin{pmatrix}
1/3 & 2/3 & 0 \\
1/9 & 7/9 & 1/9 \\
0   & 2/3 & 1/3
\end{pmatrix}$$

#### 3. Stationary Distribution
The stationary probability vector $\boldsymbol{\pi} = (\pi_{-1}, \pi_0, \pi_{+1})$ satisfies $\boldsymbol{\pi} \mathbf{P} = \boldsymbol{\pi}$ and $\sum \pi_j = 1$.
By sign symmetry, $\pi_{-1} = \pi_{+1} = p$, and $\pi_0 = 1 - 2p$.
Substituting into the equation for state $-1$:
$$p \cdot \frac{1}{3} + (1 - 2p) \cdot \frac{1}{9} + p \cdot 0 = p$$
$$\frac{1}{3} p + \frac{1}{9} - \frac{2}{9} p = p \implies \frac{1}{9} p + \frac{1}{9} = p \implies \frac{8}{9} p = \frac{1}{9} \implies p = \frac{1}{8}$$
Thus:
$$\pi_{-1} = \frac{1}{8}, \quad \pi_0 = \frac{3}{4}, \quad \pi_{+1} = \frac{1}{8}$$
- **Stationary Non-Zero Carry Occurrence Probability:**
  $$\pi(\text{carry} \neq 0) = \pi_{-1} + \pi_{+1} = \frac{1}{8} + \frac{1}{8} = \frac{1}{4} = 0.2500$$

#### 4. Transient Dynamics and Expected Chain Lengths

Let addition start from the quiescent initial condition $c_0 = 0$, represented by initial distribution $\mathbf{p}^{(0)} = (0, 1, 0)$.
The eigenvalues of $\mathbf{P}$ are $\lambda_1 = 1$, $\lambda_2 = \frac{1}{3}$, and $\lambda_3 = \frac{1}{9}$.
The transient non-zero carry probability at digit step $k \ge 1$ initialized with $c_0 = 0$ is:
$$P(c_k \neq 0 \mid c_0 = 0) = \frac{1}{4} \left(1 - \left(\frac{1}{9}\right)^k\right)$$
Evaluating exact steps:
- For $k=1$: $P(c_1 \neq 0) = \frac{1}{4} \left(1 - \frac{1}{9}\right) = \frac{1}{4} \cdot \frac{8}{9} = \frac{2}{9} \approx 0.2222$ (matching the quiescent generation probability).
- For $k=2$: $P(c_2 \neq 0) = \frac{1}{4} \left(1 - \frac{1}{81}\right) = \frac{20}{81} \approx 0.2469$.
- For $k=3$: $P(c_3 \neq 0) = \frac{1}{4} \left(1 - \frac{1}{729}\right) = \frac{182}{729} \approx 0.2497$.
- As $k \to \infty$: $P(c_k \neq 0) \to \frac{1}{4} = 0.2500$ (converging monotonically and rapidly to the stationary distribution).

For an $N$-digit balanced ternary addition initialized with $c_0 = 0$, the expected total number of active non-zero carries across all $N$ positions is:
$$E[N_{\text{carries}}] = \sum_{k=1}^N P(c_k \neq 0 \mid c_0 = 0) = \frac{1}{4} N - \frac{1}{32} \left(1 - \left(\frac{1}{9}\right)^N\right) \approx \frac{1}{4} N$$

**Conditional Carry Run Length:**
When a carry is generated at digit position $i$ ($c_i = +1$), it continues to propagate as $c_{i+1} = +1$ with constant probability $P(c_{\text{out}} = +1 \mid c_{\text{in}} = +1) = \frac{1}{3}$, and terminates ($c_{\text{out}} = 0$) with probability $\frac{2}{3}$.
The number of digits $L$ over which this specific carry persists is geometrically distributed with stopping probability $p_{\text{stop}} = \frac{2}{3}$:
$$E[L_{\text{ternary\_run}}] = \sum_{k=1}^\infty k \left(\frac{1}{3}\right)^{k-1} \left(\frac{2}{3}\right) = \frac{1}{2/3} = \frac{3}{2} = 1.500 \text{ trits}$$

#### 5. Comparison with Binary Carry Mechanics

In conventional binary addition $a_i + b_i + c_{\text{in}}$ with independent uniform bits $a_i, b_i \in \{0, 1\}$:
- States: $c \in \{0, 1\}$.
- From $c_{\text{in}} = 0$: $c_{\text{out}} = 1$ only if $a_i = 1, b_i = 1$ (prob $1/4$). Thus, carry generation probability is $0.2500$.
- From $c_{\text{in}} = 1$: $c_{\text{out}} = 1$ if $a_i + b_i \ge 1$ (prob $3/4$). Thus, carry continuation probability is $0.7500$.
- Binary transition matrix:
  $$\mathbf{P}_{\text{bin}} = \begin{pmatrix} 3/4 & 1/4 \\ 1/4 & 3/4 \end{pmatrix}$$
- Stationary distribution: $\pi_{\text{bin}}(c = 1) = \frac{1}{2} = 0.5000$.
- Conditional carry run length given an active carry:
  $$E[L_{\text{binary\_run}}] = \frac{1}{1 - 3/4} = 4.000 \text{ bits}$$

**Summary Comparison:**
| Metric | Balanced Ternary | Binary Unsigned | Ratio (Ternary / Binary) |
| :--- | :---: | :---: | :---: |
| Carry Generation $P(c_{\text{out}} \neq 0 \mid c_{\text{in}} = 0)$ | $2/9 \approx 0.2222$ | $1/4 = 0.2500$ | $0.889\times$ |
| Carry Continuation $P(c_{\text{out}} \neq 0 \mid c_{\text{in}} \neq 0)$ | $1/3 \approx 0.3333$ | $3/4 = 0.7500$ | $0.444\times$ |
| Stationary Carry Occurrence $\pi(c \neq 0)$ | $1/4 = 0.2500$ | $1/2 = 0.5000$ | $0.500\times$ |
| Conditional Run Length $E[L_{\text{run}}]$ | $1.500$ trits | $4.000$ bits | $0.375\times$ |
| Stationary Occurrence per Bit ($\pi / \log_2 R$) | $0.250 / 1.585 \approx 0.1577$ | $0.500 / 1.000 = 0.5000$ | $0.315\times$ |

*Mathematical Finding:* Balanced ternary reduces stationary carry occurrence by $50\%$ per digit and by $68.5\%$ per equivalent bit of information, while shortening conditional carry runs from $4.0$ bits down to $1.5$ trits.

---

### 6.3 Implementation Memory Layout and Cost Models

In evaluating the C++ arithmetic implementations, we distinguish actual struct memory layouts, logical digit capacity, host storage footprint, temporary accumulators, and hypothetical scalar-packed formats.

#### 1. Actual Implemented C++ Class Layouts (`include/t81/types/T81Limb.hpp`)

- **`T81Limb27` (27 trits):**
  ```cpp
  class T81Limb27 {
    static constexpr int TRITS = 27;
    static constexpr int TRYTES = 9;
  private:
    alignas(16) int8_t trytes_[TRYTES]{}; // 9 bytes + 7 bytes padding
  };
  ```
  - **Memory Layout:** Aligned to 16 bytes via `alignas(16)`. Contains 9 `int8_t` tryte values followed by 7 bytes of compiler padding.
  - **Logical Capacity:** $3^{27}$ values ($\approx 7.625 \times 10^{12} \approx 42.79$ bits of information).
  - **Host Size:** `sizeof(T81Limb27) == 16` bytes (128 bits).
  - **Host Information Density:** $\frac{42.79\text{ bits}}{128\text{ bits allocated}} \approx 33.43\%$.

- **`T81Limb54` (54 trits):**
  ```cpp
  class T81Limb54 {
    static constexpr int TRITS = 54;
    static constexpr int TRYTES = 18;
  private:
    alignas(16) int8_t trytes_[TRYTES]{}; // 18 bytes + 14 bytes padding
  };
  ```
  - **Memory Layout:** Aligned to 16 bytes. Contains 18 `int8_t` tryte values followed by 14 bytes of compiler padding.
  - **Logical Capacity:** $3^{54}$ values ($\approx 5.815 \times 10^{25} \approx 85.59$ bits of information).
  - **Host Size:** `sizeof(T81Limb54) == 32` bytes (256 bits).
  - **Host Information Density:** $\frac{85.59\text{ bits}}{256\text{ bits allocated}} \approx 33.43\%$.

- **Temporary Wide Accumulators:**
  During multiplication, `T81Limb27::operator*` decodes trytes into trits and accumulates into a temporary stack array:
  `std::array<int, 96> acc{}` (384 bytes on 64-bit systems), which is normalized across 54 trits and repacked into `T81Limb54`.
  This temporary accumulator layout is transient and must not be confused with the persistent limb object layout.

#### 2. Hypothetical Scalar-Packed Limb Layouts (Not Currently Implemented)
If 27 trits were packed as scalar integers using base-3 positional encoding:
- $3^{27} < 2^{43} \implies$ fits within a single 64-bit unsigned integer (`std::uint64_t`).
- Packing 54 trits requires $3^{54} < 2^{86} \implies$ fits within an unsigned 128-bit integer (`unsigned __int128`).
- Such scalar layouts would achieve $\frac{42.79}{64} \approx 66.86\%$ host word efficiency, but would require software division/modulo by 3 or balanced Booth multiplication, losing the parallel tryte lookups provided by the current `trytes_[]` Kogge-Stone adder.

---

## 7. Binary Control Requirements

To prevent unfair comparisons that favor balanced ternary through strawman baselines, any future experimental evaluation must enforce strict binary control requirements:

1. **Equal Information Capacity:**
   Binary controls must operate over the exact mathematical range $M = 3^T$ (or closest power of 2 $2^B \ge 3^T$) rather than arbitrary mismatched bit widths.
2. **Equivalent Implementation Sophistication:**
   If the ternary candidate uses hand-tuned assembly, SIMD vectorization, or unrolled loops, the binary baseline must use equivalent optimizations (e.g., AVX2/AVX-512 FMA, `_mm256_maddubs_epi16`, or optimized GMP assembly).
3. **Identical Memory Hierarchy & Caching:**
   Workloads must be tested across identical cache residency tiers (L1-resident, L2/L3-resident, and DRAM-bound) with explicit pre-faulting and cache flushing.
4. **Separation of Conversion and Computation:**
   Input radix conversion costs ($W_{\text{binary}} \to W_{\text{ternary}}$) must be measured and reported separately from core kernel execution time.
5. **Fail-Closed Assertion Parity:**
   Both binary and ternary test suites must execute under unconditional assertions active in Release builds (`-DNDEBUG`), matching the SCG-07 Phase 6 qualification standard.

---

## 8. Experimental Confounders & Threat Model

The following confounding factors have historically distorted ternary research and must be explicitly controlled:

1. **Host Word-Size and Struct Padding Taxes:**
   `T81Limb27` incurs 7 bytes of padding per 16-byte struct, and `T81Limb54` incurs 14 bytes per 32-byte struct. Experiments must not attribute host binary struct alignment penalties to inherent radix properties.
2. **Tryte-Level Composition vs Digit-Level Models:**
   The T81 library groups 3 trits into a tryte (range $[-13, 13]$), using table lookups (`ADD_TABLE`, `COMPOSITION_TABLE`) for Kogge-Stone carry lookahead. This tryte-level composition behaves differently from the scalar digit-level Markov chain, as carries across trytes occur with probability $P(|c_{\text{out}}| = 1 \mid c_{\text{in}} = 0) = \frac{2}{27} \approx 0.0741$.
3. **Compiler Optimization Discrepancies:**
   Compilers optimize binary bit-shifts (`x >> 2`, `x & 3`) aggressively, but generate multi-instruction reciprocal routines or slow division for ternary modulo operations.
4. **QEMU & Emulation Overhead:**
   Instruction-set emulators measure emulator JIT overhead, not physical silicon cycle counts.

---

## 9. Falsification Criteria

A research hypothesis is scientifically worthless without explicit falsification conditions. The following universal stopping rules govern the SCG-08 program:

```
+-------------------------------------------------------------------------------------------------------+
|                                  SCG-08 RESEARCH FALSIFICATION MATRIX                                 |
+--------------------------+----------------------------------------------------------------------------+
| Research Domain          | Falsification & Immediate Stopping Condition                               |
+--------------------------+----------------------------------------------------------------------------+
| Software Multi-Precision | If balanced-ternary carry normalization overhead exceeds total binary carry |
| Arithmetic (H02)         | resolution time by > 2.0x for all operand widths N in [64, 1024] bits.     |
+--------------------------+----------------------------------------------------------------------------+
| Memory & Grouping (H03)  | If the decode instruction tax of 2-bit or Base-81 packing prevents end-to- |
|                          | end throughput parity with INT8/INT16 across any memory hierarchy tier.    |
+--------------------------+----------------------------------------------------------------------------+
| Adaptive Policy (H04)    | If the transition and selection cost between representations eliminates    |
|                          | modeled capacity savings across all tested realistic trace distributions.  |
+--------------------------+----------------------------------------------------------------------------+
| Physical Hardware (H01)  | If required ternary noise margins degrade energy-delay product (EDP)       |
|                          | below that of scaled binary CMOS across the entire plausible device space. |
+--------------------------+----------------------------------------------------------------------------+
```

---

## 10. Recommended First Experiment: EXP-SCG08-01

### 10.1 Experiment Title
**EXP-SCG08-01: Multi-Precision Balanced-Ternary vs. Binary Carry-Propagation and Arithmetic Work Evaluation**

### 10.2 Objective & Scope
The purpose of EXP-SCG08-01 is to evaluate whether the theoretical carry-reduction advantage of balanced ternary ($\pi(\text{carry} \neq 0) = 0.2500$ vs $0.5000$) can be realized as a net reduction in arithmetic work (operation count and carry resolution complexity) in multi-precision integer addition and multiplication, when compared against an exact binary control operating over identical information domains.

This experiment operates on software algorithms using the qualified SCG-07 arithmetic implementations (`T81Limb27`, `T81Limb54`, `T81BigInt`) and an independent binary arbitrary-precision implementation. It requires **no speculative hardware** and executes using standard Linux x86_64 host resources.

### 10.3 Three-Tier Experimental Architecture

To avoid conflating mathematical carry dynamics with host CPU microarchitecture quirks, EXP-SCG08-01 is structured into three strictly decoupled evaluation tiers:

#### Tier 1: Primary Mathematical Carry-Propagation Evaluation
- **Goal:** Directly validate the mathematical 3-state carry Markov chain on pure digit-level sequences.
- **Method:** Instrument scalar digit addition across $10^7$ independent uniformly distributed balanced-ternary digit pairs and binary bit pairs using decoupled event counters.
- **Instrumented Experimental Counters:**
  1. $f_{\text{gen}|0} = \frac{N(\text{quiescent-to-active transitions})}{N(\text{digits with incoming carry } c_{\text{in}} = 0)}$: Conditional carry generation probability.
  2. $f_{\text{trans}} = \frac{N(\text{quiescent-to-active transitions})}{N(\text{total digits})}$: Stationary zero-to-active transition frequency per digit.
  3. $f_{\text{occ}} = \frac{N(\text{digits with active outgoing carry } |c_{\text{out}}| = 1)}{N(\text{total digits})}$: Stationary active-carry occurrence frequency.
  4. $f_{\text{cont}|\text{act}} = \frac{N(\text{active-to-active continuations } c_{\text{out}} = c_{\text{in}})}{N(\text{digits with incoming active carry } |c_{\text{in}}| = 1)}$: Conditional carry continuation probability.
  5. $\bar{L}_{\text{run}} = \frac{\sum \text{run lengths}}{N(\text{carry runs})}$: Average conditional carry run length.
- **Expected Theoretical Outcomes:**
  - Ternary: $f_{\text{gen}|0} = \frac{2}{9} \approx 0.2222$, $f_{\text{trans}} = \frac{1}{6} \approx 0.1667$, $f_{\text{occ}} = \frac{1}{4} = 0.2500$, $f_{\text{cont}|\text{act}} = \frac{1}{3} \approx 0.3333$, $\bar{L}_{\text{run}} = 1.500$ trits.
  - Binary: $f_{\text{gen}|0} = \frac{1}{4} = 0.2500$, $f_{\text{trans}} = \frac{1}{8} = 0.1250$, $f_{\text{occ}} = \frac{1}{2} = 0.5000$, $f_{\text{cont}|\text{act}} = \frac{3}{4} = 0.7500$, $\bar{L}_{\text{run}} = 4.000$ bits.

#### Tier 2: Secondary Implementation Operation-Count Comparison
- **Goal:** Measure the algorithmic operation work required to perform multi-precision addition and multiplication on actual limb data structures.
- **Ternary Treatment:** Multi-limb addition and convolution multiplication using `T81Limb27` and `T81Limb54` (9-tryte and 18-tryte aligned arrays).
- **Binary Control:** Multi-limb addition and convolution multiplication using standard 64-bit binary limbs (`std::uint64_t`).
- **Metrics:** Count of primitive additions, table lookups, carry propagation steps, and normalizations normalized per bit of information.

#### Tier 3: Exploratory Host Timing Comparison
- **Goal:** Quantify the execution latency on a modern binary CPU (Intel Xeon / AMD EPYC x86_64).
- **Caveat:** Explicitly labeled exploratory; does not measure native ternary silicon speed.
- **Metrics:** Nanoseconds per multi-precision addition and multiplication pass.

### 10.4 Matched Evaluation Bands
Evaluated across 5 matched information capacity tiers:
1. **Tier 1 (64-bit equivalent):** Binary $B = 64$ bits (1 limb); Ternary $T = 41$ trits (2 limbs of 27 trits).
2. **Tier 2 (128-bit equivalent):** Binary $B = 128$ bits (2 limbs); Ternary $T = 81$ trits (3 limbs of 27 trits).
3. **Tier 3 (256-bit equivalent):** Binary $B = 256$ bits (4 limbs); Ternary $T = 162$ trits (6 limbs of 27 trits).
4. **Tier 4 (512-bit equivalent):** Binary $B = 512$ bits (8 limbs); Ternary $T = 324$ trits (12 limbs of 27 trits).
5. **Tier 5 (1024-bit equivalent):** Binary $B = 1024$ bits (16 limbs); Ternary $T = 648$ trits (24 limbs of 27 trits).

### 10.5 Preregistered Success & Failure Criteria
- **Tier 1 Success (Mathematical Carry Parity):**
  - Conditional carry generation: $f_{\text{gen}|0} \in [0.217, 0.227]$ (theoretical $2/9 \approx 0.2222$).
  - Zero-to-active transition frequency: $f_{\text{trans}} \in [0.161, 0.172]$ (theoretical $1/6 \approx 0.1667$).
  - Stationary active occurrence: $f_{\text{occ}} \in [0.245, 0.255]$ (theoretical $1/4 = 0.2500$).
  - Conditional carry continuation: $f_{\text{cont}|\text{act}} \in [0.328, 0.338]$ (theoretical $1/3 \approx 0.3333$).
  - Average conditional run length: $\bar{L}_{\text{run}} \in [1.450, 1.550]$ trits (theoretical $1.500$).
- **Tier 2 Success (Algorithmic Operation Work):** For wide multi-precision multiplication ($\ge 512$ bits equivalent), balanced-ternary limb convolution requires $\le 1.10\times$ total operations per bit compared to binary.
- **Falsification Threshold (Immediate Closure):** If the total elementary operations per bit for balanced ternary exceeds that of binary by $> 2.0\times$ across all evaluated bands, the hypothesis of software multi-precision advantage is formally **FALSIFIED**.

---

## 11. Reproducibility Requirements & Artifact Inventory

To ensure complete scientific auditability, the implementation of EXP-SCG08-01 (when authorized) must conform to the following standards:

1. **Independent Verification Oracle:**
   The experiment harness must link the decoupled arbitrary-precision `ExactBigIntOracle` (base $10^9$) established in SCG-07 Phase 6 to verify the mathematical correctness of every computed product and sum independently of both candidate arms.
2. **Deterministic PRNG Seeding:**
   All pseudo-random operand vectors must be generated using an explicit, seeded PRNG (`std::mt19937_64` with valid literal seed `0x5C608001ULL`), ensuring 100% reproducible test vectors across independent platforms.
3. **Machine-Readable JSON Evidence Schema:**
   All measured metrics, carry distributions, and cycle measurements must be emitted to a validated JSON artifact conforming to schema `t81.scg08.carry-experiment.v1`.
4. **Build & Test Isolation:**
   The test executable must be registered in `CMakeLists.txt` under target `t81_exp_scg08_01_test` with unconditional assertions active under `-DNDEBUG`.

---

## 12. Research Limitations

This charter operates under explicit, non-negotiable boundaries:
1. **Emulated Substrate Boundary:** All software arithmetic evaluations are executed on binary host processors (x86_64 / AArch64). Microarchitectural instruction execution times reflect host binary CPU designs, not native ternary semiconductor hardware.
2. **No Physical Semiconductor Fabrication:** T81 does not possess custom ternary foundry access. All native hardware models remain analytical and simulation-based.
3. **No Unapproved Normative Amendments:** This charter does not alter TISC ISA specifications, does not modify `T81Int`/`T81Limb` production contracts, and does not alter runtime policies.

---

## 13. Explicit Authorization Boundaries

In accordance with T81 research continuity governance:
- **Phase 0 Status:** RESEARCH DESIGN, HISTORICAL EVIDENCE RECONCILIATION, AND EXPERIMENT PREREGISTRATION ONLY.
- **Implementation Authorization:** **NOT GRANTED**.
- **Execution Authorization:** **NOT GRANTED**.
- **Production Code Modifications:** **STRICTLY PROHIBITED**.
- **Normative Policy Changes:** **STRICTLY PROHIBITED**.
- **Automatic Merge:** **STRICTLY PROHIBITED**.

Any progression to implementation or benchmarking of EXP-SCG08-01 requires explicit, dated maintainer review and approval of this charter document.
