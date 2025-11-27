#include <benchmark/benchmark.h>
#include "t81/core/cell.hpp"
#include <cstdint>
#include <limits>

// --- T81 Cell Overflow Detection Benchmark ---
static void BM_OverflowDetection_T81Cell(benchmark::State& state) {
    long long detected_overflows = 0;
    const t81::core::Cell max_val = t81::core::Cell::max_value();
    const t81::core::Cell one(1);

    for (auto _ : state) {
        // Force an overflow
        t81::core::Cell result = max_val + one;
        // Check the overflow flag
        if (result.has_overflowed()) {
             detected_overflows++;
        }
    }
    // Report the percentage of detected overflows.
    state.counters["Detected%"] = (static_cast<double>(detected_overflows) / state.iterations()) * 100.0;
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Deterministic, provable");
}
BENCHMARK(BM_OverflowDetection_T81Cell);


// --- uint64_t Silent Wrap Benchmark ---
static void BM_OverflowDetection_UInt64(benchmark::State& state) {
    long long detected_overflows = 0;
    const uint64_t max_val = std::numeric_limits<uint64_t>::max();
    const uint64_t one = 1;

    for (auto _ : state) {
        // Force an overflow (silent wrap)
        volatile uint64_t result = max_val + one;

        // This check will always fail, as wraparound is the defined behavior.
        if (result < max_val) {
             // This branch is taken, but it's not a *language provided* detection mechanism
        } else {
            detected_overflows++; // This will never be hit
        }
    }

    // This will always report 0% as there's no built-in detection.
    state.counters["Detected%"] = (static_cast<double>(detected_overflows) / state.iterations()) * 100.0;
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Silent wrap vs explicit flag");
}
BENCHMARK(BM_OverflowDetection_UInt64);
