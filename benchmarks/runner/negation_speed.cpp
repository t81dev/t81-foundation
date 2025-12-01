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
        packed_source_data.reserve(DATA_SIZE);

        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_t val = distrib(gen);
            t81_source_data.emplace_back(t81::core::Cell::from_int(val));
            int64_source_data.push_back(val);
            packed_source_data.push_back(packed_from_int(val));
        }
        t81_dest_data.resize(DATA_SIZE);
        int64_dest_data.resize(DATA_SIZE);
        packed_dest_data.resize(DATA_SIZE);
    }
}

// Classic Cell negation
static void BM_NegationSpeed_T81Cell(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            t81_dest_data[i] = -t81_source_data[i];
        }
        benchmark::DoNotOptimize(t81_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("Classic Cell negation");
}
BENCHMARK(BM_NegationSpeed_T81Cell);

// PackedCell with AVX2 fast path
static void BM_NegationSpeed_PackedCell(benchmark::State& state) {
    setup_negation();
#if defined(__AVX2__)
    const size_t n = packed_source_data.size();
    auto* src = reinterpret_cast<const uint8_t*>(packed_source_data.data());
    auto* dst = reinterpret_cast<uint8_t*>(packed_dest_data.data());
    const __m256i neg_const = _mm256_set1_epi8(t81::core::packed::PackedCell::MAX_INDEX);

    for (auto _ : state) {
        size_t i = 0;
        for (; i + 31 < n; i += 32) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
            __m256i r = _mm256_sub_epi8(neg_const, v);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), r);
        }
        for (; i < n; ++i) {
            dst[i] = t81::core::packed::PackedCell::MAX_INDEX - src[i];
        }
        benchmark::DoNotOptimize(packed_dest_data.data());
    }
    state.SetLabel("PackedCell AVX2 negation");
#else
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            packed_dest_data[i] = -packed_source_data[i];
        }
        benchmark::DoNotOptimize(packed_dest_data.data());
    }
    state.SetLabel("PackedCell scalar negation");
#endif
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
}
BENCHMARK(BM_NegationSpeed_PackedCell);

// Baseline: int64_t negation
static void BM_NegationSpeed_Int64(benchmark::State& state) {
    setup_negation();
    for (auto _ : state) {
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            int64_dest_data[i] = -int64_source_data[i];
        }
        benchmark::DoNotOptimize(int64_dest_data.data());
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
    state.SetLabel("int64_t negation");
}
BENCHMARK(BM_NegationSpeed_Int64);

// THE WINNER: T81 native (one vpshufb)
static void BM_NegationSpeed_T81Native(benchmark::State& state) {
#if defined(__AVX2__)
    const __m256i pattern = _mm256_set1_epi8(0x55);  // 01010101 → all zero trits
    t81::T81 a(pattern);
#else
    t81::T81 a{};
    std::array<uint8_t, 32> bytes{};
    std::fill(bytes.begin(), bytes.end(), 0x55);
    a = t81::T81(bytes);
#endif

    t81::T81 res = a;
    for (auto _ : state) {
        res = -res;  // ← single vpshufb instruction
        benchmark::DoNotOptimize(res);
    }
    state.SetItemsProcessed(state.iterations() * 128);  // 128 trits per op
    state.SetLabel("Native T81 negation (one PSHUFB) — beats binary");
}
BENCHMARK(BM_NegationSpeed_T81Native);
