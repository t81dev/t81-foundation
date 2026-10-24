# Results & Findings — Accountable Computation & Durable Witness

**Directory:** `experiments/durable_witness/`
**Git SHAs:**
- `main`: `0005ab675d2729b2795200e8c78ded43d4520b0d`
- PR #554: `7af837ece1325752e555b814c94aacc33afd2c96`

---

## 1. Summary of Empirical Findings

1. **Reproduction of PR #554 Results**:
   - Both Arm A (T81VM) and Arm B (Wasmtime) enforce identical purchase order policy limits and issue identical protocol envelopes.
   - Without a durable witness, both arms shared identical security gaps: accepted fabricated producer evidence with valid internal hashes and allowed receipt replay across process restarts.
   - Arm A exhibited 100 out of 110 failures on Windows native runs due to CanonFS Base81 filename reserved-character restrictions (`<`/`>`), whereas Arm B had 0 failures.

2. **Durable Witness Validation**:
   - Incorporating a transactional SQLite WAL witness coupled with HMAC receipt signing successfully blocked **10 out of 10 adversarial attacks** across both runtime arms.
   - Replay attacks across process restarts and concurrent duplicate submissions were reliably caught and rejected.
   - Revocation at the witness boundary immediately halted effect admissions.

---

## 2. Adversarial Attack Suite Results (10/10 Blocked)

| Attack ID | Scenario Name | Verdict | Observed Result |
| :---: | :--- | :---: | :--- |
| **1** | Fabricated evidence with valid internal hashes | **BLOCKED** | Rejected with `INVALID_RECEIPT_SIGNATURE`. |
| **2** | Valid receipt replay after process restart | **BLOCKED** | Rejected with `REPLAY_DETECTED`. |
| **3** | Duplicate submission from concurrent threads | **BLOCKED** | Atomic SQLite constraint blocked duplicate thread with `INTEGRITY_ERROR`. |
| **4** | Authorization revocation before effect admission | **BLOCKED** | Rejected with `REVOCATION_DETECTED`. |
| **5** | Effect committed but receipt publication interrupted | **HANDLED** | Recovered existing receipt ID on retry without duplicate effect. |
| **6** | Receipt generated without claimed effect | **BLOCKED** | Verifier rejected with `TRANSACTION_NOT_IN_WITNESS_STORE`. |
| **7** | Tampered or substituted witness identities | **BLOCKED** | Rejected with `INVALID_RECEIPT_SIGNATURE`. |
| **8** | Witness storage rollback from old snapshot | **BLOCKED** | Verifier detected transaction absence in restored snapshot DB. |
| **9** | Conflicting / revoked authority versions (`v1`) | **BLOCKED** | Rejected with `REVOCATION_DETECTED`. |
| **10** | Partial persistence failure (read-only DB) | **BLOCKED** | Transaction rolled back cleanly with `INTEGRITY_ERROR`. |

---

## 3. Performance & Repeatability Benchmark (30 Trials / Arm)

| Metric | Arm A (T81VM) | Arm B (Wasmtime host capability) |
| :--- | :---: | :---: |
| **Execution & Witness Commit Mean** | 2,612.70 µs ± 1,248.31 µs | 4,352.91 µs ± 1,276.84 µs |
| **Verification Mean** | 453.94 µs | 441.87 µs |
| **Pass / Verification Rate** | 100% (30/30) | 100% (30/30) |

---

## 4. Formal Claim Classification Table

| Claim | Classification | Evidence & Justification |
| :--- | :--- | :--- |
| **Arm A and Arm B protocol parity** | **Independently Reproduced** | Confirmed PR #554 measurements across 660 trials and independent tar archive audits. |
| **Durable transactional witness closes replay/fabrication gaps** | **Implemented & Tested** | `witness.py` and `attacks.py` demonstrate 100% attack blocking across 10 scenarios. |
| **T81 custom VM instructions are required for accountable evidence** | **Experimentally Falsified** | Arm B (Wasmtime + host capability) achieves identical security guarantees as Arm A. |
| **CanonFS Base81 storage works portably on Windows** | **Experimentally Falsified** | Windows native runs suffered 100/110 storage failures due to reserved filename characters (`<`/`>`). |
| **Custom execution surface provides intrinsic security advantage** | **Unsupported** | Security properties depend entirely on the external transactional witness boundary. |
