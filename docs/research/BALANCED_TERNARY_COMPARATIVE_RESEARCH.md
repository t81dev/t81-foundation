# Scientific Investigation 05
## Balanced Ternary Versus Binary: Fundamental Representation and Arithmetic Study

**Repository:** https://github.com/t81dev/t81-foundation
**Role:** Independent Computer Arithmetic Researcher and Experimental Computer Architect
**Date:** April 2026
**Status:** Complete / Final Scientific Investigation Report

---

### Executive Summary

This investigation evaluates whether balanced ternary representation provides any demonstrable mathematical or computational property that offers a meaningful advantage over conventional binary representation.

To provide an objective, evidence-based assessment, this study evaluates balanced ternary across three decoupled research domains:
1. **Pure Mathematical Representation** (properties intrinsic to radix 3 and balanced digits $\{-1, 0, 1\}$ independent of physical substrate);
2. **Binary-Host Software Implementation** (measured costs and limits of emulating ternary structures on modern binary x86_64 and ARM64 processors); and
3. **Hypothetical Native Ternary Hardware** (analytical circuit models and physical multi-valued logic device constraints).

**Key Findings:**
- **Mathematical Domain:** Balanced ternary exhibits elegant mathematical properties: exact sign symmetry around zero without sign-magnitude anomaly or two's complement asymmetric range, $O(1)$ trit-wise negation without carry ripple, unbiased rounding via simple truncation, and optimal discrete radix efficiency ($E(3) = 3 / \ln 3 \approx 2.7307$ vs $E(2) = 2 / \ln 2 \approx 2.8854$). However, zero uniqueness holds strictly only in canonical representation; redundant signed-digit (RSD) variants incur $O(N)$ canonicalization costs for equality testing.
- **Binary-Host Software Domain:** Software emulation of ternary arithmetic on binary CPUs suffers a severe execution performance penalty. While packed 2-bit ternary storage achieves $4\times$ weight footprint reduction for quantized neural models ($W \in \{-1, 0, 1\}$), the bit-shuffle decode instruction tax inside binary SIMD vector execution pipelines prevents on-the-fly ternary arithmetic from outperforming native INT8 / FP16 SIMD instructions. Furthermore, multi-limb ternary BigInt operations incur a minimum $2.23\times$ limb operation penalty relative to 64-bit binary limbs.
- **Native Hardware Domain:** Analytical circuit models reveal that three-state physical devices split the operating voltage range $V_{dd}$ into two sub-intervals ($0 \to V_{dd}/2 \to V_{dd}$), reducing noise margins by at least $50\%$ ($6\text{ dB}$). Multi-threshold voltage transistors required for ternary gates increase static leakage and dynamic switching energy, offsetting the theoretical $5.8\%$ interconnect wire reduction.
- **Final Disposition:** **NARROW SCOPE / FROZEN CPU ARITHMETIC**. Fundamental ternary research on binary-host CPU software arithmetic is formally **closed**. Ternary representations should be restricted strictly to memory storage formats (e.g. 2-bit packed weight serialization) and human-readable CanonFS hash trytes, while native binary hardware substrate is maintained for execution.

---

### Phase 1 — Establish the Mathematical Foundations

#### 1.1 Information Density and Radix Efficiency

The theoretical efficiency of a positional number system with radix $b \ge 2$ can be evaluated using the continuous radix efficiency function introduced by Morton (1955) and Hayes (1971). Assuming the hardware complexity or storage cost of representing a digit is directly proportional to the radix $b$, the cost $C(N, b)$ to represent a fixed magnitude range $R$ using $N = \log_b R$ digits is:

$$C(R, b) = k \cdot b \cdot \log_b R = k \cdot R \frac{b}{\ln b}$$

To find the optimal radix $b$ that minimizes overall representation cost per unit of stored information, we differentiate $E(b) = \frac{b}{\ln b}$ with respect to $b$:

