# T81 Research Continuity — EVIDENCE_AND_FINDINGS

**Document ID:** `docs/ai-context/EVIDENCE_AND_FINDINGS.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
**Status:** Non-normative index to source-reported findings; no experiments independently rerun in this correction

---

## 1. Executive Scientific Evidence Index

This document indexes source-reported benchmark results, mathematical models, negative findings, and historical reports. Evidence tiers below refer to their **originating packets**, not independent reproduction during this documentation correction. For each claim consult its exact oracle, raw artifacts, correction and tested scope.

### Summary Disposition Table
| Finding / Investigation | Target Domain | Evidence Tier | Primary Outcome | Scientific Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **Hanoi Arithmetic Probes (H02)** | Positional Integer Math | **Tier A** | Addition failed 422/547 cases; multiplication failed 140/201 cases; negation passed 27/27 cases. | **REJECTED** for positional integer claims (CWEB routines use componentwise multiplication). |
| **TISC Code Compactness Inv 09-E1 (H03)** | ISA Density | **Tier A** | TISC binary: 1,240 bytes vs conforming WebAssembly: 157 bytes (size ratio $7.898$). | **NOT SUPPORTED** for compactness claim versus WASM. |
| **Packed Ternary MatVec (H03, H06)** | CPU SIMD Inference | **Tier A** | $75\%$ DRAM memory footprint reduction; VPSHUFB SIMD decode tax limits CPU speed to $75.3\%$ of INT8 AVX2 in cache ($12.91$ vs $17.15\text{ GFLOP/s}$). | **FALSIFIED for CPU MatVec speed**; retained for storage compression. |
| **Radix Addition Comparison (H01, H02)** | ALU Execution Speed | **Tier A** | Software tryte addition ($11.7\text{ ns}$) is $10.6\times$ slower than native 64-bit binary ALU addition ($1.1\text{ ns}$). | **FALSIFIED & FROZEN** for software CPU ternary arithmetic speedup. |
| **Adaptive Tiering H04-R01 (H04)** | Radix × Adaptation | **Tier D** | Theoretical $2\times2$ trace model. Counterexample ($M=256$, $C_3/C_2=1.5$) yields higher ternary cost (9 vs 8). | **CONDITIONAL MODEL SUPPORT ONLY**; physical hardware benefit open. |
| **Adaptive Tiering H04-R02 (H04)** | Four-arm capacity/transition model | **Tier B/C here** | An existing [RESULTS](../../experiments/h04_r02_adaptive_representation/RESULTS.md) and [HANDOFF](../../experiments/h04_r02_adaptive_representation/HANDOFF.md) report completed FB/AB/FT/AT traces across eight workloads; modeled advantage disappears at the binary two-bit-per-trit cost ratio. | **COMPLETED PACKET, NOT INDEPENDENTLY RERUN HERE**; conditional cost-model result, not universal H04 or silicon closure. |
| **Ternary Hardware Feasibility Inv 07 (H01, H05)** | RTL Synthesis / Area | **Tier B** | Verilog RTL (`rtl/core/t81_top.sv`) synthesizes using two 2-bit binary signals per trit. | **BOUNDED / NOT SUPPORTED** for native 3-state silicon advantage. |
| **Physical Storage Evidence Inv 10A (H01)** | Semiconductor Memory | **Tier C** | No matched 3-state semiconductor cell area, energy, or noise-margin data available. | **PHYSICAL EVALUATION BLOCKED** — missing matched device characterization data. |
| **Integrated System Advantage Inv 08 (H07)** | System Architecture | **Tier A** | Corrective audit established: Computational (NOT SUPPORTED), Assurance (SUPPORTED), Compositional (INCONCLUSIVE), Ternary-Specific (NOT SUPPORTED). | **BOUNDED ASSURANCE SUPPORTED**. |
| **Accountable Computation precursor (H07)** | Governance / evidence protocol | **Tier B/C here** | Local A/B/C arms reported 0/150, 90/150 and 0/150 unauthorized result creations respectively in the negative-case set; the full Wasmtime/OPA protocol was **not run**. | **INCONCLUSIVE** for custom-VM necessity and conventional parity; draft RFC-00D4 not certified. |
| **AI Research Methodology (H08)** | Prompt-to-code provenance | **Tier C / E** | Session histories and provenance records exist; no controlled matched productivity study is cited. | **PROCESS DOCUMENTED; comparative benefit UNTESTED**. |

---

## 2. Detailed Empirical Investigation Findings

### 2.1 Hanoi Historical Arithmetic Probes (H02)
- **Source Pin:** Repository `Hanoi`, commit `59205d76db4aca3da7c0bdc521924b8f9bc52cc3`, file `src/ternary_arithmetic_optimization.cweb`.
- **Methodology:** Isolated extraction and C11 compilation of verbatim CWEB arithmetic routines against an exact balanced integer oracle ($a + 3b + 9c$) across representable non-overflow ranges $[-13, 13]$.
- **Measured Results:**
  - **Addition:** 422 failures among 547 cases. Root cause: normalizes into unbalanced $0/1/2$ digits rather than balanced $-1/0/1$.
  - **Multiplication:** 140 failures among 201 cases. Root cause: componentwise multiplication without positional cross-products (e.g., $(0,1,0) \times (1,0,0)$ returns zero instead of 3).
  - **Negation:** 0 failures among 27 cases ($100\%$ pass rate).
- **Scientific Impact:** Rejects these historical CWEB routines as evidence for positional integer math. Does not invalidate abstract balanced ternary algebra or correct positional adders.

### 2.2 TISC ISA Code Compactness Investigation 09-E1 (H03)
- **Source Pin:** `spec/tisc-spec.md` at commit `bf6dd85834a010044f59766a6aca8c40ac2ba0b9`.
- **Workload:** Standardized benchmark function compiled to TISC bytecode vs WebAssembly MVP bytecode.
- **Measured Results:**
  - Complete TISC artifact: 1,240 bytes.
  - Smallest conforming WebAssembly artifact: 157 bytes.
  - Size ratio: $1,240 / 157 \approx 7.898\times$ larger.
- **Scientific Impact:** Formally **NOT SUPPORTED**. Unpadded WebAssembly variable-length LEB128 encoding is significantly more compact than fixed 81-opcode TISC byte layouts.

### 2.3 Packed Ternary MatVec Performance (`experiments/packed_ternary_matvec/`) (H03, H06)
- **Source Pin:** Commit `f6d4d680709c5c6c98b305628e04346f5ef61620`, file `experiments/packed_ternary_matvec/benchmark.cpp`.
- **Setup:** 2-bit packed ternary weight matrix ($[-1, 0, +1]$ packed 4 trits per byte) multiplied by INT8 activation vectors using x86_64 AVX2 SIMD vectorization.
- **Measured Results:**
  - **Storage:** Footprint reduced by $75\%$ ($0.25$ bytes/weight vs $1.0$ byte/weight for INT8).
  - **L1/L2 Cache Speed:** Reached $12.91\text{ GFLOP/s}$ vs INT8 AVX2 baseline of $17.15\text{ GFLOP/s}$ ($75.3\%$ speed ratio).
- **Confounder:** SIMD `VPSHUFB` bit-shuffle decode instruction overhead neutralizes memory bandwidth savings when operating within L1/L2 cache boundaries.
- **Verdict:** Falsified for CPU MatVec execution speed in cache; retained strictly as a bandwidth-efficient storage and interchange format.

### 2.4 Software Radix Addition Benchmark (`experiments/radix_comparison/`) (H01, H02)
- **Source Pin:** `experiments/radix_comparison/benchmark.cpp`.
- **Workload:** 10,000,000 additions executed in a tight loop comparing software tryte addition (4 trits packed in 8 bits) against native 64-bit binary ALU addition.
- **Measured Results:**
  - Native 64-bit Binary Addition: $1.1\text{ ns}$ per operation.
  - Software 4-Trit Tryte Addition: $11.7\text{ ns}$ per operation ($10.6\times$ slower).
  - Software Un-Grouped Trit Addition: $105.3\text{ ns}$ per operation ($95.7\times$ slower).
- **Verdict:** Software CPU ternary arithmetic optimization is formally **FALSIFIED and FROZEN (STOP)**.

### 2.5 Hardware Feasibility & Storage Evidence Assessment (Inv 07 & Inv 10A) (H01, H05)
- **Investigation 07 (Hardware Feasibility):** Synthesized Verilog RTL (`rtl/core/t81_top.sv` at `834521d4e6b37cf8c871b1ac64af449ff6493b7d`). Findings confirmed that RTL maps trits to 2-bit binary signals.
- **Investigation 10A (Physical Storage Evidence):** Corrective disposition:
  > **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE.**
- **Scientific Impact:** Prevents converting theoretical radix density ($\log_2 3 \approx 1.585$) into claims of physical silicon hardware superiority without matched semiconductor fabrication measurements.

### 2.6 Accountable Computation Precursor & Baseline-X (H07)
- **Sources:** [bounded report](../../examples/governance/accountable-computation/REPORT.md), [Protocol 001 proposal](../../experiments/research_001/RESEARCH_PROPOSAL.md), [claim register](../../experiments/research_001/CLAIM_REGISTER.md), and [draft RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md).
- **Actually tested:** Three **local precursor** arms: A used conventional local mechanisms, B reused existing T81 paths, and C used experiment-local integration. These were **not** the specified Python/FastAPI/Postgres/OPA, T81VM, and Wasmtime/SQLite/Sigstore deployment arms.
- **Reported negative-case observations:** Unauthorized result creation was **0/150 in A**, **90/150 in B** and **0/150 in C** under the report's defined cases. The B authorization failures are negative evidence to preserve. The local detached verifier met its bounded sub-second target, but no external host-trust, physical side-effect, or cross-platform guarantee follows.
- **Unresolved:** The full Wasmtime/OPA comparison, 10× complexity/latency claim, concurrent revocation and RFC-00D4 conformance. The report explicitly calls the broader architectural outcome **INCONCLUSIVE**; CLM-03/04 remain `UNDER_ACTIVE_TEST`.
- **Interpretation:** A runtime-agnostic evidence protocol is a plausible research direction, **not** an established comparative equivalence or an adopted RFC from this packet.
### 2.7 Adaptive Representation Tiering (H04 & H04-R01)
- **Source Pin:** `docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md` and `docs/research/continuity/probes/storage_boundary.py`.
- **Setup:** Formal $2\times2$ factorial matrix (Fixed Binary, Adaptive Binary, Fixed Ternary, Adaptive Ternary) decoupling radix representation from dynamic adaptation.
- **Measured Results:** Mathematical model proves ternary requires specific physical element cost thresholds ($C_3/C_2 < \log_2 3 \approx 1.585$) to compete, but finite capacity quantization overhead creates counterexamples (e.g., $M=256$, $C_3/C_2=1.5$ requires 6 trytes = 9 cost units vs binary 8 bits = 8 cost units).
- **Verdict:** Conditional mathematical model support; physical advantage open.

### 2.8 AI-Assisted Research Methodology (H08)
- **Source Pin:** `docs/research/continuity/SESSION_HANDOFF.md` and `docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md`.
- **Setup:** Auditing prompt provenance, AI generated claims, error rates, and session handoffs across iterative research cycles.
- **Verdict:** Methodological process tracking prompt-to-code provenance and session continuity established.

---

## 3. Reproduction Commands

The following are **suggested reproduction commands, not executed during this documentation correction**. Under the normal Ninja/CMake layout, the two experiment executables reside in their experiment build directories. Verify the exact build configuration and prerequisites before interpreting results:

```bash
# 1. Build release binaries with test flags enabled
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
ninja -C build t81 t81_radix_comparison t81_packed_ternary_matvec

