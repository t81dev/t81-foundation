# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `7fbfc215bce41c2d2921031e6d5aec9ed7f4ebec52ddc64309dc35edc96ea167`

| Opcode | AI Header | TISC Enum | VM Dispatch | CTest Evidence | Output Hash | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | yes | `db742b617767af01` | runtime_bound |
| QMATMUL | yes | yes | yes | yes | `14d41b8df3810394` | runtime_bound |
| EMBED | yes | yes | yes | yes | `8715026970757876` | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `core/vm/vm.cpp`
- `build/ai-opcodes/ai_phase1_opcode_ctest.log`

Phase-1 Conformance Evidence:
- all_required_tests_present: `True`
- all_required_output_hashes_present: `True`
- ctest_success_marker_present: `True`
