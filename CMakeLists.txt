cmake_minimum_required(VERSION 3.16)
project(t81 LANGUAGES C CXX)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)
option(T81_BUILD_BENCHMARKS "Build T81 benchmark suite" ON)

# AVX2 only on real x86_64 (never on Apple Silicon) and only for GCC/Clang.
if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_compile_options(-mavx2 -mfma)
endif()

# Global compiler flags (ANSI warnings for GCC/Clang; use /W4 for MSVC)
if(MSVC)
    add_compile_options(/W4 /O2 /DNDEBUG)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -DNDEBUG)
endif()

if(MSVC)
    add_compile_options(/wd4244 /wd4307 /wd4018 /wd4101 /wd4189)
endif()

# =============================================================================
# Core library
# =============================================================================
add_library(t81_core STATIC
  src/core/bigint.cpp
  src/core/fraction.cpp
  src/tisc/encoding.cpp
  src/vm/vm.cpp
  src/canonfs/in_memory_driver.cpp
  src/hanoi/error.cpp
  src/hanoi/in_memory_kernel.cpp        # ← fixed path
  src/axion/engine.cpp
  src/tools/weights.cpp
  src/tools/weights_t81w_emitter.cpp
  src/cog/promotion.cpp
  src/codec/base243.cpp
  src/codec/base81.cpp
  src/bigint/divmod.cpp
  src/bigint/gcd.cpp
  src/hash/canonhash81.cpp
  src/crypto/sha3.cpp
)
target_compile_features(t81_core PUBLIC cxx_std_20)
target_include_directories(t81_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# =============================================================================
# Other libraries
# =============================================================================
add_library(t81_io STATIC src/io/tensor_loader.cpp)
target_link_libraries(t81_io PUBLIC t81_core)
target_include_directories(t81_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(t81_c_api STATIC src/c_api/t81_c_api.cpp)
target_link_libraries(t81_c_api PUBLIC t81_core)
target_include_directories(t81_c_api
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(t81_frontend STATIC
  src/frontend/lexer.cpp
  src/frontend/parser.cpp
  src/frontend/semantic_analyzer.cpp
  src/frontend/symbol_table.cpp
)
target_link_libraries(t81_frontend PUBLIC t81_core)
target_include_directories(t81_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(t81_tisc STATIC
  src/tisc/pretty_printer.cpp
  src/tisc/binary_emitter.cpp
  src/tisc/binary_io.cpp
)
target_link_libraries(t81_tisc PUBLIC t81_core)
target_include_directories(t81_tisc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(t81_vm INTERFACE)
target_include_directories(t81_vm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(t81_vm INTERFACE t81_core)

add_library(t81_llvm INTERFACE)
target_include_directories(t81_llvm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(t81_llvm INTERFACE t81_core)

# =============================================================================
# CLI & Examples
# =============================================================================
add_library(t81_cli_driver STATIC src/cli/driver.cpp)
target_link_libraries(t81_cli_driver PUBLIC t81_frontend t81_tisc t81_vm)
target_include_directories(t81_cli_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(t81 src/cli/main.cpp)
target_link_libraries(t81 PRIVATE t81_frontend t81_tisc t81_vm t81_cli_driver)

if(T81_BUILD_EXAMPLES)
add_executable(t81_demo examples/demo.cpp)
target_link_libraries(t81_demo PRIVATE t81_core)

add_executable(t81_tensor_ops examples/tensor_ops.cpp)
target_link_libraries(t81_tensor_ops PRIVATE t81_core)

add_executable(t81_ir_roundtrip examples/ir_roundtrip.cpp)
target_link_libraries(t81_ir_roundtrip PRIVATE t81_core)

add_executable(axion_demo examples/axion_demo.cpp)
target_link_libraries(axion_demo PRIVATE t81_core)

endif()

# =============================================================================
# All tests — exactly as you wrote them
# =============================================================================
if(T81_BUILD_TESTS)
  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81_core)

  add_executable(t81_fraction_test tests/cpp/fraction_roundtrip.cpp)
  target_link_libraries(t81_fraction_test PRIVATE t81_core)

  add_executable(t81_tensor_transpose_test tests/cpp/tensor_transpose_test.cpp)
  target_link_libraries(t81_tensor_transpose_test PRIVATE t81_core)

  add_executable(t81_tensor_slice_test tests/cpp/tensor_slice_test.cpp)
  target_link_libraries(t81_tensor_slice_test PRIVATE t81_core)

  add_executable(t81_tensor_reshape_test tests/cpp/tensor_reshape_test.cpp)
  target_link_libraries(t81_tensor_reshape_test PRIVATE t81_core)

  add_executable(t81_tensor_loader_test tests/cpp/tensor_loader_test.cpp)
  target_link_libraries(t81_tensor_loader_test PRIVATE t81_io)

  add_executable(t81_weights_quantize_test tests/cpp/weights_quantize_test.cpp)
  target_link_libraries(t81_weights_quantize_test PRIVATE t81_core)

  add_executable(t81_canonfs_io_test tests/cpp/canonfs_io_test.cpp)
  target_link_libraries(t81_canonfs_io_test PRIVATE t81_core)

  add_executable(t81_ir_encoding_test tests/cpp/ir_encoding_test.cpp)
  target_link_libraries(t81_ir_encoding_test PRIVATE t81_core)

  add_executable(t81_tisc_encoding_test tests/cpp/tisc_encoding_test.cpp)
  target_link_libraries(t81_tisc_encoding_test PRIVATE t81_core)

  add_executable(t81_vm_tensor_test tests/cpp/vm_tensor_test.cpp)
  target_link_libraries(t81_vm_tensor_test PRIVATE t81_core)

  add_executable(t81_vm_fault_test tests/cpp/vm_fault_test.cpp)
  target_link_libraries(t81_vm_fault_test PRIVATE t81_core)
  add_executable(t81_vm_memory_test tests/cpp/vm_memory_test.cpp)
  target_link_libraries(t81_vm_memory_test PRIVATE t81_core)

  add_executable(t81_weights_load_test tests/cpp/weights_load_test.cpp)
  target_link_libraries(t81_weights_load_test PRIVATE t81_core)

  add_executable(t81_hash_stub_test tests/cpp/hash_stub_test.cpp)
  target_link_libraries(t81_hash_stub_test PRIVATE t81_core)

  add_executable(t81_axion_stub_test tests/cpp/axion_stub_test.cpp)
  target_link_libraries(t81_axion_stub_test PRIVATE t81_core)

  add_executable(t81_codec_base243_test tests/cpp/codec_base243_test.cpp)
  target_link_libraries(t81_codec_base243_test PRIVATE t81_core)
  add_executable(t81_codec_base81_test tests/cpp/codec_base81_test.cpp)
  target_link_libraries(t81_codec_base81_test PRIVATE t81_core)

  add_executable(t81_tensor_shape_test tests/cpp/tensor_shape_test.cpp)
  target_link_libraries(t81_tensor_shape_test PRIVATE t81_core)

  add_executable(t81_ternary_arith_test tests/cpp/ternary_arith_test.cpp)
  target_link_libraries(t81_ternary_arith_test PRIVATE t81_core)

  add_executable(t81_tensor_matmul_test tests/cpp/tensor_matmul_test.cpp)
  target_link_libraries(t81_tensor_matmul_test PRIVATE t81_core)

  add_executable(t81_tensor_reduce_test tests/cpp/tensor_reduce_test.cpp)
  target_link_libraries(t81_tensor_reduce_test PRIVATE t81_core)

  add_executable(t81_tensor_broadcast_test tests/cpp/tensor_broadcast_test.cpp)
  target_link_libraries(t81_tensor_broadcast_test PRIVATE t81_core)

  add_executable(t81_entropy_test tests/cpp/entropy_test.cpp)
  target_link_libraries(t81_entropy_test PRIVATE t81_core)

  add_executable(t81_tensor_elementwise_test tests/cpp/tensor_elementwise_test.cpp)
  target_link_libraries(t81_tensor_elementwise_test PRIVATE t81_core)

  add_executable(t81_c_api_bigint_test tests/cpp/c_api_bigint_test.c)
  target_link_libraries(t81_c_api_bigint_test PRIVATE t81_c_api)
  target_include_directories(t81_c_api_bigint_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_executable(t81_tensor_unary_test tests/cpp/tensor_unary_test.cpp)
  target_link_libraries(t81_tensor_unary_test PRIVATE t81_core)

  add_executable(t81_hanoi_integration_test tests/cpp/hanoi_integration_test.cpp)
  target_link_libraries(t81_hanoi_integration_test PRIVATE t81_core)

  add_executable(t81_vm_load_store_test tests/cpp/vm_load_store_test.cpp)
  target_link_libraries(t81_vm_load_store_test PRIVATE t81_core)

  add_executable(t81_vm_divmod_test tests/cpp/vm_divmod_test.cpp)
  target_link_libraries(t81_vm_divmod_test PRIVATE t81_core)

  add_executable(t81_vm_illegal_test tests/cpp/vm_illegal_test.cpp)
  target_link_libraries(t81_vm_illegal_test PRIVATE t81_core)

  add_executable(t81_vm_bounds_test tests/cpp/vm_bounds_test.cpp)
  target_link_libraries(t81_vm_bounds_test PRIVATE t81_core)

  add_executable(t81_canonfs_driver_test tests/cpp/canonfs_driver_test.cpp)
  target_link_libraries(t81_canonfs_driver_test PRIVATE t81_core)

  add_executable(t81_vm_trace_test tests/cpp/vm_trace_test.cpp)
  target_link_libraries(t81_vm_trace_test PRIVATE t81_core)

  add_executable(t81_vm_jump_flags_test tests/cpp/vm_jump_flags_test.cpp)
  target_link_libraries(t81_vm_jump_flags_test PRIVATE t81_core)

  add_executable(t81_vm_float_fraction_ops_test tests/cpp/vm_float_fraction_ops_test.cpp)
  target_link_libraries(t81_vm_float_fraction_ops_test PRIVATE t81_core)

  add_executable(t81_vm_neg_jumps_test tests/cpp/vm_neg_jumps_test.cpp)
  target_link_libraries(t81_vm_neg_jumps_test PRIVATE t81_core)

  add_executable(t81_frontend_lexer_test tests/cpp/frontend_lexer_test.cpp)
  target_link_libraries(t81_frontend_lexer_test PRIVATE t81_frontend)

  add_executable(t81_frontend_parser_test tests/cpp/frontend_parser_test.cpp)
  target_link_libraries(t81_frontend_parser_test PRIVATE t81_frontend)

  add_executable(t81_frontend_parser_generics_test tests/cpp/frontend_parser_generics_test.cpp)
  target_link_libraries(t81_frontend_parser_generics_test PRIVATE t81_frontend)

  add_executable(t81_frontend_parser_legacy_rejection_test tests/cpp/frontend_parser_legacy_rejection_test.cpp)
  target_link_libraries(t81_frontend_parser_legacy_rejection_test PRIVATE t81_frontend)

  add_executable(t81_frontend_ir_generator_test tests/cpp/frontend_ir_generator_test.cpp)
  target_link_libraries(t81_frontend_ir_generator_test PRIVATE t81_frontend t81_tisc)

  add_executable(t81_semantic_analyzer_option_result_test tests/cpp/semantic_analyzer_option_result_test.cpp)
  target_link_libraries(t81_semantic_analyzer_option_result_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_match_test tests/cpp/semantic_analyzer_match_test.cpp)
  target_link_libraries(t81_semantic_analyzer_match_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_generic_test tests/cpp/semantic_analyzer_generic_test.cpp)
  target_link_libraries(t81_semantic_analyzer_generic_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_vector_literal_test tests/cpp/semantic_analyzer_vector_literal_test.cpp)
  target_link_libraries(t81_semantic_analyzer_vector_literal_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_record_enum_test tests/cpp/semantic_analyzer_record_enum_test.cpp)
  target_link_libraries(t81_semantic_analyzer_record_enum_test PRIVATE t81_frontend)

  add_executable(t81_tisc_binary_emitter_test tests/cpp/tisc_binary_emitter_test.cpp)
  target_link_libraries(t81_tisc_binary_emitter_test PRIVATE t81_tisc)

  add_executable(t81_tisc_type_alias_io_test tests/cpp/tisc_type_alias_io_test.cpp)
  target_link_libraries(t81_tisc_type_alias_io_test PRIVATE t81_tisc)

  add_executable(t81_semantic_analyzer_loop_test tests/cpp/semantic_analyzer_loop_test.cpp)
  target_link_libraries(t81_semantic_analyzer_loop_test PRIVATE t81_frontend)

  add_executable(ir_inspector tools/ir_inspector.cpp)
  target_link_libraries(ir_inspector PRIVATE t81_frontend)

  add_executable(e2e_let_statement_test tests/cpp/e2e_let_statement_test.cpp)
  target_link_libraries(e2e_let_statement_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_arithmetic_test tests/cpp/e2e_arithmetic_test.cpp)
  target_link_libraries(e2e_arithmetic_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_if_statement_test tests/cpp/e2e_if_statement_test.cpp)
  target_link_libraries(e2e_if_statement_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_option_result_test tests/cpp/e2e_option_result_test.cpp)
  target_link_libraries(e2e_option_result_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(t81_cli_pipeline_test tests/cpp/cli_pipeline_test.cpp)
  target_link_libraries(t81_cli_pipeline_test PRIVATE t81_cli_driver)

  add_executable(t81_cli_option_result_test tests/cpp/cli_option_result_test.cpp)
  target_link_libraries(t81_cli_option_result_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_cli_structural_types_test tests/cpp/cli_structural_types_test.cpp)
  target_link_libraries(t81_cli_structural_types_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_cli_record_enum_test tests/cpp/cli_record_enum_test.cpp)
  target_link_libraries(t81_cli_record_enum_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(axion_loop_metadata_test tests/cpp/axion_loop_metadata_test.cpp)
  target_link_libraries(axion_loop_metadata_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(axion_instruction_counter_test tests/cpp/axion_instruction_counter_test.cpp)
  target_link_libraries(axion_instruction_counter_test PRIVATE t81_core t81_vm)

  add_executable(t81_t81int_test tests/cpp/test_t81int.cpp)
  target_link_libraries(t81_t81int_test PRIVATE t81_core)

  add_executable(t81_division_test tests/cpp/test_division.cpp)
  target_link_libraries(t81_division_test PRIVATE t81_core)

  add_executable(t81_float_test tests/cpp/test_T81Float.cpp)
  target_link_libraries(t81_float_test PRIVATE t81_core)

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

  enable_testing()
  set(T81_TEST_TARGETS
    t81_float_test
    t81_t81int_to_binary_test
    t81_division_test
    t81_t81int_test
    t81_bigint_test
    t81_fraction_test
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
    t81_maybe_test
    t81_result_test
    t81_quaternion_test
    t81_time_test
    t81_bytes_test
    t81_tree_test
    t81_tensor_transpose_test
    t81_tensor_slice_test
    t81_tensor_reshape_test
    t81_tensor_loader_test
    t81_canonfs_io_test
    t81_ir_encoding_test
    t81_tisc_encoding_test
    t81_hash_stub_test
    t81_axion_stub_test
    t81_codec_base243_test
    t81_codec_base81_test
    t81_tensor_shape_test
    t81_ternary_arith_test
    t81_tensor_matmul_test
    t81_tensor_reduce_test
    t81_tensor_broadcast_test
    t81_entropy_test
    t81_tensor_elementwise_test
    t81_c_api_bigint_test
    t81_tensor_unary_test
    t81_hanoi_integration_test
    t81_vm_load_store_test
    t81_vm_divmod_test
    t81_vm_illegal_test
    t81_vm_bounds_test
    t81_vm_tensor_test
    t81_vm_fault_test
    t81_vm_memory_test
    t81_canonfs_driver_test
    t81_weights_load_test
    t81_vm_trace_test
    t81_vm_jump_flags_test
    t81_vm_float_fraction_ops_test
    t81_vm_neg_jumps_test
    t81_limb_bohemian_add_test
    t81_limb_bohemian_mul_test
    t81_limb_booth_mul_test
    t81_limb_mul_wide_high_test
    t81_limb_add_test
    t81_native_arith_test
    t81_limb54_add_test
    t81_limb54_booth_mul_test
    t81_frontend_lexer_test
    t81_frontend_parser_test
    t81_frontend_parser_generics_test
    t81_frontend_parser_legacy_rejection_test
    t81_frontend_ir_generator_test
    t81_semantic_analyzer_option_result_test
    t81_semantic_analyzer_match_test
    t81_semantic_analyzer_generic_test
    t81_semantic_analyzer_vector_literal_test
    t81_semantic_analyzer_loop_test
    t81_tisc_binary_emitter_test
    t81_tisc_type_alias_io_test
    e2e_option_result_test
    t81_cli_pipeline_test
    t81_cli_option_result_test
    axion_loop_metadata_test
    axion_instruction_counter_test
  )
  foreach(tgt IN LISTS T81_TEST_TARGETS)
    add_test(NAME ${tgt} COMMAND $<TARGET_FILE:${tgt}>)
  endforeach()
endif()

# =============================================================================
# Benchmarks
# =============================================================================
if(T81_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# =============================================================================
# Doxygen
# =============================================================================
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif()
