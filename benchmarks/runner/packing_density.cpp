#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"
#include <cmath>

static void BM_PackingDensity_Theoretical(benchmark::State& state) {
    for (auto _ : state) {}
    state.counters["Bits/Trit"] = log2(3);
    state.SetLabel("Theoretical maximum without compression");
}
BENCHMARK(BM_PackingDensity_Theoretical);

static void BM_PackingDensity_Achieved(benchmark::State& state) {
    for (auto _ : state) {}
    double bits_per_cell = 8.0 * sizeof(t81::core::Cell);
    state.counters["Bits/Trit"] = bits_per_cell / t81::core::Cell::TRITS;
    state.SetLabel("log2(states) / trit_count");
}
BENCHMARK(BM_PackingDensity_Achieved);

static void BM_PackingDensity_Practical(benchmark::State& state) {
    for (auto _ : state) {}
    size_t cell_bytes = sizeof(t81::core::Cell);
    size_t binary_bytes = sizeof(int16_t);
    state.counters["T81 Bytes"] = static_cast<double>(cell_bytes);
    state.counters["Binary Bytes"] = static_cast<double>(binary_bytes);
    state.counters["Ratio"] = static_cast<double>(binary_bytes) / cell_bytes;
    state.SetLabel("Size ratio for equivalent range");
}
BENCHMARK(BM_PackingDensity_Practical);
