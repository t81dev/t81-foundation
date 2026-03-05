# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `4e5556cc72de60174214f521be68718b084e1575e12f8bd82a9994e694f33428`

| Opcode | AI Header | TISC Enum | VM Dispatch | CTest Evidence | Status |
| :--- | :---: | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | yes | runtime_bound |
| QMATMUL | yes | yes | yes | yes | runtime_bound |
| EMBED | yes | yes | yes | yes | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
- `build/ai-opcodes/ai_phase1_opcode_ctest.log`

Phase-1 Conformance Evidence:
- all_required_tests_present: `True`
- ctest_success_marker_present: `True`
