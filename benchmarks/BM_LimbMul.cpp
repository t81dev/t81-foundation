#include <benchmark/benchmark.h>
#include "t81/types/T81Limb.hpp"
#include "t81/t81.hpp"
#include <cstdint>

using namespace t81::core;
namespace {
constexpr int kBatchOps = 4096;
}

static void BM_LimbMul_Booth(benchmark::State& state) {
    T81Limb a, b;
    for (int i = 0; i < T81Limb::TRYTES; ++i) {
        a.set_tryte(i, (i % 27) - 13);
        b.set_tryte(i, ((i + 7) % 27) - 13);
    }
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a * b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbMul_Booth)->Unit(benchmark::kMillisecond);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

static void BM_LimbMul_Booth_Binary(benchmark::State& state) {
    std::uint64_t a = 0x123456789abcdef0ULL;
    std::uint64_t b = 0x0fedcba987654321ULL;
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
#if defined(_MSC_VER) && !defined(__clang__)
            // MSVC does not support __int128. Use _umul128
            unsigned __int64 highProduct;
            benchmark::DoNotOptimize(_umul128(a, b, &highProduct));
#else
            benchmark::DoNotOptimize(static_cast<unsigned __int128>(a) * b);
#endif
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbMul_Booth_Binary)->Unit(benchmark::kMillisecond);

static void BM_LimbMul_Booth_Real(benchmark::State& state) {
    T81Limb a, b;
    for (int i = 0; i < T81Limb::TRYTES; ++i) {
        a.set_tryte(i, (i % 26) - 13);
        b.set_tryte(i, ((i + 5) % 26) - 13);
    }

    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            T81Limb result = T81Limb::booth_mul(a, b);
            benchmark::DoNotOptimize(result);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbMul_Booth_Real)->Unit(benchmark::kMillisecond);

static void BM_LimbMul_Booth_Real_Binary(benchmark::State& state) {
    std::uint64_t a = 0x9abcdef012345678ULL;
    std::uint64_t b = 0x13579bdf2468ace0ULL;
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
#if defined(_MSC_VER) && !defined(__clang__)
            unsigned __int64 highProduct;
            benchmark::DoNotOptimize(_umul128(a, b, &highProduct));
#else
            benchmark::DoNotOptimize(static_cast<unsigned __int128>(a) * b);
#endif
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbMul_Booth_Real_Binary)->Unit(benchmark::kMillisecond);

static void BM_LimbAdd_KoggeStone(benchmark::State& state) {
    T81Limb a, b;
    for (int i = 0; i < T81Limb::TRYTES; ++i) {
        a.set_tryte(i, (i % 27) - 13);
        b.set_tryte(i, ((i + 5) % 27) - 13);
    }
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a + b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbAdd_KoggeStone)->Unit(benchmark::kMillisecond);

static void BM_LimbAdd_KoggeStone_Binary(benchmark::State& state) {
    std::uint64_t a = 0x123456789abcdef0ULL;
    std::uint64_t b = 0x0fedcba987654321ULL;
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a + b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbAdd_KoggeStone_Binary)->Unit(benchmark::kMillisecond);

static void BM_LimbAdd_KoggeStone_T81Native(benchmark::State& state) {
    T81Limb classic_a, classic_b;
    for (int i = 0; i < T81Limb::TRYTES; ++i) {
        classic_a.set_tryte(i, (i % 27) - 13);
        classic_b.set_tryte(i, ((i + 5) % 27) - 13);
    }
    t81::T81 a = t81::from_classic(classic_a);
    t81::T81 b = t81::from_classic(classic_b);
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a + b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_LimbAdd_KoggeStone_T81Native)->Unit(benchmark::kMillisecond);

static void BM_Limb54Mul_Booth(benchmark::State& state) {
    T81Limb54 a, b;
    for (int i = 0; i < T81Limb54::TRYTES; ++i) {
        a.set_tryte(i, (i % 27) - 13);
        b.set_tryte(i, ((i + 7) % 27) - 13);
    }
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a * b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_Limb54Mul_Booth)->Unit(benchmark::kMillisecond);

static void BM_Limb54Mul_Booth_Binary(benchmark::State& state) {
    std::uint64_t a = 0x123456789abcdef0ULL;
    std::uint64_t b = 0x0fedcba987654321ULL;
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
#if defined(_MSC_VER) && !defined(__clang__)
            unsigned __int64 highProduct;
            benchmark::DoNotOptimize(_umul128(a, b, &highProduct));
#else
            benchmark::DoNotOptimize(static_cast<unsigned __int128>(a) * b);
#endif
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
BENCHMARK(BM_Limb54Mul_Booth_Binary)->Unit(benchmark::kMillisecond);

static void BM_Limb54Add_KoggeStone(benchmark::State& state) {
    T81Limb54 a, b;
    for (int i = 0; i < T81Limb54::TRYTES; ++i) {
        a.set_tryte(i, (i % 27) - 13);
        b.set_tryte(i, ((i + 5) % 27) - 13);
    }
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a + b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_Limb54Add_KoggeStone)->Unit(benchmark::kMillisecond);

static void BM_Limb54Add_KoggeStone_Binary(benchmark::State& state) {
    std::uint64_t a = 0x123456789abcdef0ULL;
    std::uint64_t b = 0x0fedcba987654321ULL;
    state.counters["work_per_iter"] = static_cast<double>(kBatchOps);
    state.SetLabel("work: ops/iter=4096");
    for (auto _ : state) {
        for (int i = 0; i < kBatchOps; ++i) {
            benchmark::DoNotOptimize(a + b);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchOps);
}
BENCHMARK(BM_Limb54Add_KoggeStone_Binary)->Unit(benchmark::kMillisecond);
