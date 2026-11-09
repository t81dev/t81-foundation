# SCG-08 Phase 0: Architectural Advantage Research Charter

**Document ID:** `T81-SCG-08-PHASE0-CHARTER-001-R1`
**Classification:** RESEARCH CHARTER & EVIDENCE RECONCILIATION
**Task Classification:** Research Design, Evidence Harmonization & Experiment Preregistration
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Pinned Baseline Commit SHA:** `cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3` (`origin/main`)
**Preceding Milestone:** SCG-07 Phase 6 Consolidated Arithmetic Qualification (`8dec43ac2a9a702c9ae54229cf6891ee63f285c0`, PR #741)
**Status:** COMPLETE — READY FOR MAINTAINER DECISION

---

## 1. Executive Research Objective

The primary objective of SCG-08 Phase 0 is to formulate a rigorous, falsifiable, and evidence-grounded research charter investigating whether balanced-ternary representation and arithmetic can offer measurable computational, architectural, or structural advantages over conventional binary computation.

Following the successful completion of the SCG-07 arithmetic qualification program (Phases 1 through 6, culminating in PR #741), the T81 core numeric foundation (`T81Int`, `T81Limb27`, `T81Limb54`, `T81BigInt`, and `T81Fraction`) has been verified correct under documented contracts. However, correctness does not establish architectural superiority. This charter establishes:
1. An adversarial reconciliation of all prior experimental findings and negative dispositions across the T81 project lineage.
2. A representation-neutral mathematical cost model that cleanly separates mathematical identities (such as radix economy $\log_2 3 \approx 1.585$) from binary CPU instruction overhead and physical semiconductor scaling realities.
3. Explicit mapping to the authoritative `H01`–`H08` central hypothesis register without introducing conflicting renumbering.
4. A small set of prioritized, genuinely unresolved research questions with matched binary controls and preregistered falsification criteria.
5. Recommendation and full preregistration specification for one narrowly scoped next experiment (`EXP-SCG08-01`) executable with available repository resources.

This charter operates strictly within research-design boundaries: no production arithmetic modifications, no ISA changes, no unapproved benchmark executions, and no speculative hardware claims are authorized.

---

## 2. Pinned Repository Baseline

### 2.1 Pinned Commit SHA
- **Baseline Git Commit SHA:** `cfd60998fb0b55d8c2b56529cf4e3fb0a4c78ac3` (`origin/main` as of 2026-11-09).
- **Merge Point of SCG-07 Phase 6:** Commit `8dec43ac2a9a702c9ae54229cf6891ee63f285c0` (PR #741, merging head `3e471695b15a5ee5e969f15c04df1a801c99b7b7` amending base `ecb1e32b9458a1a1df51d7cdd4a19496c0584103`).

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
- **H02 (Arithmetic Advantage):** Core focus of SCG-08. Prior software tryte benchmarks were FALSIFIED on single-word ALU additions. However, multi-precision carry-propagation behavior and balanced-ternary carry-chain lengths across wide limb representations (`T81Limb27`, `T81Limb54`, `T81BigInt`) remain mathematically distinct and require precise qualification.
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

- **Formal Mathematical Claim:** In balanced-ternary integer addition $A + B$ where digit positions $d_i \in \{-1, 0, +1\}$ are independent and uniformly distributed, the probability of an active carry out of any single digit position is $P(\text{carry} \neq 0) = \frac{7}{27} \approx 0.2593$, compared to $P(\text{carry} = 1) = 0.5000$ in conventional unsigned binary addition and $0.5000$ in two's complement binary addition. Consequently, the expected length of a continuous carry-propagation chain across an $N$-trit balanced-ternary addition is strictly bounded by $E[L_{\text{ternary}}] = \sum_{k=1}^N \left(\frac{7}{27}\right)^k < \frac{7}{20} = 0.3500$, compared to $E[L_{\text{binary}}] = \sum_{k=1}^N \left(\frac{1}{2}\right)^k \to 1.0000$ in binary.
- **Independent Binary Baseline:** Standard binary multi-precision arithmetic addition and carry-chain simulation operating on 64-bit limbs and bit-level carry-propagation chains.
- **Balanced-Ternary Treatment:** Multi-precision addition evaluated on balanced-ternary representations, utilizing the qualified `T81Limb27` and `T81Limb54` digit carry normalization routines.
- **Controlled Variables:** Total information capacity in bits ($M = 2^k$, for $k \in \{64, 128, 256, 512, 1024\}$), operand distribution (uniform random over range $[-(M-1)/2, (M-1)/2]$), and deterministic PRNG seeds.
- **Primary Outcome Metric:**
  1. Empirical carry generation frequency $f_{\text{carry}} = \frac{N_{\text{carries}}}{N_{\text{digits}}}$.
  2. Maximum and average carry propagation length across $10^6$ independent addition trials.
  3. Total carry-resolution operations required per unit of information.
- **Explicit Success Threshold:** Empirical carry generation frequency matches theoretical $\frac{7}{27} \pm 0.005$, and average carry chain length is $\le 0.360$ (demonstrating a $>2.7\times$ reduction in carry propagation events per digit relative to binary).
- **Explicit Failure Threshold:** Carry generation frequency exceeds $0.300$, or the number of carry events per equivalent bit of information ($\frac{f_{\text{carry}}}{\log_2 3}$) is greater than or equal to binary carry frequency ($0.500$).
- **Confounding Factors:**
  - Non-uniform operand distributions (e.g., localized zero digits).
  - Software emulation overhead on binary CPUs (which masks algorithmic carry savings behind bit-masking loops).
- **Required Instrumentation:** Hardware-independent exact carry-counter harness tracking digit-level carry generation and propagation events without host CPU cycle timers.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Quantifies the exact mathematical carry-reduction advantage of balanced ternary, providing the theoretical foundation for asynchronous or carry-skip arithmetic architectures.
  - *Negative Outcome:* Proves that when normalized per bit of information ($\log_2 3 \approx 1.585$), the carry reduction is insufficient to justify balanced-ternary representations even in ideal mathematical models.

---

### Hypothesis H03-A: Tryte Limb Boundary Alignment and Modular Convolution Complexity

- **Formal Mathematical Claim:** Multi-limb balanced-ternary multiplication utilizing 27-trit limbs (`T81Limb27`, capacity $3^{27} \approx 7.625 \times 10^{12} < 2^{43}$) and 54-trit accumulators (`T81Limb54`, capacity $3^{54} \approx 5.815 \times 10^{25} < 2^{86}$) achieves exact digit convolution without intermediate carry truncation error modulo $3^{54}$, requiring strictly fewer limb cross-multiplications than binary multi-precision multiplication for equivalent information capacities that align with powers of $3^{27}$.
- **Independent Binary Baseline:** Exact multi-precision integer multiplication implemented on standard 64-bit binary limbs (e.g., using 64-bit $\times$ 64-bit $\to$ 128-bit hardware widening multipliers).
- **Balanced-Ternary Treatment:** The qualified SCG-07 `T81Limb27::operator*` and `T81Limb54::reference_mul` / `karatsuba` convolution engines.
- **Controlled Variables:** Information domain width (equivalent to $N \in \{128, 256, 512, 1024\}$ bits), test operand distributions, multiplication algorithm (Schoolbook $O(K^2)$ vs Karatsuba $O(K^{1.585})$).
- **Primary Outcome Metric:**
  1. Total number of scalar limb multiply-accumulate operations required to evaluate an $N$-bit equivalent product.
  2. Total normalization operations required per product.
- **Explicit Success Threshold:** For arbitrary-precision integer sizes where $\lceil \log_3(2^N) \rceil$ creates natural limb alignment, balanced-ternary limb multiplication requires fewer or equal primitive multiplications than 64-bit binary multiplication.
- **Explicit Failure Threshold:** The fractional information waste ($\frac{64 - \log_2(3^{27})}{64} \approx 33.1\%$ capacity underutilization per 64-bit host word) causes the limb count $K_{\text{ternary}} = \lceil \frac{N \log_2 2}{42.79} \rceil$ to exceed binary limb count $K_{\text{binary}} = \lceil \frac{N}{64} \rceil$ such that total scalar operations are $>1.5\times$ higher across all tested widths $N \le 1024$.
- **Confounding Factors:** Host 64-bit word size mismatch; register utilization inefficiencies on x86_64/AArch64 architectures.
- **Required Instrumentation:** Operation-counting execution proxy counting scalar multiplications, additions, and normalization steps.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Identifies specific operand width regimes where ternary limb structures minimize algorithmic complexity.
  - *Negative Outcome:* Demonstrates that binary word-size impedance mismatch fundamentally penalizes emulated ternary BigInt arithmetic regardless of radix properties.

---

### Hypothesis H01/H05-A: Device Parameter Thresholds for Native-Ternary Energy-Delay Product Advantage

- **Formal Mathematical Claim:** A native three-state logic cell (operating at states $\{-V_{dd}, 0, +V_{dd}\}$ or tri-level voltages $\{0, V_{dd}/2, V_{dd}\}$) can achieve a superior Energy-Delay Product ($\text{EDP} = \text{Energy} \times \text{Delay}$) over binary CMOS if and only if the physical cell area ratio satisfies $A_3 / A_2 < \log_2 3 \approx 1.585$ and the effective switching energy per state transition satisfies $E_3 / E_2 < \frac{2}{3} \log_2 3 \approx 1.056$ under equivalent signal-to-noise margins (SNM).
- **Independent Binary Baseline:** Standard binary CMOS inverter and full-adder cells at established technology nodes (e.g., 7nm / 14nm FinFET / GAAFET) with standard static noise margins ($\text{SNM} \ge V_{dd}/5$).
- **Balanced-Ternary Treatment:** Analytical SPICE / device-level models of Standard Ternary Inverter (STI) and ternary full adders (TFA) reported in peer-reviewed literature (e.g., CNTFET or multi-threshold CMOS).
- **Controlled Variables:** Technology node feature size, operating temperature ($300\text{ K}$), interconnect parasitics, target reliability/error rate.
- **Primary Outcome Metric:** Theoretical Energy-Delay Product ratio $\text{EDP}_3 / \text{EDP}_2$ as a function of the ternary noise-margin degradation factor $\alpha = \frac{\text{SNM}_3}{\text{SNM}_2}$.
- **Explicit Success Threshold:** Identification of an empirically plausible device parameter regime ($\alpha \ge 0.70$, $A_3/A_2 \le 1.40$) where $\text{EDP}_3 < \text{EDP}_2$.
- **Explicit Failure Threshold:** If maintaining equal reliability requires higher supply voltage $V_{dd}$ such that dynamic switching energy $E \propto C V_{dd}^2$ causes $\text{EDP}_3 > \text{EDP}_2$ everywhere in the realistic parameter space.
- **Confounding Factors:** Non-silicon substrate assumptions (e.g., CNTFETs vs silicon CMOS); speculative fabrication yields.
- **Required Instrumentation:** Mathematical sensitivity analysis and parameter sweep scripts; literature meta-analysis.
- **Scientific Value of Either Outcome:**
  - *Positive Outcome:* Establishes rigorous physical requirements for any future hardware or silicon initiative.
  - *Negative Outcome:* Decisively closes speculative claims of native hardware advantage, directing project resources exclusively to software and protocol governance.

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
   $$\text{Base-81 Trytes (4 trits): } Y = \lceil T / 4 \rceil = \lceil \frac{1}{4} \lceil \log_3 M \rceil \rceil$$
3. **Radix Economy:**
   The complexity or hardware cost of an $R$-state digit system is often modeled as $C(R, M) = k \cdot R \cdot \lceil \log_R M \rceil$.
   Taking the continuous derivative of $f(R) = R / \ln R$ yields a minimum at $R = e \approx 2.718$.
   Evaluating integers:
   $$f(2) = \frac{2}{\ln 2} \approx 2.8854$$
   $$f(3) = \frac{3}{\ln 3} \approx 2.7307 \quad (\approx 5.36\% \text{ lower than binary})$$
   $$f(4) = \frac{4}{\ln 4} \approx 2.8854$$
   *Mathematical Invariant:* Base 3 is the integer radix with the highest theoretical radix economy. However, this theoretical optimum assumes that an $R$-state physical device has cost proportional to $R$, which is false in planar binary CMOS.

4. **Balanced-Ternary Value Representation:**
   For digits $d_i \in \{-1, 0, +1\}$:
   $$V = \sum_{i=0}^{T-1} d_i \cdot 3^i, \quad V \in \left[ -\frac{3^T - 1}{2}, \, +\frac{3^T - 1}{2} \right]$$
   - *Inherent Sign Representation:* Negation is strictly componentwise with zero carry propagation: $-V = \sum_{i=0}^{T-1} (-d_i) \cdot 3^i$.
   - *Absence of Sign Asymmetry:* Range is perfectly symmetric around zero; no counterpart to binary two's complement asymmetry (where $-\text{INT\_MIN}$ overflows).

### 6.2 Carry Generation & Propagation Analysis

In single-digit balanced-ternary addition $s_i = a_i + b_i + c_{\text{in}}$, where $a_i, b_i \in \{-1, 0, +1\}$ and $c_{\text{in}} \in \{-1, 0, +1\}$:
The input sum lies in $\Sigma = a_i + b_i + c_{\text{in}} \in \{-3, -2, -1, 0, 1, 2, 3\}$.
The balanced ternary carry $c_{\text{out}}$ and sum digit $r_i$ are uniquely defined by:
$$\Sigma = 3 \cdot c_{\text{out}} + r_i, \quad r_i \in \{-1, 0, +1\}, \, c_{\text{out}} \in \{-1, 0, +1\}$$

| $\Sigma$ | $c_{\text{out}}$ | $r_i$ | Description |
| :---: | :---: | :---: | :--- |
| $-3$ | $-1$ | $0$ | Negative carry generation |
| $-2$ | $-1$ | $+1$ | Negative carry generation |
| $-1$ | $0$ | $-1$ | Zero carry |
| $0$ | $0$ | $0$ | Zero carry |
| $+1$ | $0$ | $+1$ | Zero carry |
| $+2$ | $+1$ | $-1$ | Positive carry generation |
| $+3$ | $+1$ | $0$ | Positive carry generation |

**Carry Generation Probability (Zero Initial Carry $c_{\text{in}} = 0$):**
Assuming independent uniform inputs $P(a_i = k) = \frac{1}{3}$, $P(b_i = k) = \frac{1}{3}$ for $k \in \{-1, 0, 1\}$:
- $\Sigma = a_i + b_i \in \{-2, -1, 0, 1, 2\}$.
- Out of 9 equiprobable input pairs $(a_i, b_i)$:
  - $\Sigma = -2$: pair $(-1, -1) \implies c_{\text{out}} = -1$ (1 case).
  - $\Sigma = +2$: pair $(+1, +1) \implies c_{\text{out}} = +1$ (1 case).
  - $\Sigma \in \{-1, 0, 1\}$: 7 cases $\implies c_{\text{out}} = 0$ (7 cases).
- **Probability of non-zero carry generation:**
  $$P(|c_{\text{out}}| = 1 \mid c_{\text{in}} = 0) = \frac{2}{9} \approx 0.2222$$

**Carry Propagation Probability ($c_{\text{in}} \neq 0$):**
- A carry propagates ($|c_{\text{out}}| = 1$) if $\Sigma = a_i + b_i + c_{\text{in}} \in \{\pm 2, \pm 3\}$.
- If $c_{\text{in}} = +1$:
  - $a_i + b_i \in \{+1, +2\} \implies \Sigma \in \{+2, +3\} \implies c_{\text{out}} = +1$.
  - Number of pairs with $a_i + b_i \ge 1$ is 3 (pairs $(0,1), (1,0), (1,1)$).
  - Thus, $P(c_{\text{out}} = +1 \mid c_{\text{in}} = +1) = \frac{3}{9} = \frac{1}{3} \approx 0.3333$.
  - For pairs with $a_i + b_i = -2$ (pair $(-1, -1)$), $\Sigma = -1 \implies c_{\text{out}} = 0$.
  - Consequently, **carry cancellation** occurs naturally when carries meet opposing digit sums.

**Comparison with Binary Carry Behavior:**
- Binary unsigned addition: $P(c_{\text{out}} = 1 \mid c_{\text{in}} = 0) = \frac{1}{4} = 0.2500$; $P(c_{\text{out}} = 1 \mid c_{\text{in}} = 1) = \frac{3}{4} = 0.7500$.
- Average binary carry propagation probability: $0.5000$.
- Expected carry chain length:
  $$E[L_{\text{binary}}] = \sum_{k=1}^\infty \left(\frac{1}{2}\right)^k = 1.000$$
  $$E[L_{\text{ternary}}] \le \sum_{k=1}^\infty \left(\frac{1}{3}\right)^k = 0.500$$
*Mathematical Finding:* Balanced ternary intrinsically reduces carry-propagation probability and chain length compared to binary addition.

### 6.3 Physical Storage and Decoding Overhead Models

When mapped to physical computing systems, we distinguish three execution regimes:

#### Regime 1: Software Emulation on Binary CPUs (Current Reality)
- **Packed 2-Bit Representation:** 4 trits per byte.
  - Storage: $S_{\text{packed}} = \lceil T/4 \rceil$ bytes.
  - Information Density: $\frac{4 \log_2 3}{8} = \frac{6.33985}{8} \approx 79.25\%$ bit efficiency (20.75% storage waste vs binary).
- **Instruction Overhead:**
  To extract trit $i$ from byte $B$:
  $$\text{trit}_i = ((B \gg (2i)) \ \& \ 3) - 1$$
  On x86/ARM, this requires 1 shift, 1 mask, and 1 subtraction, or a vector lookup table instruction (`VPSHUFB` / `tbl`).
- **Inner Loop Cost Function:**
  $$T_{\text{op}} = T_{\text{arith}} + T_{\text{decode}} + T_{\text{normalize}}$$
  Empirical measurements in `experiments/packed_ternary_matvec/` demonstrated that $T_{\text{decode}} \approx 3 \times T_{\text{arith}}$, which overwhelms any cache bandwidth reduction.

#### Regime 2: Host Multi-Limb Representation (Qualified SCG-07 Subsystem)
- **`T81Limb27`:** 27 trits stored in a 64-bit unsigned integer (`std::uint64_t`).
  - Maximum value: $(3^{27} - 1)/2 \approx 3.81 \times 10^{12} < 2^{42}$.
  - Storage efficiency: $\frac{27 \log_2 3}{64} = \frac{42.79}{64} \approx 66.86\%$ bit efficiency (33.14% headroom).
  - Normalization: Balanced ternary carry normalization requires division/modulo by 3 or balanced Booth carry sweeps:
    $$r_i = ((s_i + 1) \pmod 3 + 3) \pmod 3 - 1, \quad c_{i+1} = (s_i - r_i) / 3$$
- **`T81Limb54`:** 54 trits stored in an unsigned 128-bit integer (`unsigned __int128`).
  - Maximum value: $(3^{54} - 1)/2 \approx 2.91 \times 10^{25} < 2^{85}$.
  - Storage efficiency: $\frac{54 \log_2 3}{128} = \frac{85.59}{128} \approx 66.86\%$ bit efficiency.
  - Headroom: The 42 unused bits in `__int128` guarantee that limb additions and intermediate multiply-accumulations never overflow host machine words prior to normalization.

#### Regime 3: Hypothetical Native Ternary Hardware (Theoretical Model)
- Let $C_3$ be the physical silicon area of a 3-state memory cell, and $C_2$ be that of a 2-state cell.
- For equal information $M$:
  $$\text{Cost}_{\text{ternary}} = C_3 \cdot \lceil \log_3 M \rceil$$
  $$\text{Cost}_{\text{binary}} = C_2 \cdot \lceil \log_2 M \rceil$$
- Breakeven requires:
  $$\frac{C_3}{C_2} \le \frac{\lceil \log_2 M \rceil}{\lceil \log_3 M \rceil} \approx \log_2 3 \approx 1.585$$
- In modern planar and 3D FinFET CMOS, multi-valued voltage storage degrades static noise margin by $\ge 50\%$, requiring higher signal-to-noise ratios, charge-pump complexity, or multi-level sensing amplifiers that drive $C_3 / C_2 > 2.0$. Consequently, native ternary memory is economically unviable in standard CMOS unless non-volatile multi-state resistive switching (RRAM/PCM) achieves native 3-state stability without peripheral circuit penalties.

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

1. **Host Word-Size Quantization Tax:**
   Implementing 27-trit or 54-trit limbs on 64-bit and 128-bit binary microprocessors incurs an inherent $33.1\%$ capacity waste. Experiments must not attribute host binary register mismatches to inherent radix properties.
2. **Compiler Optimization Discrepancies:**
   Modern compilers (GCC, Clang) have decades of pattern-matching optimization for binary bit-shifts (`x >> 2`, `x & 3`), but generate slow integer division routines (`idiv` or multi-instruction reciprocal multiplication) for ternary modulo operations (`% 3`, `/ 3`).
3. **Cache-Resident Microbenchmark Distortions:**
   In small microbenchmarks fitting within L1 cache, memory bandwidth is not the bottleneck, so memory-footprint reductions (e.g., 2-bit packing) will appear disadvantageous due to ALU unpacking overhead. Conversely, in DRAM-bound regimes, memory savings can improve throughput only if decode pipelining matches memory streaming rates.
4. **QEMU & Emulation Overhead:**
   Benchmarking under instruction-set emulators (such as `qemu-aarch64`) measures the translation efficiency of the emulator's dynamic binary recompiler (TCG), not physical microarchitectural cycle counts.
5. **Algorithmic Aliasing:**
   Changes in representation often coincide with unintentional changes in algorithm (e.g., comparing Horner polynomial evaluation with sparse indexing, or comparing schoolbook multiplication with Karatsuba). Algorithmic structure must be held strictly invariant.

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
The purpose of EXP-SCG08-01 is to evaluate whether the theoretical carry-reduction advantage of balanced ternary ($P(\text{carry} \neq 0) \approx 0.2593$ vs $0.5000$) can be realized as a net reduction in arithmetic work (operation count and carry resolution complexity) in multi-precision integer addition and multiplication, when compared against an exact binary control operating over identical information domains.

This experiment operates on software algorithms using the qualified SCG-07 arithmetic implementations (`T81Limb27`, `T81Limb54`, `T81BigInt`) and an independent binary arbitrary-precision implementation. It requires **no speculative hardware** and executes using standard Linux x86_64 host resources.

### 10.3 Experimental Design & Controls

#### Arm A: Balanced-Ternary Treatment
- Multi-precision operand representation: $K$ limbs of 27 trits each (`T81Limb27`), normalized via exact balanced-ternary carry resolution.
- Addition: Parallel limb add followed by sequential 27-trit carry normalization.
- Multiplication: Pairwise $K \times K$ limb cross-multiplication via `T81Limb27::operator*` accumulating into wide 54-trit limbs (`T81Limb54`), followed by multi-limb carry resolution.

#### Arm B: Matched Binary Control
- Multi-precision operand representation: $M$ limbs of 64 bits each (`std::uint64_t`), normalized via standard two's complement carry propagation (`_addcarry_u64`).
- Addition: Multi-limb add with standard carry chain.
- Multiplication: $M \times M$ limb multiplication using 64-bit $\times$ 64-bit $\to$ 128-bit unsigned hardware products (`unsigned __int128`), with 64-bit carry propagation.

#### Matched Evaluation Bands
Evaluated across 5 matched information capacity tiers:
1. **Tier 1 (64-bit equivalent):** Binary $B = 64$ bits (1 limb); Ternary $T = 41$ trits (2 limbs of 27 trits).
2. **Tier 2 (128-bit equivalent):** Binary $B = 128$ bits (2 limbs); Ternary $T = 81$ trits (3 limbs of 27 trits).
3. **Tier 3 (256-bit equivalent):** Binary $B = 256$ bits (4 limbs); Ternary $T = 162$ trits (6 limbs of 27 trits).
4. **Tier 4 (512-bit equivalent):** Binary $B = 512$ bits (8 limbs); Ternary $T = 324$ trits (12 limbs of 27 trits).
5. **Tier 5 (1024-bit equivalent):** Binary $B = 1024$ bits (16 limbs); Ternary $T = 648$ trits (24 limbs of 27 trits).

### 10.4 Primary Metrics & Measurements
1. **Carry Event Count ($N_{\text{carry}}$):** Exact count of non-zero carries generated during $10^6$ uniform random additions per tier.
2. **Carry Propagation Run-Length ($L_{\text{run}}$):** Distribution and maximum length of continuous carry propagation chains.
3. **Normalized Operation Workload ($W_{\text{norm}}$):** Total number of elemental operations (additions, shifts, masks, normalization steps) required per bit of information:
   $$W_{\text{norm}} = \frac{\text{Total Elementary Operations}}{\text{Information Capacity (bits)}}$$
4. **Host Execution Latency:** Median and 99th-percentile execution time (measured in nanoseconds via steady-state monotonic clock) across $10^6$ runs.

### 10.5 Preregistered Success & Failure Criteria
- **Success Criterion 1 (Mathematical Carry Verification):** Empirical carry generation frequency in Arm A matches theoretical expectation $\frac{7}{27} \approx 0.2593 \pm 0.005$, and average carry run-length is $\le 0.360$.
- **Success Criterion 2 (Computational Advantage):** For wide operands ($\ge 512$ bits equivalent), balanced-ternary multi-limb addition achieves fewer total normalization operations per bit than binary carry propagation.
- **Falsification Threshold (Immediate Closure):** If the total elementary operations per bit $W_{\text{norm}}$ for balanced ternary exceeds that of binary by $> 2.0\times$ across all evaluated tiers, the hypothesis that balanced ternary provides an algorithmic or carry-propagation advantage on multi-precision arithmetic is formally **FALSIFIED**.

---

## 11. Reproducibility Requirements & Artifact Inventory

To ensure complete scientific auditability, the implementation of EXP-SCG08-01 (when authorized) must conform to the following standards:

1. **Independent Verification Oracle:**
   The experiment harness must link the decoupled arbitrary-precision `ExactBigIntOracle` (base $10^9$) established in SCG-07 Phase 6 to verify the mathematical correctness of every computed product and sum independently of both candidate arms.
2. **Deterministic PRNG Seeding:**
   All pseudo-random operand vectors must be generated using an explicit, seeded PRNG (`std::mt19937_64` with fixed seed `0x5C608_001ULL`), ensuring 100% reproducible test vectors across independent platforms.
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
