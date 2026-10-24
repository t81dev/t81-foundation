# Negative Controls Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/NEGATIVE_CONTROLS.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Approved Experimental Controls

---

## 1. Purpose & Threat Taxonomy

Negative controls are essential experimental calibrations that prove the sensitivity, correctness, and fail-closed security of both the target execution systems (T81 and Conventional Baseline) and the independent verification tools.

A negative control introduces a deliberate fault, corruption, or policy violation into the execution pipeline or evidence bundle. An experimental harness or verifier that fails to detect a negative control is declared compromised or defective.

Fault injections are classified into three distinct threat categories and trust boundaries:
1. **Malformed Evidence Artifacts (Evidence Boundary):** Tampering with serialized evidence, formatting, keys, or hashes after execution completed.
2. **Malicious Guest Execution (Guest Boundary):** Attempting unauthorized operations or limit bypasses from within the sandboxed workload code.
3. **Malicious Host / Infrastructure Behavior (Host Boundary):** Tampering with policy evaluation state, host FFI return handles, or system clocks outside guest code execution.

---

## 2. Predefined Negative Controls Matrix

```
+-----------------------------------------------------------------------------------+
|                           NEGATIVE CONTROLS MATRIX                                |
|                                                                                   |
| Control ID | Fault Category     | Fault Injected             | Expected Outcome   |
| ---------- | ------------------ | -------------------------- | ------------------ |
| NC-01      | Malformed Evidence | Non-canonical key order    | Digest mismatch    |
| NC-01B     | Malformed Evidence | Invalid JSON syntax        | Syntax error FAIL  |
| NC-02      | Malicious Guest    | Direct host file write     | Syscall/FFI DENY   |
| NC-03      | Malicious Guest    | Un-recorded state mutation | Trace hash divergence|
| NC-04      | Malformed Evidence | Forged signature byte      | Signature FAIL     |
| NC-05      | Malformed Evidence | Tampered input payload     | Input digest FAIL  |
| NC-06      | Malicious Guest    | Over-limit purchase order  | Pre-commit Policy DENY|
| NC-07      | Malicious Host     | Stale evaluation timestamp | Revocation DENY    |
| NC-08      | Malicious Host     | Tampered FFI return handle | Fail-closed DENY   |
+-----------------------------------------------------------------------------------+
```

---

## 3. Control Specifications & Trust Boundary Audit

### 3.1 Category 1: Malformed Evidence Artifacts (Evidence Boundary)

#### Control NC-01: Non-Canonical Serialization Key Order
- **Trust Model & Verification Rule:** The evidence protocol requires all serialized evidence artifacts on disk and on the wire to be bit-exact RFC 8785 (JCS) canonical JSON bytes. The verifier enforces canonical formatting by comparing raw file/payload bytes against the expected JCS digest (`SHA256(raw_bytes) == evidence.digest`) and confirming `raw_bytes == JCS_serialize(parse_json(raw_bytes))`.
- **Fault Injected:** Evidence bundle JSON keys are reordered (e.g. `{"b": 1, "a": 2}` instead of canonical `{"a": 2, "b": 1}`) without violating standard JSON grammar rules.
- **Expected Outcome:** Because raw payload bytes deviate from bit-exact JCS canonical formatting, direct byte hash comparison or JCS exact-byte validation fails with error code `ERR_CANONICAL_HASH_MISMATCH`. (This is explicitly distinguished from `ERR_JSON_SYNTAX_ERROR` in NC-01B, which tests syntactically malformed JSON).

#### Control NC-01B: Malformed JSON Syntax
- **Fault Injected:** A trailing comma or unescaped control character is inserted into the serialized evidence file (`{"meaning": ... ,}`).
- **Expected Outcome:** Standalone verifier parser fails JSON syntax validation. Verification fails with error code `ERR_JSON_SYNTAX_ERROR`.

#### Control NC-04: Forged Cryptographic Signature
- **Fault Injected:** A single byte of the base64-encoded signature in `accountability.signature` is flipped.
- **Expected Outcome:** Verifier signature check fails with `ERR_SIGNATURE_INVALID`. Evidence bundle rejected as untrusted.

#### Control NC-05: Tampered Input Payload Digest
- **Fault Injected:** The input payload JSON parameter `quantity` is modified from $100$ to $500$ after bundle generation, while `evidence.input_digest` remains unchanged.
- **Expected Outcome:** Verifier recomputing SHA256 of `meaning.parameters` detects mismatch with `evidence.input_digest`. Verification fails with `ERR_INPUT_DIGEST_MISMATCH`.

### 3.2 Category 2: Malicious Guest Execution (Guest Boundary)

#### Control NC-02: Direct Un-sandboxed Host File Creation
- **Fault Injected:** Execution module attempts to write a purchase order directly to `/tmp/unauthorized_po.txt` bypassing CanonFS / SQLite content-addressed storage interface.
- **Expected Outcome:** Host FFI sandbox / Axion policy kernel MUST catch the unauthorized syscall or un-imported capability, immediately terminate execution process, and emit `ERR_UNAUTHORIZED_SIDEEFFECT`. Zero bytes written to disk.

#### Control NC-03: Un-hashed State Mutation
- **Fault Injected:** Execution module modifies internal memory register state during calculation without emitting an instruction/trace hash event.
- **Expected Outcome:** Final trace hash verification recomputed from execution trace log fails to match `evidence.execution_trace_hash`. Verification fails with `ERR_TRACE_DIVERGENCE`.

#### Control NC-06: Over-Limit Purchase Order
- **Fault Injected:** Workload input parameters produce total purchase order value $V = \$35,000$ when spending limit $L = \$25,000$.
- **Expected Outcome:** Both T81VM and Wasmtime runtime policy engines MUST evaluate verdict as `DENY`. Output storage write MUST NOT be attempted. Output bundle must record `authority.verdict == DENY`.

### 3.3 Category 3: Malicious Host / Infrastructure Behavior (Host Boundary)

#### Control NC-07: Stale Policy Evaluation Timestamp
- **Fault Injected:** Host runtime records policy verdict evaluated at timestamp $T - 2$ hours, where policy revocation occurred at $T - 1$ hour.
- **Expected Outcome:** Verifier checks policy timestamp against revocation window and rejects bundle with `ERR_POLICY_REVOKED`.

#### Control NC-08: Tampered FFI Return Handle
- **Fault Injected:** A compromised host FFI layer attempts to return a success code (`0`) to guest Wasm memory when policy evaluation returned `DENY`.
- **Expected Outcome:** Pre-commit storage boundary validates host signed verdict object before committing payload blob. Storage engine detects missing `ALLOW` signature and aborts with `ERR_HOST_FFI_VERDICT_MISMATCH`.
