# T81 Research Continuity — RESEARCH_IDENTITY

**Document ID:** `docs/ai-context/RESEARCH_IDENTITY.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
**Status:** Non-normative research-identity navigation; canonical H01–H08 definitions remain in the JSON register

---

## 1. Primary Scientific Mission & Central Questions

The T81 Foundation program is a hypothesis-driven computational research platform. It is not an enterprise product, a commercial operating system, or a native silicon hardware manufacturing effort. Its purpose is to discover whether non-binary computational representations offer genuine advantages.

### Central Scientific Question
> **Under what assumptions, workloads, cost models, and physical substrates can ternary or alternative computational representations demonstrate reproducible, practically meaningful advantages over appropriately optimized conventional alternatives?**

### Companion Question
> **Can representation-independent computational objects retain stable identity, deterministic semantics, pre-side-effect capabilities, and auditable evidence across arbitrary virtual and host execution environments?**

---

## 2. Intellectual Lineage & Historical Evolution

### Phase 1: Donald Knuth, Discrete Mathematics, and CWEB Lineage
The research originated in discrete mathematics, Donald Knuth’s work on positional number systems and seminumerical algorithms (*The Art of Computer Programming*, Vol. 2), recursion, formal automata, and literate programming. The earliest code experiment, **Hanoi** (`https://github.com/t81dev/Hanoi`), was authored in `.cweb` (`t81recursion.cweb`, `tier_scheduler.cweb`, `hvm_promotion.cweb`, `hanoivm_vm.cweb`, `ternary_arithmetic_optimization.cweb`).

#### Hanoi Archaeology & Findings
Source-level analysis of the 41 historical shared source blobs revealed:
- `t81recursion.cweb`: Depth tracking and recursive dispatch.
- `tier_scheduler.cweb`: Heuristic tier selection using entropy, recursion depth, and symbolic intent (e.g., T729 for entropy/depth > 100 or `AI` intent; T243 for > 40; T81 otherwise). These were static rules, not mathematically proven optima.
- `ternary_arithmetic_optimization.cweb`: Isolated tests revealed arithmetic inconsistencies when evaluated as positional integers (addition failed 422/547 cases, multiplication failed 140/201 cases due to componentwise multiplication rather than positional cross-products). These findings reject these specific CWEB routines for positional integer claims, not balanced ternary mathematics itself.

### Phase 2: Base-81 Ecosystem & TISC ISA Design
To overcome trit-level storage overhead, the project introduced Base-81 digit grouping ($81 = 3^4$, mapping 4 trits to 1 tryte). At $1.58496$ bits/trit, 4 trits yield $6.33985$ bits of entropy, which fits inside a standard 8-bit byte on binary host hardware. The ecosystem expanded into `t81-lang`, `t81-vm`, `t81lib`, and the frozen TISC ISA (v1.9.0, 81 opcodes).

### Phase 3: Modern T81 Foundation Stack
Modern T81 Foundation integrates Track R alternative representations with Track F systems substrate:
- Bit-exact cross-platform determinism (DCP) and soft-float math ([RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md)).
- Pre-side-effect policy authorization kernel (Axion).
- Content-addressed artifact storage and foreign file system interchange (CanonFS / [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)).
- DAIOS Target Architecture ([RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)).

---

## 3. Canonical Hypotheses Register (H01–H08)

The project preserves the exact identities, formal claims, baselines, assumptions, measurements, falsifiers, results, and next actions from [`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`](../research/continuity/HYPOTHESIS_REGISTER_v1.json). These canonical definitions must never be redefined or renumbered.

### Hypothesis H01: Native Ternary Storage & Logic Cost Reduction
- **Formal Claim:** Equal-information ternary storage can have lower total physical cost than binary storage under stated device and overhead assumptions.
- **Baseline:** Optimally packed binary representation of the same $M$ states; same reliability, access, and I/O requirements.
- **Assumptions:** $C_2$ and $C_3$ share the same physical cost unit; includes sensing, ECC, control, and conversion rather than treating grouping as a free physical element.
- **Measurement:** Exact $\lceil \log_b M \rceil$ counts and sensitivity over measured device costs; area/energy/latency reported separately.
- **Falsifier:** No cost reduction over matched binary anywhere in the preregistered plausible parameter region, or reduction vanishes with overhead.
- **Result:** **Tier D:** Ideal threshold derived ($\log_2 3 \approx 1.585$). For $M=256$ and $C_3/C_2=1.5$, ternary cost is 9 vs binary 8. **Tier B:** Current RTL uses 2 binary bits per trit and supplies no measured 3-state-device advantage. Software CPU tryte addition is $10.6\times$ slower than native 64-bit binary ALU addition.
- **Next Action:** Use `storage_boundary.py` parameter model; require independent device data before physical hardware claims.

