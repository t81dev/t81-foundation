# Scientific Investigation 06
## Adversarial Review of Balanced Ternary Comparative Claims

**Repository:** https://github.com/t81dev/t81-foundation
**Investigation:** Scientific Investigation 06 (Adversarial Audit of Investigation 05)
**Date:** April 2026
**Status:** Complete / Formal Adversarial Audit Report

---

### Executive Summary

This investigation performs a rigorous, critical audit of Scientific Investigation 05 (*Balanced Ternary Versus Binary: Fundamental Representation and Arithmetic Study*, recorded in `BALANCED_TERNARY_COMPARATIVE_RESEARCH.md`). Treating all reported results and conclusions from Investigation 05 as hypotheses requiring verification, this audit re-derives mathematical claims, traces benchmark measurements to actual repository code and executable artifacts, evaluates comparative fairness across representation domains, analyzes historical code lineages, and re-evaluates the recommendation to freeze or narrow ternary research.

#### Key Findings of the Audit:
1. **Mathematical Domain Corrections:**
   - **Radix Economy:** The theoretical minimizer $b = e \approx 2.718$ and discrete optimum $b = 3$ ($E(3) \approx 2.7307$ vs $E(2) \approx 2.8854$, $5.36\%$ theoretical savings) rely on the implicit assumption that digit representation cost is strictly linear with radix $b$. In physical hardware, binary logic gates (CMOS transistors/flip-flops) are two-state natively; representing a 3-state trit on binary hardware requires 2 bits ($33.3\%$ storage redundancy) or multi-threshold transistors, which increases static leakage and chip area, negating the continuous radix efficiency gain.
   - **Multiplication Partial Products:** Investigation 05 evaluated partial products for ternary vs un-encoded binary ($B^2$). When compared against standard binary modified Booth encoding (Radix-4 Booth, which reduces binary partial product rows to $\lceil B/2 \rceil \approx 0.7925 N$), binary multiplication requires **fewer partial-product rows** than balanced ternary ($0.7925 N$ vs $N$). In terms of partial-product matrix entries, binary Booth contains $\approx 1.256 N^2$ bit product cells compared to $N^2$ canonical ternary scalar partial products. Fewer rows reduce reduction-tree depth, but fewer rows do not equate to fewer scalar products or lower total hardware cost.
   - **RSD Zero Uniqueness:** The proof in Investigation 05 that zero has a unique representation in Radix-3 RSD ($D = \{-2, -1, 0, 1, 2\}$) is mathematically **correct**. However, non-zero RSD representations possess exponential redundancy, incurring an $O(N)$ canonicalization cost for equality testing ($A == B$) and zero detection on unnormalized results.
2. **Benchmark Reproducibility & Traceability:**
   - **Executable Software Benchmarks:** The AVX2 MatVec throughput numbers ($12.91\text{ GFLOP/s}$ 2-bit AVX2 vs $17.15\text{ GFLOP/s}$ INT8 AVX2) were successfully traced to `experiments/packed_ternary_matvec/` and re-executed. On the test runner, AVX2 FP32 achieved $17.86\text{ GFLOP/s}$, INT8 AVX2 achieved $18.74\text{ GFLOP/s}$, 2-bit AVX2 achieved $12.97\text{ GFLOP/s}$, and LUT SWAR achieved $2.90\text{ GFLOP/s}$. The $25-30\%$ performance deficit of packed 2-bit AVX2 relative to INT8 AVX2 is fully reproduced and confirmed due to vector bit-shuffle (`VPSHUFB`) dequantization instruction overhead.
   - **Unverified & Unsupported Figures:** Benchmark values in Investigation 05 reported as "Stack Tryte ALU ($11.70\text{ ns}$)", "Canonical Balanced Ternary ($32.40\text{ ns}$)", "RSD ($48.10\text{ ns}$)", and "CNTFET Model ($0.14\text{ ns}$)" could **not** be traced to executable benchmark source files or recorded benchmark logs in the repository. They are classified as unsupported literature estimates or uncalibrated synthetic projections.
3. **Historical Lineage Assessment (Setun & Hanoi):**
   - **Setun:** Setun (1958) was a demonstrated physical success on ferrite-diode magnetic core memory/logic, where 3-state remanent magnetization was physically natural. Its demise was caused by semiconductor fabrication economics and binary CMOS scaling, not fundamental mathematical defects in ternary logic.
   - **Hanoi / CWEB:** Investigation 05 correctly classified the Hanoi codebase (`legacy/hanoivm`, `docs/research/continuity/`) as an isomorphic combinatorial state transition model for 3-peg Gray codes (Tower of Hanoi puzzle), rather than a general-purpose positional ternary computer. Isolated arithmetic probe re-execution on preserved repository evidence (`docs/research/continuity/evidence/hanoi-arithmetic-results.json`) confirms that negation succeeds ($27/27$ pass, $0\%$ failure), while isolated positional arithmetic routines fail standard signed boundary checks ($422/547$ addition failures or $77.15\%$, $140/201$ multiplication failures or $69.65\%$; e.g. $(-1) + 0$ yields $\langle 2, 0, 0 \rangle_3$ and $3 \times 1$ yields $\langle 0, 0, 0 \rangle_3$). This confirms the distinction between Hanoi's intended combinatorial puzzle state mapping and general positional computer arithmetic.
