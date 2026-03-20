- **Title**: RFC-00C2: Deterministic Verification CLI (`t81-verify`)
- **Status**: proposed
- **Type**: standards-track
- **Author**: T81 Foundation
- **Created**: 2026-03-20
- **Depends-on**:
  - RFC-00C0: Deterministic Compliance & Governed Execution Model
  - RFC-00C1: Compliance Trace Interchange Format (CTIF)
- **Relates-to**:

---

## 1. Abstract

This RFC defines `t81-verify`, a standard, non-privileged command-line interface (CLI) for the third-party verification of Compliance Traces. This tool serves as the reference implementation of the External Verification Interface defined in RFC-00C0. It consumes a Compliance Trace Interchange Format (CTIF) object, retrieves the necessary artifacts from a CanonFS instance, and performs deterministic replay in a local sandbox to verify the trace's authenticity and correctness. The tool's primary output is a simple, binary status: `VERIFIED` or `FAILED`.

---

## 2. Motivation

For the Governed Execution Model to be a viable foundation for regulatory and enterprise compliance, the verification process must be transparent, accessible, and standardized. A theoretical verification model is insufficient; a practical, easy-to-use tool is required to build trust and enable widespread adoption.

The `t81-verify` CLI is motivated by the need to:
-   **Provide a Reference Implementation**: Offer a canonical, open-source tool that demonstrates the verification process described in RFC-00C0.
-   **Empower Third Parties**: Enable regulators, customers, and other external parties to independently verify compliance proofs without needing a full T81 development environment or privileged access to the originating system.
-   **Automate Auditing**: Provide a scriptable, machine-readable interface that can be integrated into automated auditing and continuous compliance workflows.
-   **Decouple Verification from Execution**: Reinforce the architectural separation between a system that *produces* a proof (the T81 runtime) and a system that *validates* it (the verification CLI).

---

## 3. Command Specification

### 3.1. Command

The verification tool SHALL be named `t81-verify`.

### 3.2. Usage

```sh
t81-verify [OPTIONS] [TRACE_FILE]
```

### 3.3. Arguments

-   **`[TRACE_FILE]`** (Optional): Path to the file containing the Compliance Trace in either JSON or canonical binary format (per RFC-00C1). If this argument is omitted, the CLI MUST read from standard input (`stdin`).

### 3.4. Options

-   **`--canonfs-url <URL>`** (Required): The base URL of a trusted CanonFS instance where execution artifacts (program, inputs, policies) can be fetched via their CanonHashes.
-   **`--governor-pubkey <FILE>`** (Required): Path to a file containing the public key of the Axion Governor that signed the trace. This is required to verify the trace's signature.
-   **`-q`, `--quiet`**: If present, the CLI MUST suppress all informational output (e.g., progress steps) and only print the final, minimal result to `stdout`. Error messages on `stderr` are still permitted.
-   **`--output-format <FORMAT>`**: Specifies the output format for the final result.
    -   `text` (Default): Human-readable summary.
    -   `json`: Machine-readable JSON object.

---

## 4. Verification Process

The `t81-verify` CLI MUST follow these steps in strict order. Failure at any step immediately terminates the process and results in a `FAILED` status.

1.  **Parse Input**: Read and parse the CTIF object from `[TRACE_FILE]` or `stdin`. The tool must automatically detect whether the format is JSON or binary.
2.  **Verify Signature**: Using the key from `--governor-pubkey`, cryptographically verify the `signature` within the CTIF object. The signature must match the hash of the trace's canonical binary representation.
3.  **Fetch Artifacts**: Connect to the `--canonfs-url` and retrieve the execution artifacts identified by the hashes in the trace:
    -   The program to be executed (its hash must be located within the trace metadata).
    -   The input data (`input_hash`).
    -   The full Policy Set (`policy_set_hash`).
4.  **Deterministic Replay**: Instantiate a local, non-privileged T81VM sandbox. Execute the program with the fetched inputs and Policy Set under Governed Execution mode.
5.  **Generate New Trace**: During the replay, generate a new Compliance Trace.
6.  **Compare Traces**: Compute the CanonHash of the newly generated trace's canonical binary representation. Compare this hash with the CanonHash of the original, input trace.
7.  **Report Result**: If the hashes match, the verification is successful.

---

## 5. Output Specification

### 5.1. Text Output (Default)

On success, the output to `stdout` MUST be:
```
VERIFIED
```

On failure, the output to `stderr` MUST be a descriptive error message, such as:
```
FAILED: Signature Invalid
```
or
```
FAILED: Replay trace hash mismatch
```

### 5.2. JSON Output (`--output-format json`)

On success, the JSON object written to `stdout` MUST conform to this structure:
```json
{
  "status": "VERIFIED",
  "original_trace_hash": "Z1A...",
  "replayed_trace_hash": "Z1A...",
  "timestamp": "2026-03-20T14:30:00Z"
}
```

On failure, the structure MUST include a `reason`:
```json
{
  "status": "FAILED",
  "reason": "Signature Invalid",
  "original_trace_hash": "Z1A...",
  "replayed_trace_hash": null,
  "timestamp": "2026-03-20T14:30:00Z"
}
```

---

## 6. Security Considerations

-   **Sandboxed Execution**: The deterministic replay environment MUST be strictly sandboxed. The replayed program MUST have no access to the local filesystem, network, or any other host resources beyond what is explicitly provided by the deterministic T81VM environment.
-   **Resource Limiting**: The replay environment should be resource-limited to prevent denial-of-service attacks via resource-intensive traces.
-   **Artifact Trust**: The verification process inherently trusts the artifacts fetched from the specified `--canonfs-url`. The integrity of the CanonFS instance is outside the scope of this tool's verification, but users should be aware that they are trusting the specified server.
-   **Key Management**: Users of the tool are responsible for securely managing the governor public keys used for verification.

---

## 7. Conformance Requirements

A CLI tool claiming conformance with this RFC MUST:
1.  Be named `t81-verify`.
2.  Implement all arguments and options as specified in Section 3.
3.  Follow the verification process in Section 4 in the exact order.
4.  Produce output matching the specification in Section 5.
5.  Perform replay in a strictly sandboxed, non-privileged environment.
6.  Successfully parse both JSON and canonical binary CTIF objects per RFC-00C1.