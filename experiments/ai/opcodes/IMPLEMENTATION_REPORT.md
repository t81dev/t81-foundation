# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `4993fd8e09a8c383599404a1a72167b326a51fe3e556272e125fd29223815830`

| Opcode | AI Header | TISC Enum | VM Dispatch | Status |
| :--- | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | runtime_bound |
| QMATMUL | yes | yes | yes | runtime_bound |
| EMBED | yes | yes | yes | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
