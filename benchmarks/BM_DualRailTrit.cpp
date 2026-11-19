// End-to-End Dual-Rail (pos/neg bit-sliced) Ternary Benchmark
// Compares DualRail64 (with SIMD pack/unpack) against:
//   - Competent int8_t binary control (NEON vdotq_s32 / int32_t acc, auto-vectorized min)
//   - Existing t81::ComputeTritVector (2-bit interleaved SWAR/SIMD)
// Reports both kernel-only throughput and pack/unpack conversion costs so break-even
// reuse factor K* can be evaluated honestly on conventional binary hardware.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <random>
#include <vector>

#include "t81/packed_trit_vector.hpp"

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

namespace {

struct DualRail64 {
  size_t n_trits = 0;
  std::vector<uint64_t> pos;
  std::vector<uint64_t> neg;

  static DualRail64 pack(const std::vector<int8_t>& trits) {
    DualRail64 dr;
    dr.n_trits = trits.size();
    const size_t words = (trits.size() + 63) / 64;
    dr.pos.assign(words, 0ULL);
    dr.neg.assign(words, 0ULL);

    const int8_t* src = trits.data();
    size_t w = 0;
    const size_t full_words = trits.size() / 64;

#if defined(__aarch64__) && defined(__ARM_NEON)
    const int8x16_t zero = vdupq_n_s8(0);
    static constexpr uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                                1, 2, 4, 8, 16, 32, 64, 128};
    const uint8x16_t weights = vld1q_u8(kBitWeights);

    for (; w < full_words; ++w) {
      const int8_t* p = src + w * 64;
      const int8x16_t v0 = vld1q_s8(p + 0);
      const int8x16_t v1 = vld1q_s8(p + 16);
      const int8x16_t v2 = vld1q_s8(p + 32);
      const int8x16_t v3 = vld1q_s8(p + 48);

      const uint8x16_t pm0 = vandq_u8(vcgtq_s8(v0, zero), weights);
      const uint8x16_t pm1 = vandq_u8(vcgtq_s8(v1, zero), weights);
      const uint8x16_t pm2 = vandq_u8(vcgtq_s8(v2, zero), weights);
      const uint8x16_t pm3 = vandq_u8(vcgtq_s8(v3, zero), weights);
      const uint8x16_t pq = vpaddq_u8(vpaddq_u8(pm0, pm1), vpaddq_u8(pm2, pm3));
      dr.pos[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(pq, pq)), 0);

      const uint8x16_t nm0 = vandq_u8(vcltq_s8(v0, zero), weights);
      const uint8x16_t nm1 = vandq_u8(vcltq_s8(v1, zero), weights);
      const uint8x16_t nm2 = vandq_u8(vcltq_s8(v2, zero), weights);
      const uint8x16_t nm3 = vandq_u8(vcltq_s8(v3, zero), weights);
      const uint8x16_t nq = vpaddq_u8(vpaddq_u8(nm0, nm1), vpaddq_u8(nm2, nm3));
      dr.neg[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(nq, nq)), 0);
    }
#elif defined(__x86_64__) && defined(__AVX2__)
    const __m256i zero = _mm256_setzero_si256();
    for (; w < full_words; ++w) {
      const int8_t* p = src + w * 64;
      const __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 0));
      const __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 32));
      const uint32_t plo = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(v0, zero)));
      const uint32_t phi = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(v1, zero)));
      const uint32_t nlo = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(zero, v0)));
      const uint32_t nhi = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(zero, v1)));
      dr.pos[w] = static_cast<uint64_t>(plo) | (static_cast<uint64_t>(phi) << 32);
      dr.neg[w] = static_cast<uint64_t>(nlo) | (static_cast<uint64_t>(nhi) << 32);
    }