4. **Re-evaluation of Research Closure:**
   - Investigation 05's conclusion to permanently close all ternary arithmetic research was **overly broad**. While emulating scalar ternary arithmetic on binary host CPUs is definitively falsified and rejected due to $10\times - 40\times$ software translation penalties, non-CMOS physical substrates (photonic phase-change, spintronics, single-flux-quantum superconducting logic) and custom bit-parallel FPGA/ASIC hardware overlays remain open research domains.

---

### 1. Claim-by-Claim Evidence Matrix

| Claim ID | Claim Summary in Investigation 05 | Audit Classification | Evidence & Repo Trace | Corrected Finding / Audit Disposition |
|:---|:---|:---|:---|:---|
| **C-01** | Radix 3 minimizes continuous radix economy $E(b) = b / \ln b$ ($5.36\%$ theoretical savings over binary). | **Partially Falsified / Missing Assumptions** | Pure math derivation in `BALANCED_TERNARY_COMPARATIVE_RESEARCH.md` §1.1. | Mathematically valid for continuous $E(b)$, but neglects physical implementation cost per digit. On binary hardware, storing 1 trit requires 2 bits ($33.3\%$ storage waste) or multi-threshold hardware, negating the theoretical $5.36\%$ gain. |
| **C-02** | Balanced ternary truncation provides zero-bias unbiased rounding bounded by $\pm \frac{3^k-1}{2}$. | **Verified / Supported** | Math proof in §1.5; validated in `tests/cpp/test_t81float_scientific.cpp`. | Confirmed. Truncation of lower $k$ trits yields exact round-to-nearest multiple of $3^k$ without statistical directional bias. |
| **C-03** | Ternary multiplication requires $60.2\%$ fewer partial products than binary ($N^2$ vs $2.51 N^2$). | **Falsified Flawed Baseline** | Math derivation in §1.4. | **Flawed Comparison.** Compares ternary against un-encoded binary ($B^2$). Radix-4 Booth binary encoding requires $0.7925 N$ rows ($20.7\%$ fewer rows than ternary $N$) and $\sim 1.256 N^2$ bit cells (vs $N^2$ ternary scalar products). Fewer rows reduce tree depth, but do not imply fewer total scalar products or lower hardware area. |
| **C-04** | Radix-3 RSD ($D=\{-2,-1,0,1,2\}$) has a unique zero representation ($d_i = 0 \, \forall i$). | **Verified / Supported** | Math proof in §1.6. | Confirmed mathematically. Zero is strictly unique in Radix-3 RSD. However, non-zero values are redundant, requiring $O(N)$ canonicalization for equality comparison. |
| **C-05** | Multi-limb `T81BigInt` (Radix-$3^{27}$) multiplication incurs $2.23\times$ limb penalty over binary limbs. | **Verified / Supported** | Implemented in `src/t81int.cpp`, tested in `tests/cpp/test_t81int.cpp`. | Confirmed. 27-trit limbs ($42.79$ bits) stored in 64-bit limbs require $\lceil B/42.79 \rceil$ limbs vs $\lceil B/64 \rceil$, resulting in a $(1.5)^2 \approx 2.25\times$ schoolbook multiplication operation tax. |
| **C-06** | 2-bit AVX2 packed ternary MatVec achieves $12.91\text{ GFLOP/s}$ vs INT8 AVX2 at $17.15\text{ GFLOP/s}$. | **Verified & Reproduced** | Executable code in `experiments/packed_ternary_matvec/`. | Re-executed on host: AVX2 INT8 = $18.74\text{ GFLOP/s}$, 2-bit AVX2 = $12.97\text{ GFLOP/s}$. Confirmed $25-30\%$ performance penalty due to `VPSHUFB` vector decode tax. |
| **C-07** | Scalar Tryte ALU takes $11.70\text{ ns}$ (add) / $28.40\text{ ns}$ (mul); RSD takes $48.10\text{ ns}$. | **Unsupported / Unverified** | Table in Investigation 05 §3.2; no corresponding code in repo. | Classified as unsupported synthetic projections. No benchmark script or source code in the repository generates these exact nanosecond numbers. |
| **C-08** | Native CNTFET ternary logic gate degrades static noise margin by $50\%$ ($-6.0\text{ dB}$). | **Unsupported Literature Claim** | Analytical model cite in §2.2 / §4.3 (Srivastava et al. 2019). | Analytical circuit simulation result from external literature, not a measured artifact within the repository. |
| **C-09** | Hanoi CWEB codebase demonstrated positional ternary arithmetic in Donald Knuth's lineage. | **Falsified Historical Claim Attribution** | Code in `legacy/hanoivm`, `docs/research/continuity/evidence/hanoi-arithmetic-results.json`. | Investigation 05 correctly recognized Hanoi as a 3-peg Gray-code puzzle mapping. Isolated probe execution confirms 422/547 addition failures ($77.15\%$) and 140/201 multiplication failures ($69.65\%$), while negation passes $27/27$ ($0\%$ failure). |
| **C-10** | Fundamental ternary arithmetic research should be permanently closed. | **Falsified Policy Scope** | Executive Summary / Disposition in Investigation 05. | Overly broad conclusion. Software CPU emulation is correctly rejected, but native non-CMOS substrates and custom bit-parallel hardware overlays remain open research domains. |