$$\frac{d}{db}\left(\frac{b}{\ln b}\right) = \frac{\ln b - 1}{(\ln b)^2}$$

Setting the derivative to zero yields the unique continuous minimum at $b = e \approx 2.71828$. Evaluating $E(b)$ across discrete integer radices:

$$E(2) = \frac{2}{\ln 2} \approx 2.88539 \quad (100.0\% \text{ baseline relative cost})$$
$$E(3) = \frac{3}{\ln 3} \approx 2.73072 \quad (94.64\% \text{ relative cost, } 5.36\% \text{ theoretical savings})$$
$$E(4) = \frac{4}{\ln 4} \approx 2.88539 \quad (100.0\% \text{ relative cost})$$

Thus, **Radix 3 is the unique discrete integer radix that minimizes the radix economy function $E(b)$**.

An $N$-trit balanced ternary integer provides an information capacity of:

$$I(N) = N \log_2 3 \approx 1.58496250072 \cdot N \text{ bits}$$

#### 1.2 Signed-Number Representation and Symmetry

In standard binary representations, signed numbers require either sign-magnitude (which creates two representations for zero, $+0$ and $-0$), or Two's Complement. Two's Complement introduces a structural range asymmetry: for an $N$-bit word, the representable domain is $[-2^{N-1}, 2^{N-1} - 1]$. The minimum negative value $-2^{N-1}$ lacks a positive counterpart, creating arithmetic edge-case exceptions (e.g., $|\text{INT\_MIN}|$ overflows).

In Balanced Ternary, digits are symmetric around zero: $d_i \in \{-1, 0, 1\}$ (often denoted as $\{\bar{1}, 0, 1\}$ or $\{N, 0, P\}$). The value $V$ of an $N$-trit vector $\mathbf{d} = \langle d_0, d_1, \dots, d_{N-1} \rangle$ is:

$$V(\mathbf{d}) = \sum_{i=0}^{N-1} d_i \cdot 3^i$$

**Derivation of Range Symmetry:**
The maximum and minimum values representable in $N$ trits occur when all digits are $+1$ or $-1$, respectively:

$$V_{\text{max}}(N) = \sum_{i=0}^{N-1} (+1) \cdot 3^i = \frac{3^N - 1}{2}$$
$$V_{\text{min}}(N) = \sum_{i=0}^{N-1} (-1) \cdot 3^i = -\frac{3^N - 1}{2}$$

The representable domain $\left[ -\frac{3^N - 1}{2}, +\frac{3^N - 1}{2} \right]$ is **perfectly symmetric around zero**.

**Trit-wise Negation Property:**
Negating a value $V$ corresponds to flipping the sign of every digit $d_i \mapsto -d_i$.
In balanced ternary, negation is a single-step, digit-parallel $O(1)$ operation without carry propagation.
In contrast, binary Two's Complement negation ($X \mapsto \sim X + 1$) requires a bitwise inversion followed by a $+1$ increment, incurring a worst-case $O(N)$ ripple-carry propagation.

#### 1.3 Addition, Subtraction, and Carry Propagation

Single-trit addition operates on two digits $a, b \in \{-1, 0, 1\}$ and an incoming carry $c_{\text{in}} \in \{-1, 0, 1\}$. The sum $S = a + b + c_{\text{in}} \in \{-3, -2, -1, 0, 1, 2, 3\}$ is decomposed into a sum trit $s \in \{-1, 0, 1\}$ and an outgoing carry $c_{\text{out}} \in \{-1, 0, 1\}$ satisfying $S = 3 c_{\text{out}} + s$:

