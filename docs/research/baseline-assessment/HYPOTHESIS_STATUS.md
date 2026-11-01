# T81 Scientific Research Status: Reconciled Hypotheses H01–H08 & Investigations

**Document Identifier:** T81-BASELINE-001-HYPOTHESES
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Date:** November 2026
**Status:** Authoritative Reconciliation

---

## 1. Executive Summary

This document synthesizes the empirical evidence, adversarial audit findings, corrected mathematical interpretations, and current scientific dispositions for the 8 canonical hypotheses (H01–H08) of the T81 Foundation research program, as well as auxiliary scientific investigations (Investigation 07, 08, 10A, S02, H09, and H04-R01/R02/R03).

A core objective of this assessment is to prevent historical AI overstatements, model hallucinations, and unverified literature attributions from being promoted into factual scientific claims.

---

## 2. Canonical Hypotheses Reconciliation (H01–H08)

### Hypothesis H01: Physical Storage Density
- **Original Claim:** Equal-information balanced ternary physical storage can achieve lower total physical cost (area, energy, or raw cell footprint) than binary storage on solid-state devices.
- **Experimental Method & Baseline:** Theoretical cell count formula $\lceil \log_3 M \rceil$ compared against binary $\lceil \log_2 M \rceil$, plus comparative evaluation against 10 published solid-state memory technologies (SRAM, ReRAM, PCM, FeFET, NAND).
- **Binary Control:** Optimally packed binary representation of $M$ states under equivalent endurance, sensing margins, and error correction (ECC).
- **Adversarial Audit & Findings:** Scientific Investigation 10A (`docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md`) conducted an exhaustive audit of the 10 literature citations cited in earlier research. None of the 10 original quantitative rows was verified as a complete, correctly attributed measurement record. Several citations attributed cell sizes to wrong processes or different papers entirely (e.g., TSMC 3nm SRAM misattributed to Intel 5nm; STT-MRAM numbers misattributed to ReRAM).
- **Corrected Scientific Disposition:** **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**.
  - *Critical Distinction:* H01 is not experimentally falsified on physical silicon; rather, matched empirical characterization data does not exist in the repository or cited literature to evaluate the claim.
- **Remaining Discriminating Experiment:** Investigation 10B preregistered hardware characterization on accessible physical test macros with verified noise margins, endurance, and peripheral circuitry overheads.

---

### Hypothesis H02: Signed Arithmetic & Symmetric Rounding
- **Original Claim:** Balanced ternary representations inherently improve signed arithmetic efficiency, eliminate rounding bias, and reduce carry chain propagation compared to binary two's-complement.
- **Experimental Method & Baseline:** Verification of balanced ternary addition/multiplication algorithms and round-to-nearest behavior against exact integer oracles and IEEE 754 floating-point standards.
- **Binary Control:** Two's-complement arithmetic with round-to-nearest-even (IEEE 754).
- **Adversarial Audit & Findings:** Isolated historical CWEB routines in early Hanoi prototypes failed balanced positional invariants on negative overflow boundaries. While symmetric rounding around zero is an exact mathematical property of balanced ternary digits $\{-1, 0, +1\}$, binary hardware emulation on modern superscalar CPUs requires branchy or multi-instruction SWAR/SIMD emulation, eliminating any instruction cycle advantage.
- **Corrected Scientific Disposition:** **CONDITIONAL MATHEMATICAL PROPERTY — REFUTED AS HARDWARE SPEEDUP ON BINARY SILICON**.
- **Remaining Discriminating Experiment:** Formal gate-level ASIC/FPGA synthesis measuring carry-chain propagation delay and logic depth against optimized Brent-Kung/Kogge-Stone binary adders.

---

