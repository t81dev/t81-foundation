#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"
#include "t81/core/packing.hpp"
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
    constexpr size_t trits = 19;
    constexpr size_t cells = (trits + t81::core::Cell::TRITS - 1) / t81::core::Cell::TRITS;
    constexpr size_t t81_bytes = cells * sizeof(t81::core::Cell);
    constexpr size_t binary_bits = t81::core::packing::packed_bits(trits);
    constexpr size_t binary_bytes = (binary_bits + 7) / 8;
    state.counters["T81 Bytes"] = static_cast<double>(t81_bytes);
    state.counters["Binary Bytes"] = static_cast<double>(binary_bytes);
    state.counters["Ratio"] = static_cast<double>(binary_bytes) / static_cast<double>(t81_bytes);
    state.SetLabel("Real 19-trit packing vs. 4-cell storage");
}
BENCHMARK(BM_PackingDensity_Practical);
