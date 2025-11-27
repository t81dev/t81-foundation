#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/cell.hpp"

namespace {
    const size_t DATA_SIZE = 100000;
    std::vector<t81::core::Cell> t81_source_data;
    std::vector<t81::core::Cell> t81_dest_data;
    std::vector<int64_t> int64_source_data;
    std::vector<int64_t> int64_dest_data;

    void setup_negation() {
        if (!t81_source_data.empty()) return;
        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int64_t> distrib(t81::core::Cell::MIN, t81::core::Cell::MAX);
        t81_source_data.reserve(DATA_SIZE);
        int64_source_data.reserve(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_t val = distrib(gen);
            t81_source_data.push_back(t81::core::Cell::from_int(val));
            int64_source_data.push_back(val);
        }
        t81_dest_data.resize(DATA_SIZE);
        int64_dest_data.resize(DATA_SIZE);
    }
}

static void BM_NegationSpeed_T81Cell(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_dest_data[i] = -t81_source_data[i];
        }
        benchmark::DoNotOptimize(t81_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("Free negation (no borrow)");
}
BENCHMARK(BM_NegationSpeed_T81Cell);

static void BM_NegationSpeed_Int64(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_dest_data[i] = -int64_source_data[i];
        }
        benchmark::DoNotOptimize(int64_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("~x+1 in two’s complement");
}
BENCHMARK(BM_NegationSpeed_Int64);
