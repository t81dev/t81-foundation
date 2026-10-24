# Threat Model — Accountable Computation & Durable Witness (Audited)

**Directory:** `experiments/durable_witness/`
**Scope:** Protected side-effect authorization, execution attestation, and receipt verification for autonomous runtimes (T81VM and Wasmtime).

---

## 1. System Model & Boundaries

The accountable computation model separates execution into three distinct entities:

```
+---------------------+        1. Submit PO        +-----------------------------+
|  Producer Runtime   | -------------------------> |  Trusted Durable Witness    |
| (T81VM / Wasmtime)  | <------------------------- |  (SQLite WAL + HMAC Key)    |
+---------------------+    2. Return Receipt       +-----------------------------+
                                                                  |
                                                           3. Inspect Store
                                                                  v
                                                   +-----------------------------+
                                                   |    Independent Verifier     |
                                                   +-----------------------------+
```

1. **Producer Runtime**: Untrusted or semi-trusted execution environment (e.g. T81VM or Wasmtime) executing business logic (purchase orders, inventory replenishment).
2. **Trusted Durable Witness**: Independently trusted, persistent transactional boundary. Controls the state of authority policies, transaction logs, effect commitments, and secret cryptographic keys (`HMAC-SHA256`).
3. **Independent Verifier**: Detached verification process that inspects producer evidence, verifies cryptographic signatures across all receipt fields, and queries the trusted witness state.

---

## 2. Threat Actors & Capabilities

| Threat Actor | Description | Capabilities |
| :--- | :--- | :--- |
| **Malicious Producer** | Compromised application running inside T81VM or Wasmtime | Can manipulate internal memory, forge execution traces, generate fake hashes, submit duplicate requests, or submit requests under revoked authority versions. |
| **Replay Attacker** | Network/IPC observer or rogue client process | Can capture previously valid transactions/receipts and re-submit them across process restarts or concurrent sessions. |
| **Rogue Storage Administrator** | Operator with host filesystem access | Can roll back witness storage to old snapshots or tamper with evidence files. |

---

## 3. Evaluated Attack Vectors & Audit Results (10 Scenarios)

1. **Fabricated Producer Evidence**: Producer constructs fake evidence JSON with valid internal hashes but forged receipt signature. (**BLOCKED**: Verifier checks full HMAC over all fields).
2. **Valid Receipt Replay Across Restarts**: Replaying a previously accepted transaction ID after client/witness process restart. (**BLOCKED**: DB enforces unique `tx_id` constraint).
3. **Concurrent Duplicate Submission**: Submitting identical transaction IDs simultaneously from parallel processes or threads. (**BLOCKED**: Multi-process test confirms atomic SQLite constraint blocks duplicate submission; revocation race immediately halts admission).
4. **Boundary Authorization Revocation**: Revoking policy version immediately before effect admission. (**BLOCKED**: DB checks authority status within transaction).
5. **Interrupted Receipt Publication**: Witness commits effect to database, but network/process dies before returning receipt to producer. (**HANDLED**: Idempotent re-submission retrieves existing receipt without duplicate effect creation).
6. **Phantom Receipt Generation**: Constructing evidence claiming a receipt without writing to witness store. (**BLOCKED when querying DB**: Verifier checks presence in witness DB. *Note: Offline verifier without DB access accepts forged receipt if producer knows secret key*).
7. **Substituted Witness Identity**: Forging evidence with invalid witness secret keys or fake witness identifiers. (**BLOCKED**: Signature verification fails).
8. **Witness Storage Rollback**: Restoring witness SQLite store from a snapshot taken prior to transaction execution. (**VULNERABLE WITHOUT EXTERNAL CHECKPOINT**: Restoring older DB snapshot allows re-submitting previously committed transaction IDs. Witness storage alone cannot detect rollback without an external monotonic sequence or transparency log).
9. **Conflicting Authority Versions**: Submitting transactions under an obsolete or revoked policy version (`v1`). (**BLOCKED**: Rejected by authority policy lookup).
10. **Partial Persistence Failure**: Simulated database write failure / disk read-only condition mid-transaction. (**BLOCKED**: SQLite transaction rolls back cleanly).

---

## 4. Security Guarantees & Boundary Non-Goals

### Demonstrated Security Guarantees
* **Full-Field Signature Verification**: Receipts cover `witness_id`, `tx_id`, `authority_version`, `effect_id`, `payload_hash`, `total_amount`, and `timestamp`. Modifying any field invalidates the signature.
* **In-Session Replay Protection**: Within a non-rolled-back witness store, transaction IDs are strictly one-shot.
* **Authority Boundary Enforcement**: Active policy versions gating transaction admission are evaluated atomically inside SQLite transactions.

### Non-Guarantees & Boundary Limits
* **Rollback Protection**: An isolated SQLite witness database cannot detect host snapshot rollback. External monotonic sequence anchors (e.g., Sigstore Rekor log, TPM counter, or distributed consensus) are required to achieve rollback resistance.
* **Offline Attestation**: Verifying evidence offline without `--db` only proves that the issuer possessed the secret key; it does not prove transaction commitment or non-revocation.
* **Physical Effect Occurrence**: A database row proves state commitment inside the software TCB, but does not authenticate physical external world execution (e.g. goods shipping) without an independent physical sensor attestation.
