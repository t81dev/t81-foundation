# Architecture Decision Record — Accountable Computation Runtime Strategy (Audited)

**Directory:** `experiments/durable_witness/`
**Status:** Decided (Revised after PR #558 Audit)
**Decision:** **Conditional Decision D — Bounded Host Witness Capability with Explicit Security Boundaries**

---

## Context & Problem Statement

Prior research in T81 Foundation explored whether a custom execution architecture (TISC ISA, Axion policy engine, CanonFS filesystem, ternary-inspired VM) was required to achieve accountable computation—establishing that a protected side effect occurred under valid authority and cannot be falsely replayed.

PR #554 and PR #558 evaluated a durable transactional witness (`witness.py`) coupled with HMAC receipts to prevent replay attacks and fabricated evidence across both T81VM (Arm A) and Wasmtime (Arm B).

A security correctness audit of PR #558 challenged the previously claimed "10/10 attack blocking" and absolute Decision D, revealing critical boundary assumptions, implementation shortcuts, and security limitations.

---

## Evaluated Options

* **Option A — Validated improvement in T81VM**: Custom VM instructions provide unique accountable evidence guarantees. (Falsified: Security properties depend on the external witness boundary, not VM opcodes).
* **Option B — Unconditional Runtime Equivalence**: Conventional runtimes with a local transactional witness provide complete, unassisted accountability for all side effects. (Refuted by audit: Local SQLite witness cannot resist storage rollback or verify unobserved external effects).
* **Conditional Decision D — Bounded Host Witness Capability**: A host capability with a transactional witness is sufficient for *in-process/local transaction admission*, but requires external monotonic checkpoints and independent observation proofs to guarantee rollback resistance and physical effect attestation.

---

## Audit Findings & Corrected Evidence

1. **HMAC Coverage Defect Fixed**: Original receipts signed only `tx_id:authority_version:effect_id`, leaving `payload_hash`, `total_amount`, `timestamp`, and `witness_id` vulnerable to substitution. Coverage has been expanded to all receipt fields.
2. **Rollback Vulnerability Demonstrated**: Restoring an older SQLite database snapshot wipes committed transaction history. An attacker can re-submit a previously executed transaction ID and obtain a second valid receipt. Local database state **cannot resist rollback** without an external monotonic checkpoint or append-only transparency log (Attack 8).
3. **Offline Verification Boundary**: Verifying evidence offline without querying the witness store (`--db`) checks only cryptographic key knowledge, NOT transaction admission. An actor knowing the signing secret can forge valid receipts for unadmitted transactions.
4. **Database Record vs. Physical Effect**: A witness database row proves an internal state transition occurred inside SQLite. It does **not** prove an external physical effect (e.g. shipping disbursement) occurred without a separate sensor or external audit attestation.
5. **Runtime Dependency Enforcement**: Arm A silent Python arithmetic fallback was removed. Unavailable native runtimes now explicitly fail or report `SKIPPED`.

---

## Decision Outcome

**Decision: Conditional Decision D — Bounded Host Witness Capability**

### Rationale:
1. **Host Boundary Sufficiency**:
   For local, linearized transaction admission and replay protection within an active witness session, a host capability (Wasmtime + host import) is functionally equivalent to T81VM. Neither runtime supplies intrinsic VM-level rollback or witness protection.
2. **Narrowed Security Guarantees**:
   The host witness mechanism guarantees:
   - **Verified Cryptographic Integrity**: Receipts authenticate signed fields when verified against secret material.
   - **Durable Uniqueness (Honest Store)**: Duplicate transaction IDs are rejected as long as witness storage is continuous and un-rolled-back.
   - **Boundary Revocation**: Revoked authority policies halt transaction admission at the host interface.
3. **Explicit Non-Guarantees (Requires External Anchors)**:
   - **Rollback Resistance**: Requires external monotonic counters or distributed consensus.
   - **Offline Receipt Authenticity**: Requires querying the live witness database or public key infrastructure (PKI).
   - **Physical Effect Occurrence**: Requires independent external sensor / payment system attestation.

---

## Consequences & Recommendations

* **Retain Decoupled Host Capability Model**: Do not add custom TISC ISA instructions for accountable computation.
* **Document Threat Model Limits**: Clearly distinguish between internal SQLite state commitment and external world effect verification.
* **Future Enhancement Requirement**: Any deployment requiring rollback resistance must integrate an external monotonic sequence anchor (e.g. Sigstore rekor log or hardware TPM counter).
