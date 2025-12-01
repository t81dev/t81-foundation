#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/T81Limb.hpp"
#include "t81/t81.hpp"

namespace {
    const size_t DATA_SIZE = 100000;
    std::vector<t81::core::T81Limb> t81_source_data_a;
    std::vector<t81::core::T81Limb> t81_source_data_b;
    std::vector<t81::core::T81Limb> t81_dest_data;
    std::vector<t81::T81> t81_native_a;
    std::vector<t81::T81> t81_native_b;
    std::vector<t81::T81> t81_native_dest_data;
    std::vector<__int128> int128_data_a;
    std::vector<__int128> int128_data_b;
    std::vector<__int128> int128_dest_data;

    // Helper to create a valid T81Limb from an int64_t for testing purposes.
    void setup_limb_arith() {
        if (!t81_source_data_a.empty()) return;

        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int> distrib(-13, 13);
        std::uniform_int_distribution<int64_t> int128_dist(-(1LL << 30), (1LL << 30) - 1);

        t81_source_data_a.resize(DATA_SIZE);
        t81_source_data_b.resize(DATA_SIZE);
        t81_dest_data.resize(DATA_SIZE);

        for (size_t i = 0; i < DATA_SIZE; ++i) {
            for (int j = 0; j < t81::core::T81Limb::TRYTES; ++j) {
                t81_source_data_a[i].set_tryte(j, distrib(gen));
                t81_source_data_b[i].set_tryte(j, distrib(gen));
            }
        }
        t81_native_a.resize(DATA_SIZE);
        t81_native_b.resize(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_native_a[i] = t81::from_classic(t81_source_data_a[i]);
            t81_native_b[i] = t81::from_classic(t81_source_data_b[i]);
        }
        t81_native_dest_data.resize(DATA_SIZE);
        int128_data_a.resize(DATA_SIZE);
        int128_data_b.resize(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int128_data_a[i] = static_cast<__int128>(int128_dist(gen));
            int128_data_b[i] = static_cast<__int128>(int128_dist(gen));
        }
        int128_dest_data.resize(DATA_SIZE);
    }
}

static void BM_LimbArithThroughput_T81Limb(benchmark::State& state) {
    setup_limb_arith();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_dest_data[i] = t81_source_data_a[i] + t81_source_data_b[i];
        }
        auto* dest_ptr = static_cast<const void*>(t81_dest_data.data());
        benchmark::DoNotOptimize(dest_ptr);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("48-trit Kogge-Stone addition");
}
BENCHMARK(BM_LimbArithThroughput_T81Limb);

static void BM_LimbArithThroughput_Int128(benchmark::State& state) {
    __int128 a = 0x123456789ABCDEF0ULL;
    __int128 b = 0xFEDCBA9876543210ULL;
    for (auto _ : state) {
        benchmark::DoNotOptimize(a += b);
        b ^= a;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("__int128 addition baseline");
}
BENCHMARK(BM_LimbArithThroughput_Int128);

static void BM_LimbAdd_T81Native(benchmark::State& state) {
    setup_limb_arith();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_native_dest_data[i] = t81_native_a[i] + t81_native_b[i];
        }
        auto* native_ptr = static_cast<const void*>(t81_native_dest_data.data());
        benchmark::DoNotOptimize(native_ptr);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("Native T81 SIMD addition");
}
BENCHMARK(BM_LimbAdd_T81Native);

static void BM_vs_int128(benchmark::State& state) {
    setup_limb_arith();
    size_t idx = 0;
    for (auto _ : state) {
        t81_native_dest_data[idx] = t81_native_a[idx] + t81_native_b[idx];
        benchmark::DoNotOptimize(t81_native_dest_data[idx]);
        int128_dest_data[idx] = int128_data_a[idx] + int128_data_b[idx];
        benchmark::DoNotOptimize(int128_dest_data[idx]);
        idx = (idx + 1) % DATA_SIZE;
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE * 2);
    state.SetLabel("T81 native vs __int128 addition");
}
BENCHMARK(BM_vs_int128);
