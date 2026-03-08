# RFC-0025: Policy-Gated Tensor Loading via CanonFS

**Status:** accepted
**Type:** standards-track
**Applies-To:** `spec/tisc-spec.md`, `spec/t81vm-spec.md`, `spec/supplemental/axion-policy-grammar.md`, `spec/supplemental/canonfs-spec.md`
**Created:** 2026-02-11
**Updated:** 2026-03-08
**Requires:** `RFC-0004`, `RFC-0009`, `RFC-0020`, `RFC-0022`

---

## 1. Summary

Status note: accepted for the `TLOADHASH` policy-gated load path and active
`allowed-tensor-hashes` policy surface. Operational hardening and CI evidence
tracking remain backlog work, not blockers to design acceptance.

This RFC specifies a new, formally-verified mechanism for loading tensors into the T81 Virtual Machine. It introduces a privileged TISC instruction, `TLOADHASH`, which loads a tensor `CanonObject` from the `CanonFS` content-addressed storage system by its cryptographic hash. The entire operation is supervised by the Axion kernel, which MUST validate the tensor's hash against a new `allowed-tensor-hashes` list in the active Axion policy.

This change is a direct, normative implementation of the **"Policy-Gated Large Model Inference"** pattern and the **"Cognitive Provenance"** principle outlined in `docs/systems-integration-agi.md`. It formally bridges the gap between the high-level AGI architecture and the TISC-level execution model, ensuring all AI models are loaded with full, auditable, and deterministic-provenance.

## 2. Motivation

The `docs/systems-integration-agi.md` specification (Sections 2.2 and 6.2) mandates a "chain of custody" for all data entering an agent's cognitive space. This is a cornerstone of the T81 safety model. The current `examples/llama32_demo.cpp` implementation uses mock weights created directly in memory, which, while useful for testing, bypasses this critical security and provenance check.

This RFC rectifies this by introducing a formal, secure, and policy-governed loading mechanism as a first-class citizen of the T81 architecture, fully integrated with `CanonFS` and `Axion`.

## 3. Proposal

### 3.1. Axion Policy Language (APL) Extension

The APL, specified in `spec/supplemental/axion-policy-grammar.md` and `RFC-0022`, is extended to support a new directive for whitelisting tensor hashes.

#### 3.1.1. Formal Syntax (EBNF)

The top-level policy grammar is amended as follows:

```ebnf
policy_directive ::= ... | allowed_tensor_hashes_directive;

allowed_tensor_hashes_directive ::= '(' 'allowed-tensor-hashes' '[' hash_string* ']' ')';

hash_string ::= '"' 'sha3-256:' ( '0' | ... | '9' | 'a' | ... | 'f' ){64} '"';
```

#### 3.1.2. Semantics

- The `allowed-tensor-hashes` directive contains a list of strings, each representing a permitted `CanonHash-81` of a tensor `CanonObject`.
- If the `allowed-tensor-hashes` list is absent or empty in a policy, any attempt to use `TLOADHASH` MUST result in a `Security Fault` (`POLICY_VIOLATION`).
- The Axion kernel MUST load and parse this list upon policy initialization.

#### 3.1.3. Canonical Hash Representation

The term `CanonHash-81` in the context of this RFC's policy files and traces refers to the **ASCII representation of a binary hash digest**, not a ternary-native hash algorithm. To prevent ambiguity, the normative representation is:
- **Algorithm:** `sha3-256` applied to the canonical `CanonObject` byte stream.
- **Encoding:** The resulting 32-byte binary digest MUST be encoded as a 64-character lowercase hexadecimal string.
- **Format:** The final string in policy files and traces MUST be prefixed with `sha3-256:`.

This ensures that the identity of `CanonObjects` is deterministic and verifiable across both binary and ternary components of the ecosystem.

### 3.2. CanonFS Interaction

The `TLOADHASH` instruction interacts with `CanonFS` as the sole storage abstraction, per `canonfs-spec.md`.

#### 3.2.1. Object Format and Serialization

- The tensor to be loaded MUST be stored as a `CanonObject`.
- The canonical serialization format for this `TensorObject` is defined as:
    1.  **Type ID (1 byte):** `0x20` (provisional ID for `TensorObject`).
    2.  **Header (71 bytes):**
        -   `version` (1 byte): `1`.
        -   `format` (1 byte): `NativeFormat` enum value (e.g., `T3_K`).
        -   `rank` (1 byte): Number of dimensions, `0` to `8`.
        -   `reserved` (4 bytes): MUST be all zeros.
        -   `shape` (64 bytes): An array of 8 `uint64_t` values representing the size of each dimension. Dimensions for ranks less than 8 are padded with zeros.
    3.  **Data Payload:** The raw byte data of the tensor.
- **Endianness:** All multi-byte integer fields in the header (`version`, `rank`, `shape`) **MUST be little-endian**.

A utility, `t81-canonize-tensor`, will be created to serialize a raw tensor into this `CanonObject` format, compute its `sha3-256` `CanonHash-81`, and place it in the `CanonFS` store.

### 3.3. TISC and VM Specification

#### 3.3.1. New TISC Instruction: `TLOADHASH`

A new privileged instruction is added to the "Governed Memory Access" class.

