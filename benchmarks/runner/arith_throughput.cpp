#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/cell.hpp"

namespace {
    const size_t DATA_SIZE = 10000;
    std::vector<t81::core::Cell> t81_data_a;
    std::vector<t81::core::Cell> t81_data_b;
    std::vector<int64_t> int64_data_a;
    std::vector<int64_t> int64_data_b;

    void setup() {
        if (!t81_data_a.empty()) return;
        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int64_t> distrib(t81::core::Cell::MIN, t81::core::Cell::MAX);
        t81_data_a.reserve(DATA_SIZE);
        t81_data_b.reserve(DATA_SIZE);
        int64_data_a.reserve(DATA_SIZE);
        int64_data_b.reserve(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_t val_a = distrib(gen);
            int64_t val_b = distrib(gen);
            if (val_b == 0) val_b = 1;

            t81_data_a.push_back(t81::core::Cell::from_int(val_a));
            t81_data_b.push_back(t81::core::Cell::from_int(val_b));
            int64_data_a.push_back(val_a);
            int64_data_b.push_back(val_b);
        }
    }
}

static void BM_ArithThroughput_T81Cell(benchmark::State& state) {
    setup();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            benchmark::DoNotOptimize(t81_data_a[i] + t81_data_b[i]);
            benchmark::DoNotOptimize(t81_data_a[i] - t81_data_b[i]);
            benchmark::DoNotOptimize(t81_data_a[i] * t81_data_b[i]);
            benchmark::DoNotOptimize(t81_data_a[i] / t81_data_b[i]);
        }
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("Cell vs int64_t (+-*/)");
}
BENCHMARK(BM_ArithThroughput_T81Cell);

static void BM_ArithThroughput_Int64(benchmark::State& state) {
    setup();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            benchmark::DoNotOptimize(int64_data_a[i] + int64_data_b[i]);
            benchmark::DoNotOptimize(int64_data_a[i] - int64_data_b[i]);
            benchmark::DoNotOptimize(int64_data_a[i] * int64_data_b[i]);
            benchmark::DoNotOptimize(int64_data_a[i] / int64_data_b[i]);
        }
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("Cell vs int64_t (+-*/)");
}
BENCHMARK(BM_ArithThroughput_Int64);
