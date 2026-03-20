
- **Title**: RFC-00C0: Deterministic Compliance & Governed Execution Model
- **Status**: proposed
- **Type**: standards-track
- **Author**: T81 Foundation
- **Created**: 2026-03-20
- **Depends-on**:
  - RFC-0000: T81 Base-81 Ternary Computing Stack
  - RFC-0001: Architecture Principles
  - RFC-0002: Deterministic Execution Contract
  - RFC-0003: Axion Safety Model
  - RFC-0009: Axion Policy Language
  - RFC-0022: Axion Policy Language (Update)
  - RFC-0025: Policy-Gated Tensor Loading
  - RFC-0026: AI-Native Inference Opcodes
  - RFC-0031: Deterministic AI Execution Contract
  - RFC-0054: CanonFS Object Identity and Persistence Contract
  - RFC-00A6: Axion Policy Hooks
- **Relates-to**:
  - RFC-0008: Formal Verification Harness
  - RFC-0036: T81Lang FFI Grammar
  - RFC-0053: Distributed Deterministic Execution Protocol
  - RFC-00A1: Deterministic Evidence Protocol
  - RFC-DPE-0007: Epoch Execution Timeout

---

## 1. Abstract

This RFC defines a **Governed Execution Model** for the T81 architecture, wherein regulatory and operational compliance is an intrinsic, non-bypassable property of computation. It introduces the concept of **Compliance-as-Code**, where policies are compiled into deterministic execution constraints enforced by the T81VM and Axion Governor. Under this model, non-compliant operations are unrepresentable at the instruction level or result in a deterministic halt. This specification establishes that for a T81 system, compliance is not a post-execution audit activity but a continuous, verifiable state enforced during execution itself. A canonical, cryptographically verifiable **Compliance Trace** is produced as definitive proof of a given execution's adherence to its governing policy set.

---

## 2. Motivation

Modern AI systems face significant challenges in providing robust guarantees of auditability, accountability, and safety. Post-hoc audits and logging mechanisms are inherently probabilistic, subject to tampering, and fail to prevent non-compliant actions from occurring in the first place. This gap is particularly acute in high-risk domains subject to regulatory oversight, such as child safety, content integrity, and national security.

To address this fundamental gap, a new model is required—one that shifts compliance from an external, after-the-fact process to an internal, preventative mechanism. The T81 architecture, with its root invariant of deterministic execution, provides the ideal foundation for such a model.

This RFC is motivated by the need to create a computing paradigm that enables compliance at the execution layer. It aims to provide a formal specification for a system where:
-   **Auditability** is achieved through deterministic, replayable execution traces.
-   **Accountability** is established via cryptographically verifiable links between inputs, policies, and outputs.
-   **Safety** is enforced by making prohibited states and operations computationally inaccessible.

By standardizing this Governed Execution Model, T81 can serve as a foundational technology for building verifiably compliant AI systems, enabling regulatory sandboxes and fostering national standardization efforts based on provable execution rather than probabilistic assessment.

---

## 3. Definitions

-   **Compliance-as-Code**: The principle that compliance policies are expressed in a machine-readable format, compiled into canonical execution constraints, and enforced deterministically by the runtime.
-   **Governed Execution**: A mode of T81VM operation where every instruction and data access is subject to a set of active, compiled compliance policies.
-   **Policy Object**: A canonical data structure, stored in CanonFS, that defines a set of compliance rules. Each Policy Object is identified by its CanonHash.
-   **Policy Set**: A collection of one or more Policy Objects, identified by a canonical hash of its constituent member hashes, that defines the complete compliance environment for a given execution.
-   **Compliance Domain**: A category of compliance constraints corresponding to a specific regulatory or operational area (e.g., Identity, Content, Safety).
-   **Policy-Gated Execution**: A mechanism where specific TISC instructions or runtime events are intercepted by the Axion Governor and require explicit policy approval to proceed.
-   **Compliance Trace**: A deterministic, canonical artifact emitted for every Governed Execution, composed of a deterministic payload and an optional attestation envelope.
-   **AXHALT_COMPLIANCE**: A specific, deterministic halt state triggered by a compliance violation that cannot be caught, suppressed, or altered.

---

## 4. Compliance Model Overview

The Governed Execution Model integrates compliance enforcement directly into the T81 execution lifecycle. The process is as follows:

