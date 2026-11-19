# Narrowed TISC Tensor + Policy Subgraph Verifier & Heterogeneous Dispatcher

Stage 3 proof-of-concept demonstrating what a **narrowed, coarse-grained TISC subgraph** (`WeightsLoad` → `ChkShape` → `AxVerify` → `TMatMul*` → `Halt`) delivers over conventional stacks:

1. **~60-Line Static Pre-Execution Verifier:** Consumes locked `t81::tisc::Program` bytecode ([opcodes.hpp](../../../include/t81/isa/opcodes.hpp)) and statically proves *before execution* that every `TMatMul` is dominated by content-addressed `WeightsLoad`, `ChkShape`, and `AxVerify` with zero unguarded ops.
2. **Fail-Closed Negative Tests:** Automatically verifies that:
   - unverified `TMatMul` (missing `AxVerify`) is statically rejected,
   - unchecked-shape `TMatMul` (missing `ChkShape`) is statically rejected, and
   - tampered weight content hash fails closed at load time.
3. **Heterogeneous Fused Dispatch + Bit-Exact Receipt Equivalence:** Executes the **exact same verified TISC bytecode** on **CPU NEON `vcntq_u8`** and **Fused Metal GPU `popcount`** (1 command buffer over zero-copy `MTLResourceStorageModeShared` `DualRail (P, N)` weights on macOS) and proves **identical `SHA3-512` execution receipts** across both processors.

## Quick Start

```bash
./examples/research/tisc-subgraph-dispatch/run_subgraph_demo.sh
```
