# Durable Witness Accountable Computation Experiment

This isolated experiment demonstrates a minimal durable authorization-and-effect witness that closes replay and fabrication security gaps across runtime arms (T81VM and Wasmtime) without requiring custom TISC VM instructions.

## Scope & Bounded Corrective Implementation

The bounded corrections in `runner.py`, `witness.py`, and `verify.py` ensure:
1. **Execution Provenance**: Native Arm A execution must explicitly fail (`GUEST_EXECUTION_FAILED`, `receipt: None`) when the Arm A binary is missing, inaccessible, or fails. Missing or failing native execution cannot generate a witness receipt or committed transaction.
2. **Explicit Simulation Isolation**: If simulation is explicitly enabled (`allow_simulation=True` or `arm="A_SIMULATED"`), the output evidence is explicitly tagged with `"simulated": True` to prevent simulation results from being presented as native execution or cross-platform parity evidence.
3. **Stored Authority Policy Limits**: Stored authority policy limits are strictly enforced during transaction execution. Caller-provided limits can further restrict policy bounds but cannot relax stored DB authority rules.
4. **Input Validation**: Malformed or invalid transaction inputs (negative prices/quantities, floats, booleans) are rejected cleanly with `INVALID_TRANSACTION_INPUT`.
5. **Database Inspection Safeguard**: Database paths explicitly supplied to `verify_evidence` that do not exist or cannot be inspected trigger verification failure with `MISSING_WITNESS_DATABASE`.
6. **Verification Claim Boundaries**: Verification status flags explicitly disaggregate cryptographic authenticity (`crypto_authentic`), historical policy authorization (`authorization_valid`), current authority status (`current_authority_active`), witness store commitment (`witness_committed`), and external physical occurrence (`effect_observed`).
7. **Historical Authorization Inference**: Historical authorization is inferred from trusted committed records in the witness store, not independently reconstructed from immutable historical policy snapshots.
8. **Documented Rollback Limitation**: Restoring an older SQLite database snapshot allows reusing transaction IDs unless an external monotonic checkpoint or transparency log is integrated.

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
