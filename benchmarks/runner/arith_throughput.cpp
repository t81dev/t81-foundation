#include <benchmark/benchmark.h>
#include <cstdint>
#include <exception>
#include <random>
#include <stdexcept>
#include <vector>
#include "t81/types/cell.hpp"

namespace {
constexpr size_t DATA_SIZE = 10000;
constexpr int64_t SAFE_LIMIT = 10;
constexpr uint64_t INPUT_SEED = 0x781ULL;

std::vector<t81::core::Cell> t81_data_a;
std::vector<t81::core::Cell> t81_data_b;
std::vector<int64_t> int64_data_a;
std::vector<int64_t> int64_data_b;
bool oracle_validated = false;
bool oracle_ok = false;
size_t oracle_failure_count = 0;
uint64_t input_sequence_fnv1a = 0;

void setup_and_prevalidate() {
    if (oracle_validated) return;
    oracle_validated = true;

    t81_data_a.clear();
    t81_data_b.clear();
    int64_data_a.clear();
    int64_data_b.clear();

    std::mt19937_64 gen(INPUT_SEED);
    std::uniform_int_distribution<int64_t> distrib(-SAFE_LIMIT, SAFE_LIMIT);
    t81_data_a.reserve(DATA_SIZE);
    t81_data_b.reserve(DATA_SIZE);
    int64_data_a.reserve(DATA_SIZE);
    int64_data_b.reserve(DATA_SIZE);

    uint64_t fnv = 14695981039346656037ULL;
    auto fnv_mix = [&](int64_t v) {
        const uint64_t u = static_cast<uint64_t>(v);
        for (int b = 0; b < 8; ++b) {
            fnv ^= (u >> (b * 8)) & 0xFFULL;
            fnv *= 1099511628211ULL;
        }
    };

    for (size_t i = 0; i < DATA_SIZE; ++i) {
        int64_t val_a = distrib(gen);
        int64_t val_b = distrib(gen);
        if (val_b == 0) val_b = 1;
        fnv_mix(val_a);
        fnv_mix(val_b);

        t81_data_a.push_back(t81::core::Cell::from_int(val_a));
        t81_data_b.push_back(t81::core::Cell::from_int(val_b));
        int64_data_a.push_back(val_a);
        int64_data_b.push_back(val_b);
    }
    input_sequence_fnv1a = fnv;

    // Untimed correctness validation against host int64_t oracle:
    // 1. Validate boundary, zero, signed values, overflow, and division-by-zero semantics.
    size_t failures = 0;
    try {
        for (int64_t v = t81::core::Cell::MIN; v <= t81::core::Cell::MAX; ++v) {
            if (t81::core::Cell::from_int(v).to_int() != v) {
                ++failures;
            }
        }
        bool add_ovf_caught = false;
        try {
            [[maybe_unused]] auto ov =
                t81::core::Cell::from_int(t81::core::Cell::MAX) + t81::core::Cell::from_int(1);
        } catch (const std::overflow_error&) {
            add_ovf_caught = true;
        }
        if (!add_ovf_caught) ++failures;

        bool sub_ovf_caught = false;
        try {
            [[maybe_unused]] auto ov =
                t81::core::Cell::from_int(t81::core::Cell::MIN) - t81::core::Cell::from_int(1);
        } catch (const std::overflow_error&) {
            sub_ovf_caught = true;
        }
        if (!sub_ovf_caught) ++failures;

        bool div_zero_caught = false;
        try {
            [[maybe_unused]] auto dz =
                t81::core::Cell::from_int(1) / t81::core::Cell::from_int(0);
        } catch (const std::domain_error&) {
            div_zero_caught = true;
        }
        if (!div_zero_caught) ++failures;

        // 2. Prevalidate every timed input pair against the int64_t oracle.
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            const int64_t a = int64_data_a[i];
            const int64_t b = int64_data_b[i];
            const auto& ca = t81_data_a[i];
            const auto& cb = t81_data_b[i];
            if (ca.to_int() != a || cb.to_int() != b) ++failures;
            if ((ca + cb).to_int() != (a + b)) ++failures;
            if ((ca - cb).to_int() != (a - b)) ++failures;
            if ((ca * cb).to_int() != (a * b)) ++failures;
            if (b == 0 || (ca / cb).to_int() != (a / b)) ++failures;
        }
    } catch (const std::exception&) {
        ++failures;
    }

    oracle_failure_count = failures;
    oracle_ok = (failures == 0);
}
}  // namespace

