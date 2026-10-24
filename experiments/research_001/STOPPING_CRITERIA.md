# Stopping Criteria Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/STOPPING_CRITERIA.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Approved Stopping Rules

---

## 1. Overview

This document specifies the predefined acceptance thresholds, early termination rules, statistical confidence requirements, and decision matrix for concluding Research Protocol 001.

Predefined stopping criteria prevent benchmark overfitting ("p-hacking") and guarantee scientific discipline: the experiment terminates immediately once conclusive statistical data is gathered.

---

## 2. Predefined Acceptance & Success Thresholds

An experimental outcome is classified as conclusive if and only if all corresponding acceptance criteria are satisfied across $N \ge 30$ repetitions (and $N = 300$ for safety expanded runs) per platform.

| System Capability | Target Metric | Required Threshold | Falsification Boundary |
| :--- | :--- | :--- | :--- |
| **Policy Safety** | Unauthorized Action Rate | Exactly **0.0%** ($p \le 0.095$ for $N=30$; $p \le 0.0099$ for $N=300$) | $>0.0\%$ (Any unauthorized side effect) |
| **Policy Safety** | Mid-Flight Revocation Interception | Exactly **0.0%** breaches | $>0.0\%$ breaches |
| **Auditability** | Offline Verification Success Rate | Exactly **100.0%** | $<100.0\%$ on valid bundles |
| **Auditability** | Offline Verification Latency | **<1,000 ms** per bundle | $>1,000$ ms per bundle |
| **Determinism** | Cross-Platform Digest Agreement | Exactly **100.0%** | $<100.0\%$ (Any x86_64 vs ARM64 mismatch) |
| **Controls** | Negative Controls Detection Rate | Exactly **100.0%** | $<100.0\%$ (Failure to detect injected fault) |

---

## 3. Early Termination Rules

The experimental execution runner MUST immediately halt testing and generate a terminal report upon encountering any of the following conditions:

1. **Rule ET-1 (Catastrophic Policy Breach):** Any system arm allows an over-limit or revoked purchase order to commit a side effect to storage (Unauthorized Action Rate $> 0.0\%$).
   - *Action:* Instantly stop execution for that arm. Tag arm as `FAILED_SECURITY_GATE`.
2. **Rule ET-2 (Determinism Divergence):** Running the identical workload on x86_64 and ARM64 produces different canonical evidence digests for the same input and policy.
   - *Action:* Halt cross-platform test. Tag arm as `FAILED_DETERMINISM_GATE`.
3. **Rule ET-3 (Negative Control Blindness):** System or verifier fails to detect an injected fault in [NEGATIVE_CONTROLS.md](NEGATIVE_CONTROLS.md) (e.g. valid verdict accepted on forged signature).
   - *Action:* Halt experiment. Tag verifier or harness as `COMPROMISED_HARNESS`.
4. **Rule ET-4 (Statistical Convergence):** For performance latency measurements, once $N \ge 30$ samples are collected and the 95% Confidence Interval width is $< 5\%$ of the mean ($\frac{CI_{95}}{\mu} < 0.05$).
   - *Action:* Conclude performance measurement phase for that workload.

---

## 4. Decision Matrix for Outcomes A, B, and C

Upon experiment completion (or early termination), the evaluation committee maps results onto one of three outcome classifications:

```
+-----------------------------------------------------------------------------------+
|                            EVALUATION DECISION MATRIX                             |
|                                                                                   |
|  [ Outcome A: Integrated VM Advantage ]                                           |
|  IF Arm (ii) achieves 0.0% breaches AND Arm (iii) suffers >0.0% breaches          |
|  OR Arm (ii) delivers statistically superior verification/latency performance      |
|     at equivalent host runtime complexity                                         |
|  THEN: Support Hypothesis H1. Validate custom T81 VM/Kernel for this workload.   |
|                                                                                   |
|  [ Outcome B: Decoupled Architecture Equivalence / Workload-Bounded Focus ]       |
|  IF Arm (iii) achieves 0.0% breaches AND 100% determinism AND 100% verification   |
|  AND Arm (iii) delivers equal/lower latency with >=5x lower host runtime LOC      |
|  THEN: Support Hypothesis H2. Restrict T81 VM scope for this workload class.      |
|                                                                                   |
|  [ Outcome C: Environmental / Transport Dependence ]                              |
|  IF deltas vanish when IPC transport or serialization formats are normalized      |
|  THEN: Support Hypothesis H3. Re-evaluate harness under transport control.        |
+-----------------------------------------------------------------------------------+
```

### Workload-Bounded Decision Directive (If Outcome B)
If Outcome B is validated, the evaluation committee SHALL execute the following workload-bounded research directive:
1. Conclude that for deterministic integer decision workloads of this complexity class, custom virtual machine dispatch is not required to maintain computational accountability and pre-side-effect policy enforcement.
2. Direct future protocol development toward lightweight, runtime-agnostic evidence formats based on internal `spec/rfcs/RFC-00D4-accountable-decision-bundle.md` schemas for standard WebAssembly and Linux environments.
3. Restrict all conclusions and engineering decisions strictly to the measured workload and guarantees. Automatic ecosystem-wide architectural pivots or global ISA deprecations are prohibited; broader changes require separate empirical research protocols and explicit governance review.
