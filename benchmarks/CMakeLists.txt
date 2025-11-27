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

add_executable(benchmark_runner
    runner/benchmark_runner.cpp
    runner/arith_throughput.cpp
    runner/negation_speed.cpp
    runner/roundtrip_accuracy.cpp
    runner/overflow_detection.cpp
    runner/packing_density.cpp
)

target_link_libraries(benchmark_runner PRIVATE benchmark::benchmark t81_core)
