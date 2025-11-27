#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/cell.hpp"

namespace {
    const size_t DATA_SIZE = 10000;
    std::vector<int64_t> source_data;

    void setup_roundtrip() {
        if (!source_data.empty()) return;
        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int64_t> distrib(t81::core::Cell::MIN, t81::core::Cell::MAX);
        source_data.reserve(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            source_data.push_back(distrib(gen));
        }
    }
}

static void BM_RoundtripAccuracy_T81Cell(benchmark::State& state) {
    setup_roundtrip();
    long long lossless_conversions = 0;
    for (auto _ : state) {
        for (const auto& val : source_data) {
            if (t81::core::Cell::from_int(val).to_int() == val) {
                lossless_conversions++;
            }
        }
    }
    state.counters["Lossless%"] = (static_cast<double>(lossless_conversions) / (state.iterations() * DATA_SIZE)) * 100.0;
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("int64_t -> Cell -> int64_t");
}
BENCHMARK(BM_RoundtripAccuracy_T81Cell);

static void BM_RoundtripAccuracy_Int64(benchmark::State& state) {
    setup_roundtrip();
    for (auto _ : state) {
        for (const auto& val : source_data) {
            volatile int64_t temp = val;
            benchmark::DoNotOptimize(temp);
        }
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("No sign-bit tax");
}
BENCHMARK(BM_RoundtripAccuracy_Int64);
