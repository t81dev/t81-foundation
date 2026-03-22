# AI Opcode Runtime Report

- schema: `t81.ai.opcode-runtime-report.v1`
- phase_status: `runtime_bound`
- report_sha256: `2bdfac23c77130cbf54deebdecf94199ce313fb8228c83b990d0974a6a47dade`

| Opcode | AI Header | TISC Enum | VM Dispatch | CTest Evidence | Output Hash | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| ATTN | yes | yes | yes | yes | `d878d6629816575f` | runtime_bound |
| QMATMUL | yes | yes | yes | yes | `14d41b8df3810394` | runtime_bound |
| EMBED | yes | yes | yes | yes | `8715026970757876` | runtime_bound |

Evidence Files:
- `include/t81/isa/ai_native_opcodes.hpp`
- `include/t81/isa/opcodes.hpp`
- `vm/vm.cpp`
- `build/ai-opcodes/ai_phase1_opcode_ctest.log`

Phase-1 Conformance Evidence:
- all_required_tests_present: `True`
- all_required_output_hashes_present: `True`
- ctest_success_marker_present: `True`

Phase-1 Baseline Hash Comparison:
- baseline_path: `experiments/ai/opcodes/PHASE1_BASELINE_HASHES_HISTORY.json`
- all_match: `True`
- baseline_window_id: `2026-03b`
- baseline_window_start: `2026-03-06`
- baseline_window_end: `2026-03-31`
- baseline_as_of_date: `2026-03-06`
- baseline_window_provenance:
  - cross_lane_evidence_ref: `build/ai-cross-lane/ai_cross_lane_evidence.json`
  - ctest_log_ref: `build/ai-opcodes/ai_phase1_opcode_ctest.log`
  - fixture_set_ref: `tests/fixtures/llama_cpp_repro`
  - runtime_report_ref: `build/ai-opcodes-runtime/ai_opcode_runtime_report.json`
- ATTN: expected=`d878d6629816575f` actual=`d878d6629816575f` match=`True`
- QMATMUL: expected=`14d41b8df3810394` actual=`14d41b8df3810394` match=`True`
- EMBED: expected=`8715026970757876` actual=`8715026970757876` match=`True`
