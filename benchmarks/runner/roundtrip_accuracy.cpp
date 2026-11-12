#include <benchmark/benchmark.h>
#include <cstdint>
#include <exception>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/types/cell.hpp"

namespace {
constexpr size_t DATA_SIZE = 10000;
constexpr uint64_t INPUT_SEED = 0x781ULL;

std::vector<int64_t> source_data;
bool roundtrip_validated = false;
bool roundtrip_oracle_ok = false;
size_t roundtrip_oracle_failures = 0;

void setup_and_prevalidate_roundtrip() {
    if (roundtrip_validated) return;
    roundtrip_validated = true;

    source_data.clear();
    std::mt19937_64 gen(INPUT_SEED);
    std::uniform_int_distribution<int64_t> distrib(t81::core::Cell::MIN, t81::core::Cell::MAX);
    source_data.reserve(DATA_SIZE);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        source_data.push_back(distrib(gen));
    }

    // Untimed correctness validation against host int64_t oracle:
    // 1. Exhaustive round-trip across [-121, +121] and overflow rejection outside range.
    size_t failures = 0;
    try {
        for (int64_t v = t81::core::Cell::MIN; v <= t81::core::Cell::MAX; ++v) {
            if (t81::core::Cell::from_int(v).to_int() != v) {
                ++failures;
            }
        }
        for (int64_t out_v : {t81::core::Cell::MIN - 1, t81::core::Cell::MAX + 1}) {
            bool caught = false;
            try {
                [[maybe_unused]] auto bad = t81::core::Cell::from_int(out_v);
            } catch (const std::overflow_error&) {
                caught = true;
            }
            if (!caught) ++failures;
        }
        // 2. Prevalidate all DATA_SIZE timed inputs.
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            const int64_t val = source_data[i];
            if (t81::core::Cell::from_int(val).to_int() != val) {
                ++failures;
            }
        }
    } catch (const std::exception&) {
        ++failures;
    }

    roundtrip_oracle_failures = failures;
    roundtrip_oracle_ok = (failures == 0);
}
}  // namespace

static void BM_CellRoundtrip_T81Cell(benchmark::State& state) {
    setup_and_prevalidate_roundtrip();
    if (!roundtrip_oracle_ok) {
        state.SkipWithError("Cell conversion round-trip oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["roundtrips_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(roundtrip_oracle_failures);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(source_data.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                int64_t val = source_data[i];
                benchmark::DoNotOptimize(val);
                t81::core::Cell cell = t81::core::Cell::from_int(val);
                benchmark::DoNotOptimize(cell);
                int64_t roundtrip = cell.to_int();
                benchmark::DoNotOptimize(roundtrip);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    state.counters["Accuracy%"] = (failed_ops == 0 && roundtrip_oracle_ok) ? 100.0 : 0.0;
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell conversion round-trip");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=not-evaluated; op=cell_roundtrip; semantics=from_int+to_int; "
        "domain=[-121,121]; seed=0x781; unit=ops/s; timing=cpu_time; work: ops/iter=" +
        std::to_string(DATA_SIZE));
}
BENCHMARK(BM_CellRoundtrip_T81Cell)->Repetitions(5);

static void BM_RoundtripAccuracy_T81Cell(benchmark::State& state) {
    setup_and_prevalidate_roundtrip();
    if (!roundtrip_oracle_ok) {
        state.SkipWithError("Cell conversion round-trip oracle prevalidation failed");
        return;
    }
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    state.counters["oracle_failures"] = static_cast<double>(roundtrip_oracle_failures);
    size_t completed_ops = 0;
    size_t failed_ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(source_data.data());
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            try {
                int64_t val = source_data[i];
                benchmark::DoNotOptimize(val);
                t81::core::Cell cell = t81::core::Cell::from_int(val);
                benchmark::DoNotOptimize(cell);
                int64_t roundtrip = cell.to_int();
                benchmark::DoNotOptimize(roundtrip);
                ++completed_ops;
            } catch (const std::exception&) {
                ++failed_ops;
            }
        }
        benchmark::ClobberMemory();
    }
    state.counters["failed_ops"] = static_cast<double>(failed_ops);
    state.counters["Accuracy%"] = (failed_ops == 0 && roundtrip_oracle_ok) ? 100.0 : 0.0;
    if (failed_ops > 0) {
        state.SkipWithError("Unexpected exception during timed Cell round-trip");
        return;
    }
    state.SetItemsProcessed(static_cast<int64_t>(completed_ops));
    state.SetLabel(
        "comparison=semantic-tax; op=cell_roundtrip; semantics=from_int+to_int; "
        "domain=[-121,121]; seed=0x781; unit=ops/s; timing=cpu_time; work: ops/iter=" +
        std::to_string(DATA_SIZE));
}
BENCHMARK(BM_RoundtripAccuracy_T81Cell)->Repetitions(5);

static void BM_RoundtripAccuracy_Int64(benchmark::State& state) {
    setup_and_prevalidate_roundtrip();
    state.counters["work_per_iter"] = static_cast<double>(DATA_SIZE);
    for (auto _ : state) {
        benchmark::DoNotOptimize(source_data.data());
        for (const auto& val : source_data) {
            int64_t temp = val;
            benchmark::DoNotOptimize(temp);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel(
        "comparison=semantic-tax; op=int64_identity_copy; "
        "domain=[-121,121]; seed=0x781; unit=ops/s; timing=cpu_time; work: ops/iter=" +
        std::to_string(DATA_SIZE));
}
BENCHMARK(BM_RoundtripAccuracy_Int64)->Repetitions(5);
