#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/cell.hpp"

// --- Helper for deterministic random data ---
namespace {
    std::vector<t81::core::Cell> generate_random_cells_negation(size_t count) {
        std::vector<t81::core::Cell> data;
        data.reserve(count);
        std::mt19937_64 gen(0x781); // Deterministic seed
        std::uniform_int_distribution<int64_t> distrib(
            -t81::core::Cell::max_value().to_i64(),
            t81::core::Cell::max_value().to_i64()
        );

        for (size_t i = 0; i < count; ++i) {
            data.emplace_back(distrib(gen));
        }
        return data;
    }

    std::vector<int64_t> generate_random_int64s_negation(size_t count) {
        std::vector<int64_t> data;
        data.reserve(count);
        std::mt19937_64 gen(0x781); // Deterministic seed
        std::uniform_int_distribution<int64_t> distrib;

        for (size_t i = 0; i < count; ++i) {
            data.push_back(distrib(gen));
        }
        return data;
    }

    const size_t NEGATION_DATA_SIZE = 1'000'000;
    const auto t81_source_data = generate_random_cells_negation(NEGATION_DATA_SIZE);
    const auto int64_source_data = generate_random_int64s_negation(NEGATION_DATA_SIZE);
    auto t81_dest_data = t81_source_data;
    auto int64_dest_data = int64_source_data;
}

// --- T81 Cell Negation Benchmark ---
static void BM_NegationSpeed_T81Cell(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t i = 0; i < NEGATION_DATA_SIZE; ++i) {
            t81_dest_data[i] = -t81_source_data[i];
        }
        benchmark::DoNotOptimize(t81_dest_data);
    }
    state.SetItemsProcessed(state.iterations() * NEGATION_DATA_SIZE);
    state.SetLabel("Free negation (no borrow)");
}
BENCHMARK(BM_NegationSpeed_T81Cell);


// --- int64_t Negation Benchmark ---
static void BM_NegationSpeed_Int64(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t i = 0; i < NEGATION_DATA_SIZE; ++i) {
            int64_dest_data[i] = -int64_source_data[i];
        }
        benchmark::DoNotOptimize(int64_dest_data);
    }
    state.SetItemsProcessed(state.iterations() * NEGATION_DATA_SIZE);
    state.SetLabel("~x+1 in two’s complement");
}
BENCHMARK(BM_NegationSpeed_Int64);
