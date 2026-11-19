// Portable Dual-Rail Research Benchmark:
//   1. Zero-Dequantization Saturated Ternary Adapter Composition: clamp(W + delta, -1, +1)
//   2. Kleene 3-Valued Policy Composition & Conflict Audit (ALLOW=+1, UNKNOWN=0, DENY=-1)
//   3. Dual-Rail Tri-State Policy Revocation Filter (256 KB fail-closed Bloom filter)

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#define T81_HAS_NEON 1
#else
#define T81_HAS_NEON 0
#endif

namespace {

using Clock = std::chrono::steady_clock;

double elapsed_us(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::micro>(b - a).count();
}

void kleene_dualrail(const uint64_t* __restrict ap, const uint64_t* __restrict an,
                     const uint64_t* __restrict bp, const uint64_t* __restrict bn,
                     uint64_t* __restrict op, uint64_t* __restrict on, size_t words,
                     uint64_t& allow_c, uint64_t& deny_c, uint64_t& conf_c) {
  uint64_t ac = 0, dc = 0, cc = 0;
  size_t w = 0;
#if T81_HAS_NEON
  while (w + 2 <= words) {
    uint8x16_t a_acc = vdupq_n_u8(0);
    uint8x16_t d_acc = vdupq_n_u8(0);
    uint8x16_t c_acc = vdupq_n_u8(0);
    const size_t chunk_end = std::min(words & ~size_t(1), w + 60);
    for (; w < chunk_end; w += 2) {
      uint64x2_t vap = vld1q_u64(ap + w);
      uint64x2_t van = vld1q_u64(an + w);
      uint64x2_t vbp = vld1q_u64(bp + w);
      uint64x2_t vbn = vld1q_u64(bn + w);

      uint64x2_t vop = vandq_u64(vap, vbp);
      uint64x2_t von = vorrq_u64(van, vbn);
      uint64x2_t vconf = vorrq_u64(vandq_u64(vap, vbn), vandq_u64(van, vbp));

      vst1q_u64(op + w, vop);
      vst1q_u64(on + w, von);

      a_acc = vaddq_u8(a_acc, vcntq_u8(vreinterpretq_u8_u64(vop)));
      d_acc = vaddq_u8(d_acc, vcntq_u8(vreinterpretq_u8_u64(von)));
      c_acc = vaddq_u8(c_acc, vcntq_u8(vreinterpretq_u8_u64(vconf)));
    }
    ac += vaddlvq_u8(a_acc);
    dc += vaddlvq_u8(d_acc);
    cc += vaddlvq_u8(c_acc);
  }
#endif
  for (; w < words; ++w) {
    const uint64_t rp = ap[w] & bp[w];
    const uint64_t rn = an[w] | bn[w];
    const uint64_t rc = (ap[w] & bn[w]) | (an[w] & bp[w]);
    op[w] = rp;
    on[w] = rn;
    ac += static_cast<uint64_t>(std::popcount(rp));
    dc += static_cast<uint64_t>(std::popcount(rn));
    cc += static_cast<uint64_t>(std::popcount(rc));
  }
  allow_c = ac;
  deny_c = dc;
  conf_c = cc;
}

void kleene_int8(const int8_t* __restrict a, const int8_t* __restrict b, int8_t* __restrict out,
                 size_t n, uint64_t& allow_c, uint64_t& deny_c, uint64_t& conf_c) {
  uint64_t ac = 0, dc = 0, cc = 0;
  size_t i = 0;
#if T81_HAS_NEON
  const int8x16_t v_pos1 = vdupq_n_s8(1);
  const int8x16_t v_neg1 = vdupq_n_s8(-1);
  const int8x16_t v_conf = vdupq_n_s8(-2);
  while (i + 16 <= n) {
    uint8x16_t a_acc = vdupq_n_u8(0);
    uint8x16_t d_acc = vdupq_n_u8(0);
    uint8x16_t c_acc = vdupq_n_u8(0);
    const size_t chunk_end = std::min(n & ~size_t(15), i + 255 * 16);
    for (; i < chunk_end; i += 16) {
      int8x16_t va = vld1q_s8(a + i);
      int8x16_t vb = vld1q_s8(b + i);
      int8x16_t vo = vminq_s8(va, vb);
      vst1q_s8(out + i, vo);
      a_acc = vsubq_u8(a_acc, vceqq_s8(vo, v_pos1));
      d_acc = vsubq_u8(d_acc, vceqq_s8(vo, v_neg1));
      c_acc = vsubq_u8(c_acc, vceqq_s8(veorq_s8(va, vb), v_conf));
    }
    ac += vaddlvq_u8(a_acc);
    dc += vaddlvq_u8(d_acc);
    cc += vaddlvq_u8(c_acc);
  }
#endif
  for (; i < n; ++i) {
    const int8_t r = std::min(a[i], b[i]);
    out[i] = r;
    ac += static_cast<uint64_t>(r == 1);
    dc += static_cast<uint64_t>(r == -1);
    cc += static_cast<uint64_t>((a[i] * b[i]) == -1);
  }
  allow_c = ac;
  deny_c = dc;
  conf_c = cc;
}

void sat_add_dualrail(const uint64_t* __restrict wp, const uint64_t* __restrict wn,
                      const uint64_t* __restrict dp, const uint64_t* __restrict dn,
                      uint64_t* __restrict op, uint64_t* __restrict on, size_t words,
                      uint64_t& flips) {
  uint64_t f = 0;
  size_t w = 0;
#if T81_HAS_NEON
  while (w + 2 <= words) {
    uint8x16_t f_acc = vdupq_n_u8(0);
    const size_t chunk_end = std::min(words & ~size_t(1), w + 60);
    for (; w < chunk_end; w += 2) {
      uint64x2_t vwp = vld1q_u64(wp + w);
      uint64x2_t vwn = vld1q_u64(wn + w);
      uint64x2_t vdp = vld1q_u64(dp + w);
      uint64x2_t vdn = vld1q_u64(dn + w);

      uint64x2_t vop = vorrq_u64(vbicq_u64(vwp, vdn), vbicq_u64(vdp, vwn));
      uint64x2_t von = vorrq_u64(vbicq_u64(vwn, vdp), vbicq_u64(vdn, vwp));

      vst1q_u64(op + w, vop);
      vst1q_u64(on + w, von);

      uint64x2_t diff = vorrq_u64(veorq_u64(vwp, vop), veorq_u64(vwn, von));
      f_acc = vaddq_u8(f_acc, vcntq_u8(vreinterpretq_u8_u64(diff)));
    }
    f += vaddlvq_u8(f_acc);
  }
#endif
  for (; w < words; ++w) {
    const uint64_t rp = (wp[w] & ~dn[w]) | (dp[w] & ~wn[w]);
    const uint64_t rn = (wn[w] & ~dp[w]) | (dn[w] & ~wp[w]);
    op[w] = rp;
    on[w] = rn;
    f += static_cast<uint64_t>(std::popcount((wp[w] ^ rp) | (wn[w] ^ rn)));
  }
  flips = f;
}

void sat_add_int8(const int8_t* __restrict w, const int8_t* __restrict d, int8_t* __restrict out,
                  size_t n, uint64_t& flips) {
  uint64_t f = 0;
  size_t i = 0;
#if T81_HAS_NEON
  const int8x16_t v_pos1 = vdupq_n_s8(1);
  const int8x16_t v_neg1 = vdupq_n_s8(-1);
  while (i + 16 <= n) {
    uint8x16_t f_acc = vdupq_n_u8(0);
    const size_t chunk_end = std::min(n & ~size_t(15), i + 255 * 16);
    for (; i < chunk_end; i += 16) {
      int8x16_t vw = vld1q_s8(w + i);
      int8x16_t vd = vld1q_s8(d + i);
      int8x16_t vs = vminq_s8(vmaxq_s8(vaddq_s8(vw, vd), v_neg1), v_pos1);
      vst1q_s8(out + i, vs);
      f_acc = vsubq_u8(f_acc, vmvnq_u8(vceqq_s8(vw, vs)));
    }
    f += vaddlvq_u8(f_acc);
  }
#endif
  for (; i < n; ++i) {
    const int s = static_cast<int>(w[i]) + static_cast<int>(d[i]);
    const int8_t c = static_cast<int8_t>(std::max(-1, std::min(1, s)));
    out[i] = c;
    f += static_cast<uint64_t>(c != w[i]);
  }
  flips = f;
}

struct TriStateBloomFilter {
  std::vector<uint64_t> pos;
  std::vector<uint64_t> neg;
  uint64_t mask = 0;

