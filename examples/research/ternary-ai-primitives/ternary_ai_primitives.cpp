// Ternary AI Primitives:
//   1. Zero-Float Verifiable Agent Memory (Ternary Dual-Rail RAG vs FP32 & 1-Bit Binary)
//   2. Proof-Carrying Ternary Model Diffs (XOR Delta + Axion Weight-Flip Budget Gate)
//   3. Block-Structured Activation-Sparsity Zero-Skipping (1x4096x4096 Dual-Rail GEMV)

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "t81/crypto/sha3.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

std::string sha3_hex(const void* data, size_t bytes) {
  return t81::crypto::sha3_512_hex(
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data), bytes));
}

float fp32_dot(const float* a, const float* b, size_t D) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  float32x4_t s0 = vdupq_n_f32(0), s1 = vdupq_n_f32(0), s2 = vdupq_n_f32(0), s3 = vdupq_n_f32(0);
  for (size_t i = 0; i < D; i += 16) {
    s0 = vfmaq_f32(s0, vld1q_f32(a + i + 0), vld1q_f32(b + i + 0));
    s1 = vfmaq_f32(s1, vld1q_f32(a + i + 4), vld1q_f32(b + i + 4));
    s2 = vfmaq_f32(s2, vld1q_f32(a + i + 8), vld1q_f32(b + i + 8));
    s3 = vfmaq_f32(s3, vld1q_f32(a + i + 12), vld1q_f32(b + i + 12));
  }
  return vaddvq_f32(vaddq_f32(vaddq_f32(s0, s1), vaddq_f32(s2, s3)));
#else
  float acc = 0.0f;
  for (size_t i = 0; i < D; ++i) acc += a[i] * b[i];
  return acc;
#endif
}

int32_t bin1_sim(const uint64_t* a, const uint64_t* b, size_t W64) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  uint16x8_t acc = vdupq_n_u16(0);
  for (size_t w = 0; w < W64; w += 2) {
    uint8x16_t x =
        veorq_u8(vreinterpretq_u8_u64(vld1q_u64(a + w)), vreinterpretq_u8_u64(vld1q_u64(b + w)));
    acc = vpadalq_u8(acc, vcntq_u8(x));
  }
  return static_cast<int32_t>(W64 * 64) - 2 * static_cast<int32_t>(vaddlvq_u16(acc));
#else
  int32_t h = 0;
  for (size_t w = 0; w < W64; ++w) h += std::popcount(a[w] ^ b[w]);
  return static_cast<int32_t>(W64 * 64) - 2 * h;
#endif
}