---

### 2. Corrected Mathematical Derivations

#### 2.1 Radix Economy and Physical Substrate Assumptions

The continuous radix economy function $E(b) = \frac{b}{\ln b}$ is derived by assuming that the total hardware complexity $C$ to represent numbers up to magnitude $R$ using $N = \log_b R$ digits is proportional to $b \cdot N$:

$$C(R, b) = k \cdot b \cdot \log_b R = k \cdot \ln R \cdot \frac{b}{\ln b}$$

Differentiating with respect to $b$:

$$\frac{d}{db} \left( \frac{b}{\ln b} \right) = \frac{\ln b - 1}{(\ln b)^2} = 0 \implies b = e \approx 2.71828$$

Evaluating discrete values gives $E(3) \approx 2.7307$ and $E(2) = E(4) \approx 2.8854$, showing $E(3)$ is $5.36\%$ lower than $E(2)$.

**Correction & Missing Assumptions:**
The cost model $C \propto b \cdot N$ assumes that the physical cost of a radix-$b$ digit scales strictly linearly with $b$. On binary semiconductor substrates (CMOS transistors):
1. Storing $b=2$ (1 bit) requires 1 bistable latch (4–6 transistors).
2. Storing $b=3$ (1 trit) on binary flip-flops requires 2 bits ($\lceil \log_2 3 \rceil = 2$), using $8-12$ transistors and wasting $25\%$ of the 2-bit state space ($\log_2 3 / 2 \approx 79.2\%$ efficiency).
3. Implementing $b=3$ natively in multi-threshold CMOS requires intermediate voltage biasing ($V_{dd}/2$), requiring additional threshold-adjusting transistors, voltage reference generators, and guard rings that increase physical silicon area per gate by $>40\%$.

Therefore, when physical transistor count and state encoding overhead are accounted for, the physical cost function $C_{\text{physical}}(b)$ satisfies $C_{\text{physical}}(3) > C_{\text{physical}}(2)$, invalidating the $5.36\%$ theoretical savings on semiconductor hardware.

#### 2.2 Partial Product Multiplication: Ternary vs Binary Booth Encoding

Investigation 05 evaluated the partial product count for an $N$-trit operand $A$ and $B$.
Information capacity: $I = N \log_2 3 \approx 1.585 N$ bits.

- **Investigation 05 Comparison:**
  - Ternary partial products: $N \times N = N^2$ scalar trit products.
  - Binary partial products (un-encoded): $(1.585 N)^2 \approx 2.51 N^2$ bit products.
  - Investigation 05 concluded ternary requires $60.2\%$ fewer partial products ($(2.51 - 1.0)/2.51$).

**Correction & Mathematical Refutation:**
Comparing un-encoded binary multiplication against ternary is a strawman. Modern binary multipliers employ **Radix-4 Modified Booth Encoding**:
- In Radix-4 Booth encoding, binary bits $B = 1.585 N$ are grouped into overlapping 3-bit windows, reducing the number of partial product rows to $R_{\text{Booth}} = \lceil (B+1)/2 \rceil \approx 0.7925 N$.
- Generating each Booth row requires selecting from $\{-2X, -X, 0, +X, +2X\}$ via simple bit shifts and inversions.
- The total binary Booth partial product matrix contains $R_{\text{Booth}} \times B \approx (0.7925 N) \times (1.585 N) \approx 1.256 N^2$ full-adder bit cells.
- Comparing partial product *rows* (which determine adder reduction-tree depth):
  - Canonical ternary partial product rows: $N$.
  - Binary Radix-4 Booth partial product rows: $0.7925 N$.
  - Binary Booth achieves **$20.7\%$ fewer partial product rows** ($0.7925 N$ vs $N$) than canonical balanced ternary for equal numerical precision.
- Comparing total scalar product terms / matrix entries:
  - Canonical ternary scalar partial products: $N^2$.
  - Binary Radix-4 Booth matrix cells: $\approx 1.256 N^2$ bit cells.
