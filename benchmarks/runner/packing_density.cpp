#include <benchmark/benchmark.h>
#include "t81/types/cell.hpp"
#include "t81/types/packing.hpp"
#include <cmath>

static void BM_PackingDensity_Theoretical(benchmark::State& state) {
    double val = log2(3);
    for (auto _ : state) {
        benchmark::DoNotOptimize(val);
    }
    state.counters["metric_value"] = val;
    state.counters["metric_unit"] = 1.0;
    state.SetLabel("comparison=none; type=theoretical; unit=bits/trit");
}
BENCHMARK(BM_PackingDensity_Theoretical);

static void BM_PackingDensity_Achieved(benchmark::State& state) {
    double bits_per_cell = 8.0 * sizeof(t81::core::Cell);
    double val = bits_per_cell / t81::core::Cell::TRITS;
    for (auto _ : state) {
        benchmark::DoNotOptimize(val);
    }
    state.counters["metric_value"] = val;
    state.counters["metric_unit"] = 1.0;
    state.SetLabel("comparison=none; type=achieved; unit=bits/trit");
}
BENCHMARK(BM_PackingDensity_Achieved);

static void BM_PackingDensity_Practical(benchmark::State& state) {
    constexpr size_t trits = 19;
    constexpr size_t cells = (trits + t81::core::Cell::TRITS - 1) / t81::core::Cell::TRITS;
    constexpr size_t t81_bytes = cells * sizeof(t81::core::Cell);
    // 19 trits can be represented by ceil(log2(3^19)) = 31 bits
    constexpr size_t binary_bits = 31;
    constexpr size_t binary_bytes = (binary_bits + 7) / 8;
    double ratio = static_cast<double>(binary_bytes) / static_cast<double>(t81_bytes);

    for (auto _ : state) {
        benchmark::DoNotOptimize(ratio);
    }
    state.counters["T81 Bytes"] = static_cast<double>(t81_bytes);
    state.counters["Binary Bytes"] = static_cast<double>(binary_bytes);
    state.counters["metric_value"] = ratio;
    state.counters["metric_unit"] = 1.0;
    state.SetLabel("comparison=none; type=practical; unit=binary-bytes/t81-byte");
}
BENCHMARK(BM_PackingDensity_Practical);