### Hypothesis H03: Base-81 Architectural Interoperability
- **Original Claim:** Grouping four trits into a Base-81 symbol ($3^4 = 81$) provides an optimal architectural boundary for human readability, compact byte-aligned serialization, and efficient instruction encoding.
- **Experimental Method & Baseline:** Benchmarking Base-81 encoding/decoding throughput and serialization density against binary bit-packing and Base-64/Hex encoding.
- **Binary Control:** Standard 8-bit byte representations and Base-64 serialization.
- **Adversarial Audit & Findings:** Four-trit grouping is mathematically sound ($81 < 256$, fitting inside an 8-bit unsigned byte with 175 unused states). However, packing 4 trits per byte achieves an information efficiency of only $\log_2(81) / 8 \approx 6.34 / 8 = 79.2\%$, meaning 20.8% of the byte capacity is wasted compared to dense packing (5 trits in 8 bits yields $3^5 = 243 < 256$, achieving $7.92 / 8 = 99.1\%$ efficiency).
- **Corrected Scientific Disposition:** **PARTIALLY SUPPORTED (CONVENIENCE ENCODING) — REFUTED AS OPTIMAL STORAGE DENSITY**.
- **Remaining Discriminating Experiment:** Benchmark dense 5-trit packing (RFC-0044) vs 4-trit Base-81 in high-throughput network and memory serialization pipelines.

---

### Hypothesis H04: Adaptive Representation & Codec Advantage
- **Original Claim:** Dynamically adapting precision between binary and balanced ternary representations yields superior space-time efficiency compared to fixed representations across diverse data distributions.
- **Experimental Method & Baseline:** Evaluated across four arms: Fixed Binary (FB), Adaptive Binary (AB), Fixed Ternary (FT), and Adaptive Balanced Ternary (AT) across multiple synthetic workload traces.
- **Binary Control:** Matched adaptive binary representation with identical thresholding and block-framing overheads.
- **Adversarial Audit & Findings:**
  1. *H04-R01 (Radix × Adaptation Discriminator):* Proved that AT's apparent advantage over FB was entirely an artifact of adaptive precision. When compared against an equivalent Adaptive Binary (AB) control, AB matched or outperformed AT across all traces and symbol cost ratios $C_3/C_2 \in [1.0, 2.5]$.
  2. *H04-R02 (Adaptive Representation Audit):* Reclassified to **Classification D (Invalid Experimental Methodology)** due to metadata scaling defects, division math errors, and comparator capacity asymmetry (comparing a 6-trit domain with 729 states against a 10-bit domain with 1024 states). Under corrected 10-bit Tier-0 control on `boundary_oscillation`, AT=1210 < AB=1250 at $r=1.60$ with crossover $5/3$, but this conditional result does not establish hardware superiority.
- **Corrected Scientific Disposition:** **REFUTED WITHIN GENERAL SCOPE — CONDITIONAL ON RESTRICTED MATHEMATICAL DOMAINS WITH UNEQUAL CAPACITY**.
- **Remaining Discriminating Experiment:** EXP-H04-R03 (Phase B Deterministic Correctness) with strictly matched state domains ($[-364, +364]$ across 729 states) and bounded-access touch-count assertions.

---

### Hypothesis H05: Three-State Circuit & Primitive Efficiency
- **Original Claim:** Native three-state logic primitives (e.g., ternary multiplexers, consensus gates) reduce total transistor count and interconnect wiring complexity compared to equivalent Boolean gate networks.
- **Experimental Method & Baseline:** Gate-level synthesis of 8 ternary arithmetic primitives against standard CMOS Boolean cell libraries.
- **Binary Control:** Optimized CMOS Boolean synthesis using Synopsys/Yosys on equivalent multi-bit functions.
- **Adversarial Audit & Findings:** Simulation in software does not account for static power leakage, reduced noise margins, or intermediate voltage rail stabilization in multi-valued logic (MVL). Standard silicon processes impose severe energy penalties when driving non-rail intermediate states.
- **Corrected Scientific Disposition:** **THEORETICALLY PLAUSIBLE AT ABSTRACT LEVEL — UNVERIFIED ON PHYSICAL SILICON**.
- **Remaining Discriminating Experiment:** Physical CMOS ASIC tape-out or FPGA LUT emulation with calibrated power and noise margin measurements.

---

