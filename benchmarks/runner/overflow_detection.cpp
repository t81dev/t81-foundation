// benchmarks/runner/overflow_detection.cpp
#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"
#include <limits>

using namespace t81::core;

static void BM_overflow_ternary_auto(benchmark::State& state) {
    // Build max value — but force it to be runtime, not compile-time
    Cell max_val = Cell::from_int(0);
    for (int i = 0; i < 40; ++i) {
        max_val = max_val * Cell::from_int(3) + Cell::from_int(1);
    }

    // This single line defeats constant folding completely
    benchmark::DoNotOptimize(&max_val);

    int64_t detected = 0;
    for (auto _ : state) {
        try {
            Cell result = max_val + Cell::from_int(1);
            benchmark::DoNotOptimize(result);
        } catch (const std::overflow_error&) {
            detected++;
        }
    }
    state.counters["Detected"] = detected;
}
BENCHMARK(BM_overflow_ternary_auto)->Iterations(5'000'000);

static void BM_overflow_binary_silent(benchmark::State& state) {
    const int64_t max_val = std::numeric_limits<int64_t>::max();
    for (auto _ : state) {
        volatile int64_t r = max_val + 1;
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_overflow_binary_silent)->Iterations(5'000'000);

static void BM_overflow_binary_checked(benchmark::State& state) {
    const int64_t max_val = std::numeric_limits<int64_t>::max();
    int64_t detected = 0;
    for (auto _ : state) {
        if (max_val == std::numeric_limits<int64_t>::max()) {
            detected++;
        } else {
            volatile int64_t r = max_val + 1;
            benchmark::DoNotOptimize(r);
        }
    }
    state.counters["Detected"] = detected;
}
BENCHMARK(BM_overflow_binary_checked)->Iterations(5'000'000);