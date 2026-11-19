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

> **Scope & Caveats:** Results reflect memory-bandwidth-bound `M = 1` ternary decode on Apple Silicon unified memory (`100 GB/s` peak). Single-shot (`K = 1`) operations starting from `int8` or small vectors (`N <= 256`) still favor native `int8`; Dual-Rail wins when weights stay resident in `(P, N)` bitplanes and multi-layer dispatches are fused.