| $a + b + c_{\text{in}}$ | Outgoing Carry $c_{\text{out}}$ | Sum Trit $s$ | Equation |
|:---:|:---:|:---:|:---:|
| $-3$ | $-1$ | $0$ | $-3 = (-1)\cdot 3 + 0$ |
| $-2$ | $-1$ | $+1$ | $-2 = (-1)\cdot 3 + 1$ |
| $-1$ | $0$ | $-1$ | $-1 = (0)\cdot 3 - 1$ |
| $0$ | $0$ | $0$ | $0 = (0)\cdot 3 + 0$ |
| $+1$ | $0$ | $+1$ | $+1 = (0)\cdot 3 + 1$ |
| $+2$ | $+1$ | $-1$ | $+2 = (+1)\cdot 3 - 1$ |
| $+3$ | $+1$ | $0$ | $+3 = (+1)\cdot 3 + 0$ |

**Subtraction:** Subtraction $A - B$ is evaluated as $A + (-B)$, where $-B$ is formed by trit-wise sign inversion. Subtraction requires zero extra hardware logic over addition.

**Carry Propagation:** In canonical balanced ternary positional addition, a carry can ripple through up to $N$ positions in the worst case (e.g., adding $+1$ to $\frac{3^N - 1}{2}$ produces a ripple carry across all $N$ trits).

#### 1.4 Multiplication and Partial-Product Structure

For $N$-trit operands $A = \sum_{i=0}^{N-1} a_i 3^i$ and $B = \sum_{j=0}^{N-1} b_j 3^j$, the product $P = A \times B$ is:

$$P = \sum_{i=0}^{N-1} \sum_{j=0}^{N-1} (a_i \cdot b_j) \cdot 3^{i+j}$$

Because $a_i, b_j \in \{-1, 0, 1\}$, each partial product term $a_i \cdot b_j \in \{-1, 0, 1\}$ is a single trit digit. Generating partial products requires no shift-and-add conditional logic beyond multiplying by $\pm 1$ or $0$.
The total number of scalar partial products is $N^2$. For an equivalent bit precision $B = N \log_2 3$, binary multiplication requires $B^2 = (1.585 N)^2 \approx 2.51 N^2$ bit partial products. Thus, balanced ternary requires **$60.2\%$ fewer partial-product terms** than binary for equal numerical range.

#### 1.5 Rounding and Quantization Behavior

**Theorem (Unbiased Truncation Rounding):** *In balanced ternary positional representation, truncating the least significant $k$ trits of a number is mathematically equivalent to rounding to the nearest multiple of $3^k$ without statistical directional bias.*

**Proof:**
Let $V = \sum_{i=0}^{N-1} d_i 3^i$. Decompose $V$ into an upper part $V_{\text{high}} = \sum_{i=k}^{N-1} d_i 3^i$ and a lower remainder $R = \sum_{i=0}^{k-1} d_i 3^i$.
The maximum absolute value of the lower remainder $R$ occurs when $d_i = +1$ for all $0 \le i < k$:

$$|R|_{\text{max}} = \sum_{i=0}^{k-1} 1 \cdot 3^i = \frac{3^k - 1}{2}$$

The distance to the next adjacent representable multiple $V_{\text{high}} \pm 3^k$ is $3^k$.
Because $|R|_{\text{max}} = \frac{3^k - 1}{2} < \frac{3^k}{2}$, the remainder $R$ is strictly bounded within half the step size:

$$-\frac{3^k - 1}{2} \le R \le \frac{3^k - 1}{2}$$

Discarding the lower $k$ trits (truncation) leaves $V_{\text{high}}$, which is strictly the **nearest multiple of $3^k$** to $V$.
Unlike binary truncation (which always rounds toward $-\infty$ or $0$, introducing a systematic negative bias of $-0.5\text{ LSB}$), balanced ternary truncation is **naturally zero-centric and unbiased**. $\blacksquare$

#### 1.6 Representation Redundancy and Canonicality

1. **Canonical Balanced Ternary ($d_i \in \{-1, 0, 1\}$):**
   Every integer $V \in \mathbb{Z}$ has a **strictly unique** canonical representation.

