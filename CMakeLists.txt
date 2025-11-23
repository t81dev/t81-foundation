cmake_minimum_required(VERSION 3.16)
project(t81 LANGUAGES CXX)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)

add_library(t81 INTERFACE)
target_compile_features(t81 INTERFACE cxx_std_17)
target_include_directories(t81 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Examples
if (T81_BUILD_EXAMPLES)
  add_executable(t81_demo examples/demo.cpp)
  target_link_libraries(t81_demo PRIVATE t81)
endif()

# Tests
if (T81_BUILD_TESTS)
  # BigInt + Fraction (requires nlohmann/json header if using JSON vectors)
  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81)
  add_executable(t81_fraction_test tests/cpp/fraction_roundtrip.cpp)
  target_link_libraries(t81_fraction_test PRIVATE t81)

  # Tensor ops tests
  add_executable(t81_tensor_transpose_test tests/cpp/tensor_transpose_test.cpp)
  target_link_libraries(t81_tensor_transpose_test PRIVATE t81)

  add_executable(t81_tensor_slice_test tests/cpp/tensor_slice_test.cpp)
  target_link_libraries(t81_tensor_slice_test PRIVATE t81)

  add_executable(t81_tensor_reshape_test tests/cpp/tensor_reshape_test.cpp)
  target_link_libraries(t81_tensor_reshape_test PRIVATE t81)

  add_executable(t81_tensor_loader_test tests/cpp/tensor_loader_test.cpp)
  target_link_libraries(t81_tensor_loader_test PRIVATE t81)

  # CanonFS IO test
  add_executable(t81_canonfs_io_test tests/cpp/canonfs_io_test.cpp)
  target_link_libraries(t81_canonfs_io_test PRIVATE t81)
endif()