  explicit TriStateBloomFilter(size_t bits_pow2)
      : pos(bits_pow2 / 64, 0ULL), neg(bits_pow2 / 64, 0ULL), mask(bits_pow2 - 1) {}

  static inline uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

  void insert(uint64_t key, int8_t verdict) {
    const uint64_t h1 = mix(key);
    const uint64_t h2 = mix(h1 ^ 0x9e3779b97f4a7c15ULL);
    auto& plane = (verdict > 0) ? pos : neg;
    for (int k = 0; k < 4; ++k) {
      const uint64_t b = (h1 + static_cast<uint64_t>(k) * h2) & mask;
      plane[b >> 6] |= (1ULL << (b & 63));
    }
  }

  inline int8_t query(uint64_t key) const {
    const uint64_t h1 = mix(key);
    const uint64_t h2 = mix(h1 ^ 0x9e3779b97f4a7c15ULL);
    uint64_t deny_hit = 1, allow_hit = 1;
    for (int k = 0; k < 4; ++k) {
      const uint64_t b = (h1 + static_cast<uint64_t>(k) * h2) & mask;
      const uint64_t bit = 1ULL << (b & 63);
      deny_hit &= static_cast<uint64_t>((neg[b >> 6] & bit) != 0);
      allow_hit &= static_cast<uint64_t>((pos[b >> 6] & bit) != 0);
    }
    if (deny_hit) return -1;
    if (allow_hit) return 1;
    return 0;
  }
};

}  // namespace

