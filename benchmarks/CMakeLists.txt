cmake_minimum_required(VERSION 3.11)

include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/v1.8.3.tar.gz
    URL_HASH SHA256=ab8c3fe6d6d7d03b5f7a95c4d5d7f8b6f3c3e9c8f0f5a5d2b69e1f583b2a8f0b
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(benchmark_runner
    runner/benchmark_runner.cpp
    runner/arith_throughput.cpp
    runner/negation_speed.cpp
    runner/roundtrip_accuracy.cpp
    runner/overflow_detection.cpp
    runner/packing_density.cpp
)

target_link_libraries(benchmark_runner PRIVATE benchmark::benchmark t81_core)