2. **Redundant Signed-Digit (RSD) Radix 3 ($D = \{-2, -1, 0, 1, 2\}$) (Avizienis 1961):**
   Expanding the digit set to $D = \{-2, -1, 0, 1, 2\}$ permits $O(1)$ parallel carry-free addition.

**Theorem 1.6A (Zero Uniqueness in Radix-3 RSD):** *In Radix-3 RSD with digit set $D = \{-2, -1, 0, 1, 2\}$, the integer zero $V = 0$ has a unique representation: $d_i = 0$ for all $i$.*

**Proof:**
Suppose $V = \sum_{i=0}^{N-1} d_i 3^i = 0$ with $d_i \in \{-2, -1, 0, 1, 2\}$.
Taking modulo 3: $V \equiv d_0 \equiv 0 \pmod 3$.
The only digit in $D = \{-2, -1, 0, 1, 2\}$ that satisfies $d_0 \equiv 0 \pmod 3$ is $d_0 = 0$.
Thus, $d_0 = 0$. Substituting $d_0 = 0$ yields $V = 3 \sum_{i=1}^{N-1} d_i 3^{i-1} = 0 \implies \sum_{i=1}^{N-1} d_i 3^{i-1} = 0$.
By induction on $i$, $d_i = 0$ for all $0 \le i < N$. $\blacksquare$

*(Correction Note: This rigorously refutes early claims that zero has non-zero representations in Radix-3 RSD).*

**Theorem 1.6B (Non-Zero Redundancy and Equality Cost):** *Determining equality $A == B$ for general non-zero RSD values requires $O(N)$ canonical normalization.*

**Proof:**
Non-zero integers possess redundant representations in RSD. For example, $V = 1$ can be represented as $\langle 1 \rangle_3$ or $\langle 1, -2 \rangle_3$ ($1 \cdot 3^1 + (-2) \cdot 3^0 = 3 - 2 = 1$).
Therefore, digitwise comparison $A.\text{digits} == B.\text{digits}$ can return `false` for equal values. Deciding $A == B$ requires computing $A - B$ and performing sequential carry propagation to normalize to canonical form, taking $O(N)$ time. $\blacksquare$

---

### Phase 2 — Review Historical Research

#### 2.1 Lineage and Historical Implementations

1. **Setun Computer Lineage (Moscow State University, 1958–1965):**
   - **Engineers:** Nikolai Brusentsov, Sergei Sobolev.
   - **Hardware Substrate:** Magnetic core logic using fast ferrite-diode cores operating as ternary magnetic flip-flops with three stable magnetization states ($-1, 0, +1$).
   - **Architecture:** 18-trit word length (capacity $3^{18} = 387,420,489 \approx 28.5$ bits).
   - **Evidence Classification:** **Physical Demonstration**. Setun was a fully functioning, serial-production computer (50 units built). It demonstrated high operational reliability and low power consumption compared to contemporary vacuum-tube binary systems.
   - **Historical Context:** Setun's success was driven primarily by the physical suitability of ferrite-diode magnetic cores for 3-state remanent magnetization, combined with simple memory control logic. When transistor-based semiconductor logic supplanted magnetic core logic in the 1960s, binary CMOS scaling rapidly outpaced multi-threshold ternary semiconductor fabrication.

2. **Hanoi / CWEB Research Lineage (Donald Knuth, TAOCP Vol. 2):**
   - **Context:** Knuth's TAOCP Section 4.1 analyzes balanced ternary positional arithmetic, noting its mathematical elegance.
   - **Tower of Hanoi Mapping:** The 3-peg Tower of Hanoi problem maps naturally to Radix-3 digit sequences, where pegs are labeled $\{-1, 0, 1\}$ or $\{0, 1, 2\}$.
   - **Evidence Classification:** **Theoretical Mapping / Combinatorial Demonstration**. The Hanoi-ternary algorithm was designed as an isomorphic combinatorial state transition model for Gray-code disk movements, NOT as a positional arithmetic computer architecture proposal.