### Hypothesis H06: Ternary Neural Network Inference Advantage
- **Original Claim:** Quantizing deep learning weights to balanced ternary values $\{-1, 0, +1\}$ eliminates expensive floating-point multiplications, reducing memory bandwidth and accelerating inference.
- **Experimental Method & Baseline:** Kernel benchmarks comparing ternary dot products and GEMM against FP32, FP16, and INT8 implementations.
- **Binary Control:** Vectorized INT8 matrix multiplication (using AVX2 VNNI / ARM NEON `sdot`).
- **Adversarial Audit & Findings:** While ternary dot products replace multiplications with conditional additions or zero-skips, modern binary CPUs possess highly optimized hardware SIMD matrix multipliers. As shown in `benchmarks/results/inference_comparison.md`, INT8 dot product achieves 72.2 Gops/s on ARM64 NEON compared to only 4.96 Gops/s for T81 Ternary NEON—making INT8 **14.5x faster** than ternary on binary silicon. Early claims of a "74.6x speedup" were methodologically invalid artifacts of comparing against an unvectorized scalar FP32 baseline.
- **Corrected Scientific Disposition:** **REFUTED ON BINARY SILICON — CONDITIONAL ON SPECIALIZED HARDWARE WITHOUT HARDWARE MULTIPLIERS**.
- **Remaining Discriminating Experiment:** ASIC/FPGA matrix-vector accelerator implementing native ternary add-accumulate arrays.

---

### Hypothesis H07: Deterministic Policy-Governed Execution
- **Original Claim:** Embedding declarative policy enforcement (Axion) and content-addressed storage (CanonFS) directly into the execution runtime guarantees verifiable audit trails without sacrificing deterministic replay.
- **Experimental Method & Baseline:** End-to-end VM execution tracking instruction step counts, memory hashes, and policy interception logs across repeated runs on x86_64 and ARM64.
- **Binary Control:** Conventional non-deterministic execution environments with external logging side-effects.
- **Adversarial Audit & Findings:** Extensively tested across 27-program conformance suites, `BM_DeterminismValidation`, and CanonFS CLI contract tests. Bit-exact replay and fail-closed policy enforcement are fully verified. However, this is an architecture/software engineering achievement, entirely independent of ternary arithmetic.
- **Corrected Scientific Disposition:** **CONFIRMED & PRODUCTION-VERIFIED (AS SOFTWARE RUNTIME ARCHITECTURE)**.
- **Remaining Discriminating Experiment:** Stress testing under high concurrency in the freestanding TernaryOS EL0 userland.

---

### Hypothesis H08: Recursive Cognitive Self-Improvement & Meta-Reasoning
- **Original Claim:** Multi-tier cognitive architectures with self-reflective observation rings enable autonomous, bounded self-improvement and superior task routing.
- **Experimental Method & Baseline:** Experiment H09-E1 evaluating Compositional Graph Routing (CGR) across 4 arms: Arm A (Fixed), Arm B (Adaptive), Arm C (Recursive Intelligence), Arm D (Centralized Planner).
- **Binary Control:** Centralized optimization planner (Arm D) with deterministic heuristic search.
- **Adversarial Audit & Findings:** Experiment H09-E1 demonstrated that Arm C (Recursive Intelligence) incurred a catastrophic **>96% management overhead tax**, remaining 7x to 66x more computationally expensive than the centralized planner across all overhead regimes.
- **Corrected Scientific Disposition:** **REFUTED WITHIN EXPERIMENTAL SCOPE (RECOMMENDATION: STOP)**.
- **Remaining Discriminating Experiment:** No further experiments recommended until management tax can be bounded mathematically.

---

## 3. Supplementary Investigations Status

| Investigation ID | Focus Domain | Key Finding / Outcome | Current Status |
| :--- | :--- | :--- | :--- |
| **Investigation 07** | Ternary Hardware Feasibility | Explored synthesis of ternary logic; lacked calibrated physical transistor models. | CLOSED (INCONCLUSIVE) |
| **Investigation 08** | Integrated System Advantage | Differential parity testing revealed missing model admission guards (`llama-dense-v1`). | CORRECTED & MERGED |
| **Investigation 10A** | Physical Storage Evidence | Audited 10 solid-state memory citations; all 10 unverified in primary literature. | BLOCKED (UNVERIFIED) |
| **Experiment S02** | Stochastic State-Topology Computation | Evaluated binary/ternary spin models; corrected report revised STOP to REVISE (H2 untested). | REVISED (OPEN PILOT) |
| **Experiment CMP3-R1** | 3-Way Branchless Comparison | PMU cycle benchmarks showed ARM64 parity with `csel`/`cset`; no speedup on superscalar CPUs. | COMPLETED (PARITY) |
| **Durable Witness** | Detached Evidence Verification | Disaggregated crypto, authority, witness, and effect verification flags. | COMPLETED & VERIFIED |
