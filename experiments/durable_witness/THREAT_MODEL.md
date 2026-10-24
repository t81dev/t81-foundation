# Threat Model — Accountable Computation & Durable Witness

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
3. **Independent Verifier**: Detached verification process that inspects producer evidence, verifies cryptographic signatures, and checks the trusted witness state.

---

## 2. Threat Actors & Capabilities

| Threat Actor | Description | Capabilities |
| :--- | :--- | :--- |
| **Malicious Producer** | Compromised application running inside T81VM or Wasmtime | Can manipulate internal memory, forge execution traces, generate fake hashes, submit duplicate requests, or submit requests under revoked authority versions. |
| **Replay Attacker** | Network/IPC observer or rogue client process | Can capture previously valid transactions/receipts and re-submit them across process restarts or concurrent sessions. |
| **Rogue Administrator** | Operator with host storage access | Can attempt to roll back witness storage to old snapshots or tamper with evidence files. |

---

## 3. Evaluated Attack Vectors (10 Scenarios)

1. **Fabricated Producer Evidence**: Producer constructs fake evidence JSON with valid internal hashes but forged receipt signature.
2. **Valid Receipt Replay Across Restarts**: Replaying a previously accepted transaction ID after client/witness process restart.
3. **Concurrent Duplicate Submission**: Submitting identical transaction IDs simultaneously from parallel threads/processes.
4. **Boundary Authorization Revocation**: Revoking policy version immediately before effect admission.
5. **Interrupted Receipt Publication**: Witness commits effect to database, but network/process dies before returning receipt to producer.
6. **Phantom Receipt Generation**: Constructing evidence claiming a receipt without writing to witness store.
7. **Substituted Witness Identity**: Forging evidence with invalid witness secret keys or fake witness identifiers.
8. **Witness Storage Rollback**: Restoring witness SQLite store from a snapshot taken prior to transaction execution.
9. **Conflicting Authority Versions**: Submitting transactions under an obsolete or revoked policy version (`v1`).
10. **Partial Persistence Failure**: Simulated database write failure / disk read-only condition mid-transaction.

---

## 4. Trust Assumptions & Non-Goals

### Trust Assumptions
* The trusted witness holds a secure cryptographic key (`HMAC-SHA256`) in a distinct security domain.
* The SQLite storage layer in the witness uses write-ahead logging (`PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL`) and provides atomic multi-table writes.

### Non-Goals
* Physical hardware tampering / side-channel attacks on host CPUs.
* Physical external world side-effects outside the computer system (e.g. physical shipping container movements) unless bound to the witness API.
