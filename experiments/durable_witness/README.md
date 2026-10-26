# Durable Witness Accountable Computation Experiment

This isolated experiment demonstrates a minimal durable authorization-and-effect witness that closes replay and fabrication security gaps across runtime arms (T81VM and Wasmtime) without requiring custom TISC VM instructions.

## Scope & Bounded Corrective Implementation

The bounded corrections in `witness.py` and `verify.py` ensure:
1. Stored authority policy limits are strictly enforced during transaction execution. Caller-provided limits can further restrict policy bounds but cannot relax stored DB authority rules.
2. Malformed or invalid transaction inputs (negative prices/quantities, floats, booleans) are rejected cleanly with `INVALID_TRANSACTION_INPUT`.
3. Database paths explicitly supplied to `verify_evidence` that do not exist or cannot be inspected trigger verification failure with `MISSING_WITNESS_DATABASE`.
4. Verification status flags explicitly separate cryptographic authenticity (`crypto_authentic`), policy authorization (`authorization_valid`), witness store commitment (`witness_committed`), and external physical occurrence (`effect_observed`).
5. Detached/offline signature verification returns `DETACHED_OFFLINE_SIGNATURE_ONLY` (`valid: False`, `witness_committed: False`) to prevent offline signature validity from being promoted into a witness-backed success claim.

## Directory Structure

- `THREAT_MODEL.md`: System model and 10-attack threat matrix
- `EXPERIMENT_PLAN.md`: Falsifiable hypothesis and acceptance criteria
- `RESULTS.md`: Empirical benchmark findings and claim classification table
- `ARCHITECTURE_DECISION.md`: Final architectural decision (Decision D)
- `witness.py`: SQLite WAL transactional witness with HMAC signatures
- `runner.py`: Workload execution runner for Arm A and Arm B
- `verify.py`: Independent evidence verification script
- `attacks.py`: 10-scenario adversarial attack suite
- `run.py`: Performance benchmark harness
- `test_durable_witness.py`: Python unit test suite

## Quickstart & Verification

```sh
# Run unit tests
python3 experiments/durable_witness/test_durable_witness.py

# Run the 10-scenario adversarial attack suite
PYTHONPATH=experiments/durable_witness python3 experiments/durable_witness/attacks.py

# Run performance measurements
PYTHONPATH=experiments/durable_witness python3 experiments/durable_witness/run.py

# Verify a single evidence JSON file against witness DB
python3 experiments/durable_witness/verify.py \
  /tmp/durable-witness-perf/evidence_tx-A-0.json \
  --db /tmp/durable-witness-perf/witness_perf.db
```
