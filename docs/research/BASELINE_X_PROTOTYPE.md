# Project Baseline-X Bootstrap & Architecture Specification

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Project Baseline-X Bootstrap & Architecture Specification](#project-baseline-x-bootstrap-&-architecture-specification)
  - [1. Executive Summary & Purpose](#1-executive-summary-&-purpose)
  - [2. Technology Stack & Scope Constraints](#2-technology-stack-&-scope-constraints)
    - [Minimal Stack Selection](#minimal-stack-selection)
    - [Explicit "Do Not Build" List](#explicit-"do-not-build"-list)
  - [3. Directory Layout & Repository Structure](#3-directory-layout-&-repository-structure)
  - [4. Primary Adversarial Scenario: Mid-Flight Policy Revocation](#4-primary-adversarial-scenario-mid-flight-policy-revocation)
    - [Flow Sequence](#flow-sequence)
  - [5. Build, Run, and Verification Commands](#5-build-run-and-verification-commands)
    - [Build](#build)
- [Build Wasm guest module](#build-wasm-guest-module)
- [Bootstrap Baseline-X environment](#bootstrap-baseline-x-environment)
    - [Execution & Audit](#execution-&-audit)
- [Run the inventory replenishment decision](#run-the-inventory-replenishment-decision)
- [Verify the resulting RFC-00D4 decision bundle offline](#verify-the-resulting-rfc-00d4-decision-bundle-offline)
  - [6. Comparative Evaluation Matrix against Arms (i) and (ii)](#6-comparative-evaluation-matrix-against-arms-i-and-ii)

<!-- T81-TOC:END -->


**Date:** April 2026
**Status:** Experimental Bootstrap Specification
**Target Surface:** `docs/research/BASELINE_X_PROTOTYPE.md`

---

## 1. Executive Summary & Purpose

**Project Baseline-X** is a minimal (~500 LOC target) control prototype designed to evaluate **Arm (iii)** of the Accountable Computation Experiment Framework ([`docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md`](./ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md)).

Baseline-X tests the hypothesis that computational accountability (governance, evidence, authority, and challengeability) can be achieved through **lightweight cryptographic protocols and standard sandboxed runtimes (Wasm/SQLite/Sigstore)** without requiring a bespoke C++ virtual machine, custom ISA (TISC), or balanced ternary representation.

---

## 2. Technology Stack & Scope Constraints

### Minimal Stack Selection
* **Runtime:** Wasmtime (WebAssembly runtime) or Python `wasmtime` bindings for sandboxed execution of decision logic.
* **Storage:** Embedded SQLite database (for inventory, quote logs, and audit trails).
* **Signing & Attestation:** Cosign / Sigstore (or standard ECDSA P-256 / Ed25519) for detached artifact signing.
* **Evidence Schema:** JSON-LD / Canonical JSON conforming strictly to **RFC-00D4 (Accountable Decision Bundle)**.

### Explicit "Do Not Build" List
To keep the prototype bounded within the ~500 LOC target, the following MUST NOT be built or imported:
1. **NO custom ISA or bytecode interpreter** (use standard Wasm compiled from Rust/C/AssemblyScript).
2. **NO balanced ternary representation** or SWAR emulation algorithms.
3. **NO Axion DSL parser or C++ Axion kernel porting.**
4. **NO full CanonFS C++ reimplementation** (content addressing done via standard SHA-256 or BLAKE3 digests).
5. **NO external network microservices** or Kubernetes dependencies.

---

## 3. Directory Layout & Repository Structure

The prototype will be bootstrapped under `experiments/baseline_x/`:

```
experiments/baseline_x/
├── README.md                  # Quickstart & test instructions
├── Cargo.toml / pyproject.toml # Minimal dependencies
├── src/
│   ├── main.rs / app.py       # Core runner & CLI entrypoint (~150 LOC)
│   ├── host_ffi.rs / ffi.py   # Wasm host function bindings with capability checks (~100 LOC)
│   ├── policy.rs / policy.py  # Pre-side-effect policy validator (~100 LOC)
│   └── bundle.rs / bundle.py  # RFC-00D4 Decision Bundle generator & verifier (~150 LOC)
├── wasm_guest/                # Target guest decision module
│   └── src/lib.rs             # Inventory replenishment logic (<100 LOC)
└── tests/
    └── test_revocation.py     # Adversarial mid-flight policy revocation test
```

---

## 4. Primary Adversarial Scenario: Mid-Flight Policy Revocation

Baseline-X will implement the **Mid-Flight Policy Revocation** scenario first:

```
[ Wasm Guest ] ---> (1) Calculate PO ($25,000) ---> [ Host FFI Interceptor ]
                                                           |
                                  [ Mid-Flight Policy Revocation Event ]
                                  (Spending Limit Reduced to $10,000)
                                                           |
                                                           v
                                              (2) Validate Policy Hash
                                              (3) FAIL-CLOSED: DENY
                                              (4) Emit Signed RFC-00D4 Deny Bundle
```

### Flow Sequence
1. The Wasm guest receives a valid inventory trigger and supplier quote, calculating a proposed Purchase Order of $25,000.
2. The guest calls host FFI `submit_purchase_order(amount=25000)`.
3. The Host FFI interceptor evaluates the currently active policy. Concurrently, a policy update event revokes the $50,000 limit and enforces a $10,000 limit.
4. The host policy engine detects the limit reduction, rejects the side effect (`verdict: DENY`), and emits an RFC-00D4 Accountable Decision Bundle documenting the blocked attempt.

---

## 5. Build, Run, and Verification Commands

### Build
```bash
# Build Wasm guest module
cd experiments/baseline_x/wasm_guest && cargo build --target wasm32-unknown-unknown --release

# Bootstrap Baseline-X environment
cd experiments/baseline_x && python3 -m venv venv && source venv/bin/activate
pip install wasmtime jsonschema cryptography
```

### Execution & Audit
```bash
# Run the inventory replenishment decision
python3 src/app.py run --inventory-data examples/inventory.json

# Verify the resulting RFC-00D4 decision bundle offline
python3 src/app.py verify --bundle output/decision_bundle_8491.json --policy-key keys/policy_pub.pem
```

---

## 6. Comparative Evaluation Matrix against Arms (i) and (ii)

Baseline-X results will be directly benchmarked against Arms (i) and (ii) across the agreed experiment metrics:

| Metric | Arm (i): Conventional (Python+OPA) | Arm (ii): Current T81 (T81VM+Axion) | Arm (iii): Baseline-X (Wasm+RFC-00D4) |
| :--- | :--- | :--- | :--- |
| **Unauthorized Action Rate (%)** | Evaluated under prompt injection | Target: 0.0% | Target: 0.0% |
| **Verification Latency (ms)** | High (DB + HTTP OPA query) | Target: <10ms | Target: <1ms (Offline local verify) |
| **Mid-Flight Policy Revocation** | Race condition risk (time-of-check vs time-of-use) | Fail-closed in `eval_axion_call` | Fail-closed at Host FFI boundary |
| **Stack Complexity (LOC)** | ~15,000 LOC (deps) | ~120,000 LOC C++ VM/OS | **~500 LOC** |
| **Cross-Platform Bit-Identity** | Non-deterministic (Python GC/FP) | Bit-exact (DCP) | Bit-exact Wasm execution |
