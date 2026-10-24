# Negative Controls Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/NEGATIVE_CONTROLS.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Approved Experimental Controls

---

## 1. Purpose

Negative controls are essential experimental calibrations that prove the sensitivity, correctness, and fail-closed security of both the target execution systems (T81 and Conventional Baseline) and the independent verification tools.

A negative control introduces a deliberate fault, corruption, or policy violation into the execution pipeline or evidence bundle. An experimental harness or verifier that fails to detect a negative control is declared compromised or defective.

---

## 2. Predefined Negative Controls Matrix

```
+-----------------------------------------------------------------------------------+
|                           NEGATIVE CONTROLS MATRIX                                |
|                                                                                   |
| Control ID  | Fault Injected                      | Expected Target Outcome       |
| ----------- | ----------------------------------- | ----------------------------- |
| NC-01       | Non-canonical JSON formatting       | Hash mismatch / Verification  |
| NC-02       | Un-sandboxed host file write        | Intercept breach / Policy DENY|
| NC-03       | Un-hashed state mutation            | Replay trace hash divergence |
| NC-04       | Forged cryptographic signature      | Signature verification FAIL  |
| NC-05       | Tampered input payload digest       | Digest mismatch FAIL          |
| NC-06       | Over-limit purchase order           | Pre-side-effect Policy DENY   |
| NC-07       | Stale policy evaluation timestamp  | Time-window policy DENY       |
+-----------------------------------------------------------------------------------+
```

---

## 3. Control Specifications & Expected Behaviors

### Control NC-01: Non-Canonical Serialization
- **Fault Injected:** Evidence bundle key order is mutated or whitespace/formatting changes are introduced (violating RFC 8785 JSON Canonicalization Scheme).
- **Expected System Outcome:** Offline verifier recomputing evidence digest MUST generate a different hash string. Verification fails with error code `ERR_CANONICAL_HASH_MISMATCH`.

### Control NC-02: Direct Un-sandboxed Host File Creation
- **Fault Injected:** Execution module attempts to write a purchase order directly to `/tmp/unauthorized_po.txt` bypassing CanonFS / SQLite content-addressed storage interface.
- **Expected System Outcome:** Host FFI sandbox / Axion policy kernel MUST catch the unauthorized syscall or un-imported capability, immediately terminate execution process, and emit `ERR_UNAUTHORIZED_SIDEEFFECT`. Zero bytes written to disk.

### Control NC-03: Un-hashed State Mutation
- **Fault Injected:** Execution module modifies internal memory register state during calculation without emitting an instruction/trace hash event.
- **Expected System Outcome:** Final trace hash verification recomputed from execution trace log fails to match `evidence.execution_trace_hash`. Verification fails with `ERR_TRACE_DIVERGENCE`.

### Control NC-04: Forged Cryptographic Signature
- **Fault Injected:** A single byte of the base64-encoded signature in `accountability.signature` is flipped.
- **Expected System Outcome:** Verifier signature check fails with `ERR_SIGNATURE_INVALID`. Evidence bundle rejected as untrusted.

### Control NC-05: Tampered Input Payload Digest
- **Fault Injected:** The input payload JSON parameter `quantity` is modified from $100$ to $500$ after bundle generation, while `evidence.input_digest` remains unchanged.
- **Expected System Outcome:** Verifier recomputing SHA256 of `meaning.parameters` detects mismatch with `evidence.input_digest`. Verification fails with `ERR_INPUT_DIGEST_MISMATCH`.

### Control NC-06: Over-Limit Purchase Order
- **Fault Injected:** Workload input parameters produce total purchase order value $V = \$35,000$ when spending limit $L = \$25,000$.
- **Expected System Outcome:** Both T81VM and Wasmtime runtime policy engines MUST evaluate verdict as `DENY`. Output storage write MUST NOT be attempted. Output bundle must record `authority.verdict == DENY`.

### Control NC-07: Stale Policy Evaluation
- **Fault Injected:** Bundle records policy verdict evaluated at timestamp $T - 2$ hours, where policy revocation occurred at $T - 1$ hour.
- **Expected System Outcome:** Verifier checks policy timestamp against revocation window and rejects bundle with `ERR_POLICY_REVOKED`.
