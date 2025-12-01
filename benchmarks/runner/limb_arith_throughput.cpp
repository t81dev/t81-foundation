#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/core/T81Limb.hpp"

namespace {
    const size_t DATA_SIZE = 100000;
    std::vector<t81::core::T81Limb> t81_source_data_a;
    std::vector<t81::core::T81Limb> t81_source_data_b;
    std::vector<t81::core::T81Limb> t81_dest_data;

    // Helper to create a valid T81Limb from an int64_t for testing purposes.
    void setup_limb_arith() {
        if (!t81_source_data_a.empty()) return;

        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int> distrib(-13, 13);

        t81_source_data_a.resize(DATA_SIZE);
        t81_source_data_b.resize(DATA_SIZE);
        t81_dest_data.resize(DATA_SIZE);

        for (size_t i = 0; i < DATA_SIZE; ++i) {
            for (int j = 0; j < t81::core::T81Limb::TRYTES; ++j) {
                t81_source_data_a[i].set_tryte(j, distrib(gen));
                t81_source_data_b[i].set_tryte(j, distrib(gen));
            }
        }
    }
}

static void BM_LimbArithThroughput_T81Limb(benchmark::State& state) {
    setup_limb_arith();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_dest_data[i] = t81_source_data_a[i] + t81_source_data_b[i];
        }
        benchmark::DoNotOptimize(t81_dest_data.data());
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
