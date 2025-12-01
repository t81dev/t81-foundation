#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cstdint>
#include "t81/core/cell.hpp"
#include "t81/core/cell_packed.hpp"
#include "t81/t81.hpp"

namespace {
    const size_t DATA_SIZE = 100000;
    std::vector<t81::core::Cell> t81_source_data;
    std::vector<t81::core::Cell> t81_dest_data;
    std::vector<int64_t> int64_source_data;
    std::vector<int64_t> int64_dest_data;
    std::vector<t81::core::packed::PackedCell> packed_source_data;
    std::vector<t81::core::packed::PackedCell> packed_dest_data;

    t81::core::packed::PackedCell packed_from_int(int64_t v) {
        bool negative = v < 0;
        if (negative) v = -v;
        std::array<t81::core::Trit, t81::core::Cell::TRITS> trits{};
        for (int i = 0; v != 0 && i < t81::core::Cell::TRITS; ++i) {
            int rem = static_cast<int>(v % 3);
            if (rem == 2) {
                trits[i] = t81::core::Trit::M;
                v = v / 3 + 1;
            } else {
                trits[i] = static_cast<t81::core::Trit>(rem - 1);
                v /= 3;
            }
        }
        if (negative) {
            for (auto& t : trits) {
                t = static_cast<t81::core::Trit>(-static_cast<int>(t));
            }
        }
        return t81::core::packed::PackedCell::from_trits(trits);
    }

    void setup_negation() {
        if (!t81_source_data.empty()) return;
        std::mt19937_64 gen(0x781);
        std::uniform_int_distribution<int64_t> distrib(t81::core::Cell::MIN, t81::core::Cell::MAX);
        t81_source_data.reserve(DATA_SIZE);
        int64_source_data.reserve(DATA_SIZE);
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_t val = distrib(gen);
            t81_source_data.push_back(t81::core::Cell::from_int(val));
            int64_source_data.push_back(val);
            packed_source_data.push_back(packed_from_int(val));
            packed_dest_data.emplace_back();
        }
        t81_dest_data.resize(DATA_SIZE);
        int64_dest_data.resize(DATA_SIZE);
        packed_dest_data.resize(DATA_SIZE);
    }
}

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

static void BM_NegationSpeed_T81Cell(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_dest_data[i] = -t81_source_data[i];
        }
        benchmark::DoNotOptimize(t81_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("Free negation (no borrow)");
}
BENCHMARK(BM_NegationSpeed_T81Cell);

static void BM_NegationSpeed_PackedCell(benchmark::State& state) {
    setup_negation();
#if defined(__x86_64__) && defined(__AVX2__)
    const size_t n = packed_source_data.size();
    auto* src = reinterpret_cast<const uint8_t*>(packed_source_data.data());
    auto* dst = reinterpret_cast<uint8_t*>(packed_dest_data.data());
    const __m256i neg_const = _mm256_set1_epi8(t81::core::packed::PackedCell::MAX_INDEX);

    for (auto _ : state) {
        size_t i = 0;
        // Process 32 elements at a time with AVX2
        for (; i + 31 < n; i += 32) {
            __m256i v_src = _mm256_loadu_si256((const __m256i*)(src + i));
            __m256i v_res = _mm256_sub_epi8(neg_const, v_src);
            _mm256_storeu_si256((__m256i*)(dst + i), v_res);
        }
        // Process remaining elements scalerly
        for (; i < n; ++i) {
            dst[i] = t81::core::packed::PackedCell::MAX_INDEX - src[i];
        }
        benchmark::DoNotOptimize(packed_dest_data.data());
    }
    state.SetLabel("Packed AVX2 negation");
#else
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            packed_dest_data[i] = -packed_source_data[i];
        }
        benchmark::DoNotOptimize(packed_dest_data.data());
    }
    state.SetLabel("Packed arithmetic negation");
#endif
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
}
BENCHMARK(BM_NegationSpeed_PackedCell);

static void BM_NegationSpeed_Int64(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_dest_data[i] = -int64_source_data[i];
        }
        benchmark::DoNotOptimize(int64_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("~x+1 in two’s complement");
}
BENCHMARK(BM_NegationSpeed_Int64);

static void BM_NegationSpeed_T81Native(benchmark::State& state) {
    t81::T81 a{};
#if defined(__x86_64__) && defined(__AVX2__)
    a = t81::T81{_mm256_set1_epi8(0x55)};
#else
    int8_t digits[128];
    for (int idx = 0; idx < 128; ++idx) {
        digits[idx] = static_cast<int8_t>((idx % 3) - 1);
    }
    a = t81::T81{t81::detail::pack_digits(digits)};
#endif
    t81::T81 res{};
    for (auto _ : state) {
        benchmark::DoNotOptimize(res = -a);
        a = res;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Native T81 negation (PSHUFB)");
}
BENCHMARK(BM_NegationSpeed_T81Native);
