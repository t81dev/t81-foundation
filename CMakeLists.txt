cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
project(t81 LANGUAGES C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)
option(T81_BUILD_BENCHMARKS "Build T81 benchmark suite" ON)
option(T81_BUILD_FUZZ_TESTS "Build frontend fuzz target" ON)
option(T81_RUN_FRONTEND_FUZZ_IN_CTEST "Run frontend fuzz target via ctest matrix" OFF)
option(T81_ENABLE_ASAN "Enable AddressSanitizer for supported compilers" OFF)
option(T81_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for supported compilers" OFF)
option(T81_USE_CXX23 "Build with C++23 language mode (fallback: C++20)" ON)

if(T81_USE_CXX23)
  set(T81_CXX_STD_FEATURE cxx_std_23)
else()
  set(T81_CXX_STD_FEATURE cxx_std_20)
endif()

# CPU feature gating used for SIMD source/flags.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" T81_SYSTEM_PROCESSOR_LOWER)
set(T81_IS_X86_64 OFF)
if(T81_SYSTEM_PROCESSOR_LOWER STREQUAL "x86_64" OR T81_SYSTEM_PROCESSOR_LOWER STREQUAL "amd64")
  set(T81_IS_X86_64 ON)
endif()

# Global compiler flags (ANSI warnings for GCC/Clang; use /W4 for MSVC)
if(MSVC)
    add_compile_options(/W4 /O2 /DNDEBUG /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -DNDEBUG)
endif()

if(MSVC)
    add_compile_options(/wd4244 /wd4307 /wd4018 /wd4101 /wd4189)
endif()

if(NOT MSVC)
  set(T81_SANITIZER_FLAGS "")
  if(T81_ENABLE_ASAN)
    list(APPEND T81_SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
    add_compile_definitions(T81_ASAN_ENABLED)
  endif()
  if(T81_ENABLE_UBSAN)
    list(APPEND T81_SANITIZER_FLAGS -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  endif()
  if(T81_SANITIZER_FLAGS)
    add_compile_options(${T81_SANITIZER_FLAGS})
    add_link_options(${T81_SANITIZER_FLAGS})
    message(STATUS "Enabled sanitizers: ${T81_SANITIZER_FLAGS}")
  endif()
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
  src/canonfs/persistent_driver.cpp
  src/canonfs/axion_hook.cpp
  src/hanoi/error.cpp
  src/hanoi/in_memory_kernel.cpp        # ← fixed path
  src/axion/axion_api.cpp
  src/axion/engine.cpp
  src/axion/policy_engine.cpp
  src/axion/policy_serialization.cpp
  src/tools/weights.cpp
  src/tools/weights_t81w_emitter.cpp
  src/cog/promotion.cpp
  src/cog/tier1/symbolic.cpp
  src/cog/tier2/reflective.cpp
  src/cog/tier3/recursive.cpp
  src/cog/tier4/tier4_loop.cpp
  src/cog/tier4/distributed.cpp
  src/cog/tier5/infinite.cpp
  src/codec/base243.cpp
  src/codec/base81.cpp
  src/codec/base81_balanced.cpp
  src/codec/base81_packed.cpp
  src/codec/metadata.cpp
  src/codec/trit_packing.cpp
  src/vm/jit_compiler.cpp
  src/hash/canonhash81.cpp
  src/crypto/sha3.cpp
  src/setun/bridge.cpp
)
target_compile_features(t81_core PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(T81_IS_X86_64)
  set(T81_AVX2_SOURCE src/simd/base81_digits_avx2.cpp)
  target_sources(t81_core PRIVATE ${T81_AVX2_SOURCE})
  if(NOT MSVC)
    set_source_files_properties(${T81_AVX2_SOURCE} PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
  endif()
endif()

# =============================================================================
# Other libraries
# =============================================================================
add_library(t81_io STATIC src/io/tensor_loader.cpp)
target_link_libraries(t81_io PUBLIC t81_core)
target_include_directories(t81_io PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_c_api STATIC src/c_api/t81_c_api.cpp)
target_link_libraries(t81_c_api PUBLIC t81_core)
target_include_directories(t81_c_api
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(t81_frontend STATIC
  src/frontend/lexer.cpp
  src/frontend/parser.cpp
  src/frontend/semantic_analyzer.cpp
  src/frontend/symbol_table.cpp
)
target_link_libraries(t81_frontend PUBLIC t81_core)
target_include_directories(t81_frontend PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_tisc STATIC
  src/tisc/pretty_printer.cpp
  src/tisc/binary_emitter.cpp
  src/tisc/binary_io.cpp
  src/tisc/base81_view.cpp
)
target_link_libraries(t81_tisc PUBLIC t81_core)
target_include_directories(t81_tisc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_vm INTERFACE)
target_include_directories(t81_vm INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_vm INTERFACE t81_core)

add_library(t81_llvm INTERFACE)
target_include_directories(t81_llvm INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_llvm INTERFACE t81_core)

# =============================================================================
# CLI & Examples
# =============================================================================
add_library(t81_cli_driver STATIC
  src/cli/driver.cpp
  src/cli/debugger.cpp
  src/cli/canonize_tensor.cpp
)
target_link_libraries(t81_cli_driver PUBLIC t81_frontend t81_tisc t81_vm)
target_include_directories(t81_cli_driver PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

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

add_executable(axion_guard_trace examples/axion_guard_trace.cpp)
target_link_libraries(axion_guard_trace PRIVATE t81_cli_driver t81_vm)

add_executable(axion_policy_trace examples/axion_policy_trace.cpp)
target_link_libraries(axion_policy_trace PRIVATE t81_cli_driver t81_vm)

add_executable(axion_heap_compaction_trace_test tests/cpp/axion_heap_compaction_trace_test.cpp)
target_link_libraries(axion_heap_compaction_trace_test PRIVATE t81_cli_driver t81_tisc t81_vm)

add_executable(axion_policy_runner examples/axion_policy_runner.cpp)
target_link_libraries(axion_policy_runner PRIVATE t81_cli_driver t81_vm)

add_executable(llama32_demo examples/llama32_demo.cpp)
target_link_libraries(llama32_demo PRIVATE t81_cli_driver t81_vm)

add_executable(canonfs_trace_demo examples/canonfs_trace_demo.cpp)
target_link_libraries(canonfs_trace_demo PRIVATE t81_core)

add_executable(axion_nested_guard_test tests/cpp/axion_nested_guard_test.cpp)
target_link_libraries(axion_nested_guard_test PRIVATE t81_cli_driver t81_vm)

add_executable(e2e_axion_trace_test tests/cpp/e2e_axion_trace_test.cpp)
target_link_libraries(e2e_axion_trace_test PRIVATE t81_cli_driver t81_vm)

add_executable(complex_demo examples/complex_demo.cpp)
target_link_libraries(complex_demo PRIVATE t81_core asio)

endif()

# =============================================================================
# All tests — exactly as you wrote them
# =============================================================================
if(T81_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
  )
  FetchContent_GetProperties(asio)
  if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
    # Define asio as an interface library if not already defined
    if(NOT TARGET asio)
      add_library(asio INTERFACE)
      target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
      target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
    endif()
  endif()

  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81_core)

  add_executable(t81_fraction_test tests/cpp/fraction_roundtrip.cpp)
  target_link_libraries(t81_fraction_test PRIVATE t81_core)

  add_executable(t81_bigint_properties_test tests/cpp/bigint_properties_test.cpp)
  target_link_libraries(t81_bigint_properties_test PRIVATE t81_core)

  add_executable(t81_prob_properties_test tests/cpp/prob_properties_test.cpp)
  target_link_libraries(t81_prob_properties_test PRIVATE t81_core)

  add_executable(t81_float_arithmetic_test tests/cpp/test_T81Float_arithmetic.cpp)
  target_link_libraries(t81_float_arithmetic_test PRIVATE t81_core)

  add_executable(t81_t81int_properties_test tests/cpp/t81int_properties_test.cpp)
  target_link_libraries(t81_t81int_properties_test PRIVATE t81_core)

  add_executable(t81_bigint_division_edge_properties_test tests/cpp/bigint_division_edge_properties_test.cpp)
  target_link_libraries(t81_bigint_division_edge_properties_test PRIVATE t81_core)

  add_executable(t81_bigint_division_semantics_test tests/cpp/bigint_division_semantics_test.cpp)
  target_link_libraries(t81_bigint_division_semantics_test PRIVATE t81_core)

  add_executable(t81_bigint_gcd_divmod_property_test tests/cpp/bigint_gcd_divmod_property_test.cpp)
  target_link_libraries(t81_bigint_gcd_divmod_property_test PRIVATE t81_core)

  add_executable(t81_bigint_mod_test tests/cpp/test_T81BigInt_mod.cpp)
  target_link_libraries(t81_bigint_mod_test PRIVATE t81_core)

  add_executable(t81_bigint_sqrt_test tests/cpp/test_T81BigInt_sqrt.cpp)
  target_link_libraries(t81_bigint_sqrt_test PRIVATE t81_core)

  add_executable(fuzz_bigint_div tests/cpp/fuzz_bigint_div.cpp)
  target_link_libraries(fuzz_bigint_div PRIVATE t81_core)

  add_executable(t81_fraction_properties_test tests/cpp/fraction_properties_test.cpp)
  target_link_libraries(t81_fraction_properties_test PRIVATE t81_core)

  add_executable(t81_core_numeric_compat_test tests/cpp/core_numeric_compat_test.cpp)
  target_link_libraries(t81_core_numeric_compat_test PRIVATE t81_core)

  add_executable(t81_core_bigint_compat_properties_test tests/cpp/core_bigint_compat_properties_test.cpp)
  target_link_libraries(t81_core_bigint_compat_properties_test PRIVATE t81_core)

  add_executable(t81_core_fraction_compat_properties_test tests/cpp/core_fraction_compat_properties_test.cpp)
  target_link_libraries(t81_core_fraction_compat_properties_test PRIVATE t81_core)

  add_executable(t81_v1_canonical_numeric_contract_test tests/cpp/v1_canonical_numeric_contract_test.cpp)
  target_link_libraries(t81_v1_canonical_numeric_contract_test PRIVATE t81_core)

  add_executable(t81_tensor_transpose_test tests/cpp/tensor_transpose_test.cpp)
  target_link_libraries(t81_tensor_transpose_test PRIVATE t81_core)

  add_executable(t81_core_improvements_test tests/cpp/test_core_improvements.cpp)
  target_link_libraries(t81_core_improvements_test PRIVATE t81_core)

  add_executable(t81_core_features_test tests/cpp/test_core_features.cpp)
  target_link_libraries(t81_core_features_test PRIVATE t81_core)

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

  add_executable(canonfs_axion_trace_test tests/cpp/canonfs_axion_trace_test.cpp)
  target_link_libraries(canonfs_axion_trace_test PRIVATE t81_core)

  add_executable(canonfs_persistent_driver_test tests/cpp/canonfs_persistent_driver_test.cpp)
  target_link_libraries(canonfs_persistent_driver_test PRIVATE t81_core)

  add_executable(t81_ir_encoding_test tests/cpp/ir_encoding_test.cpp)
  target_link_libraries(t81_ir_encoding_test PRIVATE t81_core)

  add_executable(t81_tisc_encoding_test tests/cpp/tisc_encoding_test.cpp)
  target_link_libraries(t81_tisc_encoding_test PRIVATE t81_core)

  add_executable(t81_vm_tensor_test tests/cpp/vm_tensor_test.cpp)
  target_link_libraries(t81_vm_tensor_test PRIVATE t81_core)

  add_executable(t81_vm_fault_test tests/cpp/vm_fault_test.cpp)
  target_link_libraries(t81_vm_fault_test PRIVATE t81_core)

  add_executable(t81_vm_new_opcodes_test tests/cpp/vm_new_opcodes_test.cpp)
  target_link_libraries(t81_vm_new_opcodes_test PRIVATE t81_core)

  add_executable(t81_vm_memory_test tests/cpp/vm_memory_test.cpp)
  target_link_libraries(t81_vm_memory_test PRIVATE t81_core)

  add_executable(t81_axion_log_determinism_test tests/cpp/axion_log_determinism_test.cpp)
  target_link_libraries(t81_axion_log_determinism_test PRIVATE t81_core)

  add_executable(t81_reflection_non_interference_test tests/cpp/reflection_non_interference_test.cpp)
  target_link_libraries(t81_reflection_non_interference_test PRIVATE t81_core)

  add_executable(t81_weights_load_test tests/cpp/weights_load_test.cpp)
  target_link_libraries(t81_weights_load_test PRIVATE t81_core)

  add_executable(t81_hash_stub_test tests/cpp/hash_stub_test.cpp)
  target_link_libraries(t81_hash_stub_test PRIVATE t81_core)

  add_executable(t81_sha3_test tests/cpp/test_sha3.cpp)
  target_link_libraries(t81_sha3_test PRIVATE t81_core)

  add_executable(t81_axion_stub_test tests/cpp/axion_stub_test.cpp)
  target_link_libraries(t81_axion_stub_test PRIVATE t81_core)

  add_executable(t81_codec_base243_test tests/cpp/codec_base243_test.cpp)
  target_link_libraries(t81_codec_base243_test PRIVATE t81_core)
  add_executable(t81_codec_base81_test tests/cpp/codec_base81_test.cpp)
  target_link_libraries(t81_codec_base81_test PRIVATE t81_core)

  add_executable(t81_trit_packing_test tests/cpp/trit_packing_test.cpp)
  target_link_libraries(t81_trit_packing_test PRIVATE t81_core)

  add_executable(t81_base81_balanced_test tests/cpp/test_base81_balanced.cpp)
  target_link_libraries(t81_base81_balanced_test PRIVATE t81_core)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_executable(t81_base81_simd_test tests/cpp/test_base81_simd.cpp)
    target_link_libraries(t81_base81_simd_test PRIVATE t81_core)
  endif()

  add_executable(t81_base81_packed_test tests/cpp/test_base81_packed.cpp)
  target_link_libraries(t81_base81_packed_test PRIVATE t81_core)

  add_executable(t81_metadata_test tests/cpp/test_metadata.cpp)
  target_link_libraries(t81_metadata_test PRIVATE t81_core)

  add_executable(t81_tisc_base81_view_test tests/cpp/test_tisc_base81_view.cpp)
  target_link_libraries(t81_tisc_base81_view_test PRIVATE t81_tisc)

  add_executable(tisc_base81 tools/tisc_base81.cpp)
  target_link_libraries(tisc_base81 PRIVATE t81_tisc)

  add_executable(setun_bridge_test tests/cpp/setun_bridge_test.cpp)
  target_link_libraries(setun_bridge_test PRIVATE t81_core)

  add_executable(tisc_opcode_matrix_test tests/cpp/tisc_opcode_matrix_test.cpp)
  target_link_libraries(tisc_opcode_matrix_test PRIVATE t81_tisc)

  add_executable(t81lang_conformance_baseline_test tests/cpp/t81lang_conformance_baseline_test.cpp)
  target_link_libraries(t81lang_conformance_baseline_test PRIVATE t81_frontend)

  add_executable(t81_numeric_format_test tests/cpp/t81_numeric_format_test.cpp)
  target_link_libraries(t81_numeric_format_test PRIVATE t81_core)

  add_executable(axion_recursion_guardrails_test tests/cpp/axion_recursion_guardrails_test.cpp)
  target_link_libraries(axion_recursion_guardrails_test PRIVATE t81_core)

  add_executable(t81_tensor_shape_test tests/cpp/tensor_shape_test.cpp)
  target_link_libraries(t81_tensor_shape_test PRIVATE t81_core)

  add_executable(t81_ternary_arith_test tests/cpp/ternary_arith_test.cpp)
  target_link_libraries(t81_ternary_arith_test PRIVATE t81_core)

  add_executable(t81_tensor_matmul_test tests/cpp/tensor_matmul_test.cpp)
  target_link_libraries(t81_tensor_matmul_test PRIVATE t81_core)

  add_executable(t81_tensor_conv_test tests/cpp/test_T81Tensor_conv.cpp)
  target_link_libraries(t81_tensor_conv_test PRIVATE t81_core)

  add_executable(t81_tensor_nn_test tests/cpp/test_T81Tensor_nn.cpp)
  target_link_libraries(t81_tensor_nn_test PRIVATE t81_core)

  add_executable(t81_tensor_reduce_test tests/cpp/tensor_reduce_test.cpp)
  target_link_libraries(t81_tensor_reduce_test PRIVATE t81_core)

  add_executable(t81_tensor_broadcast_test tests/cpp/tensor_broadcast_test.cpp)
  target_link_libraries(t81_tensor_broadcast_test PRIVATE t81_core)

  add_executable(t81_entropy_test tests/cpp/entropy_test.cpp)
  target_link_libraries(t81_entropy_test PRIVATE t81_core)

  add_executable(t81_expected_test tests/cpp/expected_test.cpp)
  target_link_libraries(t81_expected_test PRIVATE t81_core)

  add_executable(t81_tensor_elementwise_test tests/cpp/tensor_elementwise_test.cpp)
  target_link_libraries(t81_tensor_elementwise_test PRIVATE t81_core)

  add_executable(t81_tensor_ternary_test tests/cpp/tensor_ternary_test.cpp)
  target_link_libraries(t81_tensor_ternary_test PRIVATE t81_core)

  add_executable(t81_t729_tensor_test tests/cpp/test_t729_tensor.cpp)
  target_link_libraries(t81_t729_tensor_test PRIVATE t81_core)

  add_executable(t81_symbolic_algebra_test tests/cpp/test_symbolic_algebra.cpp)
  target_link_libraries(t81_symbolic_algebra_test PRIVATE t81_core)

  add_executable(t81_semantic_graph_test tests/cpp/test_semantic_graph.cpp)
  target_link_libraries(t81_semantic_graph_test PRIVATE t81_core)

  add_executable(t81_neural_primitives_test tests/cpp/test_neural_primitives.cpp)
  target_link_libraries(t81_neural_primitives_test PRIVATE t81_core)

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

  add_executable(vm_bounds_trace_test tests/cpp/vm_bounds_trace_test.cpp)
  target_link_libraries(vm_bounds_trace_test PRIVATE t81_core)
  add_test(NAME vm_bounds_trace_test COMMAND vm_bounds_trace_test)

  add_executable(t81_canonfs_driver_test tests/cpp/canonfs_driver_test.cpp)
  target_link_libraries(t81_canonfs_driver_test PRIVATE t81_core)

  add_executable(t81_vm_trace_test tests/cpp/vm_trace_test.cpp)
  target_link_libraries(t81_vm_trace_test PRIVATE t81_core)

  add_executable(t81_vm_determinism_property_test tests/cpp/vm_determinism_property_test.cpp)
  target_link_libraries(t81_vm_determinism_property_test PRIVATE t81_core)

  add_executable(vm_extended_ops_test tests/cpp/vm_extended_ops_test.cpp)
  target_link_libraries(vm_extended_ops_test PRIVATE t81_core)

  add_executable(llama_kernels_test tests/cpp/llama_kernels_test.cpp)
  target_link_libraries(llama_kernels_test PRIVATE t81_core)

  add_executable(t81_vm_jump_flags_test tests/cpp/vm_jump_flags_test.cpp)
  target_link_libraries(t81_vm_jump_flags_test PRIVATE t81_core)

  add_executable(t81_vm_float_fraction_ops_test tests/cpp/vm_float_fraction_ops_test.cpp)
  target_link_libraries(t81_vm_float_fraction_ops_test PRIVATE t81_core)

  add_executable(t81_vm_neg_jumps_test tests/cpp/vm_neg_jumps_test.cpp)
  target_link_libraries(t81_vm_neg_jumps_test PRIVATE t81_core)

  add_executable(t81_vm_print_test tests/cpp/vm_print_test.cpp)
  target_link_libraries(t81_vm_print_test PRIVATE t81_core)

  add_executable(t81_frontend_lexer_test tests/cpp/frontend_lexer_test.cpp)
  target_link_libraries(t81_frontend_lexer_test PRIVATE t81_frontend)

  add_executable(t81_frontend_parser_test tests/cpp/frontend_parser_test.cpp)
  target_link_libraries(t81_frontend_parser_test PRIVATE t81_frontend)

  add_executable(t81_spec_compliance_test tests/cpp/spec_compliance_test.cpp)
  target_link_libraries(t81_spec_compliance_test PRIVATE t81_frontend)

  add_executable(t81_frontend_expression_features_test tests/cpp/frontend_expression_features_test.cpp)
  target_link_libraries(t81_frontend_expression_features_test PRIVATE t81_frontend t81_tisc)

  add_executable(t81_frontend_parser_generics_test tests/cpp/frontend_parser_generics_test.cpp)
  target_link_libraries(t81_frontend_parser_generics_test PRIVATE t81_frontend)

  add_executable(t81_frontend_parser_recovery_test tests/cpp/frontend_parser_recovery_test.cpp)
  target_link_libraries(t81_frontend_parser_recovery_test PRIVATE t81_frontend)

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

  add_executable(t81_semantic_analyzer_indexing_test tests/cpp/semantic_analyzer_indexing_test.cpp)
  target_link_libraries(t81_semantic_analyzer_indexing_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_vector_literal_test tests/cpp/semantic_analyzer_vector_literal_test.cpp)
  target_link_libraries(t81_semantic_analyzer_vector_literal_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_record_enum_test tests/cpp/semantic_analyzer_record_enum_test.cpp)
  target_link_libraries(t81_semantic_analyzer_record_enum_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_equality_test tests/cpp/semantic_analyzer_equality_test.cpp)
  target_link_libraries(t81_semantic_analyzer_equality_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_numeric_test tests/cpp/semantic_analyzer_numeric_test.cpp)
  target_link_libraries(t81_semantic_analyzer_numeric_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_diagnostic_location_test tests/cpp/semantic_analyzer_diagnostic_location_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_location_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_cascade_suppression_test tests/cpp/semantic_analyzer_cascade_suppression_test.cpp)
  target_link_libraries(t81_semantic_analyzer_cascade_suppression_test PRIVATE t81_frontend)

  add_executable(t81_semantic_analyzer_diagnostic_precision_test tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_precision_test PRIVATE t81_frontend)

  add_executable(t81_tisc_binary_emitter_test tests/cpp/tisc_binary_emitter_test.cpp)
  target_link_libraries(t81_tisc_binary_emitter_test PRIVATE t81_tisc)

  add_executable(t81_tisc_type_alias_io_test tests/cpp/tisc_type_alias_io_test.cpp)
  target_link_libraries(t81_tisc_type_alias_io_test PRIVATE t81_tisc)

  add_executable(t81_tisc_binary_io_determinism_test tests/cpp/tisc_binary_io_determinism_test.cpp)
  target_link_libraries(t81_tisc_binary_io_determinism_test PRIVATE t81_tisc t81_core)
  add_executable(t81_tisc_binary_metadata_roundtrip_property_test tests/cpp/tisc_binary_metadata_roundtrip_property_test.cpp)
  target_link_libraries(t81_tisc_binary_metadata_roundtrip_property_test PRIVATE t81_tisc t81_core)

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

  add_executable(e2e_option_type_test tests/cpp/e2e_option_type_test.cpp)
  target_link_libraries(e2e_option_type_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_option_result_function_test tests/cpp/e2e_option_result_function_test.cpp)
  target_link_libraries(e2e_option_result_function_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(t81_cli_check_test tests/cpp/cli_check_test.cpp)
  target_link_libraries(t81_cli_check_test PRIVATE t81_cli_driver)

  add_executable(security_package_init_test tests/cpp/security_package_init_test.cpp)
  target_link_libraries(security_package_init_test PRIVATE t81_cli_driver)

  add_executable(t81_cli_pipeline_test tests/cpp/cli_pipeline_test.cpp)
  target_link_libraries(t81_cli_pipeline_test PRIVATE t81_cli_driver)

  add_executable(t81_cli_disasm_test tests/cpp/cli_disasm_test.cpp)
  target_link_libraries(t81_cli_disasm_test PRIVATE t81_cli_driver)

  add_executable(t81_cli_debugger_test tests/cpp/cli_debugger_test.cpp)
  target_link_libraries(t81_cli_debugger_test PRIVATE t81_cli_driver t81_tisc)

  add_executable(t81_cli_option_result_test tests/cpp/cli_option_result_test.cpp)
  target_link_libraries(t81_cli_option_result_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_cli_diagnostic_context_test tests/cpp/cli_diagnostic_context_test.cpp)
  target_link_libraries(t81_cli_diagnostic_context_test PRIVATE t81_cli_driver t81_frontend t81_tisc t81_vm)

  add_executable(t81_cli_structural_types_test tests/cpp/cli_structural_types_test.cpp)
  target_link_libraries(t81_cli_structural_types_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_cli_repl_test tests/cpp/cli_repl_test.cpp)
  target_link_libraries(t81_cli_repl_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_canonize_tensor_test tests/cpp/cli_canonize_tensor_test.cpp)
  target_link_libraries(cli_canonize_tensor_test PRIVATE t81_cli_driver t81_core)

  add_executable(cli_repro_hash_test tests/cpp/cli_repro_hash_test.cpp)
  target_compile_features(cli_repro_hash_test PRIVATE ${T81_CXX_STD_FEATURE})

  add_executable(cli_std_text_fixtures_test tests/cpp/cli_std_text_fixtures_test.cpp)
  target_link_libraries(cli_std_text_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_std_bytes_fixtures_test tests/cpp/cli_std_bytes_fixtures_test.cpp)
  target_link_libraries(cli_std_bytes_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_std_symbol_fixtures_test tests/cpp/cli_std_symbol_fixtures_test.cpp)
  target_link_libraries(cli_std_symbol_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_std_collections_fixtures_test tests/cpp/cli_std_collections_fixtures_test.cpp)
  target_link_libraries(cli_std_collections_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_std_tensor_fixtures_test tests/cpp/cli_std_tensor_fixtures_test.cpp)
  target_link_libraries(cli_std_tensor_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(cli_std_runtime_fixtures_test tests/cpp/cli_std_runtime_fixtures_test.cpp)
  target_link_libraries(cli_std_runtime_fixtures_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_cli_record_enum_test tests/cpp/cli_record_enum_test.cpp)
  target_link_libraries(t81_cli_record_enum_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(t81_tier4_test tests/cpp/tier4_test.cpp)
  target_link_libraries(t81_tier4_test PRIVATE t81_core)

  add_executable(t81_tier4_vm_test tests/cpp/tier4_vm_test.cpp)
  target_link_libraries(t81_tier4_vm_test PRIVATE t81_core)

  add_executable(tiers_structure_test tests/cpp/tiers_structure_test.cpp)
  target_link_libraries(tiers_structure_test PRIVATE t81_core)

  add_executable(t81_jit_test tests/cpp/jit_test.cpp)
  target_link_libraries(t81_jit_test PRIVATE t81_core)

  add_executable(jit_trace_equivalence_test tests/cpp/jit_trace_equivalence_test.cpp)
  target_link_libraries(jit_trace_equivalence_test PRIVATE t81_vm)

  add_executable(jit_tensor_trace_equivalence_test tests/cpp/jit_tensor_trace_equivalence_test.cpp)
  target_link_libraries(jit_tensor_trace_equivalence_test PRIVATE t81_vm)

  add_executable(axion_loop_metadata_test tests/cpp/axion_loop_metadata_test.cpp)
  target_link_libraries(axion_loop_metadata_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(axion_match_metadata_test tests/cpp/axion_match_metadata_test.cpp)
  target_link_libraries(axion_match_metadata_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(axion_enum_guard_test tests/cpp/axion_enum_guard_test.cpp)
  target_link_libraries(axion_enum_guard_test PRIVATE t81_cli_driver t81_tisc t81_vm)

add_executable(axion_policy_match_guard_test tests/cpp/axion_policy_match_guard_test.cpp)
target_link_libraries(axion_policy_match_guard_test PRIVATE t81_cli_driver t81_tisc t81_vm)

add_executable(axion_policy_gc_trace_test tests/cpp/axion_policy_gc_trace_test.cpp)
target_link_libraries(axion_policy_gc_trace_test PRIVATE t81_cli_driver t81_tisc t81_vm)

  add_executable(axion_policy_bytecode_test tests/cpp/axion_policy_bytecode_test.cpp)
  target_link_libraries(axion_policy_bytecode_test PRIVATE t81_core)

add_executable(axion_policy_segment_event_test tests/cpp/axion_policy_segment_event_test.cpp)
target_link_libraries(axion_policy_segment_event_test PRIVATE t81_tisc t81_vm)

  add_executable(axion_instruction_counter_test tests/cpp/axion_instruction_counter_test.cpp)
  target_link_libraries(axion_instruction_counter_test PRIVATE t81_core t81_vm)

  add_executable(e2e_match_expression_test tests/cpp/e2e_match_expression_test.cpp)
  target_link_libraries(e2e_match_expression_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_loop_statement_test tests/cpp/e2e_loop_statement_test.cpp)
  target_link_libraries(e2e_loop_statement_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_advanced_features_test tests/cpp/e2e_advanced_features_test.cpp)
  target_link_libraries(e2e_advanced_features_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_print_runtime_test tests/cpp/e2e_print_runtime_test.cpp)
  target_link_libraries(e2e_print_runtime_test PRIVATE t81_frontend t81_tisc t81_vm)

  add_executable(e2e_compile_determinism_test tests/cpp/e2e_compile_determinism_test.cpp)
  target_link_libraries(e2e_compile_determinism_test PRIVATE t81_cli_driver t81_tisc t81_vm t81_core)

  add_executable(e2e_match_metadata_determinism_test tests/cpp/e2e_match_metadata_determinism_test.cpp)
  target_link_libraries(e2e_match_metadata_determinism_test PRIVATE t81_cli_driver t81_vm)

  add_executable(e2e_ast_ir_canonical_determinism_test tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp)
  target_link_libraries(e2e_ast_ir_canonical_determinism_test PRIVATE t81_frontend t81_core)

  add_executable(cli_e2e_test tests/cpp/cli_e2e_test.cpp)
  target_link_libraries(cli_e2e_test PRIVATE t81_cli_driver)

  if(T81_BUILD_FUZZ_TESTS)
    add_executable(t81_frontend_fuzz_test tests/cpp/frontend_fuzz_test.cpp)
    target_link_libraries(t81_frontend_fuzz_test PRIVATE t81_frontend)
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
  target_link_libraries(performance_advancements_test PRIVATE t81_core)

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

  enable_testing()
  set(T81_TEST_TARGETS
    t81_float_test
    t81_float_properties_test
    t81_float_arithmetic_test
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
    t81_ir_encoding_test
    t81_tisc_encoding_test
    t81_hash_stub_test
    t81_sha3_test
    t81_axion_stub_test
    t81_codec_base243_test
    t81_codec_base81_test
    t81_trit_packing_test
    t81_base81_balanced_test
    t81_base81_packed_test
    t81_metadata_test
    t81_tisc_base81_view_test
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
    t81_axion_log_determinism_test
    t81_reflection_non_interference_test
    t81_canonfs_driver_test
    t81_weights_load_test
    t81_vm_trace_test
    t81_vm_determinism_property_test
    vm_extended_ops_test
    llama_kernels_test
    t81_vm_jump_flags_test
    t81_vm_float_fraction_ops_test
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
    t81_frontend_expression_features_test
    t81_frontend_parser_generics_test
    t81_frontend_parser_recovery_test
    t81_frontend_parser_legacy_rejection_test
    t81_frontend_ir_generator_test
    t81_semantic_analyzer_option_result_test
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
    t81_tisc_binary_emitter_test
    t81_tisc_type_alias_io_test
    t81_tisc_binary_io_determinism_test
    t81_tisc_binary_metadata_roundtrip_property_test
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
    cli_e2e_test
    t81_cli_check_test
    t81_cli_pipeline_test
    t81_cli_disasm_test
    t81_cli_debugger_test
    t81_cli_option_result_test
    t81_cli_diagnostic_context_test
    cli_canonize_tensor_test
    axion_policy_match_guard_test
    axion_policy_segment_event_test
    axion_loop_metadata_test
    axion_match_metadata_test
    axion_enum_guard_test
    axion_nested_guard_test
    axion_instruction_counter_test
    canonfs_axion_trace_test
    t81_tier4_test
    t81_tier4_vm_test
    t81_jit_test
    jit_trace_equivalence_test
    jit_tensor_trace_equivalence_test
    e2e_axion_trace_test
    axion_policy_bytecode_test
    test_bigint_multilimb_ext
    test_bigint_robust_features
    t81_bigint_v1_perf
    bigint_perf_guardrail_test
    t81_bigint_allocation_pathology_test
    t81_bigint_allocation_guardrail_test
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
  add_test(NAME t81_cli_std_text_fixtures_test COMMAND $<TARGET_FILE:cli_std_text_fixtures_test>)
  add_test(NAME t81_cli_std_bytes_fixtures_test COMMAND $<TARGET_FILE:cli_std_bytes_fixtures_test>)
  add_test(NAME t81_cli_std_symbol_fixtures_test COMMAND $<TARGET_FILE:cli_std_symbol_fixtures_test>)
  add_test(NAME t81_cli_std_collections_fixtures_test COMMAND $<TARGET_FILE:cli_std_collections_fixtures_test>)
  add_test(NAME t81_cli_std_tensor_fixtures_test COMMAND $<TARGET_FILE:cli_std_tensor_fixtures_test>)
  add_test(NAME t81_cli_std_runtime_fixtures_test COMMAND $<TARGET_FILE:cli_std_runtime_fixtures_test>)
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
# Python Bindings
# =============================================================================
execute_process(
    COMMAND python3 -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(pybind11_DIR)
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(pybind11 REQUIRED)
  pybind11_add_module(t81_python src/python/t81_python.cpp)
  target_link_libraries(t81_python PRIVATE t81_core t81_frontend t81_tisc)
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


if(T81_BUILD_TESTS)
  add_executable(t81_graph_test tests/cpp/test_T81Graph.cpp)
  target_link_libraries(t81_graph_test PRIVATE t81_core)
  add_test(NAME t81_graph_test COMMAND t81_graph_test)

  add_executable(t81_graph_components_test tests/cpp/test_T81Graph_components.cpp)
  target_link_libraries(t81_graph_components_test PRIVATE t81_core)
  add_test(NAME t81_graph_components_test COMMAND t81_graph_components_test)

  add_executable(t81_graph_shortest_path_test tests/cpp/test_T81Graph_shortest_path.cpp)
  target_link_libraries(t81_graph_shortest_path_test PRIVATE t81_core)
  add_test(NAME t81_graph_shortest_path_test COMMAND t81_graph_shortest_path_test)

  add_executable(t81_graph_large_test tests/cpp/test_T81Graph_large.cpp)
  target_link_libraries(t81_graph_large_test PRIVATE t81_core)
  add_test(NAME t81_graph_large_test COMMAND t81_graph_large_test)

  add_executable(t81_map_regression_test tests/cpp/test_T81Map_regression.cpp)
  target_link_libraries(t81_map_regression_test PRIVATE t81_core)
  add_test(NAME t81_map_regression_test COMMAND t81_map_regression_test)

  add_executable(t81_polynomial_test tests/cpp/test_T81Polynomial.cpp)
  target_link_libraries(t81_polynomial_test PRIVATE t81_core)
  add_test(NAME t81_polynomial_test COMMAND t81_polynomial_test)

  add_executable(t81_agent_test tests/cpp/test_T81Agent.cpp)
  target_link_libraries(t81_agent_test PRIVATE t81_core)
  add_test(NAME t81_agent_test COMMAND t81_agent_test)

  add_executable(t81_stream_test tests/cpp/test_T81Stream.cpp)
  target_link_libraries(t81_stream_test PRIVATE t81_core)
  add_test(NAME t81_stream_test COMMAND t81_stream_test)

  add_executable(t81_promise_test tests/cpp/test_T81Promise.cpp)
  target_link_libraries(t81_promise_test PRIVATE t81_core)
  add_test(NAME t81_promise_test COMMAND t81_promise_test)

  add_executable(t81_thread_test tests/cpp/test_T81Thread.cpp)
  target_link_libraries(t81_thread_test PRIVATE t81_core)
  add_test(NAME t81_thread_test COMMAND t81_thread_test)

  add_executable(t81_network_test tests/cpp/test_T81Network.cpp)
  target_link_libraries(t81_network_test PRIVATE t81_core asio)
  add_test(NAME t81_network_test COMMAND t81_network_test)

  add_executable(t81_discovery_test tests/cpp/test_T81Discovery.cpp)
  target_link_libraries(t81_discovery_test PRIVATE t81_core asio)
  add_test(NAME t81_discovery_test COMMAND t81_discovery_test)

  add_executable(t81_category_test tests/cpp/test_T81Category.cpp)
  target_link_libraries(t81_category_test PRIVATE t81_core)
  add_test(NAME t81_category_test COMMAND t81_category_test)

  add_executable(t81_proof_test tests/cpp/test_T81Proof.cpp)
  target_link_libraries(t81_proof_test PRIVATE t81_core)
  add_test(NAME t81_proof_test COMMAND t81_proof_test)

  add_executable(t81_tensor_slice_static_test tests/cpp/test_T81Tensor_slice_static.cpp)
  target_link_libraries(t81_tensor_slice_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_slice_static_test COMMAND t81_tensor_slice_static_test)

  add_executable(t81_tensor_concat_static_test tests/cpp/test_T81Tensor_concat_static.cpp)
  target_link_libraries(t81_tensor_concat_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_concat_static_test COMMAND t81_tensor_concat_static_test)

  add_executable(t81_tensor_permute_static_test tests/cpp/test_T81Tensor_permute_static.cpp)
  target_link_libraries(t81_tensor_permute_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_permute_static_test COMMAND t81_tensor_permute_static_test)

  add_executable(t81_tensor_pad_static_test tests/cpp/test_T81Tensor_pad_static.cpp)
  target_link_libraries(t81_tensor_pad_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_pad_static_test COMMAND t81_tensor_pad_static_test)

  add_executable(t81_tensor_activations_static_test tests/cpp/test_T81Tensor_activations_static.cpp)
  target_link_libraries(t81_tensor_activations_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_activations_static_test COMMAND t81_tensor_activations_static_test)

  add_executable(t81_tensor_softmax_static_test tests/cpp/test_T81Tensor_softmax_static.cpp)
  target_link_libraries(t81_tensor_softmax_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_softmax_static_test COMMAND t81_tensor_softmax_static_test)

  add_executable(t81_graph_pagerank_convergence_test tests/cpp/test_T81Graph_pagerank_convergence.cpp)
  target_link_libraries(t81_graph_pagerank_convergence_test PRIVATE t81_core)
  add_test(NAME t81_graph_pagerank_convergence_test COMMAND t81_graph_pagerank_convergence_test)

  add_executable(t81_graph_bfs_test tests/cpp/test_T81Graph_bfs.cpp)
  target_link_libraries(t81_graph_bfs_test PRIVATE t81_core)
  add_test(NAME t81_graph_bfs_test COMMAND t81_graph_bfs_test)

  add_executable(t81_float_rounding_test tests/cpp/test_T81Float_rounding.cpp)
  target_link_libraries(t81_float_rounding_test PRIVATE t81_core)
  add_test(NAME t81_float_rounding_test COMMAND t81_float_rounding_test)

  add_executable(t81_float_clamp_test tests/cpp/test_T81Float_clamp.cpp)
  target_link_libraries(t81_float_clamp_test PRIVATE t81_core)
  add_test(NAME t81_float_clamp_test COMMAND t81_float_clamp_test)

  add_executable(t81_int_pow_test tests/cpp/test_T81Int_pow.cpp)
  target_link_libraries(t81_int_pow_test PRIVATE t81_core)
  add_test(NAME t81_int_pow_test COMMAND t81_int_pow_test)

  add_executable(t81_int_gcd_lcm_test tests/cpp/test_T81Int_gcd_lcm.cpp)
  target_link_libraries(t81_int_gcd_lcm_test PRIVATE t81_core)
  add_test(NAME t81_int_gcd_lcm_test COMMAND t81_int_gcd_lcm_test)

  add_executable(t81_int_shift_test tests/cpp/test_t81int_shift.cpp)
  target_link_libraries(t81_int_shift_test PRIVATE t81_core)
  add_test(NAME t81_int_shift_test COMMAND t81_int_shift_test)

  add_executable(t81_std_test tests/cpp/test_t81_std.cpp)
  target_link_libraries(t81_std_test PRIVATE t81_core)
  add_test(NAME t81_std_test COMMAND t81_std_test)
endif()

# =============================================================================
# Installation & Packaging
# =============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install targets and export them
install(TARGETS
    t81_core
    t81_io
    t81_c_api
    t81_frontend
    t81_tisc
    t81_vm
    t81_llvm
    t81_cli_driver
    t81
  EXPORT T81Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/t81 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install export file
install(EXPORT T81Targets
  FILE T81Targets.cmake
  NAMESPACE T81::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)

# Generate config file
configure_package_config_file(
  cmake/T81Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/T81Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/T81ConfigVersion.cmake"
  VERSION 1.1.0
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/T81Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/T81ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)
