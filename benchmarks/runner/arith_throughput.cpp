#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/cell.hpp"

// --- Helper for deterministic random data ---
namespace {
    std::vector<t81::core::Cell> generate_random_cells(size_t count) {
        std::vector<t81::core::Cell> data;
        data.reserve(count);
        std::mt19937_64 gen(0x781); // Deterministic seed
        // Assuming Cell can be constructed from a wide range of int64_t
        std::uniform_int_distribution<int64_t> distrib(
            -t81::core::Cell::max_value().to_i64() / 2,
            t81::core::Cell::max_value().to_i64() / 2
        );

        for (size_t i = 0; i < count; ++i) {
            data.emplace_back(distrib(gen));
        }
        return data;
    }

    std::vector<int64_t> generate_random_int64s(size_t count) {
        std::vector<int64_t> data;
        data.reserve(count);
        std::mt19937_64 gen(0x781); // Deterministic seed
        std::uniform_int_distribution<int64_t> distrib(
            std::numeric_limits<int64_t>::min() / 2,
            std::numeric_limits<int64_t>::max() / 2
        );

        for (size_t i = 0; i < count; ++i) {
            data.push_back(distrib(gen));
        }
        return data;
    }

    const size_t DATA_SIZE = 1'000'000;
    const auto t81_data = generate_random_cells(DATA_SIZE);
    const auto int64_data = generate_random_int64s(DATA_SIZE);
}

// --- T81 Cell Benchmark ---
static void BM_ArithThroughput_T81Cell(benchmark::State& state) {
    t81::core::Cell accumulator(0);
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            accumulator = accumulator + t81_data[i];
            accumulator = accumulator - t81_data[i];
            accumulator = accumulator * t81::core::Cell(2);
            if (t81_data[i] != t81::core::Cell(0)) {
                accumulator = accumulator / t81_data[i];
            }
        }
        benchmark::DoNotOptimize(accumulator);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("Cell vs int64_t (+-*/)");
}
BENCHMARK(BM_ArithThroughput_T81Cell);


// --- int64_t Benchmark ---
static void BM_ArithThroughput_Int64(benchmark::State& state) {
    int64_t accumulator = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            accumulator = accumulator + int64_data[i];
            accumulator = accumulator - int64_data[i];
            accumulator = accumulator * 2;
             // Avoid division by zero
            if (int64_data[i] != 0) {
                accumulator = accumulator / int64_data[i];
            }
        }
        benchmark::DoNotOptimize(accumulator);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("Cell vs int64_t (+-*/)");
}
BENCHMARK(BM_ArithThroughput_Int64);