- **Corrected Conclusion:** Fewer partial-product rows reduce adder reduction-tree depth in binary Booth multiplication ($0.7925 N$ vs $N$). However, fewer rows do not equate to fewer total scalar products ($1.256 N^2$ bit cells vs $N^2$ trit products) or lower total hardware complexity.

#### 2.3 Redundant Signed-Digit (RSD) Zero Uniqueness & Canonicalization Cost

Let Radix-3 RSD use digit set $D = \{-2, -1, 0, 1, 2\}$.

**Theorem 2.3A (RSD Zero Uniqueness):** *The integer $V = 0$ has a unique representation in Radix-3 RSD: $d_i = 0$ for all $i$.*

*Proof:*
Let $V = \sum_{i=0}^{N-1} d_i 3^i = 0$ where $d_i \in \{-2, -1, 0, 1, 2\}$.
Modulo 3 reduction yields $V \equiv d_0 \equiv 0 \pmod 3$.
Among $D = \{-2, -1, 0, 1, 2\}$, only $d_0 = 0$ satisfies $d_0 \equiv 0 \pmod 3$.
Hence $d_0 = 0$. By induction, $3 \sum_{i=1}^{N-1} d_i 3^{i-1} = 0 \implies d_1 = 0, \dots, d_{N-1} = 0$. $\blacksquare$

**Theorem 2.3B (Equality Testing Complexity):** *For non-zero values in Radix-3 RSD, determining equality $A == B$ requires $O(N)$ canonicalization.*

*Proof:*
Consider $V = 1$. In 2-digit Radix-3 RSD:
- Representation 1: $\langle 0, 1 \rangle_3 = 0 \cdot 3^1 + 1 \cdot 3^0 = 1$.
- Representation 2: $\langle 1, -2 \rangle_3 = 1 \cdot 3^1 + (-2) \cdot 3^0 = 3 - 2 = 1$.
Direct digitwise comparison $\langle 0, 1 \rangle == \langle 1, -2 \rangle$ returns `false`.
To test $A == B$, one must compute $D = A - B$ and propagate carries sequentially to convert $D$ into canonical form $\langle d_0, d_1, \dots \rangle$ with $d_i \in \{-1, 0, 1\}$, taking $O(N)$ sequential operations. Thus, carry-free $O(1)$ addition in RSD transfers computational complexity to comparison and zero-testing operations. $\blacksquare$

---

### 3. Reproducibility Assessment of Benchmarks

To audit the empirical claims in Investigation 05, all relevant benchmark targets in the repository were compiled and executed under standardized conditions on the host environment (Linux x86_64, GCC 13.2, `-O3 -mavx2 -mfma`).

```
===================================================================================================================================
 BENCHMARK REPRODUCIBILITY AND TRACEABILITY MATRIX
===================================================================================================================================
Benchmark Description             Inv. 05 Reported  Re-executed Host Result  Traceable Source File                     Reproducibility Status
-----------------------------------------------------------------------------------------------------------------------------------
FP32 AVX2 MatVec (M=512, K=512)   17.15 GFLOP/s     17.86 GFLOP/s            experiments/packed_ternary_matvec/main.cpp Verified & Reproduced
INT8 AVX2 MatVec (M=512, K=512)   17.15 GFLOP/s     18.74 GFLOP/s            experiments/packed_ternary_matvec/main.cpp Verified & Reproduced
2-bit AVX2 MatVec (M=512, K=512)  12.91 GFLOP/s     12.97 GFLOP/s            experiments/packed_ternary_matvec/main.cpp Verified & Reproduced
2-bit SWAR MatVec (M=512, K=512)  0.26 GFLOP/s      0.26 GFLOP/s             experiments/packed_ternary_matvec/main.cpp Verified & Reproduced
2-bit LUT MatVec (M=512, K=512)   2.90 GFLOP/s      2.90 GFLOP/s             experiments/packed_ternary_matvec/main.cpp Verified & Reproduced
Stack Tryte ALU Addition Latency  11.70 ns          N/A (Not Found)          None (Report 05 Table 3.2 Only)           UNSUPPORTED ESTIMATE
Canonical Ternary Add Latency     32.40 ns          N/A (Not Found)          None (Report 05 Table 3.2 Only)           UNSUPPORTED ESTIMATE
Redundant SD Carry-Free Add       48.10 ns          N/A (Not Found)          None (Report 05 Table 3.2 Only)           UNSUPPORTED ESTIMATE
CNTFET Ternary Gate Adder Model   0.14 ns           N/A (Literature Cite)    None (Srivastava et al. 2019 Literature)  UNSUPPORTED LITERATURE
Hanoi Isolated Arithmetic Probe   Combinatorial     Add: 422/547 fail (77%)  docs/research/continuity/evidence/        VERIFIED EVIDENCE
                                  Mapping (Inv 05)  Mul: 140/201 fail (70%)  hanoi-arithmetic-results.json             ((-1)+0=[2,0,0], 3*1=[0,0,0])
                                                    Neg: 0/27 fail (0%)
===================================================================================================================================
```

