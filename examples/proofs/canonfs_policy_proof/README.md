# CanonFS + Axion Governance Proof

Demonstrates immutable artifact identity, deterministic execution, and pre-execution policy enforcement.

## What this proves

- artifacts are stored as hash-addressed objects
- execution is allowed only with valid policy
- execution is reproducible (`deterministic_replay: yes`)
- denied execution prevents computation (`compute_executed: no`)

## How to run

```bash
./run_canonfs_policy_proof.sh
```

## Expected output

- `status: ok`
- `deterministic_replay: yes`
- `status: error`
- `reason: policy_denied`
- `compute_executed: no`