### Hypothesis H02: Balanced Ternary Signed Arithmetic & Rounding
- **Formal Claim:** Balanced ternary can improve a specified signed arithmetic or rounding task at equal semantics and cost accounting.
- **Baseline:** Exact integer oracle, competitive binary signed arithmetic, and a correct ternary candidate.
- **Assumptions:** Candidate must be closed over its advertised digit set; representable outputs only; overflow rules explicit.
- **Measurement:** Correctness first; then matched carry/depth/work/latency or rounding-error metrics.
- **Falsifier:** Candidate fails exact arithmetic, or no advantage survives equivalent optimized binary and conversion costs.
- **Result:** **Tier A:** Isolated legacy CWEB routines fail balanced positional semantics (addition 422/547; multiplication 140/201; negation 0/27 failures). SWAR SIMD execution on binary ALUs is falsified for speed and frozen.
- **Next Action:** Preserve failure records; specify domain and correct oracle before performance comparisons; do not rewrite Hanoi.

### Hypothesis H03: Base-81 Digit Grouping & Representation Interface
- **Formal Claim:** Grouping four trits as Base-81 can provide a workload-specific interface or execution benefit beyond equivalent binary packing.
- **Baseline:** Ungrouped ternary, binary bit-packed, and binary limb representations with equal value domains.
- **Assumptions:** $81=3^4$ is grouping, not a free 81-level physical device; indexing, padding, decode, and conversion charged.
- **Measurement:** Storage bytes and end-to-end operation cost with grouping ablation.
- **Falsifier:** A matched packed-binary implementation removes the apparent benefit or grouping overhead dominates.
- **Result:** **Tier D:** Four-trit grouping mathematically established. **Tier B:** On-disk 2-bit weight format (`.t81w`) saves $75\%$ DRAM footprint; however, CPU SIMD bit-shuffle decode limits execution speed to $75.3\%$ of INT8 AVX2 in cache.
- **Next Action:** Keep mathematical grouping distinct from kernel speed; treat 2-bit packing strictly as an efficient storage/interchange format.

### Hypothesis H04: Adaptive Representation Policy Tiering
- **Formal Claim:** An explicitly defined adaptive ternary representation policy can beat fixed ternary and an equally capable adaptive binary policy on a specified workload.
- **Baseline:** Fixed binary, adaptive binary, fixed ternary, adaptive ternary; identical domains, predictor information, and conversion accounting.
- **Assumptions:** Historical 81/243/729 labels do not establish bit widths or equivalent domains; entropy/depth heuristic and promotion logic are distinct source mechanisms.
- **Measurement:** Factorial radix/adaptation contrasts; storage, selection, switching, and conversion reported separately.
- **Falsifier:** No advantage over adaptive binary, effect is equally explained by adaptation, or correctness/transition costs remove it.
- **Result:** **Tier B/D (lineage/model):** Historical Hanoi scheduler thresholds inspected; H04-R01 defines a $2\times2$ discriminator with finite-capacity counterexamples ($M=256$, $C_3/C_2=1.5$ favors binary). An [H04-R02 four-arm packet](../../experiments/h04_r02_adaptive_representation/HANDOFF.md) already exists and reports a conditional negative outcome under binary-encoded trit costs; it was not independently rerun in this index.
- **Next Action:** Audit H04-R02's oracle, cost units and completed evidence, then choose a distinct, maintainer-approved discriminator if needed. Do not repeat the completed packet.

### Hypothesis H05: Three-State Algebra Primitives & Architectures
- **Formal Claim:** A specified three-state primitive can reduce circuit or system cost against an equivalent Boolean implementation.
- **Baseline:** Boolean synthesis for exactly the same function, I/O states, throughput, and reliability.
- **Assumptions:** A two-bit binary comparator can encode three outcomes; saturating single-trit ADD differs from positional multi-trit addition.
- **Measurement:** Gate count/depth, mapped area/energy, and input/output conversion under a common process.
- **Falsifier:** Equivalent Boolean synthesis matches or beats claimed cost, or semantics differ.
- **Result:** **Tier B:** Hardware repo implements eight operations over signed two-bit trit signals; Hanoi CMP3 handler is null. Separate policy paths have scoped tests, but the [accountable-computation precursor](../../examples/governance/accountable-computation/REPORT.md) reported unauthorized result creation in its B arm. That evidence cannot certify universal side-effect prevention.
- **Next Action:** Formalize operation truth table before hardware extensions.

