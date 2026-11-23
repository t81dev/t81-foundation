# BitNet b1.58 Verifiable Token Generation (CanonFS + TISC + Dual-Rail CPU/GPU)

End-to-end verifiable ternary inference prototype connecting all proven T81 components:

1. **Real BitNet b1.58 Quantization:** Quantizes floating-point weights via `round(clip(W / mean(|W|), -1, +1))` into zero-copy `DualRail (P, N)` bitplanes (`0.25 B/trit`, `4×` smaller than `int8`, `8×` smaller than `FP16`).
2. **Content-Addressed Weight Verification (`SHA3-512`):** Commits the weight store to a canonical `SHA3-512` digest and verifies fail-closed rejection on single-bit weight tampering.
3. **Narrowed TISC Subgraph Static Verification:** Builds locked `t81::tisc::Program` bytecode (`WeightsLoad` → `ChkShape` → `AxVerify` → `TMatMul` → `Halt`) and statically verifies policy + shape dominance before execution.
4. **Cross-Accelerator Bit-Exact Autoregressive Decoding:** Generates 8 deterministic tokens via exact `int32_t` argmax on **both** CPU NEON `vcntq_u8` and Fused Metal GPU `popcount` (macOS), proving **identical token IDs and identical `SHA3-512` execution receipts** across both processors.

## Quick Start

```bash
./examples/research/bitnet-verifiable-token/run_bitnet_demo.sh
```