static void BM_CellAdd_T81Cell(benchmark::State& state) {
    setup_and_prevalidate();
    if (!oracle_ok) {
        state.SkipWithError("Cell addition oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["adds_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(oracle_failure_count);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(t81_data_a.data());
        benchmark::DoNotOptimize(t81_data_b.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto sum = t81_data_a[i] + t81_data_b[i];
                benchmark::DoNotOptimize(sum);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell addition");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=cell_add; semantics=5trit_ripple_carry; "
        "domain=[-10,10]; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=10000");
}
BENCHMARK(BM_CellAdd_T81Cell)->Repetitions(5);

static void BM_CellSub_T81Cell(benchmark::State& state) {
    setup_and_prevalidate();
    if (!oracle_ok) {
        state.SkipWithError("Cell subtraction oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["subs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(oracle_failure_count);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(t81_data_a.data());
        benchmark::DoNotOptimize(t81_data_b.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto diff = t81_data_a[i] - t81_data_b[i];
                benchmark::DoNotOptimize(diff);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell subtraction");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=cell_sub; semantics=5trit_negate_and_add; "
        "domain=[-10,10]; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=10000");
}
BENCHMARK(BM_CellSub_T81Cell)->Repetitions(5);

static void BM_CellMul_T81Cell(benchmark::State& state) {
    setup_and_prevalidate();
    if (!oracle_ok) {
        state.SkipWithError("Cell multiplication oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["muls_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(oracle_failure_count);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(t81_data_a.data());
        benchmark::DoNotOptimize(t81_data_b.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto prod = t81_data_a[i] * t81_data_b[i];
                benchmark::DoNotOptimize(prod);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell multiplication");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=cell_mul; semantics=5trit_shift_and_add; "
        "domain=[-10,10]; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=10000");
}
BENCHMARK(BM_CellMul_T81Cell)->Repetitions(5);

static void BM_CellDiv_T81Cell(benchmark::State& state) {
    setup_and_prevalidate();
    if (!oracle_ok) {
        state.SkipWithError("Cell division oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["divs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["convs_per_op"] = 3.0;  // 2x to_int() + 1x from_int() inside Cell::operator/
    state.counters["oracle_failures"] = static_cast<double>(oracle_failure_count);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(t81_data_a.data());
        benchmark::DoNotOptimize(t81_data_b.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto quot = t81_data_a[i] / t81_data_b[i];
                benchmark::DoNotOptimize(quot);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell division");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=cell_div; semantics=to_int+int64_div+from_int; "
        "domain=[-10,10]\\{0}; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=10000");
}
BENCHMARK(BM_CellDiv_T81Cell)->Repetitions(5);

static void BM_ArithThroughput_T81Cell(benchmark::State& state) {
    setup_and_prevalidate();
    if (!oracle_ok) {
        state.SkipWithError("Cell composite arithmetic oracle prevalidation failed");
        return;
    }
    constexpr double kOpsPerIter = static_cast<double>(DATA_SIZE) * 4.0;
    state.counters["work_per_iter"] = kOpsPerIter;
    state.counters["adds_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["subs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["muls_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["divs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(oracle_failure_count);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(t81_data_a.data());
        benchmark::DoNotOptimize(t81_data_b.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                auto sum = t81_data_a[i] + t81_data_b[i];
                benchmark::DoNotOptimize(sum);
                ++completed_ops;
                auto diff = t81_data_a[i] - t81_data_b[i];
                benchmark::DoNotOptimize(diff);
                ++completed_ops;
                auto prod = t81_data_a[i] * t81_data_b[i];
                benchmark::DoNotOptimize(prod);
                ++completed_ops;
                auto quot = t81_data_a[i] / t81_data_b[i];
                benchmark::DoNotOptimize(quot);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell composite arithmetic");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=composite_add_sub_mul_div; "
        "domain=[-10,10]; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=40000");
}
BENCHMARK(BM_ArithThroughput_T81Cell)->Repetitions(5);

static void BM_ArithThroughput_Int64(benchmark::State& state) {
    setup_and_prevalidate();
    constexpr double kOpsPerIter = static_cast<double>(DATA_SIZE) * 4.0;
    state.counters["work_per_iter"] = kOpsPerIter;
    state.counters["adds_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["subs_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["muls_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["divs_per_iter"] = static_cast<double>(DATA_SIZE);
    for (auto _ : state) {
        benchmark::DoNotOptimize(int64_data_a.data());
        benchmark::DoNotOptimize(int64_data_b.data());
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
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4);
    state.SetLabel(
        "comparison=not-evaluated; op=int64_composite_reference; "
        "domain=[-10,10]; seed=0x781; unit=ops/s; timing=real_time; work: ops/iter=40000");
}
BENCHMARK(BM_ArithThroughput_Int64)->Repetitions(5);
