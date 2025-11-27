#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cstdint>
#include "t81/core/cell.hpp"

// --- Helper for deterministic random data ---
namespace {
    std::vector<int64_t> generate_roundtrip_data(size_t count) {
        std::vector<int64_t> data;
        data.reserve(count);
        std::mt19937_64 gen(0x781); // Deterministic seed
        // Use a distribution that covers a wide range of values
        std::uniform_int_distribution<int64_t> distrib(
             -t81::core::Cell::max_value().to_i64(),
             t81::core::Cell::max_value().to_i64()
        );

        for (size_t i = 0; i < count; ++i) {
            data.push_back(distrib(gen));
        }
        return data;
    }

    const size_t ROUNDTRIP_DATA_SIZE = 10'000'000; // 10^7 as per spec, not 10^10
    const auto roundtrip_data = generate_roundtrip_data(ROUNDTRIP_DATA_SIZE);
}

// --- T81 Cell Roundtrip Benchmark ---
static void BM_RoundtripAccuracy_T81Cell(benchmark::State& state) {
    long long lossless_conversions = 0;
    for (auto _ : state) {
        for (const auto& val : roundtrip_data) {
            t81::core::Cell cell_val(val);
            if (cell_val.to_i64() == val) {
                lossless_conversions++;
            }
        }
    }
    // Report the percentage of lossless conversions as a custom counter.
    state.counters["Lossless%"] = (static_cast<double>(lossless_conversions) / (state.iterations() * ROUNDTRIP_DATA_SIZE)) * 100.0;
    state.SetItemsProcessed(state.iterations() * ROUNDTRIP_DATA_SIZE);
    state.SetLabel("int64_t -> Cell -> int64_t (10^7 values)");
}
BENCHMARK(BM_RoundtripAccuracy_T81Cell);

// --- Baseline int64_t "Roundtrip" for comparison ---
// This is a formality to have a 1-to-1 comparison. The operation is a no-op.
static void BM_RoundtripAccuracy_Int64(benchmark::State& state) {
    long long lossless_conversions = 0;
    for (auto _ : state) {
        for (const auto& val : roundtrip_data) {
            volatile int64_t new_val = val; // Prevent optimization
            if (new_val == val) {
                lossless_conversions++;
            }
        }
    }
    state.counters["Lossless%"] = (static_cast<double>(lossless_conversions) / (state.iterations() * ROUNDTRIP_DATA_SIZE)) * 100.0;
    state.SetItemsProcessed(state.iterations() * ROUNDTRIP_DATA_SIZE);
    state.SetLabel("No sign-bit tax");
}
BENCHMARK(BM_RoundtripAccuracy_Int64);
