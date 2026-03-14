// benchmarks/runner/overflow_detection.cpp
#include <benchmark/benchmark.h>
#include "t81/types/cell.hpp"
#include <limits>
#include <string>

using namespace t81::core;
namespace {
constexpr int kBatch = 8192;

#if defined(_MSC_VER)
#define T81_NOINLINE __declspec(noinline)
#else
#define T81_NOINLINE __attribute__((noinline))
#endif

T81_NOINLINE inline bool add_overflow_i64(int64_t a, int64_t b, int64_t* out) {
#if defined(__has_builtin)
#if __has_builtin(__builtin_add_overflow)
    return __builtin_add_overflow(a, b, out);
#else
    if ((b > 0 && a > std::numeric_limits<int64_t>::max() - b) ||
        (b < 0 && a < std::numeric_limits<int64_t>::min() - b)) {
        return true;
    }
    *out = a + b;
    return false;
#endif
#else
    if ((b > 0 && a > std::numeric_limits<int64_t>::max() - b) ||
        (b < 0 && a < std::numeric_limits<int64_t>::min() - b)) {
        return true;
    }
    *out = a + b;
    return false;
#endif
}
}

static void BM_overflow_ternary_auto(benchmark::State& state) {
    // Build max value — but force it to be runtime, not compile-time
    Cell max_val = Cell::from_int(0);
    for (int i = 0; i < 4; ++i) {
        max_val = max_val * Cell::from_int(3) + Cell::from_int(1);
    }

    // This single line defeats constant folding completely
    benchmark::DoNotOptimize(&max_val);

    int64_t detected = 0;
    state.counters["work_per_iter"] = static_cast<double>(kBatch);
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            try {
                Cell result = max_val + Cell::from_int(1);
                benchmark::DoNotOptimize(result);
            } catch (const std::overflow_error&) {
                detected++;
            }
        }
    }
    benchmark::DoNotOptimize(detected);
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.counters["Detected"] = static_cast<double>(detected);
    state.SetLabel("comparison=semantic-tax; work: ops/iter=" + std::to_string(kBatch));
}
BENCHMARK(BM_overflow_ternary_auto)->MinTime(0.1)->Repetitions(3);

static void BM_overflow_ternary_auto_Binary(benchmark::State& state) {
    volatile int64_t max_val = std::numeric_limits<int64_t>::max();
    volatile int64_t one = 1;
    int64_t detected = 0;
    state.counters["work_per_iter"] = static_cast<double>(kBatch);
    for (auto _ : state) {
      for (int i = 0; i < kBatch; ++i) {
        int64_t out = 0;
        if (add_overflow_i64(max_val, one, &out)) {
            detected++;
        } else {
            benchmark::DoNotOptimize(out);
        }
      }
    }
    benchmark::DoNotOptimize(detected);
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.counters["Detected"] = static_cast<double>(detected);
    state.SetLabel("comparison=semantic-tax; work: ops/iter=" + std::to_string(kBatch));
}
BENCHMARK(BM_overflow_ternary_auto_Binary)->MinTime(0.1)->Repetitions(3);

static void BM_overflow_binary_silent(benchmark::State& state) {
    volatile uint64_t max_val = std::numeric_limits<uint64_t>::max();
    uint64_t sink = 0;
    state.counters["work_per_iter"] = static_cast<double>(kBatch);
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            sink += static_cast<uint64_t>(max_val + 1u);
        }
    }
    benchmark::DoNotOptimize(sink);
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.SetLabel("comparison=baseline-silent; work: ops/iter=" + std::to_string(kBatch));
}
BENCHMARK(BM_overflow_binary_silent)->MinTime(0.1)->Repetitions(3);

static void BM_overflow_binary_checked(benchmark::State& state) {
    volatile int64_t max_val = std::numeric_limits<int64_t>::max();
    volatile int64_t one = 1;
    int64_t detected = 0;
    state.counters["work_per_iter"] = static_cast<double>(kBatch);
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            int64_t out = 0;
            if (add_overflow_i64(max_val, one, &out)) {
                detected++;
            } else {
                benchmark::DoNotOptimize(out);
            }
        }
    }
    benchmark::DoNotOptimize(detected);
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.counters["Detected"] = static_cast<double>(detected);
    state.SetLabel("comparison=apples-to-apples; work: ops/iter=" + std::to_string(kBatch));
}
BENCHMARK(BM_overflow_binary_checked)->MinTime(0.1)->Repetitions(3);

static void BM_overflow_binary_checked_T81(benchmark::State& state) {
    // We want to measure the overhead of checking for overflow vs. the silent version.
    // In T81, Cell addition ALWAYS checks and throws.
    // This benchmark should reflect real work: adding 1 to max_val.
    Cell max_val = Cell::from_int(Cell::MAX);
    benchmark::DoNotOptimize(&max_val);
    int64_t detected = 0;
    state.counters["work_per_iter"] = static_cast<double>(kBatch);
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            try {
                Cell result = max_val + Cell::from_int(1);
                benchmark::DoNotOptimize(result);
            } catch (const std::overflow_error&) {
                detected++;
            }
        }
    }
    benchmark::DoNotOptimize(detected);
    state.SetItemsProcessed(state.iterations() * kBatch);
    state.counters["Detected"] = static_cast<double>(detected);
    state.SetLabel("comparison=apples-to-apples; work: ops/iter=" + std::to_string(kBatch));
}
BENCHMARK(BM_overflow_binary_checked_T81)->Name("BM_overflow_binary_checked/T81")->MinTime(0.1)->Repetitions(3);