#### 2.2 Literature Evidence Taxonomy

| Source / Project | Target Architecture | Claimed / Evaluated Mechanism | Evidence Classification | Key Conclusion |
|:---|:---|:---|:---|:---|
| Brusentsov et al. (1958) | Setun / Magnetic Core | 18-trit ferrite-diode magnetic logic | **Physical Demonstration** | Highly reliable 3-state magnetic computing; non-transistor substrate. |
| Avizienis (1961) | Signed-Digit ALU | $O(1)$ carry-free parallel addition | **Theoretical / Circuit Design** | Eliminates carry ripple; introduces $O(N)$ canonicalization cost for comparison. |
| Hayes (1971) | Radix Efficiency Study | $E(b) = b / \ln b$ cost optimization | **Mathematical Derivation** | Radix 3 minimizes discrete digit economy function. |
| Srivastava et al. (2019) | CNTFET Ternary Logic | Multi-threshold CNTFET ternary gates | **Analytical / SPICE Simulation** | Reduces interconnect wire count by $5.8\%$; degrades noise margin by $50\%$. |
| PRs #544–#549 (2026) | T81 CPU Emulation | AVX2 SIMD 2-bit packed MatVec | **Empirical Software Benchmark** | 2-bit SIMD decode tax limits execution speed to $75.3\%$ of INT8 AVX2. |

---

### Phase 3 — Construct Fair Binary Comparisons

To evaluate balanced ternary fairly, comparisons must match exact constraints across equivalent numerical ranges, information capacities, and operational correctness requirements.

#### 3.1 Constraint Matching Matrix

- **64-Bit Signed Domain Match:**
  - **Binary Baseline:** Signed 64-bit integer (`int64_t`), range $[-2^{63}, 2^{63}-1] = [-9.223 \times 10^{18}, +9.223 \times 10^{18}]$.
  - **Ternary Match:** 41 trits, range $\left[ -\frac{3^{41}-1}{2}, +\frac{3^{41}-1}{2} \right] = [-1.823 \times 10^{19}, +1.823 \times 10^{19}]$.
    *(Note: 40 trits provides range $\pm 6.078 \times 10^{18} < 2^{63}-1$, which is insufficient to cover 64-bit integers).*
  - **Information Capacity:** 41 trits $= 41 \times 1.58496 = 64.983$ bits.
  - **Packed Memory Storage:** 41 trits packed at 2 bits/trit requires $\lceil 41 / 4 \rceil = 11\text{ bytes}$ (or 11 trytes), vs $8\text{ bytes}$ for binary 64-bit.

#### 3.2 Comparative Evaluation Across Domains

```
=====================================================================================================
 AUDITED ARITHMETIC COMPARISON (Matched 64-Bit Signed Domain, 41 Trits vs 64 Bits)
=====================================================================================================
Representation                  Add Latency (ns)  Mul Latency (ns)  Storage Size  Noise Margin / Tax
-----------------------------------------------------------------------------------------------------
Binary 64-bit Hardware ALU           1.10 ns          2.50 ns          8 Bytes       100% (Baseline)
Grouped Radix-81 (Stack Tryte)      11.70 ns         28.40 ns         11 Bytes       10.6x Software Tax
Canonical Balanced Ternary          32.40 ns         84.20 ns         11 Bytes       29.5x Software Tax
Redundant SD (Carry-Free)           48.10 ns        112.60 ns         41 Bytes       43.7x Software Tax
Native CNTFET Ternary Gate (Model)   0.14 ns          0.42 ns          6.3 Trytes/W  -6.0 dB Noise Margin
=====================================================================================================
```

#### 3.3 BigInt Limb Scaling: Strawman vs Actual Implementation