int32_t dualrail_sim(const uint64_t* ap, const uint64_t* an, const uint64_t* bp, const uint64_t* bn,
                     size_t W64) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  uint16x8_t pa = vdupq_n_u16(0), na = vdupq_n_u16(0);
  for (size_t w = 0; w < W64; w += 2) {
    uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
    uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
    uint8x16_t bvp = vreinterpretq_u8_u64(vld1q_u64(bp + w));
    uint8x16_t bvn = vreinterpretq_u8_u64(vld1q_u64(bn + w));
    pa = vpadalq_u8(pa, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
    na = vpadalq_u8(na, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
  }
  return static_cast<int32_t>(vaddlvq_u16(pa)) - static_cast<int32_t>(vaddlvq_u16(na));
#else
  int32_t acc = 0;
  for (size_t w = 0; w < W64; ++w) {
    acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
           std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
  }
  return acc;
#endif
}

template <typename ScoreT>
std::vector<int> topk_indices(const std::vector<ScoreT>& scores, int k) {
  std::vector<int> idx(scores.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::partial_sort(idx.begin(), idx.begin() + k, idx.end(), [&](int a, int b) {
    if (scores[a] != scores[b]) return scores[a] > scores[b];
    return a < b;
  });
  idx.resize(k);
  return idx;
}

double recall_at_k(const std::vector<int>& gt, const std::vector<int>& pred) {
  std::unordered_set<int> s(gt.begin(), gt.end());
  int hits = 0;
  for (int x : pred) {
    if (s.contains(x)) ++hits;
  }
  return static_cast<double>(hits) / static_cast<double>(gt.size());
}

// ============================================================================
// Primitive #2: Proof-Carrying Ternary Model Diffs (XOR Delta + Flip Budget)
// ============================================================================
struct SparseDualRailPatch {
  struct WordDelta {
    uint32_t word_idx;
    uint64_t dp;
    uint64_t dn;
  };
  std::vector<WordDelta> deltas;
};

uint64_t count_weight_flips(const std::vector<uint64_t>& p0, const std::vector<uint64_t>& n0,
                            const std::vector<uint64_t>& p1, const std::vector<uint64_t>& n1) {
  uint64_t flips = 0;
  for (size_t w = 0; w < p0.size(); ++w) {
    flips += static_cast<uint64_t>(std::popcount((p0[w] ^ p1[w]) | (n0[w] ^ n1[w])));
  }
  return flips;
}

SparseDualRailPatch build_xor_patch(const std::vector<uint64_t>& p0,
                                    const std::vector<uint64_t>& n0,
                                    const std::vector<uint64_t>& p1,
                                    const std::vector<uint64_t>& n1) {
  SparseDualRailPatch patch;
  for (size_t w = 0; w < p0.size(); ++w) {
    const uint64_t dp = p0[w] ^ p1[w];
    const uint64_t dn = n0[w] ^ n1[w];
    if ((dp | dn) != 0ULL) {
      patch.deltas.push_back({static_cast<uint32_t>(w), dp, dn});
    }
  }
  return patch;
}

void apply_xor_patch(std::vector<uint64_t>& p, std::vector<uint64_t>& n,
                     const SparseDualRailPatch& patch) {
  for (const auto& d : patch.deltas) {
    p[d.word_idx] ^= d.dp;
    n[d.word_idx] ^= d.dn;
  }
}

// ============================================================================
// Primitive #3: Block-Structured Activation-Sparsity Zero-Skipping GEMV
// ============================================================================
void gemv_dense(const uint64_t* ap, const uint64_t* an, const uint64_t* Bp, const uint64_t* Bn,
                int32_t* C, size_t W64, size_t N) {
  for (size_t n = 0; n < N; ++n) {
    C[n] = dualrail_sim(ap, an, Bp + n * W64, Bn + n * W64, W64);
  }
}

void gemv_block_skip(const uint64_t* ap, const uint64_t* an, const uint64_t* Bp, const uint64_t* Bn,
                     int32_t* C, size_t W64, size_t N) {
  std::vector<size_t> active;
  active.reserve(W64 / 2);
  for (size_t w = 0; w < W64; w += 2) {
    if ((ap[w] | an[w] | ap[w + 1] | an[w + 1]) != 0ULL) active.push_back(w);
  }
  if (active.empty()) {
    std::fill(C, C + N, 0);
    return;
  }
  for (size_t n = 0; n < N; ++n) {
    const uint64_t* bp = Bp + n * W64;
    const uint64_t* bn = Bn + n * W64;
#if defined(__aarch64__) && defined(__ARM_NEON)
    uint16x8_t pa = vdupq_n_u16(0), na = vdupq_n_u16(0);
    for (size_t w : active) {
      uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
      uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
      uint8x16_t bvp = vreinterpretq_u8_u64(vld1q_u64(bp + w));
      uint8x16_t bvn = vreinterpretq_u8_u64(vld1q_u64(bn + w));
      pa = vpadalq_u8(pa, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
      na = vpadalq_u8(na, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
    }
    C[n] = static_cast<int32_t>(vaddlvq_u16(pa)) - static_cast<int32_t>(vaddlvq_u16(na));
#else
    int32_t acc = 0;
    for (size_t w : active) {
      acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
             std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
      acc += std::popcount((ap[w + 1] & bp[w + 1]) | (an[w + 1] & bn[w + 1])) -
             std::popcount((ap[w + 1] & bn[w + 1]) | (an[w + 1] & bp[w + 1]));
    }
    C[n] = acc;
#endif
  }
}

}  // namespace

int main() {
  // --------------------------------------------------------------------------
  // 1. Zero-Float Ternary Dual-Rail Agent Memory (4,000 docs x 4,096 dims)
  // --------------------------------------------------------------------------
  constexpr size_t kDocs = 4000;
  constexpr size_t kDim = 4096;
  constexpr size_t kW64 = kDim / 64;
  constexpr size_t kClusters = 64;
  constexpr int kQueries = 16;
  constexpr int kTopK = 10;

  std::mt19937 rng(20261119);
  std::normal_distribution<float> noise(0.0f, 0.25f);
  std::normal_distribution<float> sig(0.0f, 1.5f);
  std::uniform_real_distribution<float> u01(0.0f, 1.0f);
  std::uniform_int_distribution<size_t> cdist(0, kClusters - 1);

  std::vector<std::vector<float>> centroids(kClusters, std::vector<float>(kDim, 0.0f));
  for (size_t c = 0; c < kClusters; ++c) {
    for (size_t d = 0; d < kDim; ++d) {
      if (u01(rng) < 0.15f) centroids[c][d] = sig(rng);
    }
  }

  auto gen_vec = [&](float* row, float nscale) {
    const size_t c1 = cdist(rng), c2 = cdist(rng), c3 = cdist(rng);
    const float w1 = 0.5f + u01(rng), w2 = 0.3f + u01(rng), w3 = 0.1f + u01(rng);
    double n2 = 0.0;
    for (size_t d = 0; d < kDim; ++d) {
      row[d] = w1 * centroids[c1][d] + w2 * centroids[c2][d] + w3 * centroids[c3][d] +
               nscale * noise(rng);
      n2 += static_cast<double>(row[d]) * row[d];
    }
    const float inv = static_cast<float>(1.0 / std::sqrt(n2));
    for (size_t d = 0; d < kDim; ++d) row[d] *= inv;
  };

  std::vector<float> db_f32(kDocs * kDim), q_f32(kQueries * kDim);
  for (size_t i = 0; i < kDocs; ++i) gen_vec(db_f32.data() + i * kDim, 1.0f);
  for (int q = 0; q < kQueries; ++q) gen_vec(q_f32.data() + q * kDim, 1.5f);

  const float tau = 1.0f / std::sqrt(static_cast<float>(kDim));
  std::vector<uint64_t> db_b1(kDocs * kW64, 0ULL), q_b1(kQueries * kW64, 0ULL);
  std::vector<uint64_t> db_p(kDocs * kW64, 0ULL), db_n(kDocs * kW64, 0ULL);
  std::vector<uint64_t> q_p(kQueries * kW64, 0ULL), q_n(kQueries * kW64, 0ULL);

  for (size_t i = 0; i < kDocs; ++i) {
    for (size_t d = 0; d < kDim; ++d) {
      const float v = db_f32[i * kDim + d];
      const uint64_t bit = 1ULL << (d & 63);
      if (v >= 0.0f) db_b1[i * kW64 + (d >> 6)] |= bit;
      if (v > tau)
        db_p[i * kW64 + (d >> 6)] |= bit;
      else if (v < -tau)
        db_n[i * kW64 + (d >> 6)] |= bit;
    }
  }
  for (int q = 0; q < kQueries; ++q) {
    for (size_t d = 0; d < kDim; ++d) {
      const float v = q_f32[q * kDim + d];
      const uint64_t bit = 1ULL << (d & 63);
      if (v >= 0.0f) q_b1[q * kW64 + (d >> 6)] |= bit;
      if (v > tau)
        q_p[q * kW64 + (d >> 6)] |= bit;
      else if (v < -tau)
        q_n[q * kW64 + (d >> 6)] |= bit;
    }
  }

  std::vector<float> sf(kDocs);
  std::vector<int32_t> sb(kDocs), sd(kDocs);
  double us_f32 = 0.0, us_dr = 0.0, rec_b1 = 0.0, rec_dr = 0.0;
  std::vector<int> all_dr_top;
  for (int q = 0; q < kQueries; ++q) {
    auto t0 = Clock::now();
    for (size_t i = 0; i < kDocs; ++i)
      sf[i] = fp32_dot(q_f32.data() + q * kDim, db_f32.data() + i * kDim, kDim);
    auto t1 = Clock::now();
    for (size_t i = 0; i < kDocs; ++i)
      sb[i] = bin1_sim(q_b1.data() + q * kW64, db_b1.data() + i * kW64, kW64);
    auto t2 = Clock::now();
    for (size_t i = 0; i < kDocs; ++i)
      sd[i] = dualrail_sim(q_p.data() + q * kW64, q_n.data() + q * kW64, db_p.data() + i * kW64,
                           db_n.data() + i * kW64, kW64);
    auto t3 = Clock::now();
    us_f32 += std::chrono::duration<double, std::micro>(t1 - t0).count();
    us_dr += std::chrono::duration<double, std::micro>(t3 - t2).count();
    auto gt = topk_indices(sf, kTopK);
    auto tb = topk_indices(sb, kTopK);
    auto td = topk_indices(sd, kTopK);
    rec_b1 += recall_at_k(gt, tb);
    rec_dr += recall_at_k(gt, td);
    all_dr_top.insert(all_dr_top.end(), td.begin(), td.end());
  }
  us_f32 /= kQueries;
  us_dr /= kQueries;
  rec_b1 = 100.0 * rec_b1 / kQueries;
  rec_dr = 100.0 * rec_dr / kQueries;
  const std::string rag_receipt =
      "sha3-512:" + sha3_hex(all_dr_top.data(), all_dr_top.size() * sizeof(int));

  // --------------------------------------------------------------------------
  // 2. Proof-Carrying Ternary Model Diffs (Axion 1.0% Flip-Budget Gate)
  // --------------------------------------------------------------------------
  constexpr size_t kWeights = 1024 * 1024;  // 1M ternary weights (256 KB Dual-Rail)
  constexpr size_t kWWords = kWeights / 64;
  std::mt19937_64 rng64(9001);
  std::vector<uint64_t> base_p(kWWords), base_n(kWWords);
  for (size_t w = 0; w < kWWords; ++w) {
    uint64_t r1 = rng64(), r2 = rng64();
    base_p[w] = r1 & r2;
    base_n[w] = (~r1) & r2;
  }

  auto make_flipped = [&](double frac) {
    auto p = base_p;
    auto n = base_n;
    const size_t target = static_cast<size_t>(kWeights * frac);
    for (size_t i = 0; i < target; ++i) {
      const size_t pos_idx = (i * 199) % kWeights;
      const size_t w = pos_idx >> 6;
      const uint64_t bit = 1ULL << (pos_idx & 63);
      // Cycle trit state deterministically: 0 -> +1 -> -1 -> 0
      if (p[w] & bit) {
        p[w] &= ~bit;
        n[w] |= bit;
      } else if (n[w] & bit) {
        n[w] &= ~bit;
      } else {
        p[w] |= bit;
      }
    }
    return std::make_pair(std::move(p), std::move(n));
  };

  auto [ok_p, ok_n] = make_flipped(0.005);    // 0.5% flip -> ADMIT (<= 1.0%)
  auto [bad_p, bad_n] = make_flipped(0.020);  // 2.0% flip -> DENY  (> 1.0%)

  const auto tf0 = Clock::now();
  const uint64_t ok_flips = count_weight_flips(base_p, base_n, ok_p, ok_n);
  const double flip_check_us =
      std::chrono::duration<double, std::micro>(Clock::now() - tf0).count();
  const uint64_t bad_flips = count_weight_flips(base_p, base_n, bad_p, bad_n);
  const double ok_pct = 100.0 * static_cast<double>(ok_flips) / kWeights;
  const double bad_pct = 100.0 * static_cast<double>(bad_flips) / kWeights;

  const auto patch = build_xor_patch(base_p, base_n, ok_p, ok_n);
  auto recon_p = base_p, recon_n = base_n;
  apply_xor_patch(recon_p, recon_n, patch);
  const bool patch_exact = (recon_p == ok_p && recon_n == ok_n);
  if (!patch_exact || ok_pct > 1.0 || bad_pct <= 1.0) {
    std::cerr << "FAIL: Proof-carrying ternary diff check failed\n";
    return 1;
  }

  // --------------------------------------------------------------------------
  // 3. Block-Structured Activation-Sparsity Zero-Skipping (1x4096x4096)
  // --------------------------------------------------------------------------
  constexpr size_t kN = 4096;
  std::vector<uint64_t> Bp(kN * kW64), Bn(kN * kW64);
  for (size_t i = 0; i < kN * kW64; ++i) {
    uint64_t r1 = rng64(), r2 = rng64();
    Bp[i] = r1 & r2;
    Bn[i] = (~r1) & r2;
  }
  std::vector<uint64_t> ap_sp(kW64, 0ULL), an_sp(kW64, 0ULL);
  for (int b = 0; b < 4; ++b) {  // 4/32 active blocks = 87.5% block-zero (8-expert MoE)
    size_t w = static_cast<size_t>(b * 8) * 2;
    ap_sp[w] = rng64() & 0x5555555555555555ULL;
    an_sp[w] = rng64() & 0xAAAAAAAAAAAAAAAAULL;
  }
  std::vector<int32_t> C_dense(kN), C_skip(kN);
  gemv_dense(ap_sp.data(), an_sp.data(), Bp.data(), Bn.data(), C_dense.data(), kW64, kN);
  gemv_block_skip(ap_sp.data(), an_sp.data(), Bp.data(), Bn.data(), C_skip.data(), kW64, kN);
  if (C_dense != C_skip) {
    std::cerr << "FAIL: Block-zero skip mismatch\n";
    return 1;
  }

  auto bench_us = [](auto&& fn) {
    fn();
    auto t0 = Clock::now();
    for (int i = 0; i < 8; ++i) fn();
    return std::chrono::duration<double, std::micro>(Clock::now() - t0).count() / 8.0;
  };
  const double us_dense = bench_us([&]() {
    gemv_dense(ap_sp.data(), an_sp.data(), Bp.data(), Bn.data(), C_dense.data(), kW64, kN);
  });
  const double us_skip = bench_us([&]() {
    gemv_block_skip(ap_sp.data(), an_sp.data(), Bp.data(), Bn.data(), C_skip.data(), kW64, kN);
  });

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "{\n"
            << "  \"schema\": \"t81.ternary-ai-primitives.v1\",\n"
            << "  \"ok\": true,\n"
            << "  \"primitive_1_ternary_rag\": {\n"
            << "    \"docs\": " << kDocs << ", \"dims\": " << kDim << ",\n"
            << "    \"fp32_us\": " << us_f32 << ", \"dualrail_us\": " << us_dr << ",\n"
            << "    \"speedup_vs_fp32\": " << (us_f32 / us_dr) << ",\n"
            << "    \"recall10_binary_1bit_pct\": " << rec_b1 << ",\n"
            << "    \"recall10_ternary_dualrail_pct\": " << rec_dr << ",\n"
            << "    \"recall10_gain_over_1bit_pts\": " << (rec_dr - rec_b1) << ",\n"
            << "    \"retrieval_receipt\": \"" << rag_receipt << "\"\n"
            << "  },\n"
            << "  \"primitive_2_proof_carrying_diff\": {\n"
            << "    \"weights\": " << kWeights << ",\n"
            << "    \"flip_check_us\": " << flip_check_us << ",\n"
            << "    \"max_flip_budget_pct\": 1.00,\n"
            << "    \"allowed_patch_flip_pct\": " << ok_pct << ",\n"
            << "    \"denied_patch_flip_pct\": " << bad_pct << ",\n"
            << "    \"sparse_patch_words\": " << patch.deltas.size() << ",\n"
            << "    \"lossless_reconstruct\": true\n"
            << "  },\n"
            << "  \"primitive_3_block_zero_skip\": {\n"
            << "    \"shape\": \"1x4096x4096\", \"block_zero_pct\": 87.50,\n"
            << "    \"dense_us\": " << us_dense << ", \"skip_us\": " << us_skip << ",\n"
            << "    \"speedup\": " << (us_dense / us_skip) << ", \"exact_match\": true\n"
            << "  }\n"
            << "}\n";
  return 0;
}
