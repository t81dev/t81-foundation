cmake_minimum_required(VERSION 3.20)
project(t81_foundation VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build Type Configuration
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# AI Experiments Option
option(T81_ENABLE_AI_EXPERIMENTS "Build AI experiments in /experiments/ai" OFF)

if(T81_TRITWISE_PROFILE)
  add_compile_definitions(T81_TRITWISE_PROFILE)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(ggml REQUIRED QUIET)

# Core T81 libraries
add_library(t81_core STATIC
  src/codec/base81_packed.cpp
  src/codec/metadata.cpp
  src/codec/trit_packing.cpp
  src/codec/ternary_quantization.cpp
  src/codec/ternary_gguf.cpp
  src/crypto/sha3.cpp
  src/setun/bridge.cpp
)

# T81 ISA library
add_library(t81_isa STATIC
  core/isa/binary_emitter.cpp
  core/isa/binary_io.cpp
  core/isa/base81_view.cpp
)

target_link_libraries(t81_core PUBLIC t81_isa)
target_include_directories(t81_isa PUBLIC include)

# Phase 3 Enhanced libraries
add_library(t81_enhanced_codec STATIC
  src/codec/enhanced_gguf_parser.cpp
  src/codec/advanced_ternary_quantization.cpp
)

add_library(t81_memory STATIC
  src/memory/advanced_memory_manager.cpp
)

add_library(t81_inference STATIC
  src/inference/batch_inference_engine.cpp
)

  add_executable(t81_frontend_parser_legacy_rejection_test tests/cpp/frontend_parser_legacy_rejection_test.cpp)
  target_link_libraries(t81_frontend_parser_legacy_rejection_test PRIVATE t81_lang_frontend)

  add_executable(t81_frontend_ir_generator_test tests/cpp/frontend_ir_generator_test.cpp)
  target_link_libraries(t81_frontend_ir_generator_test PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_frontend_logical_lowering_test tests/cpp/test_frontend_logical_lowering.cpp)
  target_link_libraries(t81_frontend_logical_lowering_test PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_ir_snapshot_audit_test tests/cpp/test_ir_snapshot_audit.cpp)
  target_link_libraries(t81_ir_snapshot_audit_test PRIVATE t81_lang_frontend t81_isa)
  add_test(NAME t81_ir_snapshot_audit_test COMMAND t81_ir_snapshot_audit_test)

  add_executable(t81_semantic_analyzer_option_result_test tests/cpp/semantic_analyzer_option_result_test.cpp)
  target_link_libraries(t81_semantic_analyzer_option_result_test PRIVATE t81_lang_frontend)

  add_executable(test_std_option_result tests/cpp/test_std_option_result.cpp)
  target_link_libraries(test_std_option_result PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_semantic_analyzer_match_test tests/cpp/semantic_analyzer_match_test.cpp)
  target_link_libraries(t81_semantic_analyzer_match_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_generic_test tests/cpp/semantic_analyzer_generic_test.cpp)
  target_link_libraries(t81_semantic_analyzer_generic_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_indexing_test tests/cpp/semantic_analyzer_indexing_test.cpp)
  target_link_libraries(t81_semantic_analyzer_indexing_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_vector_literal_test tests/cpp/semantic_analyzer_vector_literal_test.cpp)
  target_link_libraries(t81_semantic_analyzer_vector_literal_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_record_enum_test tests/cpp/semantic_analyzer_record_enum_test.cpp)
  target_link_libraries(t81_semantic_analyzer_record_enum_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_equality_test tests/cpp/semantic_analyzer_equality_test.cpp)
  target_link_libraries(t81_semantic_analyzer_equality_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_numeric_test tests/cpp/semantic_analyzer_numeric_test.cpp)
  target_link_libraries(t81_semantic_analyzer_numeric_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_stage3_rules_test
                 tests/cpp/semantic_analyzer_stage3_rules_test.cpp)
  target_link_libraries(t81_semantic_analyzer_stage3_rules_test PRIVATE t81_lang_frontend)
  add_test(NAME t81_semantic_analyzer_stage3_rules_test
           COMMAND t81_semantic_analyzer_stage3_rules_test)

  add_executable(t81_semantic_analyzer_diagnostic_location_test tests/cpp/semantic_analyzer_diagnostic_location_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_location_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_cascade_suppression_test tests/cpp/semantic_analyzer_cascade_suppression_test.cpp)
  target_link_libraries(t81_semantic_analyzer_cascade_suppression_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_diagnostic_precision_test tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_precision_test PRIVATE t81_lang_frontend)

  add_executable(t81_isa_binary_emitter_test tests/cpp/tisc_binary_emitter_test.cpp)
  target_link_libraries(t81_isa_binary_emitter_test PRIVATE t81_isa)

  add_executable(t81_isa_type_alias_io_test tests/cpp/tisc_type_alias_io_test.cpp)
  target_link_libraries(t81_isa_type_alias_io_test PRIVATE t81_isa)

  add_executable(t81_isa_binary_io_determinism_test tests/cpp/tisc_binary_io_determinism_test.cpp)
  target_link_libraries(t81_isa_binary_io_determinism_test PRIVATE t81_isa t81_core)
  add_executable(t81_isa_binary_metadata_roundtrip_property_test tests/cpp/tisc_binary_metadata_roundtrip_property_test.cpp)
  target_link_libraries(t81_isa_binary_metadata_roundtrip_property_test PRIVATE t81_isa t81_core)

  add_executable(t81_semantic_analyzer_loop_test tests/cpp/semantic_analyzer_loop_test.cpp)
  target_link_libraries(t81_semantic_analyzer_loop_test PRIVATE t81_lang_frontend)

  add_executable(ir_inspector tools/ir_inspector.cpp)
  target_link_libraries(ir_inspector PRIVATE t81_lang_frontend)

  add_executable(e2e_let_statement_test tests/cpp/e2e_let_statement_test.cpp)
  target_link_libraries(e2e_let_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_arithmetic_test tests/cpp/e2e_arithmetic_test.cpp)
  target_link_libraries(e2e_arithmetic_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_if_statement_test tests/cpp/e2e_if_statement_test.cpp)
  target_link_libraries(e2e_if_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_result_test tests/cpp/e2e_option_result_test.cpp)
  target_link_libraries(e2e_option_result_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_type_test tests/cpp/e2e_option_type_test.cpp)
  target_link_libraries(e2e_option_type_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_result_function_test tests/cpp/e2e_option_result_function_test.cpp)
  target_link_libraries(e2e_option_result_function_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(t81_cli_check_test tests/cpp/cli_check_test.cpp)
  target_link_libraries(t81_cli_check_test PRIVATE t81_tool_cli)

  add_executable(security_package_init_test tests/cpp/security_package_init_test.cpp)
  target_link_libraries(security_package_init_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_pipeline_test tests/cpp/cli_pipeline_test.cpp)
  target_link_libraries(t81_cli_pipeline_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_disasm_test tests/cpp/cli_disasm_test.cpp)
  target_link_libraries(t81_cli_disasm_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_debugger_test tests/cpp/cli_debugger_test.cpp)
  target_link_libraries(t81_cli_debugger_test PRIVATE t81_tool_cli t81_isa)

  add_executable(t81_cli_option_result_test tests/cpp/cli_option_result_test.cpp)
  target_link_libraries(t81_cli_option_result_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_diagnostic_context_test tests/cpp/cli_diagnostic_context_test.cpp)
  target_link_libraries(t81_cli_diagnostic_context_test PRIVATE t81_tool_cli t81_lang_frontend t81_isa t81_vm)

  add_executable(t81_cli_trace_export_test tests/cpp/cli_trace_export_test.cpp)
  target_link_libraries(t81_cli_trace_export_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_contract_test tests/cpp/cli_contract_test.cpp)
  target_compile_features(t81_cli_contract_test PRIVATE ${T81_CXX_STD_FEATURE})

  add_executable(t81_cli_structural_types_test tests/cpp/cli_structural_types_test.cpp)
  target_link_libraries(t81_cli_structural_types_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_repl_test tests/cpp/cli_repl_test.cpp)
  target_link_libraries(t81_cli_repl_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(cli_canonize_tensor_test tests/cpp/cli_canonize_tensor_test.cpp)
  target_link_libraries(cli_canonize_tensor_test PRIVATE t81_tool_cli t81_core)

  add_executable(cli_repro_hash_test tests/cpp/cli_repro_hash_test.cpp)
  target_compile_features(cli_repro_hash_test PRIVATE ${T81_CXX_STD_FEATURE})

  # Parameterized stdlib fixture harness (consolidates 10 per-module tests)
  add_executable(cli_stdlib_fixtures_test tests/cpp/cli_stdlib_fixtures_test.cpp)
  target_link_libraries(cli_stdlib_fixtures_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(cli_std_collections_determinism_test tests/cpp/cli_std_collections_determinism_test.cpp)
  target_link_libraries(cli_std_collections_determinism_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_record_enum_test tests/cpp/cli_record_enum_test.cpp)
  target_link_libraries(t81_cli_record_enum_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_tier4_test tests/cpp/tier4_test.cpp)
  target_link_libraries(t81_tier4_test PRIVATE t81_core t81_axion)

  add_executable(t81_tier5_test tests/cpp/tier5_test.cpp)
  target_link_libraries(t81_tier5_test PRIVATE t81_core)

  add_executable(t81_infinite_opcodes_test tests/cpp/test_infinite_opcodes.cpp)
  target_link_libraries(t81_infinite_opcodes_test PRIVATE t81_vm t81_isa t81_core)

  add_executable(t81_tier4_vm_test tests/cpp/tier4_vm_test.cpp)
  target_link_libraries(t81_tier4_vm_test PRIVATE t81_vm t81_core)

  add_executable(t81_tier4_distributed_test tests/cpp/test_tier4_distributed.cpp)
  target_link_libraries(t81_tier4_distributed_test PRIVATE t81_vm t81_core)

  add_executable(tiers_structure_test tests/cpp/tiers_structure_test.cpp)
  target_link_libraries(tiers_structure_test PRIVATE t81_core)

  add_executable(vm_reflection_tier2_test tests/cpp/vm_reflection_tier2_test.cpp)
  target_link_libraries(vm_reflection_tier2_test PRIVATE t81_vm t81_isa t81_core)

  add_executable(t81_jit_test tests/cpp/jit_test.cpp)
  target_link_libraries(t81_jit_test PRIVATE t81_vm t81_core)

  add_executable(jit_trace_equivalence_test tests/cpp/jit_trace_equivalence_test.cpp)
  target_link_libraries(jit_trace_equivalence_test PRIVATE t81_vm)

  add_executable(jit_tensor_trace_equivalence_test tests/cpp/jit_tensor_trace_equivalence_test.cpp)
  target_link_libraries(jit_tensor_trace_equivalence_test PRIVATE t81_vm)

  add_executable(vm_jit_per_instruction_policy_test tests/cpp/vm_jit_per_instruction_policy_test.cpp)
  target_link_libraries(vm_jit_per_instruction_policy_test PRIVATE t81_vm t81_core)

  add_executable(axion_loop_metadata_test tests/cpp/axion_loop_metadata_test.cpp)
  target_link_libraries(axion_loop_metadata_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_match_metadata_test tests/cpp/axion_match_metadata_test.cpp)
  target_link_libraries(axion_match_metadata_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_enum_guard_test tests/cpp/axion_enum_guard_test.cpp)
  target_link_libraries(axion_enum_guard_test PRIVATE t81_tool_cli t81_isa t81_vm)

add_executable(axion_policy_match_guard_test tests/cpp/axion_policy_match_guard_test.cpp)
target_link_libraries(axion_policy_match_guard_test PRIVATE t81_tool_cli t81_isa t81_vm)

add_executable(axion_policy_gc_trace_test tests/cpp/axion_policy_gc_trace_test.cpp)
target_link_libraries(axion_policy_gc_trace_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_policy_bytecode_test tests/cpp/axion_policy_bytecode_test.cpp)
  target_link_libraries(axion_policy_bytecode_test PRIVATE t81_core t81_axion)

add_executable(axion_policy_segment_event_test tests/cpp/axion_policy_segment_event_test.cpp)
target_link_libraries(axion_policy_segment_event_test PRIVATE t81_isa t81_vm)

  add_executable(axion_policy_invariants_test tests/cpp/axion_policy_invariants_test.cpp)
  target_link_libraries(axion_policy_invariants_test PRIVATE t81_vm t81_core)

  add_executable(axion_policy_allow_deny_determinism_test
                 tests/cpp/axion_policy_allow_deny_determinism_test.cpp)
  target_link_libraries(axion_policy_allow_deny_determinism_test PRIVATE t81_vm t81_core)

  add_executable(axion_policy_conformance_matrix_test
                 tests/cpp/axion_policy_conformance_matrix_test.cpp)
  target_link_libraries(axion_policy_conformance_matrix_test PRIVATE t81_vm t81_core)

  add_executable(axion_instruction_counter_test tests/cpp/axion_instruction_counter_test.cpp)
  target_link_libraries(axion_instruction_counter_test PRIVATE t81_core t81_vm)

  add_executable(e2e_match_expression_test tests/cpp/e2e_match_expression_test.cpp)
  target_link_libraries(e2e_match_expression_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_loop_statement_test tests/cpp/e2e_loop_statement_test.cpp)
  target_link_libraries(e2e_loop_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_advanced_features_test tests/cpp/e2e_advanced_features_test.cpp)
  target_link_libraries(e2e_advanced_features_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_print_runtime_test tests/cpp/e2e_print_runtime_test.cpp)
  target_link_libraries(e2e_print_runtime_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_compile_determinism_test tests/cpp/e2e_compile_determinism_test.cpp)
  target_link_libraries(e2e_compile_determinism_test PRIVATE t81_tool_cli t81_isa t81_vm t81_core)

  add_executable(e2e_match_metadata_determinism_test tests/cpp/e2e_match_metadata_determinism_test.cpp)
  target_link_libraries(e2e_match_metadata_determinism_test PRIVATE t81_tool_cli t81_vm)

  add_executable(e2e_ast_ir_canonical_determinism_test tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp)
  target_link_libraries(e2e_ast_ir_canonical_determinism_test PRIVATE t81_lang_frontend t81_core)

  add_executable(e2e_enum_metadata_determinism_test tests/cpp/e2e_enum_metadata_determinism_test.cpp)
  target_link_libraries(e2e_enum_metadata_determinism_test PRIVATE t81_tool_cli t81_isa)

  add_executable(cli_e2e_test tests/cpp/cli_e2e_test.cpp)
  target_link_libraries(cli_e2e_test PRIVATE t81_tool_cli)

  if(T81_BUILD_FUZZ_TESTS)
    add_executable(t81_frontend_fuzz_test tests/cpp/frontend_fuzz_test.cpp)
    target_link_libraries(t81_frontend_fuzz_test PRIVATE t81_lang_frontend)
  endif()

  add_executable(t81_t81int_test tests/cpp/test_t81int.cpp)
  target_link_libraries(t81_t81int_test PRIVATE t81_core)

  add_executable(t81_t81int_overflow_test tests/cpp/test_t81int_overflow.cpp)
  target_link_libraries(t81_t81int_overflow_test PRIVATE t81_core)

  add_executable(t81_t81int_generic_div_test tests/cpp/test_t81int_generic_div.cpp)
  target_link_libraries(t81_t81int_generic_div_test PRIVATE t81_core)

  add_executable(t81_int_sign_trit_test tests/cpp/test_t81int_sign_trit.cpp)
  target_link_libraries(t81_int_sign_trit_test PRIVATE t81_core)

  add_executable(t81_division_test tests/cpp/test_division.cpp)
  target_link_libraries(t81_division_test PRIVATE t81_core)

  add_executable(t81_float_test tests/cpp/test_T81Float.cpp)
  target_link_libraries(t81_float_test PRIVATE t81_core)

  add_executable(t81_float_deterministic_test tests/cpp/test_T81Float.cpp)
  target_compile_definitions(t81_float_deterministic_test PRIVATE T81_DETERMINISTIC)
  target_link_libraries(t81_float_deterministic_test PRIVATE t81_core)
  add_test(NAME t81_float_deterministic_test COMMAND t81_float_deterministic_test)

  add_executable(t81_bigint_float_conversion_test tests/cpp/test_T81BigInt_Float_conversion.cpp)
  target_link_libraries(t81_bigint_float_conversion_test PRIVATE t81_core)
  add_test(NAME t81_bigint_float_conversion_test COMMAND t81_bigint_float_conversion_test)

  add_executable(t81_float_properties_test tests/cpp/float_properties_test.cpp)
  target_link_libraries(t81_float_properties_test PRIVATE t81_core)

  add_executable(t81_t81int_to_binary_test tests/cpp/test_t81int_to_binary.cpp)
  target_link_libraries(t81_t81int_to_binary_test PRIVATE t81_core)

  add_executable(t81_complex_test tests/cpp/test_T81Complex.cpp)
  target_link_libraries(t81_complex_test PRIVATE t81_core)

  add_executable(t81_prob_test tests/cpp/test_T81Prob.cpp)
  target_link_libraries(t81_prob_test PRIVATE t81_core)

  add_executable(t81_symbol_test tests/cpp/test_T81Symbol.cpp)
  target_link_libraries(t81_symbol_test PRIVATE t81_core)

  add_executable(t81_string_test tests/cpp/test_T81String.cpp)
  target_link_libraries(t81_string_test PRIVATE t81_core)

  add_executable(t81_qutrit_test tests/cpp/test_T81Qutrit.cpp)
  target_link_libraries(t81_qutrit_test PRIVATE t81_core)

  add_executable(t81_fixed_test tests/cpp/test_T81Fixed.cpp)
  target_link_libraries(t81_fixed_test PRIVATE t81_core)

  add_executable(t81_uint_test tests/cpp/test_T81Uint.cpp)
  target_link_libraries(t81_uint_test PRIVATE t81_core)

  add_executable(t81_list_test tests/cpp/test_T81List.cpp)
  target_link_libraries(t81_list_test PRIVATE t81_core)

  add_executable(t81_map_test tests/cpp/test_T81Map.cpp)
  target_link_libraries(t81_map_test PRIVATE t81_core)

  add_executable(t81_set_test tests/cpp/test_T81Set.cpp)
  target_link_libraries(t81_set_test PRIVATE t81_core)

  add_executable(t81_vector_test tests/cpp/test_T81Vector.cpp)
  target_link_libraries(t81_vector_test PRIVATE t81_core)

  add_executable(t81_matrix_test tests/cpp/test_T81Matrix.cpp)
  target_link_libraries(t81_matrix_test PRIVATE t81_core)

  add_executable(t81_matrix_singular_test tests/cpp/test_T81Matrix_singular.cpp)
  target_link_libraries(t81_matrix_singular_test PRIVATE t81_core)

  add_executable(debug_matrix_crash tests/cpp/debug_matrix_crash.cpp)
  target_link_libraries(debug_matrix_crash PRIVATE t81_core)
  add_test(NAME debug_matrix_crash COMMAND debug_matrix_crash)

  add_executable(t81_maybe_test tests/cpp/test_T81Maybe.cpp)
  target_link_libraries(t81_maybe_test PRIVATE t81_core)

  add_executable(t81_result_test tests/cpp/test_T81Result.cpp)
  target_link_libraries(t81_result_test PRIVATE t81_core)

  add_executable(t81_simd_add_helpers_test tests/cpp/test_t81_simd_add_helpers.cpp)
  target_link_libraries(t81_simd_add_helpers_test PRIVATE t81_core)

  add_executable(t81_quaternion_test tests/cpp/test_T81Quaternion.cpp)
  target_link_libraries(t81_quaternion_test PRIVATE t81_core)

  add_executable(t81_time_test tests/cpp/test_T81Time.cpp)
  target_link_libraries(t81_time_test PRIVATE t81_core)

  add_executable(t81_bytes_test tests/cpp/test_T81Bytes.cpp)
  target_link_libraries(t81_bytes_test PRIVATE t81_core)

  add_executable(t81_tree_test tests/cpp/test_T81Tree.cpp)
  target_link_libraries(t81_tree_test PRIVATE t81_core)

  add_executable(t81_native_conversion_test tests/cpp/test_t81_native_conversion.cpp)
  target_link_libraries(t81_native_conversion_test PRIVATE t81_core)

  add_executable(t81_limb_bohemian_add_test tests/cpp/test_t81_limb_bohemian_add.cpp)
  target_link_libraries(t81_limb_bohemian_add_test PRIVATE t81_core)

  add_executable(t81_limb_bohemian_mul_test tests/cpp/test_t81_limb_bohemian_mul.cpp)
  target_link_libraries(t81_limb_bohemian_mul_test PRIVATE t81_core)

  add_executable(t81_limb54_add_test tests/cpp/test_t81_limb54_add.cpp)
  target_link_libraries(t81_limb54_add_test PRIVATE t81_core)

  add_executable(t81_limb54_booth_mul_test tests/cpp/test_t81_limb54_booth_mul.cpp)
  target_link_libraries(t81_limb54_booth_mul_test PRIVATE t81_core)

  add_executable(t81_limb_booth_mul_test tests/cpp/test_t81_limb_booth_mul.cpp)
  target_link_libraries(t81_limb_booth_mul_test PRIVATE t81_core)

  add_executable(t81_limb_mul_wide_high_test tests/cpp/test_t81_limb_mul_wide_high.cpp)
  target_link_libraries(t81_limb_mul_wide_high_test PRIVATE t81_core)

  add_executable(t81_limb_add_test tests/cpp/test_t81_limb_add.cpp)
  target_link_libraries(t81_limb_add_test PRIVATE t81_core)

  add_executable(t81_native_arith_test tests/cpp/test_t81_native_arith.cpp)
  target_link_libraries(t81_native_arith_test PRIVATE t81_core)

  add_executable(t81_bigint_v1_baseline_test tests/cpp/test_bigint_v1_baseline.cpp)
  target_link_libraries(t81_bigint_v1_baseline_test PRIVATE t81_core)

  add_executable(t81_bigint_v1_perf tests/cpp/test_bigint_v1_perf.cpp)
  target_link_libraries(t81_bigint_v1_perf PRIVATE t81_core)

  add_executable(test_bigint_robust_features tests/cpp/test_bigint_robust_features.cpp)
  target_link_libraries(test_bigint_robust_features PRIVATE t81_core)

  add_executable(test_bigint_multilimb_ext tests/cpp/test_bigint_multilimb_ext.cpp)
  target_link_libraries(test_bigint_multilimb_ext PRIVATE t81_core)

  add_executable(test_T81Tensor_transpose tests/cpp/test_T81Tensor_transpose.cpp)
  target_link_libraries(test_T81Tensor_transpose PRIVATE t81_core)

  add_executable(performance_advancements_test tests/cpp/performance_advancements_test.cpp)
  target_link_libraries(performance_advancements_test PRIVATE t81_core t81_axion)

  add_executable(t81_distributed_tensor_test tests/cpp/distributed_tensor_test.cpp)
  target_link_libraries(t81_distributed_tensor_test PRIVATE t81_core)

  add_executable(tensor_backend_parity_test tests/cpp/tensor_backend_parity_test.cpp)
  target_link_libraries(tensor_backend_parity_test PRIVATE t81_core)

  add_executable(bigint_perf_guardrail_test tests/cpp/bigint_perf_guardrail_test.cpp)
  target_link_libraries(bigint_perf_guardrail_test PRIVATE t81_core)

  add_executable(t81_bigint_allocation_pathology_test tests/cpp/test_bigint_allocation_pathology.cpp)
  target_link_libraries(t81_bigint_allocation_pathology_test PRIVATE t81_core)

  add_executable(t81_bigint_allocation_guardrail_test tests/cpp/bigint_allocation_guardrail_test.cpp)
  target_link_libraries(t81_bigint_allocation_guardrail_test PRIVATE t81_core)

  add_executable(t81_vm_tier_promotion_test tests/cpp/vm_tier_promotion_test.cpp)
  target_link_libraries(t81_vm_tier_promotion_test PRIVATE t81_vm t81_core t81_isa)

  add_executable(t81_code_protection_test tests/cpp/test_code_protection.cpp)
  target_link_libraries(t81_code_protection_test PRIVATE t81_vm t81_core)

  enable_testing()
  set(T81_TEST_TARGETS
    t81_float_test
    t81_float_properties_test
    t81_t81int_to_binary_test
    t81_division_test
    t81_t81int_test
    t81_t81int_overflow_test
    t81_t81int_generic_div_test
    t81_int_sign_trit_test
    t81_bigint_test
    t81_fraction_test
    t81_bigint_properties_test
    t81_prob_properties_test
    t81_t81int_properties_test
    t81_bigint_division_edge_properties_test
    t81_bigint_division_semantics_test
    t81_bigint_gcd_divmod_property_test
    t81_bigint_modular_inverse_test
    fuzz_bigint_libfuzzer
    t81_fraction_properties_test
    t81_core_numeric_compat_test
    t81_core_bigint_compat_properties_test
    t81_core_fraction_compat_properties_test
    t81_v1_canonical_numeric_contract_test
    t81_complex_test
    t81_prob_test
    t81_symbol_test
    t81_string_test
    t81_qutrit_test
    t81_fixed_test
    t81_uint_test
    t81_list_test
    t81_map_test
    t81_set_test
    t81_vector_test
    t81_matrix_test
    t81_matrix_singular_test
    t81_maybe_test
    t81_result_test
    t81_quaternion_test
    t81_time_test
    t81_bytes_test
    t81_tree_test
    t81_tensor_transpose_test
    t81_core_improvements_test
    t81_core_features_test
    t81_tensor_slice_test
    t81_tensor_reshape_test
    t81_tensor_loader_test
    t81_canonfs_io_test
    canonfs_read_verify_test
    canonfs_read_verify_env_contract_test
    canonfs_integrity_matrix_test
    t81_ir_encoding_test
    t81_isa_encoding_test
    t81_hash_stub_test
    t81_sha3_test
    t81_axion_stub_test
    t81_codec_base243_test
    t81_codec_base81_test
    t81_trit_packing_test
    t81_packed_trit_vector_test
    t81_phase2c_truth_table_test
    t81_packed_trit_vector_stress_test
    t81_base81_balanced_test
    t81_base81_packed_test
    t81_metadata_test
    t81_isa_base81_view_test
    setun_bridge_test
    tisc_opcode_matrix_test
    t81lang_conformance_baseline_test
    t81_numeric_format_test
    axion_recursion_guardrails_test
    t81_tensor_shape_test
    t81_ternary_arith_test
    t81_tensor_matmul_test
    t81_tensor_reduce_test
    t81_tensor_broadcast_test
    t81_entropy_test
    t81_expected_test
    t81_tensor_elementwise_test
    t81_tensor_ternary_test
    t81_t729_tensor_test
    t81_symbolic_algebra_test
    t81_semantic_graph_test
    t81_neural_primitives_test
    t81_frontend_neural_test
    t81_c_api_bigint_test
    t81_tensor_unary_test
    t81_hanoi_integration_test
    t81_vm_load_store_test
    t81_vm_divmod_test
    t81_vm_illegal_test
    t81_vm_bounds_test
    t81_vm_tensor_test
    t81_vm_tloadhash_conformance_test
    t81_vm_tloadhash_decodefault_determinism_matrix_test
    t81_vm_tensor_shape_faults_test
    t81_vm_tensor_get_set_conformance_test
    t81_vm_tensor_helper_predicates_test
    t81_vm_predispatch_policy_deny_logging_test
    t81_vm_fault_test
    t81_vm_deterministic_fault_test
    t81_vm_fault_family_determinism_matrix_test
    t81_vm_memory_test
    t81_axion_log_determinism_test
    t81_reflection_non_interference_test
    t81_canonfs_driver_test
    t81_weights_load_test
    t81_vm_trace_test
    t81_vm_determinism_property_test
    t81_vm_state_transition_invariants_test
    t81_vm_state_transition_conformance_matrix_test
    t81_vm_workload_determinism_tiers_test
    t81_vm_mixed_workload_conformance_matrix_test
    t81_vm_policy_parse_fail_closed_test
    t81_vm_axreport_policy_deny_fail_closed_test
    t81_vm_system_registers_deterministic_test
    t81_vm_stubbed_opcode_fail_closed_test
    vm_extended_ops_test
    llama_kernels_test
    t81_vm_jump_flags_test
    t81_vm_float_fraction_ops_test
    t81_vm_literal_pool_extension_test
    t81_vm_neg_jumps_test
    t81_vm_print_test
    t81_limb_bohemian_add_test
    t81_limb_bohemian_mul_test
    t81_limb_booth_mul_test
    t81_limb_mul_wide_high_test
    t81_limb_add_test
    t81_native_arith_test
    t81_bigint_v1_baseline_test
    t81_limb54_add_test
    t81_limb54_booth_mul_test
    t81_frontend_lexer_test
    t81_frontend_parser_test
    t81_spec_compliance_test
    tisc_semantic_equivalence_test
    tisc_opcode_family_semantics_test
    t81_frontend_expression_features_test
    t81_frontend_parser_generics_test
    t81_frontend_parser_recovery_test
    t81_frontend_parser_legacy_rejection_test
    t81_frontend_ir_generator_test
    t81_frontend_logical_lowering_test
    t81_semantic_analyzer_option_result_test
    test_std_option_result
    t81_semantic_analyzer_match_test
    t81_semantic_analyzer_generic_test
    t81_semantic_analyzer_indexing_test
    t81_semantic_analyzer_vector_literal_test
    t81_semantic_analyzer_equality_test
    t81_semantic_analyzer_loop_test
    t81_semantic_analyzer_numeric_test
    t81_semantic_analyzer_diagnostic_location_test
    t81_semantic_analyzer_cascade_suppression_test
    t81_semantic_analyzer_diagnostic_precision_test
    t81_isa_binary_emitter_test
    t81_isa_type_alias_io_test
    t81_isa_binary_io_determinism_test
    t81_isa_binary_metadata_roundtrip_property_test
    e2e_option_result_test
    e2e_option_result_function_test
    e2e_option_type_test
    e2e_match_expression_test
    e2e_loop_statement_test
    e2e_advanced_features_test
    e2e_print_runtime_test
    e2e_compile_determinism_test
    e2e_match_metadata_determinism_test
    e2e_ast_ir_canonical_determinism_test
    e2e_enum_metadata_determinism_test
    cli_e2e_test
    t81_cli_check_test
    t81_cli_pipeline_test
    t81_cli_disasm_test
    t81_cli_debugger_test
    t81_cli_option_result_test
    t81_cli_diagnostic_context_test
    t81_cli_trace_export_test
    cli_canonize_tensor_test
    axion_policy_match_guard_test
    axion_policy_segment_event_test
    axion_policy_invariants_test
    axion_policy_allow_deny_determinism_test
    axion_policy_conformance_matrix_test
    axion_loop_metadata_test
    axion_match_metadata_test
    axion_enum_guard_test
    axion_nested_guard_test
    axion_instruction_counter_test
    canonfs_axion_trace_test
    t81_tier4_test
    t81_tier5_test
    t81_infinite_opcodes_test
    t81_tier4_vm_test
    t81_tier4_distributed_test
    t81_jit_test
    jit_trace_equivalence_test
    jit_tensor_trace_equivalence_test
    vm_jit_per_instruction_policy_test
    e2e_axion_trace_test
    axion_policy_bytecode_test
    test_bigint_multilimb_ext
    test_bigint_robust_features
    t81_bigint_v1_perf
    bigint_perf_guardrail_test
    t81_bigint_allocation_pathology_test
    t81_bigint_allocation_guardrail_test
    t81_vm_tier_promotion_test
    test_T81Tensor_transpose
    performance_advancements_test
    t81_distributed_tensor_test
    tensor_backend_parity_test
  )
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND T81_TEST_TARGETS t81_base81_simd_test)
  endif()
  if(T81_BUILD_FUZZ_TESTS AND T81_RUN_FRONTEND_FUZZ_IN_CTEST)
    list(APPEND T81_TEST_TARGETS t81_frontend_fuzz_test)
  endif()
  foreach(tgt IN LISTS T81_TEST_TARGETS)
    add_test(NAME ${tgt} COMMAND $<TARGET_FILE:${tgt}>)
  endforeach()
  add_test(NAME t81_cli_repro_hash_test COMMAND $<TARGET_FILE:cli_repro_hash_test> $<TARGET_FILE:t81>)
  add_test(NAME t81_cli_contract_test_runner COMMAND $<TARGET_FILE:t81_cli_contract_test> $<TARGET_FILE:t81>)
  add_test(NAME t81_cli_stdlib_fixtures_test COMMAND $<TARGET_FILE:cli_stdlib_fixtures_test>)
  add_test(NAME cli_std_collections_determinism_test COMMAND $<TARGET_FILE:cli_std_collections_determinism_test>)
  add_test(
    NAME t81_legacy_core_numeric_include_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_core_numeric_includes_test.py
  )
  set_tests_properties(t81_legacy_core_numeric_include_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_legacy_core_numeric_type_usage_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_core_numeric_type_usage_test.py
  )
  set_tests_properties(t81_legacy_core_numeric_type_usage_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_legacy_v1_numeric_include_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_v1_numeric_includes_test.py
  )
  set_tests_properties(t81_legacy_v1_numeric_include_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_core_numeric_wrapper_thinness_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_core_numeric_wrapper_thinness_test.py
  )
  set_tests_properties(t81_core_numeric_wrapper_thinness_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_v1_canonical_numeric_alias_usage_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_v1_canonical_numeric_alias_usage_test.py
  )
  set_tests_properties(t81_v1_canonical_numeric_alias_usage_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()

# =============================================================================
# AI Experiments
# =============================================================================
if(T81_ENABLE_AI_EXPERIMENTS)
    message(STATUS "Building AI experiments")
    add_subdirectory(experiments/ai)
endif()

# =============================================================================
# Python Bindings
# =============================================================================
execute_process(
    COMMAND python3 -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Llama adapter library (updated for Phase 3)
add_library(t81_llama_adapter STATIC
  tooling/model/llama_cpp_adapter_working.cpp
  src/codec/gguf_parser.cpp
  src/codec/ternary_gguf.cpp
  src/axion/policy_engine.cpp
)

# Link libraries
target_link_libraries(t81_enhanced_codec PUBLIC t81_core t81_isa)
target_link_libraries(t81_memory PUBLIC t81_core t81_isa)
target_link_libraries(t81_inference PUBLIC t81_core t81_isa t81_enhanced_codec t81_memory)
target_link_libraries(t81_monitoring PUBLIC t81_core t81_isa)
target_link_libraries(t81_llama_adapter PUBLIC t81_core t81_isa t81_enhanced_codec t81_memory ggml::ggml)

# Include directories
target_include_directories(t81_enhanced_codec PUBLIC include)
target_include_directories(t81_memory PUBLIC include)
target_include_directories(t81_inference PUBLIC include)
target_include_directories(t81_monitoring PUBLIC include)
target_include_directories(t81_llama_adapter PUBLIC include tooling/model third_party/llama.cpp)

# Phase 3 Integration Example
add_executable(phase3_integration_demo
  examples/ai-integration/phase3_complete_integration_demo.cpp
)

target_link_libraries(phase3_integration_demo 
  PUBLIC 
  t81_core 
  t81_isa 
  t81_enhanced_codec 
  t81_memory 
  t81_inference 
  t81_monitoring 
  t81_llama_adapter
  ggml::ggml
)

# AI integration examples
add_executable(llama_runner_real examples/ai-integration/llama_real_simple_restored.cpp)
target_link_libraries(llama_runner_real PRIVATE t81_core t81_isa t81_llama_adapter)

add_executable(llama_cpp_governed_demo examples/llama_cpp_governed_demo.cpp)
target_link_libraries(llama_cpp_governed_demo PRIVATE t81_core t81_isa t81_llama_adapter)

# Installation
install(TARGETS t81_core t81_isa t81_llama_adapter llama_runner_real)
install(DIRECTORY include/ DESTINATION include/t81)