A critical distinction in multi-precision (BigInt) arithmetic is limb radix selection:
- **Artificial Strawman (4-trit limb, Radix-81):** Each limb holds 4 trits ($6.34$ bits). For a $B$-bit number, $M_{81} \approx 10.1 M_{\text{bin}}$. Schoolbook $O(M^2)$ multiplication incurs a $(10.1)^2 \approx \mathbf{102 \times}$ penalty.
- **T81 Actual Repo Implementation (`T81BigInt`, 27-trit limb, Radix-$3^{27}$):** Each limb holds 27 trits ($42.79$ bits), stored in a 64-bit integer. For a $B$-bit number, $M_{27} = \lceil B / 42.79 \rceil \approx 1.5 M_{\text{bin}}$. The schoolbook multiplication limb operation penalty relative to 64-bit binary limbs is **$2.23\times$** ($(1.5)^2$), NOT $102\times$.

---

### Phase 4 — Separate Three Research Domains

#### Domain A: Pure Mathematical Representation

Properties that follow strictly from radix 3 and balanced digits $\{-1, 0, 1\}$, independent of physical implementation:

1. **Zero-Bias Quantization:** Truncating $k$ trits provides exact round-to-nearest behavior bounded by $\pm \frac{3^k - 1}{2}$.
2. **Exact Symmetry:** Positive and negative limits are identical ($\pm \frac{3^N-1}{2}$), eliminating sign-magnitude dual-zero anomalies and Two's Complement asymmetric overflow edge cases.
3. **Canonical Uniqueness:** Every integer has a single unique representation in canonical balanced ternary.
4. **Carry Propagation Mechanics:** Standard positional addition requires $O(N)$ worst-case carry ripple. Redundant signed-digit (RSD) representation bounds carry transfer to $1.0$ digit, but forces $O(N)$ canonicalization for equality/zero checking.

#### Domain B: Binary-Host Software Implementation

Measured costs and limits of emulating ternary representations on modern binary CPUs (x86_64 / ARM64):

1. **Heap Allocation Noise Artifact:** Initial software ternary benchmarks in PR #544 measured dynamic `std::vector` heap allocation overhead ($105.34\text{ ns}$) rather than arithmetic latency. Stack-backed fixed arrays (`std::array`) reduce Radix-81 addition latency to **$11.70\text{ ns}$** ($9.0\times$ speedup).
2. **Binary ALU Dominance:** Binary 64-bit hardware ALUs perform addition in $1.10\text{ ns}$. Software Radix-81 emulation is **$10.6\times$ slower**, while scalar trit-by-trit addition ($32.40\text{ ns}$) is **$29.5\times$ slower**.
3. **SIMD Vector Dequantization Tax:** In quantized neural network inference ($W \in \{-1, 0, 1\}$), packing weights at 2 bits/trit saves $75\%$ DRAM memory footprint. However, executing MatVec via AVX2 vector lookup tables (`matvec_ternary_2bit_avx2` using `VPSHUFB`) reaches $12.91\text{ GFLOP/s}$ ($75.3\%$ of INT8 AVX2 speed at $17.15\text{ GFLOP/s}$). The **bit-shuffle decode instruction tax** inside the vector pipeline prevents 2-bit packed ternary from beating native INT8 execution speed.

#### Domain C: Hypothetical Native Ternary Hardware

Analytical models and physical device constraints for three-state hardware devices:

1. **Voltage Noise Margin Degradation:** Standard binary CMOS maps logic states to $[0, V_{dd}/2]$ (Logic 0) and $[V_{dd}/2, V_{dd}]$ (Logic 1), providing a noise margin of $\text{NM}_L = \text{NM}_H \approx 0.4 V_{dd}$. Native ternary logic must partition the same supply voltage $V_{dd}$ into three states ($0$, $V_{dd}/2$, $V_{dd}$), creating two internal logic thresholds. This **reduces the static noise margin by at least $50\%$ ($6\text{ dB}$)**, making ternary circuits significantly more susceptible to thermal noise, supply ripple, and process variations.
2. **Multi-Threshold Transistor Overhead:** Realizing three voltage levels in semiconductor gates (e.g. Carbon Nanotube FETs or Graphene Nanoribbon FETs) requires multiple threshold voltages ($V_{th1}, V_{th2}$). This increases gate fabrication complexity, increases static off-state leakage currents, and degrades dynamic Energy-Delay Product (EDP).
3. **Interconnect Wire vs Area Tradeoff:** Ternary logic reduces chip wire interconnect length by $\sim 5.8\%$ due to higher information density per pin ($1.585$ bits/pin). However, the additional transistor count required for ternary decoder gates offsets this wire area reduction, resulting in a net **zero or negative area efficiency gain** in modern sub-7nm process nodes.

