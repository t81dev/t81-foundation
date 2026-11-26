# BitNet-b1.58-2B-4T Streaming Ingestion & `H06-MEM-04` Apple M2 Dispatch Receipt (2026-11-26)

**Date:** 2026-11-26  
**Status:** Non-normative scoped software measurement receipt (does **not** establish universal ternary hardware superiority or production readiness; preserves all active holds `F01 M1–M6 STOP`, `SCG-08 Gate 2 NO-GO`, `H01`/10A `BLOCKED`, `AXION-AI-001 HD01–HD06`, `T81-AIOPT-001 M2` frozen).  
**Host:** Apple Silicon M2 (`Darwin arm64`, `-O3 -DNDEBUG`, Metal reference profile)  
**Base Commit:** `4bc5cf524764829b4c4abc59ff77793c58287d35` (`feat/wave4-dpe-mcp-unification-and-enclave-runner`)

---

## 1. Streaming 2B Architecture Fixture Ingestion (`exact_2b_architecture_fixture`)

Because `microsoft/bitnet-b1.58-2B-4T` was not locally cached in `models/` or `~/.cache/huggingface/`, a deterministic **exact-architecture 2B fixture** (`hidden_size=2560`, `intermediate_size=6912`, `num_hidden_layers=30`, `~40%` zero sparsity) was generated in gitignored `build/models/synthetic-bitnet-2B-4T/` and ingested with `examples/research/bitnet-verifiable-token/load_safetensors_bitnet.py --streaming --chunk-bytes 67108864`:

- **Fixture Classification:** `exact_2b_architecture_fixture` (not a downloaded HuggingFace checkpoint)
- **Shard Payload (`Q` `2560×2560` + `Gate` `2560×6912` + `Down` `6912×2560`):** `41,943,040` trits (`41,943,518` source bytes)
- **Packed Dual-Rail Size:** `10,485,760` bytes (**`4.0×` compression vs `I8`**; `300.0 MiB` across 30 layers for these 3 projections, `~476.84 MiB` for full 2.0B parameters)
- **Peak RSS During Streaming Ingestion (`--chunk-bytes 67108864`):** `828.75 MiB`
- **Deterministic `manifest_sha3_512`:** `sha3-512:c9888ca91ea6cd9c5346b9c5918d209f58b1698b098c3f4c00a1e4f4d9dcf6ac9a32e9c6932af3d8f350e698c79f2e9686e6daf04a8ac2a497ee65eff8ec8156`

---

## 2. `H06-MEM-04` Sparse-Aware Regime Dispatch & Ed25519 Receipt Verification (`t81_bitnet_2b_dispatch_test`)

Executed via `examples/research/heterogeneous-dualrail/verify_bitnet_2b_dispatch.cpp` (`-O3 -DNDEBUG`):

| Layer Shape | `B=1` Decode Route | `B=16` Dense Route (`density=0.60`) | `max_abs_err` | Dual-Rail Throughput (Apple M2 `arm64`) | Output `SHA3-512` Prefix |
| :--- | :--- | :--- | :---: | :---: | :--- |
| `attn.qkv_o_proj` (`2560×2560`) | `cpu_stream_mt` | `metal_gpu` (`R3`) | `0` | `42.51 GOPS/s` | `sha3-512:dc6ebbfb3fab22df...` |
| `mlp.gate_up_proj` (`2560×6912`) | `cpu_stream_mt` | `metal_gpu` (`R3`) | `0` | `43.06 GOPS/s` | `sha3-512:5a288e5bcbd99a8e...` |
| `mlp.down_proj` (`6912×2560`) | `cpu_stream_mt` | `metal_gpu` (`R3`) | `0` | `60.76 GOPS/s` | `sha3-512:dc6ebbfb3fab22df...` |

### Preserved Negative Controls & Accountability Receipt
- **Small Matrix (`128×512`, `B=16`):** Retained on CPU (`R0`); rejected from `metal_gpu`.
- **High-Sparsity (`2560×6912`, `75%` zero, `nonzero_density=0.25 < 0.30`):** Retained on CPU (`R4`); rejected from `metal_gpu`.
- **RFC-00D4 Ed25519 DAG Sealing:** All 3 layer receipts bound to authoritative `ProviderGovernor::compute_policy_sha3_512` and verified via native C++ `t81::crypto::verify_dag` in **`1.32 ms`** (`~0.44 ms/node`).
