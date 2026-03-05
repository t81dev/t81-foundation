# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `0db5342cf9676196b71c6994f01191d5a3ce9232e2fa6b8469f1471607582b69`

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

Phase-1 Baseline Hash Comparison:
- baseline_path: `experiments/ai/opcodes/PHASE1_BASELINE_HASHES.json`
- all_match: `True`
- ATTN: expected=`db742b617767af01` actual=`db742b617767af01` match=`True`
- QMATMUL: expected=`14d41b8df3810394` actual=`14d41b8df3810394` match=`True`
- EMBED: expected=`8715026970757876` actual=`8715026970757876` match=`True`
