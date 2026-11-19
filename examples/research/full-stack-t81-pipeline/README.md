# Full-Stack T81 Integration Pipeline (`CanonFS → Axion → TISC → T81VM → Dual-Rail`)

End-to-end integration test & demo connecting all five layers of `t81-foundation` in a single real `T81VM` execution:

1. **CanonFS (`make_in_memory_driver`):** Stores packed balanced-ternary `NativeTensor` matrices (`A: 64×128`, `B: 128×64`) as content-addressed `CanonFS` objects.
2. **Axion Policy (`PolicyEngine` + `AIHookEngine`):** Enforces tensor hash allowlisting before every `Opcode::TLoadHash` and `Opcode::TMatMul` step, proving fail-closed `Trap::SecurityFault` when a hash is not allowlisted.
3. **TISC Bytecode (`t81::tisc::Program`):** Executes `LoadImm` → `TLoadHash` → `TLoadHash` → `TMatMul` → `Halt` inside the real `t81::vm::make_interpreter_vm()`.
4. **Seam 1 Dual-Rail Fast Path (`vm/tensor_helpers.cpp`, `f38dbe956`):** Routes `Opcode::TMatMul` through the NEON `vcntq_u8` / portable `std::popcount` Dual-Rail kernel (**`24,253×`–`35,483×`** faster than scalar `t81::ops::matmul` on `T729DynamicTensor`), verified bit-exact against an independent scalar oracle and committed to a `SHA3-512` execution receipt.

## Quick Start

```bash
./examples/research/full-stack-t81-pipeline/run_pipeline.sh
```
