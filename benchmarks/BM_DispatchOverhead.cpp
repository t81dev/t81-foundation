#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/packed_trit_vector.hpp"
#include "t81/tritwise/tritwise.hpp"

using t81::ComputeTritVector;
using t81::PackedTritVector;
using namespace t81::tritwise;

// 1. Empty function call baseline
static void empty_function(const uint8_t* in, uint8_t* out, size_t len) {
    benchmark::DoNotOptimize(in);
    benchmark::DoNotOptimize(out);
    benchmark::DoNotOptimize(len);
}

static void BM_Overhead_EmptyCall(benchmark::State& state) {
    size_t len = state.range(0);
    std::vector<uint8_t> in(len), out(len);
    for (auto _ : state) {
        empty_function(in.data(), out.data(), len);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Overhead_EmptyCall)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// 2. Dispatch-only baseline (size check, threshold branch)
static void dispatch_only_function(const uint8_t* in, uint8_t* out, size_t len) {
    // Replicates the dispatch logic structure but calls empty functions
#if defined(__x86_64__) && defined(__AVX2__)
    if (len >= ComputeTritVector::AVX2_THRESHOLD_BYTES) {
        empty_function(in, out, len);
        return;
    }
#elif defined(__aarch64__) && defined(__ARM_NEON)
    if (len >= ComputeTritVector::NEON_THRESHOLD_BYTES) {
        empty_function(in, out, len);
        return;
    }
#endif
    empty_function(in, out, len);
}

static void BM_Overhead_DispatchOnly(benchmark::State& state) {
    size_t len = state.range(0);
    std::vector<uint8_t> in(len), out(len);
    for (auto _ : state) {
        dispatch_only_function(in.data(), out.data(), len);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Overhead_DispatchOnly)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// 3. SWAR kernel call directly
static void BM_Overhead_DirectSWAR(benchmark::State& state) {
    size_t len = state.range(0);
    std::vector<uint8_t> in(len), out(len);
    // Initialize with some data to avoid optimizing away
    std::fill(in.begin(), in.end(), 0x55);

    for (auto _ : state) {
        ComputeTritVector::kernel_not_swar(in.data(), out.data(), len);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Overhead_DirectSWAR)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// 4. Library API call (ComputeTritVector::t_not_inplace which calls kernel_not)
static void BM_Overhead_LibraryAPI(benchmark::State& state) {
    size_t len = state.range(0);
    std::vector<int8_t> trits(len * 4); // 4 trits per byte
    std::fill(trits.begin(), trits.end(), 0);
    auto cv = ComputeTritVector::from_trits(trits).value();
    // Ensure we are operating on 'len' bytes
    // cv.data().size() should be len

    for (auto _ : state) {
        // This includes:
        // - Result<bool> overhead
        // - t_not_inplace call
        // - kernel_not dispatch
        // - kernel_not_swar (for small sizes)
        cv.t_not_inplace();
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Overhead_LibraryAPI)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// 5. Full Tritwise API Wrapper
static void BM_Overhead_TritwiseWrapper(benchmark::State& state) {
    size_t len = state.range(0);
    std::vector<int8_t> trits(len * 4);
    std::fill(trits.begin(), trits.end(), 0);
    auto cv = ComputeTritVector::from_trits(trits).value();

    for (auto _ : state) {
        tritwise_not(cv);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Overhead_TritwiseWrapper)->Arg(8)->Arg(16)->Arg(32)->Arg(64);