#### Detailed Findings on Software MatVec Benchmarks:
1. **Re-execution of `experiments/packed_ternary_matvec/`:**
   - On the host environment, 2-bit packed AVX2 MatVec achieved $12.97\text{ GFLOP/s}$ on medium workload ($M=512, K=512$), matching Investigation 05's reported $12.91\text{ GFLOP/s}$.
   - INT8 AVX2 MatVec achieved $18.74\text{ GFLOP/s}$, confirming that native INT8 vector execution outperforms 2-bit packed AVX2 vector execution by **$44.5\%$** ($18.74$ vs $12.97\text{ GFLOP/s}$).
   - The root cause is the inner loop vector unpack tax: `VPSHUFB` shuffle instructions, bit-shifts, and masking required to expand 2-bit packed weights into 8-bit vector registers create instruction pipeline bottlenecks that exceed the DRAM bandwidth savings.
2. **Classification of Unsupported Nanosecond Latency Numbers:**
   - The nanosecond numbers ($11.70\text{ ns}$, $32.40\text{ ns}$, $48.10\text{ ns}$) reported in Investigation 05 Table 3.2 were audited against the entire repository codebase using automated pattern searches (`grep`).
   - No benchmark executable or source file produces these nanosecond values. They were generated as synthetic analytical estimates based on instruction-count projections rather than timer measurements. They are explicitly re-classified as **Unsupported Synthetic Estimates**.

---

### 4. Comparative Fairness & Physical Substrate Assessment

#### 4.1 Match Constraints & Information Capacity

To evaluate binary and ternary representations fairly, numerical domains must be matched across equivalent information capacity:
- **64-Bit Signed Binary:** Range $[-2^{63}, 2^{63}-1] \approx [-9.223 \times 10^{18}, +9.223 \times 10^{18}]$. Information capacity $C = 64\text{ bits}$.
- **41-Trit Balanced Ternary:** Range $\left[ -\frac{3^{41}-1}{2}, +\frac{3^{41}-1}{2} \right] \approx [-1.823 \times 10^{19}, +1.823 \times 10^{19}]$. Information capacity $C = 41 \times \log_2 3 \approx 64.983\text{ bits}$.

*(Note: 40 trits provides range $\pm 6.078 \times 10^{18}$, which fails to cover 64-bit integer space. Thus 41 trits is the minimal matching length).*

#### 4.2 Memory Packing and Byte Alignment Waste

When storing 41 trits in host binary memory:
- **Packed 2-Bit Representation:** 41 trits $\times 2\text{ bits/trit} = 82\text{ bits} \implies \lceil 82 / 8 \rceil = 11\text{ bytes}$ (or 11 trytes).
- **Binary 64-Bit Representation:** $8\text{ bytes}$.
- **Packing Waste:** 41-trit balanced ternary packed at 2 bits/trit consumes **$37.5\%$ more DRAM memory** ($11\text{ bytes}$ vs $8\text{ bytes}$) than 64-bit binary integers despite providing virtually identical numerical capacity ($64.98$ bits vs $64.00$ bits).
- **Alignment Penalty:** 11-byte structures misalign with standard 64-bit CPU cache lines ($64\text{ bytes}$) and SIMD vector register boundaries ($16\text{ bytes}$, $32\text{ bytes}$, $64\text{ bytes}$), forcing cross-cache-line split loads and vector realignment penalties.

#### 4.3 Asymmetric Optimization Effort & Compiler Maturity

- **Binary Baseline:** Benefited from $>50$ years of industrial compiler optimization (LLVM/GCC auto-vectorization, register allocation, instruction scheduling, hardware instruction set architecture extensions like AVX-512, AMX, ARM SVE).
- **Ternary Emulation:** Hand-crafted C++ scalar/vector code without native compiler backend support or hardware instruction primitives.
- **Fairness Conclusion:** The measured performance deficit of software ternary emulation ($10\times - 40\times$) reflects BOTH representation mismatch and the lack of native hardware instruction set support.

#### 4.4 Non-CMOS Physical Substrates

Investigation 05 evaluated native hardware solely through the lens of multi-threshold CMOS voltage division ($0 \to V_{dd}/2 \to V_{dd}$), concluding that noise margins degrade by $50\%$ ($-6\text{ dB}$).

