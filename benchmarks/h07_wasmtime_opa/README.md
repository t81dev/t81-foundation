# H07 Head-to-Head Benchmark: `T81` vs. `Wasmtime + OPA`

Reproducible comparative benchmark for canonical research hypothesis **H07** (*"Representation-independent governed execution can improve a defined auditability property"*) comparing:

- **T81 Stack:** `t81` (`T81VM` precompiled `.tisc` + `Axion` immutable policy + `CanonFS` + native C++23 RFC 8032 / Wycheproof Ed25519 receipts via `t81 mcp serve` and `t81 receipt export`).
- **Conventional Control Stack (`Wasmtime + OPA`):** `wasmtime` (precompiled Cranelift AOT `.cwasm`) + `opa` (`opa eval` for cold-start, persistent `opa run --server` HTTP keep-alive daemon for warm steady-state) + standard `SHA-256` / OpenSSL Ed25519 receipt signing & offline verification.

## Prerequisites

- Built `t81` CLI binary (`cmake --build build --target t81 -j4`)
- `wasmtime` (e.g., `brew install wasmtime` — pinned M2 evidence uses `wasmtime 51.0.0`)
- `opa` (e.g., `brew install opa` — pinned M2 evidence uses `opa 1.23.0`)
- `openssl` (pinned M2 evidence uses `OpenSSL 3.6.2`)

## Usage

```bash
# Fast CI smoke pass (skips cleanly if wasmtime/opa are absent when --allow-missing-deps is set)
python3 benchmarks/h07_wasmtime_opa/run_h07_benchmark.py --smoke --allow-missing-deps

# Full reproducible campaign (W1=20 trials, W2=1,000 steady-state calls, W3=25 verify trials)
python3 benchmarks/h07_wasmtime_opa/run_h07_benchmark.py \
  --w1-trials 20 --w2-iters 1000 --w3-trials 25 \
  --out-json benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.json \
  --out-csv  benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.csv
```

## Workloads

- **W1 (Cold-Start Governance Gate):** Fresh process invocation loading policy + precompiled module (`.tisc` vs `.cwasm`), evaluating allow + deny decisions, executing the matched pure-compute kernel (`compute(1000) == 58038`), and emitting a signed receipt.
- **W2 (Warm Steady-State Loop, `N = 1,000`):**
  - `W2_warm_policy_only_us`: persistent `t81 mcp serve` (`t81.gate_check`) vs persistent `opa run --server` HTTP keep-alive.
  - `W2_warm_governed_commit_us`: persistent `t81 mcp serve` (`tx_begin` + `tx_commit` + CanonFS + in-process Ed25519) vs persistent `opa run --server` HTTP keep-alive + OpenSSL CLI Ed25519 signing.
  - `W2_compute_kernel_cli_ms`: precompiled `.tisc` (`t81 vm run`) vs precompiled `.cwasm` (`wasmtime --allow-precompiled`).
- **W3 (Fault Injection & Offline Verification):** Verifies `100%` fail-closed rejection on both stacks across (a) plane/path bypass, (b) policy tampering, and (c) 1-byte receipt payload tampering, and measures offline verification latency.

See [../../docs/research/H07_WASMTIME_OPA_COMPARISON.md](../../docs/research/H07_WASMTIME_OPA_COMPARISON.md) for the full analysis and falsification verdict.
