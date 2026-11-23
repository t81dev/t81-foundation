# Zero-Drift Exact Rational Lab

Demonstrates T81's DCP-verified exact arithmetic (`T81Fraction` and `T81BigInt`) paired with deterministic execution certification and self-verifying `CanonFS` storage.

## What It Exercises

- Exact rational identities (`1/10 + 2/10 == 3/10`, `(10^12 + 1/3) - 10^12 == 1/3`, harmonic sum `H_4 = 25/12`) without IEEE-754 rounding drift.
- `t81 determinism multi-run` and `t81 determinism certify` over the compiled `.tisc` artifact.
- `t81 canonfs import` and `t81 canonfs fsck --verify-hashes` in an isolated temporary CanonFS root.

## Evidence Boundary

This example demonstrates **exact deterministic arithmetic and content-addressed provenance**. It does not claim ternary execution speed superiority over binary baselines.

## Run

```bash
chmod +x examples/research/exact-rational-lab/run_lab.sh
T81_BIN=./build-audit/t81 ./examples/research/exact-rational-lab/run_lab.sh
```
