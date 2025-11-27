#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"

static void BM_OverflowDetection_T81Cell(benchmark::State& state) {
    const auto max_cell = t81::core::Cell::from_int(t81::core::Cell::MAX);
    const auto one_cell = t81::core::Cell::from_int(1);
    long long detected_overflows = 0;
    for (auto _ : state) {
        try {
            benchmark::DoNotOptimize(max_cell + one_cell);
        } catch (const std::overflow_error&) {
            detected_overflows++;
        }
    }
    state.counters["Detected%"] = (static_cast<double>(detected_overflows) / state.iterations()) * 100.0;
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Deterministic, provable");
}
BENCHMARK(BM_OverflowDetection_T81Cell);

static void BM_OverflowDetection_Int64(benchmark::State& state) {
    const int64_t max_int = t81::core::Cell::MAX;
    const int64_t one_int = 1;
    long long detected_overflows = 0;
    for (auto _ : state) {
        volatile int64_t result = max_int + one_int;
        if (result < max_int) {
            // This is the manual check for wrap-around
        } else {
            detected_overflows++;
        }
        benchmark::DoNotOptimize(result);
    }
    state.counters["Detected%"] = (static_cast<double>(detected_overflows) / state.iterations()) * 100.0;
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Silent wrap vs explicit flag");
}
BENCHMARK(BM_OverflowDetection_Int64);