int main() {
  constexpr size_t N = 1 << 20;
  constexpr size_t WORDS = N / 64;
  constexpr int ITERS = 100;

  std::mt19937_64 rng(7);
  std::uniform_int_distribution<int> dist(-1, 1);

  std::vector<int8_t> a8(N), b8(N), o8(N);
  std::vector<uint64_t> ap(WORDS, 0), an(WORDS, 0), bp(WORDS, 0), bn(WORDS, 0);
  std::vector<uint64_t> op(WORDS, 0), on(WORDS, 0);
  for (size_t i = 0; i < N; ++i) {
    const int8_t va = static_cast<int8_t>(dist(rng));
    const int8_t vb = static_cast<int8_t>(dist(rng));
    a8[i] = va;
    b8[i] = vb;
    if (va > 0)
      ap[i >> 6] |= (1ULL << (i & 63));
    else if (va < 0)
      an[i >> 6] |= (1ULL << (i & 63));
    if (vb > 0)
      bp[i >> 6] |= (1ULL << (i & 63));
    else if (vb < 0)
      bn[i >> 6] |= (1ULL << (i & 63));
  }

  uint64_t ac1 = 0, dc1 = 0, cc1 = 0, ac2 = 0, dc2 = 0, cc2 = 0, f1 = 0, f2 = 0;
  kleene_dualrail(ap.data(), an.data(), bp.data(), bn.data(), op.data(), on.data(), WORDS, ac1, dc1,
                  cc1);
  kleene_int8(a8.data(), b8.data(), o8.data(), N, ac2, dc2, cc2);
  const bool kleene_exact = (ac1 == ac2 && dc1 == dc2 && cc1 == cc2);

  volatile uint64_t sink = 0;
  auto t0 = Clock::now();
  for (int it = 0; it < ITERS; ++it) {
    kleene_dualrail(ap.data(), an.data(), bp.data(), bn.data(), op.data(), on.data(), WORDS, ac1,
                    dc1, cc1);
    sink += ac1 + dc1 + cc1 + op[static_cast<size_t>(it) & 63U];
  }
  auto t1 = Clock::now();
  for (int it = 0; it < ITERS; ++it) {
    kleene_int8(a8.data(), b8.data(), o8.data(), N, ac2, dc2, cc2);
    sink += ac2 + dc2 + cc2 + static_cast<uint64_t>(o8[static_cast<size_t>(it) & 63U]);
  }
  auto t2 = Clock::now();

  sat_add_dualrail(ap.data(), an.data(), bp.data(), bn.data(), op.data(), on.data(), WORDS, f1);
  sat_add_int8(a8.data(), b8.data(), o8.data(), N, f2);
  const bool sat_add_exact = (f1 == f2);

  auto t3 = Clock::now();
  for (int it = 0; it < ITERS; ++it) {
    sat_add_dualrail(ap.data(), an.data(), bp.data(), bn.data(), op.data(), on.data(), WORDS, f1);
    sink += f1 + op[static_cast<size_t>(it) & 63U];
  }
  auto t4 = Clock::now();
  for (int it = 0; it < ITERS; ++it) {
    sat_add_int8(a8.data(), b8.data(), o8.data(), N, f2);
    sink += f2 + static_cast<uint64_t>(o8[static_cast<size_t>(it) & 63U]);
  }
  auto t5 = Clock::now();

  // Tri-State Policy Revocation Filter
  TriStateBloomFilter filter(1 << 20);
  for (uint64_t i = 0; i < 10000; ++i) filter.insert(i, +1);
  for (uint64_t i = 0; i < 5000; ++i) filter.insert(i, -1);
  uint64_t fn_revoked = 0;
  for (uint64_t i = 0; i < 5000; ++i) {
    if (filter.query(i) != -1) ++fn_revoked;
  }
  constexpr uint64_t Q = 1000000;
  auto t6 = Clock::now();
  int64_t qs = 0;
  for (uint64_t i = 0; i < Q; ++i) qs += filter.query(i);
  sink += static_cast<uint64_t>(qs);
  auto t7 = Clock::now();

  const double k_dr = elapsed_us(t0, t1) / ITERS;
  const double k_i8 = elapsed_us(t1, t2) / ITERS;
  const double s_dr = elapsed_us(t3, t4) / ITERS;
  const double s_i8 = elapsed_us(t4, t5) / ITERS;
  const double f_ns = (elapsed_us(t6, t7) * 1000.0) / static_cast<double>(Q);

  // Sparse-Block Dual-Rail (SBDR1) vs Base-243 at 95% block-zero adapter sparsity (N=1M trits)
  int8_t lut243[243][5];
  for (int b = 0; b < 243; ++b) {
    int x = b;
    for (int k = 0; k < 5; ++k) {
      lut243[b][k] = static_cast<int8_t>((x % 3) - 1);
      x /= 3;
    }
  }
  std::vector<int8_t> sp_raw(N, 0);
  std::vector<uint64_t> sp_p(WORDS, 0), sp_n(WORDS, 0);
  std::uniform_int_distribution<int> sp_gate(0, 19);  // 5% active blocks (95% block-zero)
  for (size_t w = 0; w < WORDS; ++w) {
    if (sp_gate(rng) != 0) continue;
    for (size_t b = 0; b < 64; ++b) {
      const int8_t v = static_cast<int8_t>(dist(rng));
      sp_raw[w * 64 + b] = v;
      if (v > 0)
        sp_p[w] |= (1ULL << b);
      else if (v < 0)
        sp_n[w] |= (1ULL << b);
    }
  }
  std::vector<uint8_t> b243((N + 4) / 5, 0);
  for (size_t i = 0, o = 0; i + 5 <= N; i += 5, ++o) {
    b243[o] =
        static_cast<uint8_t>((sp_raw[i] + 1) + 3 * (sp_raw[i + 1] + 1) + 9 * (sp_raw[i + 2] + 1) +
                             27 * (sp_raw[i + 3] + 1) + 81 * (sp_raw[i + 4] + 1));
  }
  std::vector<uint64_t> sb_mask(WORDS / 64, 0), sb_p, sb_n;
  for (size_t w = 0; w < WORDS; ++w) {
    if ((sp_p[w] | sp_n[w]) != 0ULL) {
      sb_mask[w >> 6] |= (1ULL << (w & 63));
      sb_p.push_back(sp_p[w]);
      sb_n.push_back(sp_n[w]);
    }
  }
  std::vector<int8_t> dec243(N, 0);
  auto t8 = Clock::now();
  for (size_t i = 0, o = 0; i + 5 <= N; i += 5, ++o) {
    const int8_t* t = lut243[b243[o]];
    dec243[i] = t[0];
    dec243[i + 1] = t[1];
    dec243[i + 2] = t[2];
    dec243[i + 3] = t[3];
    dec243[i + 4] = t[4];
  }
  sink += static_cast<uint64_t>(dec243[0] + dec243[N - 5]);
  auto t9 = Clock::now();
  std::vector<uint64_t> rec_p(WORDS, 0), rec_n(WORDS, 0);
  size_t sidx = 0;
  for (size_t m = 0; m < sb_mask.size(); ++m) {
    uint64_t bits = sb_mask[m];
    while (bits != 0ULL) {
      const unsigned tz = static_cast<unsigned>(std::countr_zero(bits));
      const size_t w = (m << 6) + tz;
      rec_p[w] = sb_p[sidx];
      rec_n[w] = sb_n[sidx];
      ++sidx;
      bits &= (bits - 1ULL);
    }
  }
  sink += rec_p[0] + rec_n[WORDS - 1];
  auto t10 = Clock::now();
  const bool sbdr_exact = (rec_p == sp_p && rec_n == sp_n);
  const double sbdr_bpt =
      static_cast<double>(16 + sb_mask.size() * 8 + (sb_p.size() + sb_n.size()) * 8) * 8.0 /
      static_cast<double>(N);
  const double b243_us = elapsed_us(t8, t9);
  const double sbdr_us = elapsed_us(t9, t10);

  std::cout << std::fixed << std::setprecision(2) << "{\n"
            << "  \"schema\": \"t81.policy-and-adapters-bench.v1\",\n"
            << "  \"simd\": \"" << (T81_HAS_NEON ? "arm64_neon" : "portable_scalar") << "\",\n"
            << "  \"kleene_policy_1m\": {\n"
            << "    \"exact_match\": " << (kleene_exact ? "true" : "false") << ",\n"
            << "    \"dualrail_us\": " << k_dr << ",\n"
            << "    \"int8_us\": " << k_i8 << ",\n"
            << "    \"speedup_vs_int8\": " << (k_i8 / k_dr) << "\n"
            << "  },\n"
            << "  \"sat_add_adapter_1m\": {\n"
            << "    \"exact_match\": " << (sat_add_exact ? "true" : "false") << ",\n"
            << "    \"dualrail_us\": " << s_dr << ",\n"
            << "    \"int8_us\": " << s_i8 << ",\n"
            << "    \"speedup_vs_int8\": " << (s_i8 / s_dr) << "\n"
            << "  },\n"
            << "  \"tristate_revocation_filter_256kb\": {\n"
            << "    \"revoked_false_negatives\": " << fn_revoked << ",\n"
            << "    \"ns_per_query\": " << f_ns << "\n"
            << "  },\n"
            << "  \"sbdr1_vs_base243_95pct_sparse_1m\": {\n"
            << "    \"exact_match\": " << (sbdr_exact ? "true" : "false") << ",\n"
            << "    \"base243_bits_per_trit\": 1.60,\n"
            << "    \"sbdr1_bits_per_trit\": " << sbdr_bpt << ",\n"
            << "    \"base243_decode_us\": " << b243_us << ",\n"
            << "    \"sbdr1_decode_us\": " << sbdr_us << ",\n"
            << "    \"decode_speedup\": " << (b243_us / std::max(0.01, sbdr_us)) << "\n"
            << "  }\n"
            << "}\n";
  return (kleene_exact && sat_add_exact && sbdr_exact && fn_revoked == 0) ? 0 : 1;
}
