#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"
#include <cmath>
#include <vector>
#include <random>

namespace {

// 1. Theoretical Density
static void BM_PackingDensity_Theoretical(benchmark::State& state) {
    for (auto _ : state) {
        // This is a static calculation, so we don't loop.
        // The value is reported directly.
    }
    state.counters["Bits/Trit"] = log2(3);
    state.SetLabel("Theoretical maximum without compression");
}
BENCHMARK(BM_PackingDensity_Theoretical);


// 2. Achieved Density
static void BM_PackingDensity_Achieved(benchmark::State& state) {
    for (auto _ : state) {}
    // Calculate the number of distinct values based on the actual trit capacity.
    double distinct_values = pow(3, t81::core::Cell::TRIT_CAPACITY);

    state.counters["Bits/Trit"] = log2(distinct_values) / t81::core::Cell::TRIT_CAPACITY;
    state.SetLabel("log2(3^TRIT_CAPACITY) / TRIT_CAPACITY");
}
BENCHMARK(BM_PackingDensity_Achieved);


// 3. Practical Density
static void BM_PackingDensity_Practical(benchmark::State& state) {
     for (auto _ : state) {}
    // Generate random data
    const size_t COUNT = 1'000'000;
    std::vector<int64_t> int_data;
    int_data.reserve(COUNT);
    std::mt19937_64 gen(0x781);
    std::uniform_int_distribution<int64_t> distrib(
        -t81::core::Cell::max_value().to_i64(),
        t81::core::Cell::max_value().to_i64()
    );
    for(size_t i = 0; i < COUNT; ++i) {
        int_data.push_back(distrib(gen));
    }

    // Measure size of int64_t array
    size_t int64_bytes = COUNT * sizeof(int64_t);

    // Measure size of T81::Cell array
    std::vector<t81::core::Cell> cell_data;
    cell_data.reserve(COUNT);
    for(const auto& val : int_data) {
        cell_data.emplace_back(val);
    }
    size_t cell_bytes = COUNT * sizeof(t81::core::Cell);

    state.counters["T81 Bytes"] = static_cast<double>(cell_bytes);
    state.counters["int64 Bytes"] = static_cast<double>(int64_bytes);
    state.counters["Ratio"] = static_cast<double>(int64_bytes) / cell_bytes;
    state.SetLabel("Size ratio for 1M serialized integers");
}
BENCHMARK(BM_PackingDensity_Practical);

} // namespace