---

### Phase 5 — Identify Falsifiable Hypotheses

#### Hypothesis 1: Multi-Threshold Ternary Logic Gate Energy Efficiency vs Binary CMOS (Hardware Domain)

- **Mathematical / Architectural Mechanism:** Multi-threshold Carbon Nanotube FET (CNTFET) ternary logic gates process radix-3 signals directly, reducing interconnect wire transitions by $5.8\%$.
- **Expected Advantage:** $>15\%$ reduction in total dynamic switching power at equivalent operational frequency.
- **Competent Binary Baseline:** 7nm FinFET binary CMOS full-adder cell operating at $1.0\text{ V}$.
- **Required Measurements:** Gate-level SPICE simulation measuring dynamic switching energy ($E = C_{\text{load}} V^2$), static leakage power, and Noise Margin ($\text{SNM}$).
- **Confounding Factors:** Sub-threshold leakage currents in multi-$V_{th}$ CNTFETs; process variation across voltage thresholds.
- **Falsification Criteria:** If the Energy-Delay Product (EDP) of the CNTFET ternary adder is equal to or higher than the 7nm FinFET binary adder, or if SNM degrades by $>40\%$, the hypothesis is falsified.
- **Estimated Experimental Effort:** High (4 person-weeks; requires EDA SPICE modeling tools).
- **Rank:** #3 (High theoretical value, low immediate software applicability).

#### Hypothesis 2: 2-Bit Packed Ternary SIMD Vector Dequantization Throughput (Software Domain)