#endif

    for (; w < words; ++w) {
      uint64_t p = 0;
      uint64_t n = 0;
      const size_t base = w * 64;
      const size_t lim = std::min(trits.size(), base + 64);
      for (size_t i = base; i < lim; ++i) {
        const int8_t t = src[i];
        const uint64_t bit = 1ULL << (i - base);
        if (t > 0)
          p |= bit;
        else if (t < 0)
          n |= bit;
      }
      dr.pos[w] = p;
      dr.neg[w] = n;
    }
    return dr;
  }

  std::vector<int8_t> unpack() const {
    std::vector<int8_t> out(n_trits, 0);
    int8_t* dst = out.data();
    const size_t words = pos.size();
    const size_t full_words = n_trits / 64;
    size_t w = 0;

#if defined(__aarch64__) && defined(__ARM_NEON)
    static constexpr uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                                1, 2, 4, 8, 16, 32, 64, 128};
    const uint8x16_t weights = vld1q_u8(kBitWeights);
    static constexpr uint8_t kShuffleIdx[4][16] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
        {4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5},
        {6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7},
    };
    const uint8x16_t idx0 = vld1q_u8(kShuffleIdx[0]);
    const uint8x16_t idx1 = vld1q_u8(kShuffleIdx[1]);
    const uint8x16_t idx2 = vld1q_u8(kShuffleIdx[2]);
    const uint8x16_t idx3 = vld1q_u8(kShuffleIdx[3]);

    for (; w < full_words; ++w) {
      const uint8x16_t pb = vreinterpretq_u8_u64(vdupq_n_u64(pos[w]));
      const uint8x16_t nb = vreinterpretq_u8_u64(vdupq_n_u64(neg[w]));
      int8_t* d = dst + w * 64;
      auto emit16 = [&](uint8x16_t idx, int8_t* ptr) {
        const uint8x16_t pm = vtstq_u8(vqtbl1q_u8(pb, idx), weights);
        const uint8x16_t nm = vtstq_u8(vqtbl1q_u8(nb, idx), weights);
        vst1q_s8(ptr, vsubq_s8(vreinterpretq_s8_u8(nm), vreinterpretq_s8_u8(pm)));
      };
      emit16(idx0, d + 0);
      emit16(idx1, d + 16);
      emit16(idx2, d + 32);
      emit16(idx3, d + 48);
    }
#endif

    for (; w < words; ++w) {
      const uint64_t p = pos[w];
      const uint64_t n = neg[w];
      const size_t base = w * 64;
      const size_t lim = std::min(n_trits, base + 64);
      for (size_t i = base; i < lim; ++i) {
        const size_t shift = i - base;
        dst[i] = static_cast<int8_t>(((p >> shift) & 1ULL) - ((n >> shift) & 1ULL));
      }
    }
    return out;
  }
};

int64_t dual_rail_dot(const DualRail64& a, const DualRail64& b) {
  int64_t acc = 0;
  const size_t words = a.pos.size();
  const uint64_t* ap = a.pos.data();
  const uint64_t* an = a.neg.data();
  const uint64_t* bp = b.pos.data();
  const uint64_t* bn = b.neg.data();
  for (size_t w = 0; w < words; ++w) {
    const uint64_t same = (ap[w] & bp[w]) | (an[w] & bn[w]);
    const uint64_t diff = (ap[w] & bn[w]) | (an[w] & bp[w]);
    acc += static_cast<int64_t>(std::popcount(same)) - static_cast<int64_t>(std::popcount(diff));
  }
  return acc;
}

void dual_rail_and(const DualRail64& a, const DualRail64& b, DualRail64& out) {
  const size_t words = a.pos.size();
  const uint64_t* ap = a.pos.data();
  const uint64_t* an = a.neg.data();
  const uint64_t* bp = b.pos.data();
  const uint64_t* bn = b.neg.data();
  uint64_t* op = out.pos.data();
  uint64_t* on = out.neg.data();
  for (size_t w = 0; w < words; ++w) {
    on[w] = an[w] | bn[w];
    op[w] = ap[w] & bp[w];
  }
}

