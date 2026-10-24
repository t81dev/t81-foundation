# Claims Register & Evidence Matrix

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/CLAIM_REGISTER.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Active Governance Claim Tracking

---

## 1. Overview & Claim Governance Rules

The **Claim Register** maintains a strict, auditable inventory of all public technical claims made regarding the T81 Foundation substrate (T81VM, Axion, CanonFS, TISC) and conventional reference alternatives.

To enforce scientific integrity:
1. **Unproven Claim Prohibition:** No architectural claim may be presented as an established fact in public documentation, whitepapers, or READMEs unless its state in this register is `SUPPORTED_BY_EVIDENCE`.
2. **Evidence Association:** Every claim MUST be mapped to explicit empirical evidence requirements and target falsification metrics.
3. **Claim States:**
   - `UNTESTED_HYPOTHESIS`: Claim is proposed but lacks comparative empirical evidence.
   - `UNDER_ACTIVE_TEST`: Claim is undergoing evaluation in Research Protocol 001.
   - `SUPPORTED_BY_EVIDENCE`: Claim is empirically proven across $N \ge 30$ samples on all target platforms with zero security/determinism failures (establishing a Clopper-Pearson 95% upper confidence limit of $p \le 0.0950$, or $p \le 0.0099$ under expanded $N=300$ safety testing).
   - `FALSIFIED`: Empirical evidence has contradicted the claim under predefined stopping criteria.
   - `SUPERSEDED_BY_PROTOCOL`: Claim has been retired following an Outcome B pivot.

---

## 2. Master Claim Register Matrix

| Claim ID | Claim Summary | Architectural Layer | Target Hypothesis | Required Empirical Evidence | Current State |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CLM-01** | T81VM delivers bit-exact integer interpreter execution across x86_64 and ARM64. | T81VM / Core Interpreter | H1 | 100% agreement on execution trace hashes across platforms ($N \ge 30$). | `SUPPORTED_BY_EVIDENCE` |
| **CLM-02** | In-kernel policy evaluation (`eval_axion_call`) prevents mid-flight side-effect breaches where decoupled sidecars fail. | Axion Kernel / Dispatch Loop | H1 | 0.0% breach rate on T81 vs $>0.0\%$ breach rate on conventional baseline under race conditions. | `UNDER_ACTIVE_TEST` |
| **CLM-03** | Custom TISC ISA/VM is strictly necessary to guarantee computational accountability and pre-side-effect policy checks. | TISC ISA / T81VM | H1 vs H2 | Conventional Wasmtime + RFC-00D4 draft pipeline suffers policy breaches or determinism failures that T81 prevents. | `UNDER_ACTIVE_TEST` |
| **CLM-04** | Off-the-shelf WebAssembly (Wasmtime) + RFC-00D4 draft evidence bundles provide equal safety and auditability with equal/lower latency and $\ge 5\times$ lower host runtime LOC footprint. | Conventional Wasm Baseline | H2 | Wasmtime baseline achieves 0.0% breach rate, 100% verification pass rate, equal or lower decision execution latency, and $\ge 5\times$ lower host runtime LOC footprint versus T81. | `UNDER_ACTIVE_TEST` |
| **CLM-05** | Balanced ternary representation (Base-81 SWAR) provides structural performance advantages over 2-bit packed binary SIMD. | SWAR / ISA Opcodes | Hypothesis E (Charter) | Benchmark showing superior compute density or throughput on binary hardware. | `FALSIFIED` (STOP) |
| **CLM-06** | Detached evidence verification using internal RFC-00D4 draft schema allows 100% offline auditability without running producer C++ VM libraries. | CanonFS / RFC-00D4 Draft Schema | H1 & H2 | Standalone Python `verify.py` passes 100% of valid bundles in $<1,000$ ms using stdlib + `cryptography` package. | `SUPPORTED_BY_EVIDENCE` |
| **CLM-07** | T81Float soft-float conversions deliver cross-platform floating-point determinism in canonical evidence. | T81Float Soft-Math | Core Float Subsystem | Bit-exact float conversion across native x86_64 and ARM64 hosts. | `FALSIFIED` (v2 Excluded) |

---

## 3. Claim Audit & Resolution Plan

1. **CLM-01 Resolution:** Supported by existing CTest suites (`vm_determinism_property_test.cpp`, `deterministic_provenance`).
2. **CLM-05 Resolution:** Formally falsified and frozen (STOP) by research reports (`TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`, `PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md`).
3. **CLM-07 Resolution:** Formally falsified for floating-point conversions (PR #551 audit); float conversions are explicitly excluded from evidence v2 (`experiments/deterministic_provenance/README.md`).
4. **CLM-02, CLM-03, CLM-04 Resolution:** Will be conclusively resolved upon execution and completion of Research Protocol 001 under the stopping criteria defined in [STOPPING_CRITERIA.md](STOPPING_CRITERIA.md).
