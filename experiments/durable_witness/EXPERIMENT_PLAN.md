# Experiment Plan — Durable Witness Accountability Proof-of-Concept

**Directory:** `experiments/durable_witness/`
**Reference:** Independent Architecture Challenge & Durable Witness Hypothesis

---

## 1. Selected Falsifiable Hypothesis

> **Hypothesis:** A trusted, durable authorization-and-effect witness can reject fabricated execution histories and previously accepted replay attacks across process restarts, without depending on T81VM-specific instruction semantics.

---

## 2. Acceptance Criteria

1. **Replay Protection**: The witness MUST reject duplicate transaction IDs submitted across process restarts or concurrent threads with error code `REPLAY_DETECTED` / `INTEGRITY_ERROR`.
2. **Fabrication Detection**: The independent verifier MUST detect and reject producer-generated evidence where receipt signatures fail HMAC validation (`INVALID_RECEIPT_SIGNATURE`).
3. **Revocation Enforcement**: Policy revocations executed in the witness store MUST immediately block pending effect admissions (`REVOCATION_DETECTED`).
4. **Transaction Atomicity**: Effect commitment and transaction log insertion MUST execute in a single SQLite WAL transaction (no partial commits or phantom receipts).
5. **Runtime Parity**: Both Arm A (T81VM) and Arm B (Wasmtime host capability) MUST achieve identical security properties (100% attack blocking).

---

## 3. Experimental Architecture

```
experiments/durable_witness/
├── README.md                  # Quickstart guide
├── THREAT_MODEL.md            # System threat model and attack matrix
├── EXPERIMENT_PLAN.md         # This plan
├── RESULTS.md                 # Empirical findings and claim classification
├── ARCHITECTURE_DECISION.md   # Final architectural decision (Decision D)
├── workload-contract.json     # Business workload parameters (Purchase Order)
├── witness.py                 # SQLite WAL transactional witness service
├── guest.wat                  # Wasm guest module for Arm B
├── runner.py                  # Dual-arm workload runner (Arm A and Arm B)
├── verify.py                  # Detached independent evidence verifier
├── attacks.py                 # 10-scenario adversarial attack suite
├── run.py                     # Performance and repeatability harness
└── test_durable_witness.py    # Unit test suite
```

---

## 4. Method of Measurement & Audit

* Execution and verification timing measured in microseconds via Python `time.perf_counter_ns()`.
* Raw inputs, execution traces, verifier verdicts, and attack outputs preserved in `raw.jsonl` and `attacks.json`.
* Independent verifier `verify.py` checks both cryptographic signatures and direct witness SQLite store state without relying on producer trace assertions.
