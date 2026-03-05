# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `bae94c83e1e70c6bb19aac2b19d585c5c9b21cb41004838020ec462d7d1a84f3`

| Opcode | AI Header | TISC Enum | VM Dispatch | Status |
| :--- | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | runtime_bound |
| QMATMUL | yes | yes | yes | runtime_bound |
| EMBED | yes | yes | yes | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