- **Opcode:** `TLOADHASH`
- **Opcode Index:** `0x48` (proposed).
- **Assembly Syntax:** `TLOADHASH rd, rs`
- **Instruction Format (81-trit word):**
  ```
  | Field    | Trit Range | Description                                                |
  |----------|------------|------------------------------------------------------------|
  | OPC      | 0-8        | 0x48 (TLOADHASH)                                           |
  | MODE     | 9-17       | 0x00 (Register-indirect addressing)                        |
  | RD       | 18-26      | Destination register for the received tensor handle        |
  | RS       | 27-35      | Source register containing a handle to a string symbol     |
  | UNUSED   | 36-80      | Reserved, must be zero. Faults if not.                     |
  ```

#### 3.3.2. Privilege and Tier Requirements

- The `TLOADHASH` instruction is **universally available** and can be invoked from any Cognitive Tier.
- It is classified as **privileged** because its execution is non-bypassable and fully mediated by the Axion kernel. Its safety is derived from this governance, not from restricted access.

#### 3.3.3. VM Execution and Fault Semantics

- `STATE' = TLOADHASH(STATE, rd, rs)`
- The VM MUST verify that `R[rs]` contains a valid, non-zero handle to a `Symbol` in the program's symbol pool. If not, trigger `DecodeFault`.
- Let `hash_string` be the string referenced by the symbol handle.
- The VM MUST delegate to Axion for verification. Let `(verdict, reason) = Axion.verify(OPC_TLOADHASH, hash_string)`.
- **If `verdict == DENY`:**
    - The VM MUST emit an Axion trace event with the exact reason: `"TLOADHASH policy_violation hash=<hash_string>"`.
    - `STATE' = FAULT(SecurityFault, {sub_type: POLICY_VIOLATION, hash: hash_string, reason: reason})`.
- **Else (`verdict == ALLOW`):**
    - The VM requests the object from `CanonFS`: `tensor_object = CanonFS.fetch(hash_string)`.
    - **If `tensor_object == NOT_FOUND`:**
      - The VM MUST emit an Axion trace event with the exact reason: `"TLOADHASH canonfs_miss hash=<hash_string>"`.
      - `STATE' = FAULT(BoundsFault, {sub_type: CANONFS_MISS, hash: hash_string})`.
    - **Else:**
      - The VM deserializes the `tensor_object`. If the header is malformed (invalid magic number, version, or padding), trigger `DecodeFault`.
      - Let `tensor_handle = VM.TensorPool.allocate(deserialized_data)`.
      - `R'[rd] = tensor_handle`. The register tag for `rd` is set to `TensorHandle`.
      - The VM MUST emit an Axion trace event with the exact reason: `"TLOADHASH success hash=<hash_string> handle=<tensor_handle>"`.
      - All other registers and flags in `STATE'` are unchanged from `STATE`.

#### 3.3.4. Tensor Pool Determinism

The `T81VM.TensorPool` MUST adhere to the following rules to ensure determinism:
1.  The pool is cleared at the start of each program execution.
2.  Allocation via `TLOADHASH` is **strictly append-only**.
3.  The returned handle is the next available 1-based index.
4.  Since TISC execution order is deterministic, the handle sequence is also deterministic.
5.  No deallocation or implicit compaction is permitted during program execution. Memory reclamation is the responsibility of the Deterministic GC (`RFC-0006`) upon program termination or at defined safepoints.

## 4. Security Model Impact

This RFC is a significant hardening of the T81 security model. It operationalizes the "Cognitive Provenance" principle by ensuring that no tensor, particularly those representing AI model weights, can enter the VM execution state without being cryptographically verified against an explicit, auditable policy. This prevents model poisoning, unauthorized modifications, and provides a complete, deterministic chain-of-custody from storage to execution.

## 5. Implementation Plan

1.  **Axion:** Update the APL parser for `allowed-tensor-hashes`. Implement the `Axion::verify_tensor_load(hash)` entry point.
2.  **T81VM:** Add `OPC_TLOADHASH`. Implement the full execution and faulting logic, including the calls to Axion and CanonFS, and emitting the specified trace strings.
3.  **Toolchain:** Create `t81-canonize-tensor` to serialize and hash tensor files into the `CanonFS` store.
4.  **Demonstration:** Modify `examples/llama32_demo.cpp` to use this full, end-to-end flow.

## 6. Acceptance Criteria

1.  The `t81-canonize-tensor` tool is implemented.
2.  The `TLOADHASH` instruction is implemented in the T81VM.
3.  The modified `llama32_demo.cpp` runs successfully, loading its tensor via `TLOADHASH`.
4.  When run, the Axion trace contains the exact string `"TLOADHASH success hash=..."`.
5.  When the hash is removed from the policy, the demo MUST fail with a `SecurityFault (POLICY_VIOLATION)`, and the trace MUST contain `"TLOADHASH policy_violation hash=..."`.
6.  When the tensor object is removed from `CanonFS`, the demo MUST fail with a `BoundsFault (CANONFS_MISS)`, and the trace MUST contain `"TLOADHASH canonfs_miss hash=..."`.

---
### Cross-References

- **Data Types (`CanonObject`)**: `spec/supplemental/canonfs-spec.md`, `spec/t81-data-types.md`.
- **TISC ISA**: `spec/tisc-spec.md`, Sections 5.6, 5.11.
- **T81VM Fault Model**: `spec/t81vm-spec.md`, Section 6.
- **Axion Policy Grammar & Tracing**: `spec/supplemental/axion-policy-grammar.md`, `RFC-0020`.
- **Architectural Principles**: `docs/systems-integration-agi.md`, Sections 2.2, 6.2.