1.  **Policy Definition**: Compliance policies are authored in the Axion Policy Language (RFC-0009, RFC-0022) and stored as immutable objects in CanonFS.
2.  **Execution Request**: A request to execute a program includes the program's CanonHash and the CanonHash of the Policy Set to be enforced.
3.  **Governor Initialization**: The Axion Governor loads the specified Policy Set from CanonFS, verifying the integrity of each Policy Object. It then compiles these rules into a set of deterministic constraints and enforcement hooks.
4.  **Governed Execution**: The T81VM executes the program in Governed Execution mode. The Axion Governor, operating at a higher privilege level, mediates critical operations.
5.  **Policy Enforcement**: At defined trigger points (e.g., specific TISC instructions, memory allocations, FFI calls), the VM yields to the Governor, which checks the operation against the compiled constraints.
6.  **Trace Emission**: Throughout execution, the VM and Governor collaboratively generate the Compliance Trace, recording every policy-relevant event.
7.  **Deterministic Outcome**: If no violations occur, the execution completes, and the final Compliance Trace is written to CanonFS as a verifiable receipt. If a violation is detected, the system immediately transitions to the `AXHALT_COMPLIANCE` state, and a partial trace indicating the point of failure is emitted.

This model ensures that compliance is not an optional layer but a fundamental aspect of the system's operation, inseparable from execution itself.

### 4.1. Policy Set Composability

Policy Sets MUST be closed under deterministic composition.

Given:
- Policy Set A
- Policy Set B

The composed Policy Set MUST:
- be order-independent
- produce a canonical combined hash
- enforce the strictest constraints of both

Conflicts between policies (e.g., one allows what another denies) MUST result in pre-execution rejection of the composed Policy Set.

### 4.2. Formal Compliance Invariant

For any governed execution E under Policy Set P:

E cannot produce an output O such that O violates P.

If such a state is reached, execution MUST halt deterministically.

Therefore, non-compliant outputs are unrepresentable within the system.

---

## 5. Execution Semantics

In Governed Execution mode, the semantics of the TISC instruction set and T81VM behavior are modified to be policy-aware.

-   **Instruction-Level Gating**: A subset of TISC instructions SHALL be designated as "governed." Execution of these instructions requires mediation by the Axion Governor. An attempt to execute a governed instruction without approval SHALL result in `AXHALT_COMPLIANCE`.
-   **Data-Level Gating**: Access to memory or CanonFS objects can be gated by policy. Policies can restrict read/write access to tensors or other data structures based on their metadata, provenance, or content hash.
-   **Canonical Constraints**: Policies are compiled into simple, deterministic checks (e.g., hash comparisons, recursion depth counters, instruction counters) that have a fixed, minimal performance overhead. There SHALL be no "best effort" or advisory semantics; a check either passes or triggers a halt.

### 5.1. Policy Determinism Guarantee

All Policy Objects MUST be deterministic functions.

A policy evaluation given identical inputs MUST produce identical outcomes.

Policies that depend on:
- time
- randomness
- external state
- non-canonical data sources

are INVALID and MUST be rejected at load time by the Axion Governor.

---

## 6. Policy Enforcement Mechanisms

Enforcement is a distributed responsibility across the T81 stack, orchestrated by the Axion Governor.

-   **T81VM**: The VM is responsible for identifying trigger points for governed operations and yielding control to Axion. It also provides the immediate execution context (e.g., current instruction, operand hashes) required for policy evaluation.
-   **Axion Governor**: As the root of trust, Axion performs the policy checks. It maintains the state required for complex policies (e.g., stateful recursion tracking) and makes the final determination to permit an operation or trigger `AXHALT_COMPLIANCE`.
-   **CanonFS**: The content-addressed filesystem (RFC-0054) is the foundation of policy enforcement for data. It guarantees the immutability and integrity of policies, models, and data. Policy-gated execution relies on CanonFS to provide verifiable object identity.
    -   `WLOAD` (Weight Load): This instruction SHALL be governed. Axion MUST verify that the CanonHash of the tensor being loaded is present in an allow-list within the active Policy Set.
    -   `ATTN` / `QMATMUL` (AI Opcodes): These instructions SHALL be governed. Axion MUST verify that the input tensors possess policy-approved provenance metadata, as defined in RFC-0025.
    -   `FFI_CALL` (Foreign Function Interface): This instruction SHALL be governed. The call requires a capability token (a temporary, single-use CanonFS object) granted by a policy, per RFC-0036. The token is consumed upon use.

---

## 7. Compliance Domains

Policies are organized into machine-enforceable domains. An implementation of this specification MUST support the following domains:

-   **Identity**: Enforces provenance and integrity for all executable code and data.
    -   *Mechanism*: Mandatory CanonHash verification for all loaded models, libraries, and initial inputs against a policy-defined manifest.
