# H07 Closure Report: Reproducible Head-to-Head Comparison (`T81` vs. `Wasmtime + OPA`)

**Date:** 2026-11-27  
**Hypothesis:** `H07` ([`HYPOTHESIS_REGISTER_v1.json`](continuity/HYPOTHESIS_REGISTER_v1.json)) — *"Representation-independent governed execution can improve a defined auditability property."*  
**Verdict:** **`PARTIALLY SUPPORTED (REGIME-BOUNDED)`**  
**Evidence Tier:** `A` (reproduced on pinned source with raw JSON/CSV artifacts)  
**Harness & Raw Artifacts:**
- Harness: [`benchmarks/h07_wasmtime_opa/run_h07_benchmark.py`](../../benchmarks/h07_wasmtime_opa/run_h07_benchmark.py)
- Raw JSON: [`benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.json`](../../benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.json)
- Raw CSV: [`benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.csv`](../../benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.csv)

---

## 1. Test Environment & Pinned Toolchains

| Dimension | Value |
|---|---|
| **Host** | Apple Silicon M2 (`arm64`, `8 GB` unified memory, `macOS-26.5.2-arm64`) |
| **T81 Stack** | `T81 Foundation 2.3.0` (`build/t81` release binary, `T81VM` `.tisc` + `Axion` + `CanonFS` + native C++23 RFC 8032 Ed25519 + `t81 mcp serve` + `t81 receipt export`) |
| **Conventional Control** | `wasmtime 51.0.0 (32656dc48 2026-11-20)` (precompiled Cranelift AOT `.cwasm`) + `OPA 1.23.0` (`Rego v1`, `opa eval` for cold-start & persistent `opa run --server` HTTP keep-alive for warm steady-state) + `OpenSSL 3.6.2` (`Ed25519` + `SHA-256`) |
| **Matched Compute Kernel** | Pure deterministic integer loop: `acc = 0; for i in 0..999: acc = (acc + i * 3 + 1) % 65521` -> `58038` (`kernel.t81` precompiled to `.tisc` vs. `kernel.wat` precompiled to `.cwasm`) |

### Anti-Strawman Control Parity
- `Wasmtime` executes a **precompiled Cranelift AOT `.cwasm`** module (`wasmtime compile kernel.wat -o kernel.cwasm`) — zero JIT compilation overhead on warm/cold calls.
- `OPA` runs as a **persistent local HTTP daemon (`opa run --server`) with HTTP keep-alive** during `W2` steady-state — zero `opa eval` subprocess spawning in the warm loop.
- Both stacks enforce the exact same 4-part policy contract: allowed action, root path confinement, artifact digest allowlist, and Three-Plane transition reconciliation (`ERR_PLANE_BYPASS_DENIED`).

---

## 2. Quantitative Results (Wins, Ties, and Losses Verbatim)

### 2.1 Binary & Steady-State Memory Footprint

| Metric | `T81 v2.3.0` | `Wasmtime 51.0 + OPA 1.23` | Observation |
|---|---:|---:|---|
| **Binary Size** | **`5.48 MB`** | `83.36 MB` (`43.96` + `39.40 MB`) | **T81 win (`15.2×` smaller)** |
| **Daemon RSS (initial)** | **`2.31 MiB`** (`t81 mcp serve`) | `34.31 MiB` (`opa run --server`) | **T81 win (`14.9×` lower RSS)** |
| **Daemon RSS (post-1,000 calls)** | **`3.38 MiB`** (`t81 mcp serve`) | `40.14 MiB` (`opa run --server`) | **T81 win (`11.9×` lower RSS)** |

### 2.2 Latency & Throughput (`W1`, `W2`, `W3`)

| Workload | Stack | `n` | `p50` | `p95` | `p99` | `IQR` | Outcome |
|---|---|---:|---:|---:|---:|---:|---|
| **W1 Cold-Start Gate (`ms`)** | `T81` (`.tisc` + MCP + commit) | 20 | `168.22 ms` | `180.57 ms` | `185.30 ms` | `2.00 ms` | **T81 loss (`3.1×` slower)** |
| **W1 Cold-Start Gate (`ms`)** | `Wasmtime + OPA` (`.cwasm` + `opa eval` + OpenSSL) | 20 | **`54.23 ms`** | **`69.19 ms`** | **`86.74 ms`** | `7.55 ms` | **Wasmtime+OPA win** |
| **W2a Warm Policy-Only (`µs`)** | `T81` (`t81.gate_check` via MCP stdio) | 1,000 | `166.06 µs` | **`213.59 µs`** | **`298.39 µs`** | **`11.25 µs`** | **Split: tighter `p95`/`p99`/`IQR`** |
| **W2a Warm Policy-Only (`µs`)** | `OPA` (`opa run --server` HTTP keep-alive) | 1,000 | **`152.11 µs`** | `255.76 µs` | `445.67 µs` | `34.23 µs` | **Split: `8.4%` faster `p50`** |
| **W2b Warm Governed Commit + Ed25519 (`µs`)** | `T81` (`tx_begin`+`tx_commit`+CanonFS+Ed25519) | 1,000 | **`2,750.00 µs`** | **`3,148.60 µs`** | **`3,534.76 µs`** | **`155.10 µs`** | **T81 win (`3.87×` vs CLI glue)**\* |
| **W2b Warm Governed Commit + Ed25519 (`µs`)** | `OPA HTTP + OpenSSL CLI Ed25519` | 1,000 | `10,640.33 µs` | `12,863.73 µs` | `15,614.21 µs` | `778.45 µs` | CLI OpenSSL subprocess dominates\* |
| **W2c Pure Compute Kernel CLI (`ms`)** | `T81` (`t81 vm run kernel.tisc`) | 20 | `157.11 ms` | `157.74 ms` | `159.07 ms` | `1.72 ms` | **T81 loss (`18.4×` slower)** |
| **W2c Pure Compute Kernel CLI (`ms`)** | `Wasmtime` (`--allow-precompiled kernel.cwasm`) | 20 | **`8.52 ms`** | **`12.34 ms`** | **`37.04 ms`** | **`0.77 ms`** | **Wasmtime win (`18.4×` faster)** |
| **W3 Offline Bundle Verify (`ms`)** | `T81` (`t81 receipt export` Ed25519 verify) | 25 | **`9.12 ms`** | **`10.02 ms`** | **`16.84 ms`** | **`0.32 ms`** | **T81 win (`1.11×` `p50`, `1.43×` `p95`)** |
| **W3 Offline Bundle Verify (`ms`)** | `Wasmtime + OPA` (`SHA-256` + `openssl pkeyutl -verify`) | 25 | `10.12 ms` | `14.32 ms` | `19.63 ms` | `0.65 ms` | Comparable (`+1.0 ms` `p50`) |

