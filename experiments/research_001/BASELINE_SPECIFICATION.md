# Conventional Reference Baseline Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/BASELINE_SPECIFICATION.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Approved Reference Baseline

---

## 1. Overview

This document specifies the conventional reference implementation baseline (**Arm iii**) used as the primary benchmark control against the T81 substrate (**Arm ii**).

To satisfy the **Fair Comparison Principle** (Charter §2.2), the baseline is constructed using off-the-shelf, open-source industrial components that implement content-addressed execution and detached evidence protocols without using T81-specific code or virtual machines.

---

## 2. Architecture & Components

```
+-----------------------------------------------------------------------------------+
|               CONVENTIONAL REFERENCE BASELINE ARCHITECTURE (ARM III)             |
|                                                                                   |
|  +--------------------+    +--------------------+    +-------------------------+  |
|  | WebAssembly Module | -> | Open Policy Agent  | -> | RFC-00D4 Decision       |  |
|  | (Wasmtime Runtime) |    | (Host FFI / OPA)   |    | Bundle Generator        |  |
|  +--------------------+    +--------------------+    +-------------------------+  |
|            |                         |                            |               |
|            v                         v                            v               |
|  +--------------------+    +--------------------+    +-------------------------+  |
|  | SQLite Storage /   |    | Pre-Side-Effect    |    | Detached Offline        |  |
|  | Content-Addressed  |    | Intercept Boundary |    | Verifier (verify.py)    |  |
|  +--------------------+    +--------------------+    +-------------------------+  |
+-----------------------------------------------------------------------------------+
```

### Component Stack
1. **Execution Substrate:** WebAssembly (Wasm) modules compiled from standard C/Rust sources, executed using `Wasmtime` (v18.0+ or equivalent standalone C/Python bindings).
2. **Policy Engine:** Open Policy Agent (OPA) / Rego rules embedded via Host FFI imports or local sidecar API.
3. **Storage Engine:** SQLite database for transactional local state, paired with SHA256 content-addressed blob storage under `objects/<hash>.blk`.
4. **Evidence Schema:** RFC-00D4 Accountable Decision Bundle (ADB) standard JSON format.
5. **Cryptographic Identity:** Standard Ed25519 or HMAC-SHA256 signatures covering canonical JSON representations (RFC 8785).

---

## 3. Equivalent Inputs, Privileges, and Resource Limits

To eliminate confounding variables, Arm (ii) and Arm (iii) MUST be executed under strictly identical operational constraints:

| Constraint Category | Arm (ii) T81 Substrate | Arm (iii) Conventional Baseline |
| :--- | :--- | :--- |
| **Input Corpus** | Standardized JSON payload (`workload-contract.json`) | Standardized JSON payload (`workload-contract.json`) |
| **Arithmetic Precision** | Exact 64-bit signed integer / fixed-point | Exact 64-bit signed integer / fixed-point |
| **Host CPU Quota** | 1 Dedicated CPU core (`taskset -c 2`) | 1 Dedicated CPU core (`taskset -c 2`) |
| **Memory Limit** | 512 MB RSS hard limit | 512 MB RSS hard limit |
| **Process Privileges** | Non-root `nobody` user | Non-root `nobody` user |
| **IO Sandbox** | Isolated `tmpfs` directory, zero network access | Isolated `tmpfs` directory, zero network access |
| **Policy Rule Set** | Axion Policy AST (`spending_limit <= 25000`) | OPA Rego Policy (`spending_limit <= 25000`) |

---

## 4. Workload Implementation Spec (Arm iii)

### 4.1 Execution Flow
1. **Module Instantiation:** Wasmtime instantiates the compiled `inventory_workload.wasm` module.
2. **Input Ingestion:** Input payload JSON is passed into Wasm memory. Wasm code calculates total purchase value $V = \sum q_i \times p_i$.
3. **Host FFI Policy Intercept:** Prior to executing any storage write, Wasm code invokes the host FFI function `host_eval_policy(action_type, proposed_side_effect, total_amount)`.
4. **Policy Decision:** Host FFI evaluates the OPA Rego rule against current policy configuration.
   - If policy returns `ALLOW`, host FFI grants storage write handle. Wasm writes payload blob to SQLite / content-addressed directory.
   - If policy returns `DENY`, host FFI returns policy fault. Wasm halts execution and records denial trace.
5. **Evidence Generation:** Host runtime gathers `input_digest`, `output_digest`, `execution_trace_hash`, `policy_hash`, and signs the RFC-00D4 JSON bundle using Ed25519 key.

---

## 5. Offline Detached Verification Requirement

Verification of Arm (iii) evidence bundles MUST be executed using the standalone Python verification script (`verify.py`) under the following rules:
- Zero imports of Wasmtime, OPA, or producer runtime libraries.
- Standard Python 3.12 standard library only (`json`, `hashlib`, `ed25519`).
- Step 1: Validate bundle structure against RFC-00D4 JSON Schema.
- Step 2: Recompute `SHA256(canonical_json(meaning))` and verify agreement with `evidence.input_digest`.
- Step 3: Verify cryptographic signature over bundle digest.
- Step 4: Re-evaluate policy logic locally to verify `authority.verdict == ALLOW`.

If all steps pass, output verdict is `VERIFICATION_SUCCESSFUL`.
