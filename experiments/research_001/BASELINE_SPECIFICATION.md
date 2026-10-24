# Conventional Reference Baseline Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/BASELINE_SPECIFICATION.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Approved Reference Baseline

---

## 1. Overview

This document specifies the conventional reference implementation baseline (**Arm iii**) used as the primary benchmark control against the T81 substrate (**Arm ii**).

To satisfy the **Fair Comparison Principle** ([RESEARCH_CHARTER.md](RESEARCH_CHARTER.md) §2.2), the baseline is constructed using off-the-shelf, open-source industrial components that implement content-addressed execution and detached evidence protocols without using T81-specific code or virtual machines.

---

## 2. Architecture & Components

```
+-----------------------------------------------------------------------------------+
|               CONVENTIONAL REFERENCE BASELINE ARCHITECTURE (ARM III)             |
|                                                                                   |
|  +--------------------+    +--------------------+    +-------------------------+  |
|  | WebAssembly Module | -> | Open Policy Agent  | -> | Internal RFC-00D4       |  |
|  | (Wasmtime Runtime) |    | (In-Process / FFI) |    | Draft Bundle Generator  |  |
|  +--------------------+    +--------------------+    +-------------------------+  |
|            |                         |                            |               |
|            v                         v                            v               |
|  +--------------------+    +--------------------+    +-------------------------+  |
|  | SQLite Storage /   |    | Synchronous Pre-   |    | Detached Offline        |  |
|  | Content-Addressed  |    | Commit Hook        |    | Verifier (verify.py)    |  |
|  +--------------------+    +--------------------+    +-------------------------+  |
+-----------------------------------------------------------------------------------+
```

### Component Stack
1. **Execution Substrate:** WebAssembly (Wasm) modules compiled from standard C/Rust sources, executed using `Wasmtime` (v18.0+ C/Python bindings).
2. **Policy Engine:** Open Policy Agent (OPA) / Rego rules linked via in-process FFI / C bindings for primary benchmarks to eliminate IPC transport bias. Asynchronous HTTP/gRPC sidecar OPA evaluation is measured separately as a secondary transport variable (see [HYPOTHESES.md](HYPOTHESES.md) H3).
3. **Storage Engine:** SQLite database for transactional local state, paired with SHA256 content-addressed blob storage under `objects/<hash>.blk`.
4. **Evidence Schema:** Internal experimental specification draft proposal `RFC-00D4` (Accountable Decision Bundle JSON schema residing at `spec/rfcs/RFC-00D4-accountable-decision-bundle.md`).
5. **Cryptographic Identity & Verification Dependencies:** Cryptographic signatures covering canonical JSON representations (RFC 8785 / JCS). Standalone offline verification (`verify.py`) requires Python 3.12 with the standard library and the `cryptography` PyPI package (`pip install cryptography`, for Ed25519 signature verification; Ed25519 is not present in the Python standard library). An optional pure-stdlib HMAC-SHA256 verification mode is supported for zero-external-dependency test environments.

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
| **Evidence Format** | RFC 8785 Canonical JSON (Primary) | RFC 8785 Canonical JSON (Primary) |

---

## 4. Workload Implementation Spec (Arm iii)

### 4.1 Execution Flow & Revocation Injection
1. **Module Instantiation:** Wasmtime instantiates the compiled `inventory_workload.wasm` module.
2. **Input Ingestion:** Input payload JSON is passed into Wasm memory. Wasm code calculates total purchase value $V = \sum q_i \times p_i$.
3. **Synchronous Pre-Commit Policy Intercept:** Prior to executing any storage write or side effect, Wasm code invokes the host synchronous callback `host_eval_policy(action_type, proposed_side_effect, total_amount)`.
4. **Deterministic Revocation Injection Point:** To simulate mid-flight revocation deterministically, the harness invokes a `post_calc_pre_commit` hook directly inside `host_eval_policy` immediately before evaluating the policy rules. If revocation is triggered for the trial, the policy context is updated synchronously in memory prior to rule evaluation.
5. **Policy Decision:** Host FFI evaluates the in-process OPA Rego rule against the current policy state.
   - If policy returns `ALLOW`, host FFI grants storage write handle. Wasm writes payload blob to SQLite / content-addressed directory.
   - If policy returns `DENY`, host FFI returns policy fault. Wasm halts execution and records denial trace.
6. **Evidence Generation:** Host runtime gathers `input_digest`, `output_digest`, `execution_trace_hash`, `policy_hash`, and signs the internal RFC-00D4 draft JSON bundle using Ed25519 (or HMAC-SHA256 fallback).

---

## 5. Offline Detached Verification Requirement

Verification of Arm (iii) evidence bundles MUST be executed using the standalone Python verification script (`verify.py`) under the following rules:
- Zero imports of Wasmtime, OPA, or producer C++ runtime libraries.
- Requires Python 3.12 stdlib plus `cryptography` package (or stdlib `hmac`/`hashlib` in HMAC fallback mode).
- Primary comparative measurements verify RFC 8785 canonical JSON bundles across all arms. Native secondary formats (e.g. raw tryte/binary streams) are recorded separately to quantify serialization overhead.
- Step 1: Validate bundle structure against internal RFC-00D4 experimental draft JSON Schema (`spec/rfcs/RFC-00D4-accountable-decision-bundle.md`).
- Step 2: Recompute `SHA256(RFC8785_canonical_json(meaning))` and verify agreement with `evidence.input_digest`.
- Step 3: Verify cryptographic signature over bundle digest.
- Step 4: Re-evaluate policy logic locally to verify `authority.verdict == ALLOW`.

If all steps pass, output verdict is `VERIFICATION_SUCCESSFUL`.