-   **Content**: Enforces restrictions on data produced by an execution.
    -   *Mechanism*: Content policies SHALL only operate over canonicalized output forms, deterministic classifiers with frozen model hashes (themselves executed as governed T81 programs), and explicit deny/allow Policy Objects. Open-ended semantic interpretation by non-frozen or externally dependent classifiers is INVALID. Enforcement MUST occur prior to any data externalization.
-   **Safety**: Enforces constraints on computational complexity and resource usage to prevent unsafe or unbounded behavior.
    -   *Mechanism*: Hard limits on recursion depth, total instruction count, memory allocation, and execution time (per RFC-DPE-0007), specified directly in the Policy Set.
-   **Privacy**: Enforces rules governing access to sensitive data.
    -   *Mechanism*: Data objects are tagged with a "privacy" classification. Policies grant ephemeral, capability-based access to these objects. Any attempt to access a tagged object without a valid capability token SHALL result in `AXHALT_COMPLIANCE`.
-   **Auditability**: Enforces the mandatory, non-bypassable generation of execution evidence.
    -   *Mechanism*: The generation of the Compliance Trace is not optional. If the trace mechanism fails or is tampered with, the system SHALL halt.

---

## 8. Compliance Trace Specification

The Compliance Trace SHALL consist of two components: a deterministic payload and an optional attestation envelope.

### 8.1. Deterministic Payload

The payload contains the verifiable evidence of the execution. Its structure is canonical and subject to deterministic replay equivalence. The payload contains the following fields:

-   **`trace_version`**: Version of the trace specification.
-   **`input_hash`**: The CanonHash of the program's initial input data.
-   **`policy_set_hash`**: The CanonHash of the Policy Set used for the execution.
-   **`execution_segments`**: A canonical ordered sequence of segment records. Segment omission, aggregation, or reordering is INVALID. Each segment record MUST contain:
    - `segment_index` (uint64)
    - `segment_type` (Enum)
    - `execution_locus` (e.g., instruction pointer)
    - `policy_checkpoint_id` (uint32)
    - `decision` (Enum: `ALLOW`, `DENY`, `HALT`)
    - `relevant_object_hashes` (Array of CanonHashes)
    - `segment_hash` (CanonHash)
-   **`output_hash`**: The CanonHash of the final output data. If the program has no externalized output, this field SHALL be a canonical null value.
-   **`verification_status`**: An enum indicating the outcome: `COMPLETED_PASS` or `HALTED_COMPLIANCE`.

### 8.2. Attestation Envelope

The attestation envelope contains metadata about the trace's generation, including a cryptographic signature. This envelope is NOT part of the deterministic replay equivalence check.

-   **`signature`**: A cryptographic signature over the CanonHash of the deterministic payload.
-   **`governor_id`**: An identifier for the Axion Governor instance that generated the trace.
-   **`timestamp`**: The time at which the trace was finalized.

Deterministic replay equivalence SHALL be evaluated over the payload only. If present, the attestation envelope MUST NOT alter the payload's hash identity.

---

## 9. Deterministic Replay & Verification

The Compliance Trace provides the foundation for irrefutable proof of compliance.

> If the replay of a Governed Execution—using the artifacts referenced in a Compliance Trace's **payload**—produces a bit-for-bit identical payload, then compliance is definitively and deterministically proven.

Verification is a non-probabilistic, binary outcome. The process involves:
1.  Fetching the program, input, and Policy Set from CanonFS using the hashes in the trace payload.
2.  Re-executing the program in Governed Execution mode.
3.  Comparing the CanonHash of the newly generated payload with the original. An exact match constitutes proof.

---

## 10. Failure Semantics

This model permits no "soft failures" or warnings related to compliance. Any deviation from the active Policy Set is a critical, unrecoverable fault.

### 10.1. Pre-execution Rejection

An execution request MUST be rejected before execution if any of the following conditions are met. Each rejection class MUST be programmatically distinct.
-   **`INVALID_POLICY_OBJECT`**: A referenced Policy Object is malformed or fails schema validation.
-   **`NONDETERMINISTIC_POLICY`**: A Policy Object is found to be non-deterministic, per Section 5.1.
-   **`POLICY_COMPOSITION_CONFLICT`**: The specified Policy Set contains conflicting rules that cannot be resolved to a single strictest constraint.
-   **`UNRESOLVED_POLICY_DEPENDENCY`**: A policy references a resource (e.g., a classifier model) that cannot be found in CanonFS.
-   **`POLICY_SURFACE_MISMATCH`**: The Policy Set attempts to govern a part of the Compliance Surface not supported by the current T81VM instance.

### 10.2. Deterministic Halt (`AXHALT_COMPLIANCE`)