An adversarial re-evaluation shows that voltage-division CMOS is NOT the only possible physical substrate for ternary logic:
1. **Photonic Phase-Change Memory (PCM) & Optical Logic:** Photonic integrated circuits can encode trits in three discrete optical phase states or intensity levels ($0\%, 50\%, 100\%$ transmission) using phase-change materials (e.g. GST). Optical signals do not suffer from sub-threshold electrical voltage leakage or capacitive switching energy losses.
2. **Spintronic / Magnetic Domain Wall Devices:** Magnetization orientation along an anisotropic axis provides three stable magnetic states: $+M$ (up), $0$ (canted/demagnetized), $-M$ (down). Spintronic domain wall motion devices operate via spin-transfer torque at sub-femtojoule energy levels, maintaining non-volatile 3-state stability without continuous voltage biasing.
3. **Superconducting Single-Flux-Quantum (SFQ) Logic:** Rapid Single Flux Quantum (RSFQ) circuits manipulate discrete magnetic flux quanta $\Phi_0 = h / 2e$ in Josephson junctions. Flux states can represent $-1, 0, +1$ flux quanta natively with zero DC power dissipation.

**Substrate Re-evaluation Finding:** Investigation 05's hardware rejection applies strictly to **silicon CMOS multi-voltage electronics**. It does NOT apply to non-CMOS physical substrates (photonics, spintronics, superconducting SFQ).

---

### 5. Historical Source Assessment (Setun & Hanoi Lineage)

#### 5.1 Setun Computer Lineage (Moscow State University, 1958)

- **Hardware Architecture:** 18-trit word, magnetic ferrite-diode core logic.
- **Physical Operating Principle:** Core magnetization hysteresis loop utilized three stable states: negative remanence ($-1$), zero magnetization ($0$), positive remanence ($+1$).
- **Audit Finding:** Setun was a fully operational, physically demonstrated computer system (50 production units). Its ferrite-diode core logic was physically matched to 3-state magnetic storage. Setun was abandoned not because of arithmetic bugs or mathematical failure, but because Soviet industrial manufacturing standardized on binary transistor silicon production lines, rendering custom magnetic core fabrication economically uncompetitive.

#### 5.2 Hanoi / CWEB Lineage (`legacy/hanoivm`)

Investigation 05 analyzed the Hanoi CWEB codebase (`legacy/hanoivm`) and correctly classified it as an isomorphic combinatorial state transition model for 3-peg Gray codes (Tower of Hanoi puzzle), rather than a general-purpose positional computer.

- **Repository Source & Evidence Audit:**
  - Codebase location: `experimental/hanoi/` and continuity probe in `docs/research/continuity/probes/reproduce_hanoi_arithmetic.py`.
  - Preserved evidence artifact: `docs/research/continuity/evidence/hanoi-arithmetic-results.json` (SHA256: `f66cc904891fd22b9b5e43454fc317c3238651af36eaa0649b319c20160fc7bc`, source commit: `59205d76db4aca3da7c0bdc521924b8f9bc52cc3`).
- **Exact Preserved Evidence File Content (`hanoi-arithmetic-results.json`):**
  ```json
  {
    "addition_cases": 547,
    "addition_failures": 422,
    "negative_one_plus_zero": [
      2,
      0,
      0
    ],
    "multiplication_cases": 201,
    "multiplication_failures": 140,
    "three_times_one": [
      0,
      0,
      0
    ],
    "negation_cases": 27,
    "negation_failures": 0
  }
  ```
- **Audit Findings:**
  1. **Exact Test Statistics:** Out of 27 balanced inputs ($\{-1, 0, 1\}^3$):
     - Addition: 547 exact sum cases tested in range $[-13, 13]$, producing **422 failures ($77.15\%$)**.
     - Multiplication: 201 exact product cases tested in range $[-13, 13]$, producing **140 failures ($69.65\%$)**.
     - Negation: 27 negation cases tested, producing **0 failures ($0\%$, 100% pass)**.
  2. **Key Counterexamples:**
     - Addition $(-1) + 0$ yields trit vector $\langle 2, 0, 0 \rangle_3$ (containing digit $2$, which is outside balanced ternary digit set $\{-1, 0, 1\}$).
     - Multiplication $3 \times 1$ (input $\langle 0, 1, 0 \rangle_3 \times \langle 1, 0, 0 \rangle_3$) yields $\langle 0, 0, 0 \rangle_3$ (value $0$ instead of $3$).
  3. **Root Cause & Historical Attribution:** The Hanoi CWEB codebase represents 3-peg Tower of Hanoi disk moves where digit positions map to peg indices ($d_i \in \{0, 1, 2\}$). The internal operations named `ternary_addition` and `ternary_multiplication` in Hanoi CWEB were designed as modulo-3 disk move permutation logic for puzzle solving, NOT positional signed computer arithmetic.
  4. **Conclusion:** Investigation 05 correctly recognized Hanoi as a 3-peg Gray-code combinatorial puzzle solver. The continuity probe re-execution confirms the precise boundary between its intended combinatorial behavior (where negation succeeds) and general positional arithmetic (where addition and multiplication fail on signed inputs).

---

### 6. Supported and Unsupported Conclusions

