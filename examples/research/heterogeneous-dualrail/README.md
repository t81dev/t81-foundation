# Heterogeneous Dual-Rail (CPU ↔ GPU) Capability & Verification Demo

Demonstrates a zero-copy **Dual-Rail `(P, N)`** (`0.25 B/trit`) ternary decode pipeline shared across **CPU** (ARM64 NEON `vcntq_u8` / portable fallback) and **GPU** (Apple Silicon Metal `MTLResourceStorageModeShared` fused command buffer on macOS), emitting:

1. **Bit-exact cross-accelerator verification:** Proves `SHA3-512(CPU int8) == SHA3-512(CPU DualRail) == SHA3-512(Fused Metal GPU DualRail)` with zero floating-point drift.
2. **Graph-fused dispatch measurement:** Compares unfused per-layer GPU submissions against a single fused command buffer (modeling a coarse-grained TISC `BeginGraph` → `TMatMul*` → `CommitGraph` subgraph).
3. **Advertised host capability descriptor (`t81.heterogeneous-dualrail.v1`):** Outputs measured crossover routing thresholds, weight memory savings (`4×` vs. `int8`, `8×` vs. `FP16`), and `bit_exact_integer_reduction: true`.

## Quick Start

```bash
# Default fast check (8 layers, ~118 MB DualRail, < 1s)
./examples/research/heterogeneous-dualrail/run_demo.sh

# Full 32-layer DRAM-streaming 7B-class decode (473 MB DualRail vs. 1.89 GB int8)
./examples/research/heterogeneous-dualrail/run_demo.sh --layers=32
```

## Reference Measurement (Apple M2, 32 Layers, `1.98B` Weights)

| Mode (`32 × [1×4096×4096 + 1×4096×11008]`) | Latency | Throughput | Effective BW | Cross-Device `SHA3-512` |
| :--- | ---: | ---: | ---: | :---: |
| CPU `int8` `vdotq_s32` (1-thread, `1.89 GB`) | `139.59 ms` | `7.16 tok/s` | `13.5 GB/s` | Baseline |
| CPU `DualRail` `vcntq_u8` (1-thread, `473 MB`) | `21.05 ms` | `47.51 tok/s` | `22.5 GB/s` | **Exact match** |
| GPU `DualRail` Unfused (`64` sync CBs) | `19.15 ms` | `52.22 tok/s` | `24.7 GB/s` | **Exact match** |
| **GPU `DualRail` Fused (`1` CB)** | **`5.55 ms`** | **`180.02 tok/s`** | **`89.10 GB/s`** | **Exact match** |

## 4-Way Real Silicon Crossover vs. Apple AMX & Production MPS FP16 (Apple M2)

| Workload (`M × K × N`) | CPU `DualRail` NEON (`0.25 B/w`) | Apple `AMX` FP32 (`4.0 B/w`) | Apple `MPS` FP16 GPU (`2.0 B/w`) | T81 `DualRail` GPU (`0.25 B/w`) | T81 GPU Kernel Speedup |
| :--- | ---: | ---: | ---: | ---: | :--- |
| Decode (`1×4096×4096`) | `201.2 µs` | `4,904.8 µs` | `2,494.5 µs` kern | **`34.6 µs`** kern (`969 GOPS`) | **`72.0×`** vs MPS / **`141.7×`** vs AMX |
| Decode (`1×4096×11008`) | `513.3 µs` | `4,062.5 µs` | `5,934.5 µs` kern | **`139.9 µs`** kern (`645 GOPS`) | **`42.4×`** vs MPS / **`29.0×`** vs AMX |
| Prefill (`16×2048×2048`) | `706.7 µs` | `864.8 µs` | `710.5 µs` kern | **`136.7 µs`** kern (`982 GOPS`) | **`5.20×`** vs MPS / **`6.33×`** vs AMX |
| Prefill (`256×2048×2048`) | `11,183 µs` | `3,092.4 µs` | `1,875.3 µs` kern | **`646.4 µs`** kern (**`3.32 TOPS`**) | **`2.90×`** vs MPS / **`4.78×`** vs AMX |
| Prefill (`512×2048×2048`) | `22,380 µs` | `5,627.9 µs` | `2,119.7 µs` kern | **`1,364.2 µs`** kern (**`3.15 TOPS`**) | **`1.55×`** vs MPS / **`4.13×`** vs AMX |

> **Scope & Caveats:** Results reflect ternary `{-1, 0, +1}` workloads on Apple M2 (`100 GB/s` unified memory). As `M` grows past `512` (`M >= 1024`), compute-bound FP16/NPU tensor engines approach their peak FLOPS and overtake bitwise `DualRail`. On x86_64 Linux/Windows, `heterogeneous_dualrail_demo.cpp` uses AVX2 `_mm256_movemask_epi8` / `vpshufb` popcount or portable C++23 fallback.