### Hypothesis H06: Ternary Weights & Activations for AI Inference
- **Formal Claim:** Ternary weights or activations can improve accuracy-constrained inference versus equally optimized binary low-bit alternatives.
- **Baseline:** Matched INT8/low-bit kernels and models at equal task accuracy, bandwidth, and batching.
- **Assumptions:** README speedups are claims requiring independent verification; separate compression, arithmetic, bandwidth, and accuracy effects.
- **Measurement:** Task accuracy, memory footprint, and measured latency/energy with matched kernels and conversion.
- **Falsifier:** Equivalent optimized baseline removes speed/size advantage at equal accuracy.
- **Result:** **Tier B / C:** 2-bit packed weight formats implemented. On CPU binary SIMD, decode instruction tax limits execution speed ($75.3\%$ of INT8 AVX2).
- **Next Action:** Audit model provenance before designing bounded inference experiments.

### Hypothesis H07: Representation-Independent Governed Execution
- **Formal Claim:** Representation-independent governed execution can improve a defined auditability property.
- **Baseline:** Conventional runtime plus explicit authorization/evidence boundary; equal threat model and host trust.
- **Assumptions:** Authorization, evidence integrity, factual validity, and consent are distinct properties.
- **Measurement:** Replay, tamper detection, revocation, and pre-side-effect denial under trustworthy execution.
- **Falsifier:** Conventional baseline meets the defined property at lower cost, or authorization bypass is reproduced.
- **Result:** **Tier B/C in this index:** Scoped deterministic, identity and detached-verification implementations exist; draft [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) is not adopted. The [bounded precursor](../../examples/governance/accountable-computation/REPORT.md) did not run Wasmtime + OPA; [CLM-03/04](../../experiments/research_001/CLAIM_REGISTER.md) remain under active test. Broad comparative auditability and custom-VM necessity are **INCONCLUSIVE**.
- **Next Action:** Preserve the reported local failure cases and complete equivalent conventional security/control experiments before claiming relative advantage.

### Hypothesis H08: AI-Assisted Research Trustworthiness & Discovery
- **Formal Claim:** AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process.
- **Baseline:** Human-only or alternative-assisted work under matched tasks and independent verification.
- **Assumptions:** Commit authorship does not prove prompt provenance; repeated model summaries are not independent verification.
- **Measurement:** Claim error rates, reproduction yield, correction costs, and provenance completeness.
- **Falsifier:** No improvement under controlled comparison, or benefits depend on unverified generated claims.
- **Result:** **Tier C / E:** Methodology implemented via session handoffs, prompt tracking, and commit lineage logging.
- **Next Action:** Append session handoffs and preserve failed claims rather than rewriting historical narrative.

---

## 4. The Three Research Tracks

```
+-----------------------------------------------------------------------+
|                       T81 Research Program                            |
+-----------------------------------------------------------------------+
|                                                                       |
|  Track R: Scientific Computing & Alternative Representations          |
|  - Radix economy, balanced ternary math, Base-81, adaptive tiering    |
|  - Target Hypotheses: H01, H02, H03, H04, H05, H06                    |
|                                                                       |
|  Track F: Foundation Systems & Governance Engineering                 |
|  - Bit-exact determinism (DCP), Axion policy, CanonFS identity        |
|  - Target Hypotheses: H07 (Supports Track R validation apparatus)     |
|                                                                       |
|  Track M: AI-Assisted Research Methodology                            |
|  - Prompt-to-code provenance, error tracking, research continuity     |
|  - Target Hypotheses: H08                                             |
|                                                                       |
+-----------------------------------------------------------------------+
```

---

## 5. Scientific Success and Failure Criteria

### Success Criteria
1. **Defensible Advantage:** Demonstrating an explicit workload, cost model, or device parameter region where ternary representation achieves lower area, energy, or latency than matched binary baselines.
2. **Reproducible Proof:** Every positive or negative finding is backed by a pinned commit SHA, runnable test script, and published dataset.
3. **Deterministic Governance:** Establishing bit-exact execution trace replay and pre-side-effect policy denial across heterogeneous OS environments.

### Failure Criteria (Falsification Thresholds)
1. **Software CPU Speed Claims:** Software ternary arithmetic or SWAR tryte unpacking on binary ALUs is deemed **FALSIFIED and FROZEN** for CPU execution speed.
2. **Overhead Overhead Domination:** Any theoretical radix advantage that vanishes after accounting for index decoding, digit packing, or conversion overhead is deemed **NOT SUPPORTED**.
3. **Unchecked Overclaims:** Asserting that a software prototype or CI green check proves physical silicon superiority is deemed a **SCIENTIFIC VIOLATION**.