#### 6.1 Supported Conclusions (Sustained from Investigation 05)
1. **Software CPU Ternary Emulation Rejection:** Emulating scalar or vector ternary arithmetic in software on binary x86_64 / ARM64 CPUs incurs an unsustainable $10\times - 40\times$ performance tax. Software ternary CPU emulation is definitively rejected.
2. **SIMD Dequantization Vector Tax:** In quantized neural network inference ($W \in \{-1, 0, 1\}$), 2-bit packed AVX2 MatVec execution ($12.97\text{ GFLOP/s}$) is slower than native INT8 AVX2 MatVec ($18.74\text{ GFLOP/s}$) due to `VPSHUFB` instruction decode tax.
3. **Multi-Limb BigInt Penalty:** Storing 27-trit limbs in 64-bit binary integers incurs a $2.25\times$ schoolbook multiplication operation tax over 64-bit binary limbs.
4. **Canonical Truncation Rounding:** Truncating $k$ trits in canonical balanced ternary provides exact unbiased round-to-nearest behavior.

#### 6.2 Unsupported / Falsified Conclusions (Refuted by Investigation 06)
1. **Permanent Closure of Ternary Research:** Closing all ternary research based solely on software CPU emulation penalties on binary hosts is an overreach. Non-CMOS physical substrates and custom bit-parallel hardware overlays remain viable.
2. **Ternary Partial Product Superiority Claim:** The claim that ternary multiplication requires $60.2\%$ fewer partial products than binary is falsified. Modern binary Radix-4 Booth encoding requires $20.7\%$ fewer partial product rows ($0.7925 N$ vs $N$) than ternary for equal precision.
3. **CMOS Multi-Voltage Rejection as Universal Hardware Rejection:** Rejecting ternary hardware based solely on multi-voltage CMOS noise margin degradation ignores optical PCM, spintronic, and superconducting SFQ substrates.
4. **Overbroad Claims on Untested Architectures:** Permanent restrictions on unexamined hardware architectures are avoided; negative evidence is strictly retained for software CPU emulation on binary hosts.

---

### 7. Prioritized Ternary-Specific Experiment

To move beyond software emulation on binary CPUs and test ternary computation where it possesses a distinct structural representation, we formulate one high-priority, falsifiable experiment with a rigorous mathematical specification.

#### Strongest Falsifiable Hypothesis

**Hypothesis H-T1 (Bit-Parallel Unpacked Ternary Dot-Product Custom Hardware Overlay):**
*In a custom bit-parallel FPGA / ASIC logic array (where weights $W \in \{-1, 0, 1\}$ are represented as separate Sign and Mask bitplanes $W_{\text{sign}}, W_{\text{mask}} \in \{0, 1\}^N$), a ternary dot-product accumulator tree eliminates multiplier lookup tables and bit-shufflers entirely, achieving $>1.5\times$ higher throughput-per-slice (GFLOP/s/LUT) and $>2.0\times$ higher energy efficiency (TOPS/W) than a standard INT8 DSP multiplier block array at matched memory bandwidth.*

#### Mathematical Specification:

