#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include "t81/tritwise/tritwise.hpp"
#include "t81/packed_trit_vector.hpp"

#ifdef T81_TRITWISE_PROFILE
struct ProfilingWarning {
    ProfilingWarning() {
        std::cerr << "\n[WARNING] T81_TRITWISE_PROFILE is ENABLED. Performance numbers will be distorted!"
                  << "\n          Re-run with -DT81_TRITWISE_PROFILE=OFF for clean measurements.\n" << std::endl;
    }
};
static ProfilingWarning warning_instance;
#endif

using namespace t81::tritwise;
using t81::ComputeTritVector;
using t81::PackedTritVector;

// Workload 1: Ternary Mask Apply
static void BM_MaskApply_Scalar(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x(len), m(len);
    for(size_t i=0; i<len; ++i) {
        x[i] = static_cast<int8_t>(dist(rng));
        m[i] = static_cast<int8_t>(dist(rng));
    }

    for (auto _ : state) {
        for(size_t i=0; i<len; ++i) {
            x[i] = PackedTritVector::scalar_and(x[i], m[i]);
        }
        benchmark::DoNotOptimize(x.data());
    }
}
BENCHMARK(BM_MaskApply_Scalar)->RangeMultiplier(4)->Range(256, 65536);

static void BM_MaskApply_Library(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x_raw(len), m_raw(len);
    for(size_t i=0; i<len; ++i) {
        x_raw[i] = static_cast<int8_t>(dist(rng));
        m_raw[i] = static_cast<int8_t>(dist(rng));
    }

    auto x = ComputeTritVector::from_trits(x_raw).value();
    auto m = ComputeTritVector::from_trits(m_raw).value();

    for (auto _ : state) {
        tritwise_and(x, m); // In-place
        benchmark::DoNotOptimize(x.data().data());
    }
}
BENCHMARK(BM_MaskApply_Library)->RangeMultiplier(4)->Range(256, 65536);

// Workload 2: Packed Pattern Match / Filter
// ((x TAnd a) TOr b) TAnd c
static void BM_PatternMatch_Scalar(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x(len), a(len), b(len), c(len);
    for(size_t i=0; i<len; ++i) {
        x[i] = static_cast<int8_t>(dist(rng));
        a[i] = static_cast<int8_t>(dist(rng));
        b[i] = static_cast<int8_t>(dist(rng));
        c[i] = static_cast<int8_t>(dist(rng));
    }

    for (auto _ : state) {
        for(size_t i=0; i<len; ++i) {
            int8_t tmp = PackedTritVector::scalar_and(x[i], a[i]);
            tmp = PackedTritVector::scalar_or(tmp, b[i]);
            x[i] = PackedTritVector::scalar_and(tmp, c[i]);
        }
        benchmark::DoNotOptimize(x.data());
    }
}
BENCHMARK(BM_PatternMatch_Scalar)->RangeMultiplier(4)->Range(256, 65536);

static void BM_PatternMatch_Library(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x_raw(len), a_raw(len), b_raw(len), c_raw(len);
    for(size_t i=0; i<len; ++i) {
        x_raw[i] = static_cast<int8_t>(dist(rng));
        a_raw[i] = static_cast<int8_t>(dist(rng));
        b_raw[i] = static_cast<int8_t>(dist(rng));
        c_raw[i] = static_cast<int8_t>(dist(rng));
    }

    auto x = ComputeTritVector::from_trits(x_raw).value();
    auto a = ComputeTritVector::from_trits(a_raw).value();
    auto b = ComputeTritVector::from_trits(b_raw).value();
    auto c = ComputeTritVector::from_trits(c_raw).value();

    for (auto _ : state) {
        tritwise_and(x, a);
        tritwise_or(x, b);
        tritwise_and(x, c);
        benchmark::DoNotOptimize(x.data().data());
    }
}
BENCHMARK(BM_PatternMatch_Library)->RangeMultiplier(4)->Range(256, 65536);

