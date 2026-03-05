# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `3cd873e68053112ba6b5ca69ddd59724c81947d3d0c35efbc39f366d9490c51c`

| Opcode | AI Header | TISC Enum | VM Dispatch | Status |
| :--- | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | runtime_bound |
| QMATMUL | yes | yes | yes | runtime_bound |
| EMBED | yes | yes | yes | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
