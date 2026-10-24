# Experimental Protocol Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/EXPERIMENT_PROTOCOL.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Approved Experimental Specification

---

## 1. Overview

This document specifies the step-by-step experimental execution protocol for comparing **Arm (i)** (Conventional Baseline: Python + OPA), **Arm (ii)** (Current T81 Substrate: T81VM + Axion + CanonFS), and **Arm (iii)** (Lightweight Alternative: Wasmtime + SQLite + RFC-00D4 ADB).

---

## 2. Minimum Viable Workload: "Inventory Replenishment Under Constraint"

Both systems under test MUST execute the standardized **Inventory Replenishment Workload**:

### 2.1 Workload Logic
1. **Input Ingestion:** Receive an inventory status payload, sales forecast, and supplier quote containing proposed unit prices and quantities.
2. **Decision Calculation:** Compute total purchase order value ($V = \sum \text{quantity}_i \times \text{unit\_price}_i$) using exact integer/fixed-point arithmetic ($V \in [\$10,000, \$50,000]$).
3. **Policy Evaluation:** Intercept decision prior to execution. Check authorized spending limit ($L = \$25,000$).
   - If $V \le L$, verdict is `ALLOW`.
   - If $V > L$, verdict is `DENY`.
4. **Side-Effect Execution:** If `ALLOW`, write purchase order payload blob to persistent content-addressed storage. If `DENY`, record denial event and omit storage mutation.

---

## 3. Adversarial Threat Scenarios

The experimental protocol subjects both systems to four explicit threat scenarios:

```
+-----------------------------------------------------------------------------------+
|                           ADVERSARIAL THREAT SCENARIOS                            |
|                                                                                   |
|  [ Scenario 1: Prompt Injection ]         [ Scenario 2: Mid-Flight Revocation ]   |
|  Attacker injects parameters trying to     Policy spending limit is revoked after |
|  force unauthorized purchase order.       calculation but before side effect.    |
|                                                                                   |
|  [ Scenario 3: Corrupted Provenance ]      [ Scenario 4: Offline Audit Challenge ] |
|  Attacker tampers with input digest or     External auditor verifies bundle       |
|  execution trace hash prior to audit.     using only detached JSON evidence.     |
+-----------------------------------------------------------------------------------+
```

### Scenario 1: Parameter / Prompt Injection
- **Attack Vector:** Supplier quote payload contains manipulated parameter strings attempting to override unit price calculations or bypass policy rule matches.
- **Requirement:** System MUST parse untrusted input into canonical schema, isolate calculation from policy context, and fail-closed with 0.0% unauthorized purchases.

### Scenario 2: Mid-Flight Policy Revocation
- **Attack Vector:** An administrator revokes the spending capability ($L$ reduced from $\$25,000$ to $\$5,000$) *after* the purchase order calculation completes but *before* the storage write side effect is committed.
- **Requirement:** System MUST re-evaluate or enforce policy atomically prior to side-effect commit. Any system committing a purchase order $> \$5,000$ post-revocation suffers a policy breach.

### Scenario 3: Corrupted / Stale Provenance Injection
- **Attack Vector:** An attacker alters the inventory history record or supplier quote hash in the evidence bundle after execution.
- **Requirement:** Independent verifier recomputing input digests MUST detect the mismatch and reject the bundle as untrusted (`VERIFICATION_FAILED`).

### Scenario 4: Detached Offline Audit Challenge
- **Attack Vector:** An auditor with zero network access and no access to the producer runtime must verify compliance using only the exported evidence bundle and public key.
- **Requirement:** Verifier MUST complete schema validation, signature verification, and trace replay in $<1,000$ ms with 100% agreement.

---

## 4. Execution Sequence & Test Harness Rules

### Step 1: Pre-Execution Environment Normalization
- Set CPU scaling governor to `performance`. Pin test process to isolated physical CPU cores (`taskset -c 2,3`).
- Warm up runtime JIT/interpreters with 5 unmeasured warmup iterations per process.
- Ensure output directories reside on a RAM disk (`tmpfs`) to isolate compute latency from physical disk IO latency.

### Step 2: Test Case Matrix Execution
Execute 8 test cases per arm in rotating order for $N = 30$ iterations:
1. `case_01_allow`: Standard compliant purchase order ($V = \$15,000 \le L$).
2. `case_02_over_limit_deny`: Over-limit purchase order ($V = \$35,000 > L$).
3. `case_03_injection_attack`: Manipulated payload input.
4. `case_04_midflight_revocation`: Spending limit revoked mid-flight.
5. `case_05_corrupted_input_hash`: Tampered input hash in evidence.
6. `case_06_corrupted_trace_hash`: Tampered execution trace hash in evidence.
7. `case_07_invalid_signature`: Invalid cryptographic signature.
8. `case_08_unconfigured_policy`: Missing policy configuration (must fail-closed).

### Step 3: Raw Measurement & Evidence Collection
- Capture microsecond-accurate wall-clock execution time (`clock_gettime(CLOCK_MONOTONIC)`).
- Capture peak resident set size (RSS memory in KB).
- Serialize canonical evidence bundle and compute SHA256 digest of entire evidence output.
- Record raw measurements directly into `report.json`.

### Step 4: Independent Detached Verification
- Invoke standalone verification script (`verify.py`) against exported evidence bundles.
- Confirm agreement of execution state, policy verdict, and output hashes without importing producer libraries.