int64_t int8_dot_competent(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
  const size_t n = a.size();
  const int8_t* ap = a.data();
  const int8_t* bp = b.data();
  size_t i = 0;
  int64_t total = 0;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  int32x4_t acc0 = vdupq_n_s32(0);
  int32x4_t acc1 = vdupq_n_s32(0);
  int32x4_t acc2 = vdupq_n_s32(0);
  int32x4_t acc3 = vdupq_n_s32(0);
  for (; i + 64 <= n; i += 64) {
    acc0 = vdotq_s32(acc0, vld1q_s8(ap + i + 0), vld1q_s8(bp + i + 0));
    acc1 = vdotq_s32(acc1, vld1q_s8(ap + i + 16), vld1q_s8(bp + i + 16));
    acc2 = vdotq_s32(acc2, vld1q_s8(ap + i + 32), vld1q_s8(bp + i + 32));
    acc3 = vdotq_s32(acc3, vld1q_s8(ap + i + 48), vld1q_s8(bp + i + 48));
  }
  acc0 = vaddq_s32(vaddq_s32(acc0, acc1), vaddq_s32(acc2, acc3));
  total = static_cast<int64_t>(vaddvq_s32(acc0));
#endif
  int32_t tail = 0;
  for (; i < n; ++i) {
    tail += static_cast<int32_t>(ap[i]) * static_cast<int32_t>(bp[i]);
  }
  return total + tail;
}

std::vector<int8_t> make_trits(size_t n, uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v(n);
  for (size_t i = 0; i < n; ++i) v[i] = static_cast<int8_t>(dist(rng));
  return v;
}

static void BM_DualRail_Pack(benchmark::State& state) {
  const auto v = make_trits(static_cast<size_t>(state.range(0)), 42);
  for (auto _ : state) {
    auto dr = DualRail64::pack(v);
    benchmark::DoNotOptimize(dr.pos.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DualRail_Pack)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_DualRail_Unpack(benchmark::State& state) {
  const auto dr = DualRail64::pack(make_trits(static_cast<size_t>(state.range(0)), 42));
  for (auto _ : state) {
    auto out = dr.unpack();
    benchmark::DoNotOptimize(out.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DualRail_Unpack)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_DualRail_Dot(benchmark::State& state) {
  const size_t n = static_cast<size_t>(state.range(0));
  const auto a = DualRail64::pack(make_trits(n, 42));
  const auto b = DualRail64::pack(make_trits(n, 43));
  for (auto _ : state) {
    auto r = dual_rail_dot(a, b);
    benchmark::DoNotOptimize(r);
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DualRail_Dot)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_Int8_DotCompetent(benchmark::State& state) {
  const size_t n = static_cast<size_t>(state.range(0));
  const auto a = make_trits(n, 42);
  const auto b = make_trits(n, 43);
  for (auto _ : state) {
    auto r = int8_dot_competent(a, b);
    benchmark::DoNotOptimize(r);
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Int8_DotCompetent)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_DualRail_TAnd(benchmark::State& state) {
  const size_t n = static_cast<size_t>(state.range(0));
  const auto a = DualRail64::pack(make_trits(n, 42));
  const auto b = DualRail64::pack(make_trits(n, 43));
  auto out = a;
  for (auto _ : state) {
    dual_rail_and(a, b, out);
    benchmark::DoNotOptimize(out.pos.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DualRail_TAnd)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_Int8_TAnd(benchmark::State& state) {
  const size_t n = static_cast<size_t>(state.range(0));
  const auto a = make_trits(n, 42);
  const auto b = make_trits(n, 43);
  std::vector<int8_t> out(n);
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i) out[i] = a[i] < b[i] ? a[i] : b[i];
    benchmark::DoNotOptimize(out.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Int8_TAnd)->Arg(256)->Arg(4096)->Arg(65536);

static void BM_ComputeTritVector_TAnd(benchmark::State& state) {
  const size_t n = static_cast<size_t>(state.range(0));
  const auto a = t81::ComputeTritVector::from_trits(make_trits(n, 42)).value();
  const auto b = t81::ComputeTritVector::from_trits(make_trits(n, 43)).value();
  auto work = a;
  for (auto _ : state) {
    work = a;
    auto r = work.t_and_inplace(b);
    benchmark::DoNotOptimize(r);
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ComputeTritVector_TAnd)->Arg(256)->Arg(4096)->Arg(65536);

}  // namespace