// Workload 3: Canonicalization Pipeline
// Repeated NOT/OR/AND on mixed-size arrays.
static void BM_CanonPipeline_Scalar(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x(len), y(len), z(len);
    for(size_t i=0; i<len; ++i) {
        x[i] = static_cast<int8_t>(dist(rng));
        y[i] = static_cast<int8_t>(dist(rng));
        z[i] = static_cast<int8_t>(dist(rng));
    }

    for (auto _ : state) {
        for(size_t i=0; i<len; ++i) {
            x[i] = PackedTritVector::scalar_not(x[i]);
            x[i] = PackedTritVector::scalar_or(x[i], y[i]);
            x[i] = PackedTritVector::scalar_and(x[i], z[i]);
            x[i] = PackedTritVector::scalar_not(x[i]);
        }
        benchmark::DoNotOptimize(x.data());
    }
}
BENCHMARK(BM_CanonPipeline_Scalar)->RangeMultiplier(4)->Range(256, 65536);

static void BM_CanonPipeline_Library(benchmark::State& state) {
    size_t len = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);
    std::vector<int8_t> x_raw(len), y_raw(len), z_raw(len);
    for(size_t i=0; i<len; ++i) {
        x_raw[i] = static_cast<int8_t>(dist(rng));
        y_raw[i] = static_cast<int8_t>(dist(rng));
        z_raw[i] = static_cast<int8_t>(dist(rng));
    }

    auto x = ComputeTritVector::from_trits(x_raw).value();
    auto y = ComputeTritVector::from_trits(y_raw).value();
    auto z = ComputeTritVector::from_trits(z_raw).value();

    for (auto _ : state) {
        tritwise_not(x);
        tritwise_or(x, y);
        tritwise_and(x, z);
        tritwise_not(x);
        benchmark::DoNotOptimize(x.data().data());
    }
}
BENCHMARK(BM_CanonPipeline_Library)->RangeMultiplier(4)->Range(256, 65536);

// Workload 4: Tensor-Lane Emulation
static void BM_TensorLane_Scalar(benchmark::State& state) {
    size_t lane_size = state.range(0);
    size_t num_lanes = 1000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);

    std::vector<int8_t> lane_x(lane_size), lane_y(lane_size);
    for(size_t i=0; i<lane_size; ++i) {
        lane_x[i] = static_cast<int8_t>(dist(rng));
        lane_y[i] = static_cast<int8_t>(dist(rng));
    }

    for (auto _ : state) {
        for (size_t l=0; l<num_lanes; ++l) {
            for(size_t i=0; i<lane_size; ++i) {
                lane_x[i] = PackedTritVector::scalar_and(lane_x[i], lane_y[i]);
            }
            benchmark::DoNotOptimize(lane_x.data());
        }
    }
}
// Use prime sizes to be awkward: 17, 31, 67, 127, 257
BENCHMARK(BM_TensorLane_Scalar)->Arg(17)->Arg(31)->Arg(67)->Arg(127)->Arg(257);

static void BM_TensorLane_Library(benchmark::State& state) {
    size_t lane_size = state.range(0);
    size_t num_lanes = 1000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1, 1);

    std::vector<int8_t> lane_x_raw(lane_size), lane_y_raw(lane_size);
    for(size_t i=0; i<lane_size; ++i) {
        lane_x_raw[i] = static_cast<int8_t>(dist(rng));
        lane_y_raw[i] = static_cast<int8_t>(dist(rng));
    }

    auto x = ComputeTritVector::from_trits(lane_x_raw).value();
    auto y = ComputeTritVector::from_trits(lane_y_raw).value();

    for (auto _ : state) {
        for (size_t l=0; l<num_lanes; ++l) {
            tritwise_and(x, y);
            benchmark::DoNotOptimize(x.data().data());
        }
    }
}
BENCHMARK(BM_TensorLane_Library)->Arg(17)->Arg(31)->Arg(67)->Arg(127)->Arg(257);