# 2. Run radix addition performance benchmark (H01, H02)
./build/experiments/radix_comparison/t81_radix_comparison

# 3. Run 2-bit packed ternary MatVec benchmark (H03, H06)
./build/experiments/packed_ternary_matvec/t81_packed_ternary_matvec

# 4. Run exact float conversion audit oracle
python3 experiments/float_conversion_audit/test_oracle.py

# 5. Execute full research regression suite
python3 scripts/ci/run_research_regressions.py --build-dir build
```

---

## 4. Unsupported Claims That Must Never Be Repeated

Agents, maintainers, and publications must strictly avoid repeating these unsupported assertions:

1. **"T81's software ternary SWAR vectors accelerate AI inference on binary CPUs."**
   *Correction:* Unpacking 2-bit tryte fields in SIMD vector units imposes a instruction decode tax that limits execution speed to $75.3\%$ of INT8 AVX2 in cache.
2. **"TISC bytecode is inherently more compact than WebAssembly."**
   *Correction:* Investigation 09-E1 proved TISC binary artifacts are $7.898\times$ larger than conforming WebAssembly binaries.
3. **"T81 operates on physical ternary microprocessor hardware."**
   *Correction:* No physical 3-state hardware exists in the repository. All execution relies on binary software emulation or 2-bit binary RTL signals.
4. **"Accountable computation requires a custom VM" or "Wasmtime + OPA was already proven equivalent."**
   *Correction:* Both comparative claims remain **UNRESOLVED**. The [precursor](../../examples/governance/accountable-computation/REPORT.md) did not execute the full Wasmtime/OPA deployment arm; [CLM-03/04](../../experiments/research_001/CLAIM_REGISTER.md) are under active test.

---

## 5. Key Discriminating Experiments

To separate genuine radix or representation effects from conventional software engineering optimizations, future research must prioritize:

1. **H04-R02 independent verification:** The [four-arm packet](../../experiments/h04_r02_adaptive_representation/HANDOFF.md) already reports completion and a conditional negative result on binary-encoded trits. Inspect its oracle, traces, units and source provenance before considering reproduction or a distinct follow-up. **Do not reconstruct the completed experiment as new work.**
2. **Track F Determinism Tax Measurement:** Measuring the exact execution overhead of bit-exact soft-float transcendentals ([RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md)) versus native host `libm` across x86_64, ARM64, and WebAssembly targets.