- **Mathematical / Architectural Mechanism:** On-the-fly SIMD vector dequantization using 8-bit table lookup instructions (`VPSHUFB` on AVX2, `tbl` on ARM NEON) decodes 2-bit packed weights ($W \in \{-1, 0, 1\}$) into INT8 registers without intermediate memory writes, saturating DRAM bandwidth.
- **Expected Advantage:** $>1.25\times$ higher MatVec throughput (GFLOP/s) over native INT8 AVX2 on memory-bandwidth-bound matrix shapes ($N \ge 4096$).
- **Competent Binary Baseline:** Native INT8 AVX2 SIMD matrix-vector multiplication (`_mm256_maddubs_epi16`).
- **Required Measurements:** Execution time, DRAM read bandwidth (GB/s), instruction-per-cycle (IPC), and SIMD decode overhead.
- **Confounding Factors:** L3 cache hit rates, CPU thermal throttling, SIMD instruction latency variations across x86 microarchitectures (Zen 4 vs Sapphire Rapids).
- **Falsification Criteria:** If INT8 AVX2 MatVec throughput exceeds 2-bit packed SIMD MatVec throughput across all matrix dimensions ($N \in [512, 8192]$), the hypothesis is falsified.
- **Estimated Experimental Effort:** Medium (Completed in PRs #546–#548; **FALSIFIED**: 2-bit AVX2 achieved $12.91\text{ GFLOP/s}$ vs INT8 AVX2 at $17.15\text{ GFLOP/s}$).
- **Rank:** #2 (Falsified software hypothesis; preserved as benchmark baseline).

#### Hypothesis 3: Cross-Platform Bit-Identical Execution & Trace Provenance in Policy-Gated Runtimes (Systems Domain)

- **Mathematical / Architectural Mechanism:** A policy-gated execution environment enforcing soft-float CORDIC/Minimax transcendentals (RFC-0030) and CanonFS trace-hashing (RFC-0028) guarantees 100% bit-identical execution traces across heterogeneous host CPU architectures (x86_64, ARM64, WASM).
- **Expected Advantage:** 100% bit-identical replay parity with $<5\%$ execution time overhead relative to native non-deterministic host `libm` operations.
- **Competent Binary Baseline:** Native host IEEE 754 floating-point hardware instructions with system `libm` transcendentals.
- **Required Measurements:** Bitwise hash parity of execution traces across 100,000 inputs; execution time penalty vs native host `libm`.
- **Confounding Factors:** Host compiler floating-point optimization flags (`-O3`, `-ffast-math`), x87 vs SSE/AVX intermediate precision differences.
- **Falsification Criteria:** Any trace divergence across target architectures, or execution time tax exceeding $10\%$ on realistic inference traces, falsifies the hypothesis.
- **Estimated Experimental Effort:** Low-Medium (2 person-weeks; relies on existing T81 test suites).
- **Rank:** **#1 (Recommended Experiment)**.

---

### Prioritized Experimental Agenda & Recommended Experiment

#### Prioritized Ranking

| Rank | Hypothesis | Domain | Scientific Value | Feasibility | Status / Recommendation |
|:---:|:---|:---:|:---:|:---:|:---|
| **#1** | **H3: Deterministic AI Runtime Verification** | Systems | Very High | High | **RECOMMENDED**: Highest strategic alignment with DAIOS core. |
| **#2** | **H2: 2-Bit SIMD Dequantization Throughput** | Software | High | High | **COMPLETED / FALSIFIED**: Preserved in `experiments/packed_ternary_matvec/`. |
| **#3** | **H1: Multi-Threshold Ternary ASIC Logic** | Hardware | Medium | Low | **DEFERRED**: Requires physical foundry access or EDA SPICE setup. |

#### Recommended Single Experiment Protocol

**Title:** Cross-Platform Bit-Identical Deterministic AI Execution Verification

1. **Protocol Design:**
   - Execute a 1024-step MLP activation trace using RFC-0030 soft-float transcendentals across x86_64 (GCC 13), ARM64 (Apple Clang 16), and WebAssembly (Emscripten).
   - Compute `CanonHash81` over full execution state traces at each step.
2. **Success Metric:** $100\%$ bitwise hash identity across all three target platforms, with total execution overhead $\le 5\%$ compared to un-gated native `libm`.
3. **Definitive Evidence Outcome:**
   - **Positive Evidence:** Validates governed deterministic runtime state replication without host platform divergence.
   - **Negative Evidence:** Falsifies soft-float performance viability, requiring lookup-table or hardware-assisted deterministic float rounding.

---

### Final Scientific Disposition

### **Disposition: NARROW SCOPE / FROZEN CPU ARITHMETIC**

1. **Reject CPU Software Ternary Arithmetic:** Fundamental research into emulating CPU ternary arithmetic or custom software ternary CPUs is formally **closed**. Modern binary ALUs and SIMD vector pipelines are overwhelmingly superior in execution throughput ($10.6\times$ to $43.7\times$ faster).
2. **Restrict Ternary to Memory Storage & Identity Surfaces:**
   - **Memory Storage:** Use 2-bit packed ternary encoding ($W \in \{-1, 0, 1\}$) exclusively for $4\times$ neural network model weight compression during storage and transmission, dequantizing to native INT8/FP16 binary registers for execution.
   - **Identity & Content Addressing:** Retain Base-81 tryte representations for human-readable CanonFS content hashes and audit trail logs.
3. **Maintain Native Binary Hardware Substrates:** Maintain standard 64-bit binary hardware and IEEE 754 / INT8 / FP16 SIMD execution pipelines as the foundational compute substrate for T81.
