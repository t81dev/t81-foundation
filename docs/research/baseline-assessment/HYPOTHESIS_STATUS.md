# T81 Scientific Research Status: Reconciled Hypotheses H01–H08 & Investigations

**Document Identifier:** T81-BASELINE-001-HYPOTHESES
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Temporal Validity:** Reconciled as of pinned baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05` (not live GitHub status)
**Date:** November 2026
**Status:** Authoritative Reconciliation

---

## 1. Executive Summary

This document synthesizes the empirical evidence, adversarial audit findings, corrected mathematical interpretations, and current scientific dispositions for the 8 canonical hypotheses (H01–H08) of the T81 Foundation research program, as defined in `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`, as well as auxiliary scientific investigations (Investigation 07, 08, 10A, S02, H09, and H04-R01/R02/R03).

A core directive of this assessment is to preserve the exact formal definitions of canonical hypotheses without conflation, distinguish software runtime mechanisms from physical or comparative advantages, preserve negative and inconclusive results, and avoid promoting speculative or unverified claims.

---

## 2. Canonical Hypotheses Reconciliation (H01–H08)

### Hypothesis H01: Physical Storage Density
- **Canonical Formal Claim:** Equal-information ternary storage can have lower total physical cost than binary.
- **Baseline:** Optimally packed binary representation of the same $M$ states; same reliability, access and endurance.
- **Assumptions:** $C_2$ and $C_3$ have the same physical cost unit; include sensing, ECC, control, and peripheral overheads.
- **Measurement & Falsifier:** Exact $\lceil \log_b M \rceil$ counts and sensitivity over measured device costs; area/energy/retention. Falsified if no cost reduction over matched binary anywhere in the preregistered plausible parameter space.
- **Adversarial Audit & Findings:** Scientific Investigation 10A (`docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md`) conducted an exhaustive audit of the 10 solid-state memory citations (P01–P10) cited in earlier research. None of the 10 original quantitative rows was verified as a complete, correctly attributed measurement record. Several citations attributed cell sizes to wrong processes or different papers entirely (e.g., TSMC 3nm SRAM misattributed to Intel 5nm; STT-MRAM numbers misattributed to ReRAM).
- **Corrected Scientific Disposition:** **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**.
  - *Critical Distinction:* H01 is not experimentally falsified on physical silicon; rather, matched empirical characterization data does not exist in the repository or cited literature to evaluate the claim.
- **Remaining Discriminating Experiment:** Investigation 10B preregistered hardware characterization on accessible physical test macros with verified noise margins, endurance, and peripheral circuitry overheads.

---

### Hypothesis H02: Signed Arithmetic & Symmetric Rounding
- **Canonical Formal Claim:** Balanced ternary can improve a specified signed arithmetic or rounding cost over two's-complement.
- **Baseline:** Exact integer oracle, competitive binary signed arithmetic and standard round-to-nearest-even (IEEE 754).
- **Assumptions:** Hardware/gate model or software instruction model specified before measurement; symmetric range $[-N, +N]$ without asymmetric overflow penalty.
- **Adversarial Audit & Findings:** Isolated historical CWEB routines in early Hanoi prototypes failed balanced positional invariants on negative overflow boundaries. While symmetric rounding around zero is an exact mathematical property of balanced ternary digits $\{-1, 0, +1\}$, binary hardware emulation on modern superscalar CPUs requires branchy or multi-instruction SWAR/SIMD emulation, eliminating any instruction cycle advantage.
- **Corrected Scientific Disposition:** **CONDITIONAL MATHEMATICAL PROPERTY — REFUTED AS HARDWARE SPEEDUP ON BINARY SILICON**.
- **Remaining Discriminating Experiment:** Formal gate-level ASIC/FPGA synthesis measuring carry-chain propagation delay and logic depth against optimized Brent-Kung/Kogge-Stone binary adders.

---

### Hypothesis H03: Base-81 Architectural Interoperability
- **Canonical Formal Claim:** Grouping four trits as base-81 can provide a workload-specific interface or compaction advantage over unaligned ternary.
- **Baseline:** Ungrouped ternary, binary bit-packed and binary limb representations under the same operations.
- **Assumptions:** Four trits fit within an unsigned byte ($3^4 = 81 < 256$); byte-addressable host interconnects and serialization boundaries.
- **Adversarial Audit & Findings:** Four-trit grouping is mathematically sound and practical for byte-aligned serial streams. However, packing 4 trits per byte achieves an information efficiency of only $\log_2(81) / 8 \approx 6.34 / 8 = 79.2\%$, meaning 20.8% of the byte capacity is wasted compared to dense packing (5 trits in 8 bits yields $3^5 = 243 < 256$, achieving $7.92 / 8 = 99.1\%$ efficiency).
- **Corrected Scientific Disposition:** **PARTIALLY SUPPORTED (CONVENIENCE ENCODING) — REFUTED AS OPTIMAL STORAGE DENSITY**.
- **Remaining Discriminating Experiment:** Benchmark dense 5-trit packing (RFC-0044) vs 4-trit Base-81 in high-throughput network and memory serialization pipelines.

---

### Hypothesis H04: Adaptive Representation & Codec Advantage
- **Canonical Formal Claim:** An explicitly defined adaptive ternary representation policy can beat fixed binary, adaptive binary or fixed ternary under a stated cost model.
- **Baseline:** Fixed binary, adaptive binary, fixed ternary, adaptive ternary across matched capacity bounds ($3^k \ge 2^b + 1$).
- **Adversarial Audit & Findings:**
  1. *H04-R01 (Radix × Adaptation Discriminator):* Proved that AT's apparent advantage over FB was entirely an artifact of adaptive precision. When compared against an equivalent Adaptive Binary (AB) control, AB matched or outperformed AT across all traces and symbol cost ratios $C_3/C_2 \in [1.0, 2.5]$.
  2. *H04-R02 (Adaptive Representation Audit):* Reclassified to **Classification D (Invalid Experimental Methodology)** due to metadata scaling defects, division math errors, and comparator capacity asymmetry (comparing a 6-trit domain with 729 states against a 10-bit domain with 1024 states). Under corrected 10-bit Tier-0 control on `boundary_oscillation`, AT=1210 < AB=1250 at $r=1.60$ with crossover $5/3$, but this conditional result does not establish hardware superiority or universal advantage.
- **Corrected Scientific Disposition:** **REFUTED WITHIN GENERAL SCOPE — CONDITIONAL ON RESTRICTED MATHEMATICAL DOMAINS WITH UNEQUAL CAPACITY**.
- **Remaining Discriminating Experiment / Post-Baseline Status:** EXP-H04-R03 (Phase B Deterministic Correctness) was independently reviewed and accepted at PR #658 head `983e258ed4b57b4cd6e010f036c1eb8656856e88` as a dated post-baseline delta, enforcing strictly matched 729-state domains ($[-364, +364]$) and bounded-access touch-count assertions. Phase C benchmarking remains unauthorized.

---

### Hypothesis H05: Three-State Circuit & Primitive Efficiency
- **Canonical Formal Claim:** A specified three-state primitive can reduce circuit or system cost against a Boolean baseline for the same function.
- **Baseline:** Boolean synthesis for exactly the same function, I/O states, process corner and frequency.
- **Assumptions:** Physical multi-valued logic implementation or matched FPGA/ASIC cell library.
- **Adversarial Audit & Findings:** Simulation in software does not account for static power leakage, reduced noise margins, or intermediate voltage rail stabilization in multi-valued logic (MVL). Standard silicon processes impose severe energy penalties when driving non-rail intermediate states.
- **Corrected Scientific Disposition:** **THEORETICALLY PLAUSIBLE AT ABSTRACT LEVEL — UNVERIFIED ON PHYSICAL SILICON**.
- **Remaining Discriminating Experiment:** Physical CMOS ASIC tape-out or FPGA LUT emulation with calibrated power and noise margin measurements.

---

### Hypothesis H06: Ternary Neural Network Inference Advantage
- **Canonical Formal Claim:** Ternary weights or activations can improve accuracy-constrained inference efficiency over low-bit binary.
- **Baseline:** Matched INT8/low-bit kernels and models at equal task accuracy, runtime, memory and energy.
- **Adversarial Audit & Findings:** While ternary dot products replace multiplications with conditional additions or zero-skips, modern binary CPUs possess highly optimized hardware SIMD matrix multipliers. As shown in `benchmarks/results/inference_comparison.md`, INT8 dot product achieves 72.2 Gops/s on ARM64 NEON compared to only 4.96 Gops/s for T81 Ternary NEON—making INT8 **14.5x faster** than ternary on binary silicon. Early claims of a "74.6x speedup" were methodologically invalid artifacts of comparing against an unvectorized scalar FP32 baseline.
- **Corrected Scientific Disposition:** **REFUTED ON BINARY SILICON — CONDITIONAL ON SPECIALIZED HARDWARE WITHOUT HARDWARE MULTIPLIERS**.
- **Remaining Discriminating Experiment:** Dedicated ASIC/FPGA matrix-vector accelerator implementing native ternary add-accumulate arrays.

---

### Hypothesis H07: Representation-Independent Governed Execution
- **Canonical Formal Claim:** Representation-independent governed execution can improve a defined auditability property.
- **Baseline:** Conventional runtime plus explicit authorization/evidence boundary; equal threat model and host trust.
- **Assumptions:** Authorization, evidence integrity, factual validity and consent are different; no inference-security or malicious-host claims from simulated dispatch.
- **Evidence Collected & Status:** Demonstrated as an operational software runtime mechanism: 27-program conformance suites, `BM_DeterminismValidation`, and CanonFS CLI contract tests demonstrate bit-exact replay and fail-closed policy enforcement. However, this is an architecture/software engineering contract, not an established comparative advantage over a conventional baseline with an evidence boundary, nor a production security certification. PRs #565/#566 remain open/draft.
- **Corrected Scientific Disposition:** **DEMONSTRATED IN OPERATIONAL TESTS — COMPARATIVE AUDITABILITY ADVANTAGE UNTESTED AGAINST COMPETITIVE BOUNDARY CONTROL**.
- **Remaining Discriminating Experiment:** Formal comparative benchmark measuring verification latency and tamper detection cost against an equivalent conventional boundary-audited runtime.

---

### Hypothesis H08: AI-Assisted Research Trustworthiness
- **Canonical Formal Claim:** AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process.
- **Baseline:** Human-only or alternative-assisted work under matched tasks and independent verification.
- **Assumptions:** Commit authorship does not prove prompt provenance; repeated model summaries are not independent verification.
- **Evidence Collected & Status:** The research continuity charters, research commons catalogs, and discrepancy ledgers document source claims, extraction probes, and adversarial reviews. However, no controlled comparative study comparing AI-assisted vs human-only error rates, yields, or correction costs has been performed. *(Note: Experiment H09-E1 on recursive cognition is an architectural experiment on process routing, not an evaluation of H08's research methodology hypothesis).*
- **Corrected Scientific Disposition:** **UNTESTED (METHODOLOGICAL FRAMEWORK ACTIVE; COMPARATIVE TRUSTWORTHINESS OPEN)**.
- **Remaining Discriminating Experiment:** Controlled preregistered study comparing reproduction yield and error rates between human and AI-assisted extraction under independent adversarial audit.

---

## 3. Supplementary Investigations & Experiments Status

| Investigation ID | Focus Domain | Key Finding / Outcome | Current Status |
| :--- | :--- | :--- | :--- |
| **Investigation 07** | Ternary Hardware Feasibility | Explored synthesis of ternary logic; lacked calibrated physical transistor models. | CLOSED (INCONCLUSIVE) |
| **Investigation 08** | Integrated System Advantage | Differential parity testing revealed missing model admission guards (`llama-dense-v1`). | CORRECTED & MERGED |
| **Investigation 10A** | Physical Storage Evidence | Audited 10 solid-state memory citations; all 10 unverified in primary literature. | BLOCKED (UNVERIFIED) |
| **Experiment S02** | Stochastic State-Topology Computation | Evaluated binary/ternary spin models; corrected report revised STOP to REVISE (H2 untested). | REVISED (OPEN PILOT) |
| **Experiment CMP3-R1** | 3-Way Branchless Comparison | PMU measurements uncollected/unauthorized (`measurements_collected: false, linux: null` in RESULTS.json); assembly validation exists for ARM64 `csel`/`cset` but hardware cycle parity is PMU_UNAVAILABLE. | COMPLETED / PMU_UNAVAILABLE (TIER 2) |
| **Durable Witness** | Detached Evidence Verification | Disaggregated crypto, authority, witness, and effect verification flags. | COMPLETED & VERIFIED |
| **Experiment H09-E1** | Recursive Process Intelligence | CGR benchmark protocol specified in docs/research/H09_RECURSIVE_PROCESS_INTELLIGENCE.md; numerical claims (>96% coordination tax, 7x–66x overhead) are unexecuted / withdrawn due to absence of primary execution artifacts or runner scripts in tree. | PROPOSED / UNEXECUTED (TIER 1) |
| **Experiment H04-R03** | Matched Codec Correctness | Phase B deterministic correctness independently accepted at PR #658 head `983e258ed4`. | POST-BASELINE DELTA (PHASE C UNAUTHORIZED) |
