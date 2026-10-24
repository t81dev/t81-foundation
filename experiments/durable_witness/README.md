# Durable Witness Accountable Computation Experiment

This isolated experiment demonstrates a minimal durable authorization-and-effect witness that closes replay and fabrication security gaps across runtime arms (T81VM and Wasmtime) without requiring custom TISC VM instructions.

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
