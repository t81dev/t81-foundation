# Deterministic Cyber-Defense Risk & Bayesian Triage Gate

Standalone C++23 example consuming T81's locked `v2.0.0` core data types (`include/t81/types/T81Prob.hpp`, `T81Fraction.hpp`, `T81Result.hpp`) plus `crypto/sha3.cpp`.

## What It Demonstrates

1. **Zero-Underflow Bayesian Updating (`T81Prob<27>`):** Fixed-point balanced-ternary log-odds accumulation with saturation (no `exp`/`log`/`div` in the hot path).
2. **Zero-Drift Rational Thresholds (`T81Fraction<81>`):** Exact `1/10 + 2/10 == 3/10` boundary (where IEEE-754 `0.1 + 0.2 > 0.3` would false-positive) and exact `1/3 + 1/4 == 7/12` impact aggregation.
3. **Fail-Closed Control Flow (`T81Result`):** Malformed telemetry (e.g., non-positive denominator) returns an explicit error instead of propagating NaN/Inf.
4. **Canonical SHA3-512 Commitment:** Emits a bit-identical digest over exact integer/rational state across runs and architectures.

## Run

```bash
bash examples/research/deterministic-risk-triage/run_triage_demo.sh
```
