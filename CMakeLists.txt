cmake_minimum_required(VERSION 3.16)
project(t81 LANGUAGES CXX)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)

add_library(t81 INTERFACE)
target_compile_features(t81 INTERFACE cxx_std_17)
target_include_directories(t81 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (T81_BUILD_EXAMPLES)
  add_executable(t81_demo examples/demo.cpp)
  target_link_libraries(t81_demo PRIVATE t81)
endif()

if (T81_BUILD_TESTS)
  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81)
endif()
