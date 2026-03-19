cmake_minimum_required(VERSION 3.11)

include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
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
    runner/addition.cpp
    runner/t81lang_compile.cpp
)

if(NOT MSVC)
    list(APPEND BENCHMARK_RUNNER_SOURCES runner/limb_arith_throughput.cpp)
endif()

set(BENCHMARK_SUITE_SOURCES
    BM_LimbMul.cpp
    BM_TensorMatMul.cpp
    BM_CanonFS.cpp
    BM_LlamaGguf.cpp
    BM_LlamaKernels.cpp
    BM_NativeWeightsExecution.cpp
    BM_TensorPromotion.cpp
    BM_Lexer.cpp
    BM_PackedTritVector.cpp
    BM_TritwiseWorkloads.cpp
    BM_DispatchOverhead.cpp
    BM_VMSimulation.cpp
    BM_DeterminismValidation.cpp  # RFC-00A2: determinism validation
    BM_GovernedWorkflows.cpp
)
if(T81_ENABLE_DPE)
    list(APPEND BENCHMARK_SUITE_SOURCES BM_DPE.cpp)
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND BENCHMARK_SUITE_SOURCES BM_Base81SIMD.cpp)
endif()

add_executable(benchmark_runner ${BENCHMARK_RUNNER_SOURCES} ${BENCHMARK_SUITE_SOURCES})

if(T81_ENABLE_DPE)
    target_link_libraries(benchmark_runner PRIVATE benchmark::benchmark t81_lang_frontend t81_tool_model t81_dpe)
else()
    target_link_libraries(benchmark_runner PRIVATE benchmark::benchmark t81_lang_frontend t81_tool_model t81_vm)
endif()

if(T81_ENABLE_LLAMA_CPP)
    target_compile_definitions(benchmark_runner PRIVATE T81_HAS_LLAMA_CPP=1)
    target_include_directories(benchmark_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llama.cpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llama.cpp/ggml/include)
endif()

# Enable AVX2 on GCC/Clang *and* x86/x64 targets to keep macOS/ARM builds clean.
if (NOT MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
    CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(benchmark_runner PRIVATE -mavx2 -mfma)
endif()
