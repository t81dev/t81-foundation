# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `7f7f92319e2fd7c317438a856d4388412c773dbd1284912f2500d6519d520073`

| Opcode | AI Header | TISC Enum | VM Dispatch | Status |
| :--- | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | runtime_bound |
| QMATMUL | yes | yes | yes | runtime_bound |
| EMBED | yes | yes | yes | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
