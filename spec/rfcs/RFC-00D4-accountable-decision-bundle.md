# RFC-00D4: Accountable Decision Bundle Schema and Detached Evidence Verification Protocol

* **RFC Number:** RFC-00D4
* **Title:** Accountable Decision Bundle Schema and Detached Evidence Verification Protocol
* **Author:** Research Systems Architecture Group
* **Status:** Draft
* **Created:** April 2026
* **Category:** Standards Track / Evidence Protocol

---

## 1. Abstract

This specification defines the **Accountable Decision Bundle (ADB)** schema—a runtime-agnostic, self-contained JSON data structure that encapsulates an Accountable Computational Act. The schema explicitly models the six primitives of accountable computation: **Meaning, Evidence, Method, Authority, Accountability,** and **Challengeability**.

By decoupling evidence generation from runtime execution, an ADB MAY be produced by any execution host (including T81VM, WebAssembly/Wasmtime, eBPF, Python, or standard OCI containers) and MUST be independently verifiable by an offline verifier using only the bundle itself and a designated trust policy.

---

## 2. Terminology and Conformance

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this document are to be interpreted as described in RFC 2119.

### Non-Goals
This specification explicitly does NOT:
* Define a new Virtual Machine, Instruction Set Architecture (ISA), or hardware target.
* Require balanced ternary representation, SWAR emulation, or TISC opcodes.
* Define an in-process capability delegation or memory isolation mechanism.

---

## 3. Specification Requirements

### 3.1 Primitive Mapping

An Accountable Decision Bundle MUST contain top-level fields representing all six core primitives:

1. **Meaning (`meaning`):** MUST specify the intended decision, state transition, or proposed side effect (e.g., purchase order issuance, access grant, classification outcome).
2. **Evidence (`evidence`):** MUST contain cryptographic hashes, execution traces, or signed attestations proving what computational operations occurred.
3. **Method (`method`):** MUST record the execution environment, runtime type, transformation rules, model weights hash, or algorithm version used.
4. **Authority (`authority`):** MUST record the policy rule, capability token, or authorization decision that permitted the act prior to side-effect execution.
5. **Accountability (`accountability`):** MUST specify the responsible legal or operational entity, public key, or cryptographic identity binding the output.
6. **Challengeability (`challengeability`):** MUST provide replay instructions, input digests, or audit endpoints allowing an independent verifier to test or falsify the outcome.

### 3.2 Compatibility with Existing T81 Contracts

An ADB is designed for 100% interoperability with existing CanonFS import/export schemas (`RFC-00D1`), CanonHash-81 content addressing, and the Bounded AI OS-Object bundle consumption contract (`docs/reference/AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md`).

An ADB MUST be serializable to canonical JSON and MAY be imported or exported via CanonFS commands (`t81 canonfs import`, `t81 canonfs export`).

---

## 4. Formal JSON Schema

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://t81foundation.org/schemas/v1/accountable-decision-bundle.json",
  "title": "AccountableDecisionBundle",
  "description": "Runtime-agnostic schema representing an Accountable Computational Act across the six core primitives.",
  "type": "object",
  "required": [
    "schema_version",
    "bundle_id",
    "timestamp_utc",
    "meaning",
    "evidence",
    "method",
    "authority",
    "accountability",
    "challengeability"
  ],
  "additionalProperties": false,
  "properties": {
    "schema_version": {
      "type": "string",
      "enum": ["1.0.0-draft"]
    },
    "bundle_id": {
      "type": "string",
      "description": "Unique content-addressed identifier or UUID for the bundle (e.g., CanonHash-81 or SHA256)."
    },
    "timestamp_utc": {
      "type": "string",
      "format": "date-time"
    },
    "meaning": {
      "type": "object",
      "description": "Primitive 1: Meaning - Intended state transition or action.",
      "required": ["action_type", "parameters", "proposed_side_effects"],
      "properties": {
        "action_type": { "type": "string" },
        "parameters": { "type": "object" },
        "proposed_side_effects": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["target", "mutation"],
            "properties": {
              "target": { "type": "string" },
              "mutation": { "type": "string" }
            }
          }
        }
      }
    },
    "evidence": {
      "type": "object",
      "description": "Primitive 2: Evidence - Verifiable execution artifacts.",
      "required": ["input_digest", "output_digest", "execution_trace_hash"],
      "properties": {
        "input_digest": { "type": "string" },
        "output_digest": { "type": "string" },
        "execution_trace_hash": { "type": "string" },
        "intermediate_state_hashes": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    },
    "method": {
      "type": "object",
      "description": "Primitive 3: Method - Runtime, model, and transformation specification.",
      "required": ["runtime_environment", "transformation_id"],
      "properties": {
        "runtime_environment": {
          "type": "string",
          "description": "Runtime substrate identifier (e.g., 'wasmtime-v18.0', 't81vm-v1.9.0', 'python-v3.12')."
        },
        "transformation_id": { "type": "string" },
        "model_weights_digest": { "type": "string" },
        "hyperparameters": { "type": "object" }
      }
    },
    "authority": {
      "type": "object",
      "description": "Primitive 4: Authority - Pre-side-effect policy or capability attestation.",
      "required": ["policy_id", "verdict", "evaluation_timestamp"],
      "properties": {
        "policy_id": { "type": "string" },
        "policy_hash": { "type": "string" },
        "verdict": {
          "type": "string",
          "enum": ["ALLOW", "DENY", "WARN"]
        },
        "evaluation_timestamp": { "type": "string", "format": "date-time" },
        "matched_rules": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    },
    "accountability": {
      "type": "object",
      "description": "Primitive 5: Accountability - Cryptographic identity and operational responsibility.",
      "required": ["principal_id", "signature", "public_key_uri"],
      "properties": {
        "principal_id": { "type": "string" },
        "public_key_uri": { "type": "string" },
        "signature": {
          "type": "string",
          "description": "Base64 or hex-encoded cryptographic signature covering meaning + evidence + authority."
        },
        "signing_scheme": { "type": "string" }
      }
    },
    "challengeability": {
      "type": "object",
      "description": "Primitive 6: Challengeability - Replay and audit specification.",
      "required": ["replayable", "verification_procedure"],
      "properties": {
        "replayable": { "type": "boolean" },
        "verification_procedure": { "type": "string" },
        "audit_package_uri": { "type": "string" },
        "dispute_deadline": { "type": "string", "format": "date-time" }
      }
    }
  }
}
```

---

## 5. Detached Verification Protocol

An offline verifier MUST be able to execute verification without network access or connection to the producer runtime using the following sequence:

1. **Schema Validation:** The verifier MUST validate the JSON bundle structure against the RFC-00D4 schema.
2. **Authority Check:** The verifier MUST confirm that `authority.verdict == "ALLOW"` and verify that `authority.policy_hash` matches the authorized policy for `meaning.action_type`.
3. **Cryptographic Integrity:** The verifier MUST recompute the digest over `meaning`, `evidence`, and `authority`, verifying `accountability.signature` using the public key resolved from `accountability.public_key_uri`.
4. **Evidence Replay (If Replayable):** If `challengeability.replayable == true`, the verifier MAY execute the method transformation on `evidence.input_digest` to verify that output matches `evidence.output_digest`.

---

## 6. Security Considerations

* **Fail-Closed Verification:** If any signature verification or policy hash match fails, the verifier MUST reject the bundle as untrusted.
* **Malleability Protection:** Signatures MUST cover canonical representations (such as RFC 8785 JSON Canonicalization Scheme) to prevent field reordering attacks.