If a policy violation occurs during runtime, the T81VM MUST immediately and irrevocably transition to the `AXHALT_COMPLIANCE` state. All in-flight work is discarded, and a partial Compliance Trace indicating the exact point and reason for failure is finalized.

---

## 11. Security Considerations

The security of the Governed Execution Model relies on the integrity of the underlying T81 architecture.

-   **Policy Integrity**: Malicious or malformed policies are a potential attack vector. This is mitigated by requiring all Policy Objects to be stored in and verified by CanonFS. The requirement for deterministic policies (Section 5.1) and safe composition (Section 4.1) further hardens the system against policy-based attacks.
-   **Governor Security**: The Axion Governor is the root of trust. Its implementation MUST be formally verified (per RFC-0008) to be correct and non-bypassable.
-   **Side Channels**: While execution is deterministic, timing variations could theoretically create side channels. The use of fixed-cost checks for policy enforcement and the deterministic nature of the memory model (RFC-0045) and scheduler (RFC-0046) are designed to mitigate these risks.
-   **Trace Integrity**: The separation of the payload and attestation envelope is critical. Verification tools MUST validate the signature in the envelope against the payload hash before trusting the trace's contents.

---

## 12. Conformance Requirements

A T81 system claiming conformance with this RFC MUST:

1.  Implement a Governed Execution mode in the T81VM that is distinct from standard execution.
2.  Ensure that the Axion Governor is non-bypassable in this mode.
3.  Support the compilation of Axion Policy Language into deterministic, canonical constraints.
4.  Implement all pre-execution rejection classes defined in Section 10.1.
5.  Implement deterministic, order-independent composition of Policy Sets.
6.  Implement Policy-Gated Execution for all elements defined in the Compliance Surface (Section 13).
7.  Implement all specified Compliance Domains (Identity, Content, Safety, Privacy, Auditability) with deterministic enforcement.
8.  Generate a Compliance Trace for every Governed Execution, matching the payload and envelope structure in Section 8.
9.  Implement `AXHALT_COMPLIANCE` as a distinct, irrevocable halt state.
10. Expose an External Verification Interface for third-party replay and verification (Section 14).

Future companion RFCs MAY define conformance profiles that subset this specification for specific deployment classes, provided they do not weaken the Formal Compliance Invariant within the declared profile boundary.

---

## 13. Compliance Surface Definition

The Compliance Surface is the complete set of execution elements subject to policy enforcement. A conformant implementation MUST ensure that all interactions across these surfaces are mediated by the Axion Governor under Governed Execution. The surface includes:

-   **Instruction Execution**: All TISC instructions, with special governance for privileged or high-risk opcodes.
-   **Memory Access**: All heap, stack, and tensor memory allocations, reads, and writes.
-   **CanonFS Object Resolution**: All interactions with CanonFS, including reading and writing objects.
-   **External Interaction**: All Foreign Function Interface (FFI) calls and any other form of system I/O.
-   **AI-Native Operations**: All AI-specific opcodes (e.g., `ATTN`, `QMATMUL`, `WLOAD`) defined in RFC-0026.
-   **Scheduler & Parallel Execution**: All task creation, scheduling, and synchronization primitives related to Deterministic Parallel Execution (DPE).

An implementation MAY use cached or lowered enforcement paths only if they are provably equivalent to direct Axion mediation and preserve identical trace output.

---

## 14. External Verification Interface

A conformant system MUST expose a standard, non-privileged interface for third-party verification, enabling portable compliance proof.

**Interface Definition:** `verify_trace(trace_object)`

**Inputs:**
-   A `Compliance Trace` object, as defined in Section 8.

**Output:**
-   An enum: `VERIFIED` or `FAILED`.

This interface MUST:
-   Perform deterministic replay of the execution as described in Section 9.
-   Require no privileged system access beyond read access to a CanonFS instance.
-   Be suitable for execution on independent, trusted third-party infrastructure.

The `FAILED` status MUST be returned for:
-   Trace payload mismatch during replay.
-   Unresolved referenced artifacts (e.g., hashes not found in CanonFS).
-   Invalid trace structure.
-   Invalid signature or attestation, when attestation is required by policy.

---

## 15. Distributed Compliance Semantics

In a distributed system operating under RFC-0053, the Governed Execution Model extends across all participating nodes.

-   All nodes in a distributed computation MUST load and enforce the identical Policy Set, verified by a common `policy_set_hash`.
-   All partial Compliance Traces generated by individual nodes MUST be deterministic and composable into a single, canonical global trace payload. The composition mechanism MUST be order-independent.
-   A compliance violation on any single node MUST trigger a deterministic halt (`AXHALT_COMPLIANCE`) that propagates to all other nodes in the computation, ensuring the entire distributed system halts in a consistent state.
