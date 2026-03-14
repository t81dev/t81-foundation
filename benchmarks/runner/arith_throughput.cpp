#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <exception>
#include "t81/types/cell.hpp"

namespace {
    const size_t DATA_SIZE = 10000;
    std::vector<t81::core::Cell> t81_data_a;
    std::vector<t81::core::Cell> t81_data_b;
    std::vector<int64_t> int64_data_a;
    std::vector<int64_t> int64_data_b;

    void setup() {
        if (!t81_data_a.empty()) return;
    std::mt19937_64 gen(0x781);
    constexpr int64_t SAFE_LIMIT = 10;
    std::uniform_int_distribution<int64_t> distrib(-SAFE_LIMIT, SAFE_LIMIT);
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
    constexpr double kOpsPerIter = static_cast<double>(DATA_SIZE) * 4.0;
    state.counters["work_per_iter"] = kOpsPerIter;
    state.counters["adds_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["subs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["muls_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["divs_per_iter"] = static_cast<double>(DATA_SIZE);
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto sum = t81_data_a[i] + t81_data_b[i];
                auto diff = t81_data_a[i] - t81_data_b[i];
                auto prod = t81_data_a[i] * t81_data_b[i];
                auto quot = t81_data_a[i] / t81_data_b[i];
                benchmark::DoNotOptimize(sum);
                benchmark::DoNotOptimize(diff);
                benchmark::DoNotOptimize(prod);
                benchmark::DoNotOptimize(quot);
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("comparison=apples-to-apples; work: ops/iter=40000");
}
BENCHMARK(BM_ArithThroughput_T81Cell)->Repetitions(3);

static void BM_ArithThroughput_Int64(benchmark::State& state) {
    setup();
    constexpr double kOpsPerIter = static_cast<double>(DATA_SIZE) * 4.0;
    state.counters["work_per_iter"] = kOpsPerIter;
    state.counters["adds_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["subs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["muls_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["divs_per_iter"] = static_cast<double>(DATA_SIZE);
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            auto sum = int64_data_a[i] + int64_data_b[i];
            auto diff = int64_data_a[i] - int64_data_b[i];
            auto prod = int64_data_a[i] * int64_data_b[i];
            auto quot = int64_data_a[i] / int64_data_b[i];
            benchmark::DoNotOptimize(sum);
            benchmark::DoNotOptimize(diff);
            benchmark::DoNotOptimize(prod);
            benchmark::DoNotOptimize(quot);
        }
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel("comparison=apples-to-apples; work: ops/iter=40000");
}
BENCHMARK(BM_ArithThroughput_Int64)->Repetitions(3);