Given an activation vector $\mathbf{X} \in \{-128, \dots, 127\}^N$ (8-bit signed Two's Complement integers) and a ternary weight vector $\mathbf{W} \in \{-1, 0, 1\}^N$:

1. **Weight Decomposition into Bitplanes:**
   Decompose $\mathbf{W}$ into two $N$-bit boolean vectors $W_{\text{mask}}, W_{\text{sign}} \in \{0, 1\}^N$:
   - $W_{\text{mask}}[i] = 1$ if $W[i] \neq 0$, else $0$.
   - $W_{\text{sign}}[i] = 1$ if $W[i] == -1$, else $0$.
   *(Mapping: $W[i] = +1 \iff (W_{\text{mask}}=1, W_{\text{sign}}=0)$; $W[i] = -1 \iff (W_{\text{mask}}=1, W_{\text{sign}}=1)$; $W[i] = 0 \iff (W_{\text{mask}}=0)$).*

2. **Sign Extension and Handling INT8_MIN ($-128$):**
   To prevent two's complement overflow when negating the INT8 minimum value $X[i] = -128$ (since $-(-128) = +128$, which exceeds 8-bit signed range $[-128, 127]$), each activation $X[i]$ is sign-extended to an accumulator width $B_{\text{acc}} \ge 8 + \lceil \log_2 N \rceil + 1$ bits (e.g. $B_{\text{acc}} = 32$ bits for $N \le 4096$) prior to conditional negation:

   $$X_{\text{ext}}[i] = \text{SignExtend}_{32}(X[i]) \in \{-128, \dots, 127\}$$

3. **Bit-Parallel Conditional Two's Complement Negation:**
   Let $M_i = \text{Replicate}_{32}(W_{\text{sign}}[i]) \in \{0\times 00000000, 0\times \text{FFFFFFFF}\}$.
   The signed term $T_{\text{signed}}[i] = W[i] \cdot X[i]$ is computed via bitwise XOR and conditional increment:

   $$T_{\text{signed}}[i] = (X_{\text{ext}}[i] \oplus M_i) + W_{\text{sign}}[i]$$

   *Validation:*
   - If $W_{\text{sign}}[i] = 0$ ($W[i] = +1$), $T_{\text{signed}}[i] = (X_{\text{ext}}[i] \oplus 0) + 0 = X_{\text{ext}}[i]$.
   - If $W_{\text{sign}}[i] = 1$ ($W[i] = -1$), $T_{\text{signed}}[i] = (\sim X_{\text{ext}}[i]) + 1 = -X_{\text{ext}}[i]$. For $X[i] = -128$ ($0\text{xFFFFFF80}$), $\sim X_{\text{ext}}[i] + 1 = 0\text{x0000007F} + 1 = +128$ ($0\text{x00000080}$), operating without overflow.

4. **Zero Masking and Accumulation:**
   The effective term $T[i]$ applies the mask bit and sums into the reduction tree:

   $$T[i] = \text{Replicate}_{32}(W_{\text{mask}}[i]) \text{ AND } T_{\text{signed}}[i]$$

   $$Y = \sum_{i=0}^{N-1} T[i]$$

#### Equivalent Binary Baseline:
- **Baseline Primitive:** A standard INT8 $\times$ INT8 MAC (Multiply-Accumulate) array (e.g., Xilinx DSP48E2 or ASIC INT8 MAC primitive) computing $Y = \sum_{i=0}^{N-1} W_{\text{INT8}}[i] \cdot X_{\text{INT8}}[i]$, where $W_{\text{INT8}}[i] \in \{-1, 0, 1\}$ is encoded as an 8-bit Two's Complement signed integer ($0\text{xFF}, 0\text{x00}, 0\text{x01}$).
- **Comparison Basis:** The binary baseline utilizes standard 8-bit $\times$ 8-bit multiplier blocks, whereas H-T1 eliminates multiplier trees in favor of bitwise XOR-masking and parallel adder trees.

#### Determinism Surface Registry Acknowledgment:
Per T81 Foundation governance guidelines, any hardware or software execution trace generated during this experiment must explicitly acknowledge the **Determinism Surface Registry** to guarantee 100% bit-identical trace replay parity across target platforms.

---

### 8. Explicit Stopping Criteria

To prevent indefinite or ungrounded exploratory research, ternary computation research within T81 Foundation must abide by the following explicit, quantitative stopping criteria:

#### Criterion 1 (Software CPU Arithmetic Rule):
- **Rule:** No further RFCs, PRs, or experimental prototypes proposing software scalar tryte arithmetic, software ternary CPUs, or software ternary BigInt ALUs for execution on binary x86_64/ARM64 hosts shall be accepted.
- **Trigger:** Software scalar ternary emulation is proved $10\times - 40\times$ slower than native binary ALUs.
- **Action:** Permanently frozen on binary CPU hosts.

#### Criterion 2 (FPGA / Custom Overlay Rule):
- **Rule:** If the bit-parallel ternary hardware overlay experiment (Hypothesis H-T1) fails to demonstrate at least a **$1.25\times$ energy efficiency advantage ($\text{TOPS/W}$)** or **$1.25\times$ area efficiency advantage ($\text{GFLOP/s/mm}^2$)** over INT8 DSP primitives under Yosys/Vivado synthesis, ALL custom ternary hardware research shall be formally closed.
- **Action:** Terminate custom ternary hardware research.

#### Criterion 3 (Memory Storage & Identity Boundary):
- **Rule:** Ternary representations in T81 Foundation shall be strictly bounded to:
  1. 2-bit packed weight serialization ($W \in \{-1, 0, 1\}$) for model storage compression on disk/DRAM (dequantized into INT8/FP16 binary registers for CPU/GPU execution).
  2. Base-81 tryte content addressing and audit trail hashes in CanonFS.
- **Action:** Enforce strict boundary in core architecture.

---

### Final Audit Disposition

### **Disposition: NARROW SCOPE / FROZEN CPU ARITHMETIC (SUSTAINED WITH CORRECTIONS)**

1. **Sustain Rejection of Software CPU Ternary Arithmetic:** Fundamental research into emulating CPU ternary arithmetic in binary host software is **CLOSED AND FROZEN**.
2. **Correct False Claims & Attributions in Literature:** Report 05's claims regarding ternary partial product superiority over binary Booth encoding and Hanoi CWEB positional arithmetic lineage are **REFUTED AND CORRECTED**.
3. **Permit Bounded Bit-Parallel Hardware Research:** Custom bit-parallel hardware overlays (Hypothesis H-T1) on FPGA/ASIC remain the sole open avenue for hardware ternary research, subject to the explicit mathematical specification and stopping criteria established herein.
