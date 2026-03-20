- **Title**: RFC-00C1: Compliance Trace Interchange Format (CTIF)
- **Status**: proposed
- **Type**: standards-track
- **Author**: T81 Foundation
- **Created**: 2026-03-20
- **Depends-on**:
  - RFC-00C0: Deterministic Compliance & Governed Execution Model
- **Relates-to**:
  - RFC-00C2: Deterministic Verification CLI

---

## 1. Abstract

This RFC specifies the **Compliance Trace Interchange Format (CTIF)**, a canonical, portable, and machine-readable standard for encoding Compliance Traces generated under the Governed Execution Model (RFC-00C0). The specification defines two mandatory representations: a human-readable JSON format for ease of inspection and integration, and a canonical binary format for efficient storage, transport, and deterministic hashing. The primary purpose of CTIF is to enable portable, third-party verification of T81 executions, providing a stable and documented interface for regulators, auditors, and independent infrastructure.

---

## 2. Motivation

The Governed Execution Model introduced in RFC-00C0 produces a Compliance Trace as the definitive proof of a computation's adherence to policy. For this proof to be useful, it must be portable and verifiable by any interested party, not just the originating system. A standardized format is essential to prevent fragmentation and ensure interoperability between different T81 implementations.

The motivations for CTIF are:
-   **Portability**: To allow a Compliance Trace generated on one T81 system to be verified on any other conformant system, including independent verifier nodes.
-   **Auditability**: To provide a stable, well-documented format that external auditors and regulatory bodies can easily parse and analyze.
-   **Efficiency**: To define a compact binary representation suitable for long-term storage in CanonFS and efficient network transfer.
-   **Usability**: To provide a human-readable JSON representation for debugging, manual inspection, and integration with standard analysis tools.

---

## 3. Format Specification

CTIF is defined by a logical structure and two physical representations.

### 3.1. Logical Structure

A CTIF object represents a Compliance Trace and MUST contain the fields specified in RFC-00C0, Section 8.

-   **`trace_version`** (String): Version of the CTIF specification (e.g., "1.0").
-   **`input_hash`** (String): CanonHash-81 of the program's input data.
-   **`policy_set_hash`** (String): CanonHash-81 of the Policy Set used.
-   **`execution_segments`** (Array of Objects): A log of deterministic events. Each segment object contains:
    -   `segment_type` (String Enum): e.g., `INSTRUCTION_GATE`, `MEMORY_ACCESS`, `FFI_CALL`.
    -   `result` (String Enum): `PASS` or `FAIL`.
    -   `metadata` (Object): Context-specific canonical data.
-   **`output_hash`** (String/Null): CanonHash-81 of the final output, or a canonical null if no output.
-   **`verification_status`** (String Enum): `COMPLETED_PASS` or `HALTED_COMPLIANCE`.
-   **`signature`** (String): Base-81 encoded cryptographic signature of the canonical binary representation's hash.

### 3.2. JSON Representation

The JSON representation is intended for human readability and tooling. All binary data, including hashes and signatures, SHALL be encoded as Base-81 strings.

**Example:**
```json
{
  "trace_version": "1.0",
  "input_hash": "Z1A...E5T",
  "policy_set_hash": "Z1B...F6U",
  "execution_segments": [
    {
      "segment_type": "INSTRUCTION_GATE",
      "result": "PASS",
      "metadata": { "instruction": "WLOAD", "hash": "Z1C...G7V" }
    }
  ],
  "output_hash": "Z1D...H8W",
  "verification_status": "COMPLETED_PASS",
  "signature": "Z5E...I9X"
}
```

### 3.3. Canonical Binary Representation

The binary representation is used for canonical hashing and efficient storage. It is defined by a fixed-layout, byte-aligned structure. All multi-byte integers are big-endian. All hashes are raw ternary bytes.

| Field                 | Type                     | Size (Bytes)    | Description                                       |
|-----------------------|--------------------------|-----------------|---------------------------------------------------|
| `trace_version`       | UTF-8 String (fixed)     | 8               | e.g., "1.0" padded with nulls.                    |
| `input_hash`          | Binary Hash              | 81              | Raw CanonHash-81 bytes.                           |
| `policy_set_hash`     | Binary Hash              | 81              | Raw CanonHash-81 bytes.                           |
| `output_hash`         | Binary Hash              | 81              | Raw CanonHash-81 bytes. Use canonical null if none. |
| `verification_status` | uint8 Enum               | 1               | 0=`COMPLETED_PASS`, 1=`HALTED_COMPLIANCE`.        |
| `num_segments`        | uint32                   | 4               | Number of execution segments that follow.         |
| `segments_data`       | Binary Data (variable)   | Variable        | Concatenated canonical binary form of all segments. |
| `signature`           | Binary Signature         | 512             | Raw signature bytes.                              |

The canonical binary form of `execution_segments` and their `metadata` MUST be specified in a separate, dedicated RFC to allow for future extension. For CTIF v1.0, this is considered an opaque but deterministic byte blob.

---

## 4. Canonical Hashing

The definitive CanonHash of a Compliance Trace object MUST be the CanonHash-81 of its **Canonical Binary Representation**.

The JSON representation is an auxiliary format and MUST NOT be used for integrity verification or persistence in CanonFS. Any system that parses a JSON representation of a trace for verification MUST first convert it to the canonical binary format.

---

## 5. Serialization and Deserialization

-   A conformant T81 system that generates a CTIF object MUST be capable of serializing it to both the JSON representation and the Canonical Binary Representation.
-   A conformant T81 system or verification tool that consumes a CTIF object MUST be capable of parsing both representations.
-   When parsing a JSON representation, the parser MUST validate that all fields conform to the types and constraints defined in this specification (e.g., Base-81 encoding for hashes).

---

## 6. Security Considerations

The CTIF standard itself does not introduce new security vulnerabilities; however, consumers of the format MUST adhere to strict verification procedures.
-   **Signature Verification is Mandatory**: The `signature` field is the primary defense against tampering. A CTIF object is invalid until its signature has been successfully verified against the hash of its binary representation using the public key of the originating Governor.
-   **Data Encoding**: Parsers for both JSON and binary formats must be robust against malformed inputs, such as incorrect string encodings or out-of-bounds length fields, to prevent denial-of-service or buffer overflow attacks.

---

## 7. Conformance Requirements

A system or tool claiming conformance with CTIF v1.0 MUST:
1.  Implement a parser for both the JSON and Canonical Binary representations.
2.  Implement a serializer for both the JSON and Canonical Binary representations.
3.  Use the Canonical Binary Representation for all hashing and integrity verification operations.
4.  Correctly handle the Base-81 string encoding for all hash and signature fields in the JSON representation.
5.  Adhere to the logical structure and field requirements defined in Section 3.1.
