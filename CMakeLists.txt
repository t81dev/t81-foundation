cmake_minimum_required(VERSION 3.16)
project(t81 LANGUAGES C CXX)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)

add_library(t81 STATIC
  src/core/bigint.cpp
  src/core/fraction.cpp
  src/tisc/encoding.cpp
  src/vm/vm.cpp
  src/lang/compiler.cpp
  src/lang/parser.cpp
  src/canonfs/in_memory_driver.cpp
  src/hanoi/error.cpp
  src/hanoi/in_memory_kernel.cpp
  src/axion/engine.cpp
  src/cog/promotion.cpp
  src/codec/base243.cpp
  src/codec/base81.cpp
  src/bigint/divmod.cpp
  src/bigint/gcd.cpp
  src/hash/canonhash81.cpp
)
target_compile_features(t81 PUBLIC cxx_std_20)
target_include_directories(t81 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# IO sources that need compilation
add_library(t81_io STATIC
  src/io/tensor_loader.cpp
)
target_link_libraries(t81_io PUBLIC t81)
target_include_directories(t81_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Build the C API shim (C++)
add_library(t81_c_api STATIC
  src/c_api/t81_c_api.cpp
)
target_link_libraries(t81_c_api PUBLIC t81)
target_include_directories(t81_c_api
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Examples
if (T81_BUILD_EXAMPLES)
  add_executable(t81_demo examples/demo.cpp)
  target_link_libraries(t81_demo PRIVATE t81)

  add_executable(t81_tensor_ops examples/tensor_ops.cpp)
  target_link_libraries(t81_tensor_ops PRIVATE t81)

  add_executable(t81_ir_roundtrip examples/ir_roundtrip.cpp)
  target_link_libraries(t81_ir_roundtrip PRIVATE t81)

  add_executable(axion_demo examples/axion_demo.cpp)
  target_link_libraries(axion_demo PRIVATE t81)
endif()

# Tests
if (T81_BUILD_TESTS)
  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81)

  add_executable(t81_fraction_test tests/cpp/fraction_roundtrip.cpp)
  target_link_libraries(t81_fraction_test PRIVATE t81)

  add_executable(t81_tensor_transpose_test tests/cpp/tensor_transpose_test.cpp)
  target_link_libraries(t81_tensor_transpose_test PRIVATE t81)

  add_executable(t81_tensor_slice_test tests/cpp/tensor_slice_test.cpp)
  target_link_libraries(t81_tensor_slice_test PRIVATE t81)

  add_executable(t81_tensor_reshape_test tests/cpp/tensor_reshape_test.cpp)
  target_link_libraries(t81_tensor_reshape_test PRIVATE t81)

  add_executable(t81_tensor_loader_test tests/cpp/tensor_loader_test.cpp)
  target_link_libraries(t81_tensor_loader_test PRIVATE t81_io)

  add_executable(t81_canonfs_io_test tests/cpp/canonfs_io_test.cpp)
  target_link_libraries(t81_canonfs_io_test PRIVATE t81)

  add_executable(t81_ir_encoding_test tests/cpp/ir_encoding_test.cpp)
  target_link_libraries(t81_ir_encoding_test PRIVATE t81)

  add_executable(t81_hash_stub_test tests/cpp/hash_stub_test.cpp)
  target_link_libraries(t81_hash_stub_test PRIVATE t81)

  add_executable(t81_axion_stub_test tests/cpp/axion_stub_test.cpp)
  target_link_libraries(t81_axion_stub_test PRIVATE t81)

  add_executable(t81_codec_base243_test tests/cpp/codec_base243_test.cpp)
  target_link_libraries(t81_codec_base243_test PRIVATE t81)
  add_executable(t81_codec_base81_test tests/cpp/codec_base81_test.cpp)
  target_link_libraries(t81_codec_base81_test PRIVATE t81)

  add_executable(t81_tensor_shape_test tests/cpp/tensor_shape_test.cpp)
  target_link_libraries(t81_tensor_shape_test PRIVATE t81)

  add_executable(t81_ternary_arith_test tests/cpp/ternary_arith_test.cpp)
  target_link_libraries(t81_ternary_arith_test PRIVATE t81)

  add_executable(t81_tensor_matmul_test tests/cpp/tensor_matmul_test.cpp)
  target_link_libraries(t81_tensor_matmul_test PRIVATE t81)

  add_executable(t81_tensor_reduce_test tests/cpp/tensor_reduce_test.cpp)
  target_link_libraries(t81_tensor_reduce_test PRIVATE t81)

  add_executable(t81_tensor_broadcast_test tests/cpp/tensor_broadcast_test.cpp)
  target_link_libraries(t81_tensor_broadcast_test PRIVATE t81)

  add_executable(t81_entropy_test tests/cpp/entropy_test.cpp)
  target_link_libraries(t81_entropy_test PRIVATE t81)

  add_executable(t81_tensor_elementwise_test tests/cpp/tensor_elementwise_test.cpp)
  target_link_libraries(t81_tensor_elementwise_test PRIVATE t81)

  add_executable(t81_c_api_bigint_test tests/cpp/c_api_bigint_test.c)
  target_link_libraries(t81_c_api_bigint_test PRIVATE t81_c_api)
  target_include_directories(t81_c_api_bigint_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_executable(t81_tensor_unary_test tests/cpp/tensor_unary_test.cpp)
  target_link_libraries(t81_tensor_unary_test PRIVATE t81)

  add_executable(t81_hanoi_integration_test tests/cpp/hanoi_integration_test.cpp)
  target_link_libraries(t81_hanoi_integration_test PRIVATE t81)

  add_executable(t81_lang_vm_test tests/cpp/lang_vm_integration_test.cpp)
  target_link_libraries(t81_lang_vm_test PRIVATE t81)

  add_executable(t81_vm_load_store_test tests/cpp/vm_load_store_test.cpp)
  target_link_libraries(t81_vm_load_store_test PRIVATE t81)

  add_executable(t81_vm_divmod_test tests/cpp/vm_divmod_test.cpp)
  target_link_libraries(t81_vm_divmod_test PRIVATE t81)

  add_executable(t81_vm_illegal_test tests/cpp/vm_illegal_test.cpp)
  target_link_libraries(t81_vm_illegal_test PRIVATE t81)

  add_executable(t81_vm_bounds_test tests/cpp/vm_bounds_test.cpp)
  target_link_libraries(t81_vm_bounds_test PRIVATE t81)

  add_executable(t81_canonfs_driver_test tests/cpp/canonfs_driver_test.cpp)
  target_link_libraries(t81_canonfs_driver_test PRIVATE t81)

  add_executable(t81_vm_trace_test tests/cpp/vm_trace_test.cpp)
  target_link_libraries(t81_vm_trace_test PRIVATE t81)

  add_executable(t81_lang_parser_vm_test tests/cpp/lang_parser_vm_test.cpp)
  target_link_libraries(t81_lang_parser_vm_test PRIVATE t81)
endif()
