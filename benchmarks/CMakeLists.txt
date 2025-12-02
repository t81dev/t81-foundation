cmake_minimum_required(VERSION 3.11)

include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/v1.8.3.tar.gz
    URL_HASH SHA256=6bc180a57d23d4d9515519f92b0c83d61b05b5bab188961f36ac7b06b0d9e9ce
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

set(BENCHMARK_RUNNER_SOURCES
    runner/benchmark_runner.cpp
    runner/arith_throughput.cpp
    runner/negation_speed.cpp
    runner/roundtrip_accuracy.cpp
    runner/overflow_detection.cpp
    runner/packing_density.cpp
    runner/memory_bandwidth.cpp
)

if(NOT MSVC)
    list(APPEND BENCHMARK_RUNNER_SOURCES runner/limb_arith_throughput.cpp)
endif()

  add_executable(benchmark_runner ${BENCHMARK_RUNNER_SOURCES} BM_LimbMul.cpp)

target_link_libraries(benchmark_runner PRIVATE benchmark::benchmark t81_core)

# Enable AVX2 on GCC/Clang *and* x86/x64 targets to keep macOS/ARM builds clean.
if (NOT MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
    CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(benchmark_runner PRIVATE -mavx2)
endif()