\* **Anti-Strawman Caveat on `W2b`:** The `Wasmtime + OPA` control in `W2b` uses zero-dependency `openssl pkeyutl` CLI signing (`~10 ms` process spawn). A custom embedded Rust/Go host linking `wasmtime` + `opa` + an in-process Ed25519 library would eliminate that `~10 ms` subprocess overhead and likely beat or match T81's `2.75 ms` commit loop (which performs atomic staging file I/O + CanonFS persistence + Ed25519 signing). The measured `3.87×` advantage applies specifically to out-of-the-box CLI/daemon composition without custom host-side cryptographic glue.

### 2.3 Fault Injection Parity (`W3`)

| Fault Class | `T81 v2.3.0` | `Wasmtime + OPA` | Audit Integration Boundary |
|---|---|---|---|
| **(a) Plane / Path Bypass** | **DENIED** (`ERR_PLANE_BYPASS_DENIED`, built-in) | **DENIED** (`ERR_PLANE_BYPASS_DENIED`, Rego rule) | Both `100%` fail-closed |
| **(b) Policy Tampering** | **DENIED** (immutable `axion_root.apl` + manifest check, built-in) | **DENIED** (requires external SHA-256 policy-pinning glue) | T81 enforces in-binary; OPA requires host glue |
| **(c) 1-Byte Receipt Tampering** | **DENIED** (offline `SHA3-512` + RFC 8032 Ed25519) | **DENIED** (offline `SHA-256` + OpenSSL Ed25519) | Both `100%` fail-closed |

---

## 3. Falsification Analysis & Scope Boundaries

1. **Verdict: `PARTIALLY SUPPORTED (REGIME-BOUNDED)`.**
   - **Supported Regime:** Out-of-the-box, single-binary governed execution for resource-constrained hosts (`8 GB` edge/laptop/CI environments) where deployment footprint (`5.48 MB` vs `83.36 MB`), low steady-state daemon memory (`3.38 MiB` vs `40.14 MiB`), tight tail policy latency (`p99 298 µs` vs `446 µs`), and integrated fail-closed transactional receipts without external glue are the primary auditability and operational properties.
   - **Falsified / Unsupported Regime:** Compute-intensive cold-start or pure sandbox execution (`W1`, `W2c`). `Wasmtime`'s Cranelift AOT native execution (`8.52 ms`) decisively outperforms `T81VM`'s interpreter (`157.11 ms`) by `18.4×`, making `Wasmtime + OPA` `3.1×` faster end-to-end on cold-start (`54.23 ms` vs `168.22 ms`). Likewise, `OPA` HTTP keep-alive achieves `8.4%` lower median (`p50`) policy-only latency (`152.1 µs` vs `166.1 µs`) and offers a far more mature policy language ecosystem.
2. **Representation Independence:** Every measured T81 advantage is attributable to **single-binary C++23 systems integration** (in-process Axion + CanonFS + Ed25519), **not** to balanced-ternary arithmetic or radix choice. This is consistent with `H07`'s representation-independent framing and does not alter `H01`–`H06` or `H08`.
3. **Preserved Holds:** This closure does **not** activate Protocol 001, certify production security, or alter `SCG-08 Gate 2` (`NO-GO`), `F01-HOST-BOLQ-v1` (`NOT ADOPTED`), `AXION-AI-001` (`#789`–`#791`), `H01` (`BLOCKED`), `T81-AIOPT-001`, or Horizon D (`DEC-004`).

---

## 4. Exact Reproduction Command

```bash
python3 benchmarks/h07_wasmtime_opa/run_h07_benchmark.py \
  --w1-trials 20 --w2-iters 1000 --w3-trials 25 \
  --out-json benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.json \
  --out-csv  benchmarks/h07_wasmtime_opa/results/m2_arm64_2026-11-27.csv
```
