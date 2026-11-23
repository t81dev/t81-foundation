// H06-MEM-01: Memory-Bandwidth-Bound & Block-Sparse Inference Falsification Benchmark
//
// Compares Dual-Rail (P, N) and sparse-block SBDR1T2D weight streaming against
// competent binary controls in a DRAM-bandwidth-bound regime (L = 16 distinct
// 4096x1024 matrices = 67.1M weights = 64 MB INT8, defeating L2/SLC caching):
//   - B_INT8_Dense   : 4-col NEON vdotq_s32 / AVX2 int8_t control (1.0 B/weight)
//   - B_INT4_Dense   : Packed 4-bit nibble control unpacked in-register (0.5 B/weight)
//   - B_INT8_Sparse  : Block-64 sparse INT8 control with identical active-block mask
//   - T81_DR_Dense   : Dual-Rail (P, N) dense bitplanes (0.25 B/weight)
//   - T81_SBDR_Sparse: Sparse-block SBDR1T2D streaming active blocks only
//   - T81_ActPack    : Runtime activation packing cost (int8 -> Dual-Rail P/N)
//   - T81_SBDR_Decode: Cold SBDR1T2D wire validation + header/mask load cost
//
// Every candidate is checked for bit-exact integer agreement (max_abs_err == 0)
// against an independent int64_t oracle before timing begins.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

namespace {

constexpr size_t kM_Layers = 16;
constexpr size_t kK = 4096;
constexpr size_t kN = 1024;
constexpr size_t kW64 = kK / 64;           // 64 words per column
constexpr size_t kTotalWords = kN * kW64;  // 65,536 64-trit blocks per layer
constexpr size_t kMaskWords = kTotalWords / 64;

struct LayerSet {
  int sparsity_pct = 0;
  std::vector<int8_t> act_i8;
  std::vector<uint64_t> act_p;
  std::vector<uint64_t> act_n;
  std::vector<int32_t> oracle_layer0;

  // Dense INT8: L x (N x K) row-major per output column
  std::vector<std::vector<int8_t>> int8_dense;

  // Dense INT4: L x (N x K/2) two signed 4-bit weights per byte (low nibble = even k, high = odd k)
  std::vector<std::vector<uint8_t>> int4_dense;

  // Dense Dual-Rail: L x (N x W64)
  std::vector<std::vector<uint64_t>> dr_p;
  std::vector<std::vector<uint64_t>> dr_n;

  // Sparse SBDR1T2D wire bytes + parsed active-block stream per column
  std::vector<std::vector<uint8_t>> sbdr_wire;
  struct SparseDRLayer {
    std::vector<uint32_t> col_offsets;  // size N + 1
    std::vector<uint16_t> word_idx;     // active w in [0, W64)
    std::vector<uint64_t> bp;
    std::vector<uint64_t> bn;
    size_t payload_bytes = 0;
  };
  std::vector<SparseDRLayer> sbdr_layers;

  // Mandatory sparsity control: Sparse-Block-64 INT8 with identical active-block mask
  struct SparseInt8Layer {
    std::vector<uint32_t> col_offsets;  // size N + 1
    std::vector<uint16_t> word_idx;     // active w in [0, W64)
    std::vector<int8_t> blocks;         // active_blocks * 64 bytes
    size_t payload_bytes = 0;
  };
  std::vector<SparseInt8Layer> int8_sparse_layers;
};

void pack_act_dualrail(const std::vector<int8_t>& trits, std::vector<uint64_t>& pos,
                       std::vector<uint64_t>& neg) {
  const size_t words = trits.size() / 64;
  pos.resize(words);
  neg.resize(words);
  const int8_t* src = trits.data();
  size_t w = 0;
#if defined(__aarch64__) && defined(__ARM_NEON)
  const int8x16_t zero = vdupq_n_s8(0);
  static constexpr uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                              1, 2, 4, 8, 16, 32, 64, 128};
  const uint8x16_t weights = vld1q_u8(kBitWeights);
  for (; w < words; ++w) {
    const int8_t* p = src + w * 64;
    const int8x16_t v0 = vld1q_s8(p + 0);
    const int8x16_t v1 = vld1q_s8(p + 16);
    const int8x16_t v2 = vld1q_s8(p + 32);
    const int8x16_t v3 = vld1q_s8(p + 48);
    const uint8x16_t pq = vpaddq_u8(
        vpaddq_u8(vandq_u8(vcgtq_s8(v0, zero), weights), vandq_u8(vcgtq_s8(v1, zero), weights)),
        vpaddq_u8(vandq_u8(vcgtq_s8(v2, zero), weights), vandq_u8(vcgtq_s8(v3, zero), weights)));
    pos[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(pq, pq)), 0);
    const uint8x16_t nq = vpaddq_u8(
        vpaddq_u8(vandq_u8(vcltq_s8(v0, zero), weights), vandq_u8(vcltq_s8(v1, zero), weights)),
        vpaddq_u8(vandq_u8(vcltq_s8(v2, zero), weights), vandq_u8(vcltq_s8(v3, zero), weights)));
    neg[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(nq, nq)), 0);
  }
#elif defined(__x86_64__) && defined(__AVX2__)
  const __m256i zero = _mm256_setzero_si256();
  for (; w < words; ++w) {
    const int8_t* p = src + w * 64;
    const __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 0));
    const __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 32));
    const uint32_t plo = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(v0, zero)));
    const uint32_t phi = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(v1, zero)));
    const uint32_t nlo = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(zero, v0)));
    const uint32_t nhi = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(zero, v1)));
    pos[w] = static_cast<uint64_t>(plo) | (static_cast<uint64_t>(phi) << 32);
    neg[w] = static_cast<uint64_t>(nlo) | (static_cast<uint64_t>(nhi) << 32);
  }
#else
  for (; w < words; ++w) {
    uint64_t p = 0, n = 0;
    for (size_t i = 0; i < 64; ++i) {
      const int8_t t = src[w * 64 + i];
      if (t > 0)
        p |= (1ULL << i);
      else if (t < 0)
        n |= (1ULL << i);
    }
    pos[w] = p;
    neg[w] = n;
  }
#endif
}

const LayerSet& get_layer_set(int sparsity_pct) {
  static std::vector<std::pair<int, LayerSet>> cache;
  for (const auto& entry : cache) {
    if (entry.first == sparsity_pct) return entry.second;
  }

  LayerSet ls;
  ls.sparsity_pct = sparsity_pct;
  std::mt19937_64 rng(0x543831ULL ^ static_cast<uint64_t>(sparsity_pct));
  std::uniform_int_distribution<int> trit_dist(-1, 1);
  std::uniform_int_distribution<int> pct_dist(0, 99);

  ls.act_i8.resize(kK);
  for (size_t k = 0; k < kK; ++k) ls.act_i8[k] = static_cast<int8_t>(trit_dist(rng));
  pack_act_dualrail(ls.act_i8, ls.act_p, ls.act_n);

  ls.int8_dense.resize(kM_Layers);
  ls.int4_dense.resize(kM_Layers);
  ls.dr_p.resize(kM_Layers);
  ls.dr_n.resize(kM_Layers);
  ls.sbdr_wire.resize(kM_Layers);
  ls.sbdr_layers.resize(kM_Layers);
  ls.int8_sparse_layers.resize(kM_Layers);

  for (size_t l = 0; l < kM_Layers; ++l) {
    auto& i8 = ls.int8_dense[l];
    auto& i4 = ls.int4_dense[l];
    auto& dp = ls.dr_p[l];
    auto& dn = ls.dr_n[l];
    auto& sp_dr = ls.sbdr_layers[l];
    auto& sp_i8 = ls.int8_sparse_layers[l];

    i8.assign(kN * kK, 0);
    i4.assign(kN * (kK / 2), 0);
    dp.assign(kTotalWords, 0ULL);
    dn.assign(kTotalWords, 0ULL);
    sp_dr.col_offsets.resize(kN + 1, 0);
    sp_i8.col_offsets.resize(kN + 1, 0);

    std::vector<uint64_t> mask(kMaskWords, 0ULL);
    std::vector<uint64_t> wire_payload;

    for (size_t n = 0; n < kN; ++n) {
      sp_dr.col_offsets[n] = static_cast<uint32_t>(sp_dr.word_idx.size());
      sp_i8.col_offsets[n] = static_cast<uint32_t>(sp_i8.word_idx.size());
      for (size_t w = 0; w < kW64; ++w) {
        const size_t wi = n * kW64 + w;
        if (sparsity_pct > 0 && pct_dist(rng) < sparsity_pct) {
          continue;  // 64-trit zero block
        }
        uint64_t wp = 0, wn = 0;
        int8_t* col_blk = i8.data() + n * kK + w * 64;
        for (size_t b = 0; b < 64; ++b) {
          const int8_t t = static_cast<int8_t>(trit_dist(rng));
          col_blk[b] = t;
          if (t > 0)
            wp |= (1ULL << b);
          else if (t < 0)
            wn |= (1ULL << b);
        }
        if ((wp | wn) == 0ULL) continue;
        dp[wi] = wp;
        dn[wi] = wn;
        mask[wi >> 6] |= (1ULL << (wi & 63));
        wire_payload.push_back(wp);
        wire_payload.push_back(wn);

        sp_dr.word_idx.push_back(static_cast<uint16_t>(w));
        sp_dr.bp.push_back(wp);
        sp_dr.bn.push_back(wn);

        sp_i8.word_idx.push_back(static_cast<uint16_t>(w));
        sp_i8.blocks.insert(sp_i8.blocks.end(), col_blk, col_blk + 64);

        // Pack into 4-bit signed two's-complement nibbles (low = even, high = odd)
        uint8_t* i4_blk = i4.data() + n * (kK / 2) + w * 32;
        for (size_t b = 0; b < 64; b += 2) {
          const uint8_t lo = static_cast<uint8_t>(col_blk[b]) & 0x0Fu;
          const uint8_t hi = static_cast<uint8_t>(col_blk[b + 1]) & 0x0Fu;
          i4_blk[b >> 1] = static_cast<uint8_t>(lo | (hi << 4));
        }
      }
    }
    sp_dr.col_offsets[kN] = static_cast<uint32_t>(sp_dr.word_idx.size());
    sp_i8.col_offsets[kN] = static_cast<uint32_t>(sp_i8.word_idx.size());
    sp_dr.payload_bytes = sp_dr.col_offsets.size() * sizeof(uint32_t) +
                          sp_dr.word_idx.size() * sizeof(uint16_t) +
                          sp_dr.bp.size() * sizeof(uint64_t) * 2;
    sp_i8.payload_bytes = sp_i8.col_offsets.size() * sizeof(uint32_t) +
                          sp_i8.word_idx.size() * sizeof(uint16_t) + sp_i8.blocks.size();

    // Canonical SBDR1T2D wire bytes
    auto& wire = ls.sbdr_wire[l];
    wire.resize(24 + kMaskWords * 8 + wire_payload.size() * 8);
    static constexpr char kMagic[8] = {'S', 'B', 'D', 'R', '1', 'T', '2', 'D'};
    const uint64_t K64 = kK, N64 = kN;
    std::memcpy(wire.data(), kMagic, 8);
    std::memcpy(wire.data() + 8, &K64, 8);
    std::memcpy(wire.data() + 16, &N64, 8);
    std::memcpy(wire.data() + 24, mask.data(), kMaskWords * 8);
    if (!wire_payload.empty()) {
      std::memcpy(wire.data() + 24 + kMaskWords * 8, wire_payload.data(), wire_payload.size() * 8);
    }
  }

  // Exact integer oracle on layer 0
  ls.oracle_layer0.assign(kN, 0);
  for (size_t n = 0; n < kN; ++n) {
    int64_t acc = 0;
    const int8_t* wrow = ls.int8_dense[0].data() + n * kK;
    for (size_t k = 0; k < kK; ++k) acc += static_cast<int64_t>(ls.act_i8[k]) * wrow[k];
    ls.oracle_layer0[n] = static_cast<int32_t>(acc);
  }

  cache.emplace_back(sparsity_pct, std::move(ls));
  return cache.back().second;
}

// --- Kernels ---

void matvec_int8_dense(const int8_t* act, const int8_t* W, int32_t* out) {
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  for (; n + 4 <= kN; n += 4) {
    const int8_t* w0 = W + (n + 0) * kK;
    const int8_t* w1 = W + (n + 1) * kK;
    const int8_t* w2 = W + (n + 2) * kK;
    const int8_t* w3 = W + (n + 3) * kK;
    int32x4_t a0 = vdupq_n_s32(0), a1 = vdupq_n_s32(0), a2 = vdupq_n_s32(0), a3 = vdupq_n_s32(0);
    for (size_t k = 0; k < kK; k += 16) {
      const int8x16_t av = vld1q_s8(act + k);
      a0 = vdotq_s32(a0, av, vld1q_s8(w0 + k));
      a1 = vdotq_s32(a1, av, vld1q_s8(w1 + k));
      a2 = vdotq_s32(a2, av, vld1q_s8(w2 + k));
      a3 = vdotq_s32(a3, av, vld1q_s8(w3 + k));
    }
    out[n + 0] = vaddvq_s32(a0);
    out[n + 1] = vaddvq_s32(a1);
    out[n + 2] = vaddvq_s32(a2);
    out[n + 3] = vaddvq_s32(a3);
  }
#endif
  for (; n < kN; ++n) {
    int32_t acc = 0;
    const int8_t* wn = W + n * kK;
    for (size_t k = 0; k < kK; ++k) acc += static_cast<int32_t>(act[k]) * wn[k];
    out[n] = acc;
  }
}

void matvec_int4_dense(const int8_t* act, const uint8_t* W4, int32_t* out) {
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  const uint8x16_t low_mask = vdupq_n_u8(0x0F);
  for (; n + 2 <= kN; n += 2) {
    const uint8_t* w0 = W4 + (n + 0) * (kK / 2);
    const uint8_t* w1 = W4 + (n + 1) * (kK / 2);
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    for (size_t k = 0; k < kK; k += 32) {
      const int8x16x2_t av = vld2q_s8(act + k);  // val[0] = even k, val[1] = odd k
      auto dot32 = [&](const uint8_t* wp, int32x4_t& acc) {
        const uint8x16_t packed = vld1q_u8(wp + (k >> 1));
        // Sign-extend 4-bit two's complement to int8: (x << 4) >> 4
        const int8x16_t lo =
            vshrq_n_s8(vreinterpretq_s8_u8(vshlq_n_u8(vandq_u8(packed, low_mask), 4)), 4);
        const int8x16_t hi = vshrq_n_s8(vreinterpretq_s8_u8(packed), 4);
        acc = vdotq_s32(acc, av.val[0], lo);
        acc = vdotq_s32(acc, av.val[1], hi);
      };
      dot32(w0, acc0);
      dot32(w1, acc1);
    }
    out[n + 0] = vaddvq_s32(acc0);
    out[n + 1] = vaddvq_s32(acc1);
  }
#endif
  for (; n < kN; ++n) {
    int32_t acc = 0;
    const uint8_t* wn = W4 + n * (kK / 2);
    for (size_t k = 0; k < kK; k += 2) {
      const uint8_t byte = wn[k >> 1];
      const int8_t lo = static_cast<int8_t>(static_cast<int8_t>(byte << 4) >> 4);
      const int8_t hi = static_cast<int8_t>(static_cast<int8_t>(byte) >> 4);
      acc += static_cast<int32_t>(act[k]) * lo + static_cast<int32_t>(act[k + 1]) * hi;
    }
    out[n] = acc;
  }
}

void matvec_int8_sparse(const int8_t* act, const LayerSet::SparseInt8Layer& layer, int32_t* out) {
  for (size_t n = 0; n < kN; ++n) {
    const uint32_t begin = layer.col_offsets[n];
    const uint32_t end = layer.col_offsets[n + 1];
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
    int32x4_t acc = vdupq_n_s32(0);
    for (uint32_t idx = begin; idx < end; ++idx) {
      const int8_t* ap = act + static_cast<size_t>(layer.word_idx[idx]) * 64;
      const int8_t* wp = layer.blocks.data() + static_cast<size_t>(idx) * 64;
      acc = vdotq_s32(acc, vld1q_s8(ap + 0), vld1q_s8(wp + 0));
      acc = vdotq_s32(acc, vld1q_s8(ap + 16), vld1q_s8(wp + 16));
      acc = vdotq_s32(acc, vld1q_s8(ap + 32), vld1q_s8(wp + 32));
      acc = vdotq_s32(acc, vld1q_s8(ap + 48), vld1q_s8(wp + 48));
    }
    out[n] = vaddvq_s32(acc);
#else
    int32_t acc = 0;
    for (uint32_t idx = begin; idx < end; ++idx) {
      const int8_t* ap = act + static_cast<size_t>(layer.word_idx[idx]) * 64;
      const int8_t* wp = layer.blocks.data() + static_cast<size_t>(idx) * 64;
      for (size_t b = 0; b < 64; ++b) acc += static_cast<int32_t>(ap[b]) * wp[b];
    }
    out[n] = acc;
#endif
  }
}

void matvec_dualrail_dense(const uint64_t* ap, const uint64_t* an, const uint64_t* Bp,
                           const uint64_t* Bn, int32_t* out) {
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_NEON)
  for (; n + 4 <= kN; n += 4) {
    const uint64_t* bp0 = Bp + (n + 0) * kW64;
    const uint64_t* bn0 = Bn + (n + 0) * kW64;
    const uint64_t* bp1 = Bp + (n + 1) * kW64;
    const uint64_t* bn1 = Bn + (n + 1) * kW64;
    const uint64_t* bp2 = Bp + (n + 2) * kW64;
    const uint64_t* bn2 = Bn + (n + 2) * kW64;
    const uint64_t* bp3 = Bp + (n + 3) * kW64;
    const uint64_t* bn3 = Bn + (n + 3) * kW64;
    uint16x8_t p0 = vdupq_n_u16(0), n0 = vdupq_n_u16(0);
    uint16x8_t p1 = vdupq_n_u16(0), n1 = vdupq_n_u16(0);
    uint16x8_t p2 = vdupq_n_u16(0), n2 = vdupq_n_u16(0);
    uint16x8_t p3 = vdupq_n_u16(0), n3 = vdupq_n_u16(0);
    for (size_t w = 0; w < kW64; w += 2) {
      const uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
      const uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
      auto acc = [&](const uint64_t* bp, const uint64_t* bn, uint16x8_t& pa, uint16x8_t& na) {
        const uint8x16_t bvp = vreinterpretq_u8_u64(vld1q_u64(bp + w));
        const uint8x16_t bvn = vreinterpretq_u8_u64(vld1q_u64(bn + w));
        pa = vpadalq_u8(pa, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
        na = vpadalq_u8(na, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
      };
      acc(bp0, bn0, p0, n0);
      acc(bp1, bn1, p1, n1);
      acc(bp2, bn2, p2, n2);
      acc(bp3, bn3, p3, n3);
    }
    out[n + 0] = static_cast<int32_t>(vaddlvq_u16(p0)) - static_cast<int32_t>(vaddlvq_u16(n0));
    out[n + 1] = static_cast<int32_t>(vaddlvq_u16(p1)) - static_cast<int32_t>(vaddlvq_u16(n1));
    out[n + 2] = static_cast<int32_t>(vaddlvq_u16(p2)) - static_cast<int32_t>(vaddlvq_u16(n2));
    out[n + 3] = static_cast<int32_t>(vaddlvq_u16(p3)) - static_cast<int32_t>(vaddlvq_u16(n3));
  }
#endif
  for (; n < kN; ++n) {
    int32_t acc = 0;
    const uint64_t* bp = Bp + n * kW64;
    const uint64_t* bn = Bn + n * kW64;
    for (size_t w = 0; w < kW64; ++w) {
      acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
             std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
    }
    out[n] = acc;
  }
}

void matvec_sbdr_sparse(const uint64_t* ap, const uint64_t* an,
                        const LayerSet::SparseDRLayer& layer, int32_t* out) {
  for (size_t n = 0; n < kN; ++n) {
    const uint32_t begin = layer.col_offsets[n];
    const uint32_t end = layer.col_offsets[n + 1];
    int32_t acc = 0;
    for (uint32_t idx = begin; idx < end; ++idx) {
      const uint16_t w = layer.word_idx[idx];
      const uint64_t bp = layer.bp[idx];
      const uint64_t bn = layer.bn[idx];
      acc +=
          std::popcount((ap[w] & bp) | (an[w] & bn)) - std::popcount((ap[w] & bn) | (an[w] & bp));
    }
    out[n] = acc;
  }
}

bool decode_sbdr_wire_to_sparse(const std::vector<uint8_t>& wire, LayerSet::SparseDRLayer& out) {
  if (wire.size() < 24 + kMaskWords * 8) return false;
  out.col_offsets.assign(kN + 1, 0);
  out.word_idx.clear();
  out.bp.clear();
  out.bn.clear();

  size_t payload_off = 24 + kMaskWords * 8;
  size_t cur_col = 0;
  for (size_t m = 0; m < kMaskWords; ++m) {
    uint64_t bits = 0;
    std::memcpy(&bits, wire.data() + 24 + m * 8, 8);
    while (bits != 0ULL) {
      const unsigned tz = static_cast<unsigned>(std::countr_zero(bits));
      const size_t wi = (m << 6) + tz;
      const size_t col = wi / kW64;
      const uint16_t w = static_cast<uint16_t>(wi % kW64);
      while (cur_col < col) {
        out.col_offsets[++cur_col] = static_cast<uint32_t>(out.word_idx.size());
      }
      uint64_t wp = 0, wn = 0;
      std::memcpy(&wp, wire.data() + payload_off, 8);
      std::memcpy(&wn, wire.data() + payload_off + 8, 8);
      payload_off += 16;
      if ((wp & wn) != 0ULL) return false;
      out.word_idx.push_back(w);
      out.bp.push_back(wp);
      out.bn.push_back(wn);
      bits &= (bits - 1ULL);
    }
  }
  while (cur_col < kN) {
    out.col_offsets[++cur_col] = static_cast<uint32_t>(out.word_idx.size());
  }
  return payload_off == wire.size();
}

// --- Benchmarks ---

static void BM_MemBound_INT8_Dense(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_int8_dense(ls.act_i8.data(), ls.int8_dense[0].data(), out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("INT8_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * kK;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_int8_dense(ls.act_i8.data(), ls.int8_dense[l].data(), out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_INT8_Dense)->Arg(0)->Arg(75);

static void BM_MemBound_INT4_Dense(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_int4_dense(ls.act_i8.data(), ls.int4_dense[0].data(), out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("INT4_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * (kK / 2);
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_int4_dense(ls.act_i8.data(), ls.int4_dense[l].data(), out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_INT4_Dense)->Arg(0)->Arg(75);

static void BM_MemBound_INT8_Sparse(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_int8_sparse(ls.act_i8.data(), ls.int8_sparse_layers[0], out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("INT8_Sparse oracle mismatch");
    return;
  }
  size_t total_bytes = 0;
  for (const auto& sl : ls.int8_sparse_layers) total_bytes += sl.payload_bytes;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_int8_sparse(ls.act_i8.data(), ls.int8_sparse_layers[l], out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_INT8_Sparse)->Arg(0)->Arg(50)->Arg(75)->Arg(90);

static void BM_MemBound_DualRail_Dense(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_dualrail_dense(ls.act_p.data(), ls.act_n.data(), ls.dr_p[0].data(), ls.dr_n[0].data(),
                        out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("DualRail_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kTotalWords * 16;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_dualrail_dense(ls.act_p.data(), ls.act_n.data(), ls.dr_p[l].data(), ls.dr_n[l].data(),
                            out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_DualRail_Dense)->Arg(0)->Arg(75);

static void BM_MemBound_SBDR1T2D_Sparse(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_sbdr_sparse(ls.act_p.data(), ls.act_n.data(), ls.sbdr_layers[0], out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("SBDR1T2D_Sparse oracle mismatch");
    return;
  }
  size_t total_bytes = 0;
  for (const auto& sl : ls.sbdr_layers) total_bytes += sl.payload_bytes;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_sbdr_sparse(ls.act_p.data(), ls.act_n.data(), ls.sbdr_layers[l], out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_SBDR1T2D_Sparse)->Arg(0)->Arg(50)->Arg(75)->Arg(90);

static void BM_MemBound_ActPack_Overhead(benchmark::State& state) {
  const auto& ls = get_layer_set(0);
  std::vector<uint64_t> p, n;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      pack_act_dualrail(ls.act_i8, p, n);
      benchmark::DoNotOptimize(p.data());
    }
  }
  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(kM_Layers * kK));
}
BENCHMARK(BM_MemBound_ActPack_Overhead);

static void BM_MemBound_SBDR1T2D_Decode(benchmark::State& state) {
  const auto& ls = get_layer_set(static_cast<int>(state.range(0)));
  LayerSet::SparseDRLayer tmp;
  size_t wire_bytes = 0;
  for (const auto& w : ls.sbdr_wire) wire_bytes += w.size();
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      bool ok = decode_sbdr_wire_to_sparse(ls.sbdr_wire[l], tmp);
      benchmark::DoNotOptimize(ok);
      benchmark::DoNotOptimize(tmp.bp.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(wire_bytes));
  state.counters["wire_MB"] = static_cast<double>(wire_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_SBDR1T2D_Decode)->Arg(0)->Arg(50)->Arg(75)->Arg(90);

// ============================================================================
// H06-MEM-02: 128-trit SIMD-Batched SBDR Sparse vs INT8_Sparse at L = 64
// (True DRAM-exceeding working set even at 90% sparsity: INT8_Sparse >= 26 MB)
// ============================================================================

constexpr size_t kM_Layers64 = 64;
constexpr size_t kB128 = kK / 128;  // 32 128-trit blocks per column

struct LayerSet64 {
  int sparsity_pct = 0;
  std::vector<int8_t> act_i8;
  std::vector<uint64_t> act_p;
  std::vector<uint64_t> act_n;
  std::vector<int32_t> oracle_layer0;

  // Dense Dual-Rail: 64 layers = 64 MB
  std::vector<std::vector<uint64_t>> dr_p;
  std::vector<std::vector<uint64_t>> dr_n;

  // 128-trit block-sparse Dual-Rail (1 NEON uint8x16_t per plane per block)
  struct Sparse128DR {
    std::vector<uint32_t> col_offsets;
    std::vector<uint16_t> blk_idx;  // in [0, kB128)
    std::vector<uint64_t> bp;       // 2 uint64_t per active block
    std::vector<uint64_t> bn;       // 2 uint64_t per active block
    size_t payload_bytes = 0;
  };
  std::vector<Sparse128DR> sbdr128_layers;

  // Identical 128-trit block-sparse INT8 control (128 bytes per active block)
  struct Sparse128Int8 {
    std::vector<uint32_t> col_offsets;
    std::vector<uint16_t> blk_idx;
    std::vector<int8_t> blocks;  // 128 bytes per active block
    size_t payload_bytes = 0;
  };
  std::vector<Sparse128Int8> int8_sparse128_layers;
};

const LayerSet64& get_layer_set_64(int sparsity_pct) {
  static std::vector<std::pair<int, LayerSet64>> cache;
  for (const auto& entry : cache) {
    if (entry.first == sparsity_pct) return entry.second;
  }
  LayerSet64 ls;
  ls.sparsity_pct = sparsity_pct;
  std::mt19937_64 rng(0x48303632ULL ^ static_cast<uint64_t>(sparsity_pct));
  std::uniform_int_distribution<int> trit_dist(-1, 1);
  std::uniform_int_distribution<int> pct_dist(0, 99);

  ls.act_i8.resize(kK);
  for (size_t k = 0; k < kK; ++k) ls.act_i8[k] = static_cast<int8_t>(trit_dist(rng));
  pack_act_dualrail(ls.act_i8, ls.act_p, ls.act_n);

  ls.dr_p.resize(kM_Layers64);
  ls.dr_n.resize(kM_Layers64);
  ls.sbdr128_layers.resize(kM_Layers64);
  ls.int8_sparse128_layers.resize(kM_Layers64);
  std::vector<int8_t> layer0_dense(kN * kK, 0);

  for (size_t l = 0; l < kM_Layers64; ++l) {
    auto& dp = ls.dr_p[l];
    auto& dn = ls.dr_n[l];
    auto& sdr = ls.sbdr128_layers[l];
    auto& si8 = ls.int8_sparse128_layers[l];
    dp.assign(kTotalWords, 0ULL);
    dn.assign(kTotalWords, 0ULL);
    sdr.col_offsets.resize(kN + 1, 0);
    si8.col_offsets.resize(kN + 1, 0);

    for (size_t n = 0; n < kN; ++n) {
      sdr.col_offsets[n] = static_cast<uint32_t>(sdr.blk_idx.size());
      si8.col_offsets[n] = static_cast<uint32_t>(si8.blk_idx.size());
      for (size_t b128 = 0; b128 < kB128; ++b128) {
        if (sparsity_pct > 0 && pct_dist(rng) < sparsity_pct) continue;
        int8_t blk[128];
        uint64_t p0 = 0, n0 = 0, p1 = 0, n1 = 0;
        for (size_t i = 0; i < 64; ++i) {
          const int8_t t = static_cast<int8_t>(trit_dist(rng));
          blk[i] = t;
          if (t > 0)
            p0 |= (1ULL << i);
          else if (t < 0)
            n0 |= (1ULL << i);
        }
        for (size_t i = 0; i < 64; ++i) {
          const int8_t t = static_cast<int8_t>(trit_dist(rng));
          blk[64 + i] = t;
          if (t > 0)
            p1 |= (1ULL << i);
          else if (t < 0)
            n1 |= (1ULL << i);
        }
        if ((p0 | n0 | p1 | n1) == 0ULL) continue;
        const size_t w0 = n * kW64 + b128 * 2;
        dp[w0] = p0;
        dn[w0] = n0;
        dp[w0 + 1] = p1;
        dn[w0 + 1] = n1;
        if (l == 0) std::memcpy(layer0_dense.data() + n * kK + b128 * 128, blk, 128);

        sdr.blk_idx.push_back(static_cast<uint16_t>(b128));
        sdr.bp.push_back(p0);
        sdr.bp.push_back(p1);
        sdr.bn.push_back(n0);
        sdr.bn.push_back(n1);

        si8.blk_idx.push_back(static_cast<uint16_t>(b128));
        si8.blocks.insert(si8.blocks.end(), blk, blk + 128);
      }
    }
    sdr.col_offsets[kN] = static_cast<uint32_t>(sdr.blk_idx.size());
    si8.col_offsets[kN] = static_cast<uint32_t>(si8.blk_idx.size());
    sdr.payload_bytes = sdr.col_offsets.size() * sizeof(uint32_t) +
                        sdr.blk_idx.size() * sizeof(uint16_t) +
                        sdr.bp.size() * sizeof(uint64_t) * 2;
    si8.payload_bytes = si8.col_offsets.size() * sizeof(uint32_t) +
                        si8.blk_idx.size() * sizeof(uint16_t) + si8.blocks.size();
  }

  ls.oracle_layer0.assign(kN, 0);
  for (size_t n = 0; n < kN; ++n) {
    int64_t acc = 0;
    const int8_t* wrow = layer0_dense.data() + n * kK;
    for (size_t k = 0; k < kK; ++k) acc += static_cast<int64_t>(ls.act_i8[k]) * wrow[k];
    ls.oracle_layer0[n] = static_cast<int32_t>(acc);
  }
  cache.emplace_back(sparsity_pct, std::move(ls));
  return cache.back().second;
}

void matvec_sbdr128_simd(const uint64_t* ap, const uint64_t* an,
                         const LayerSet64::Sparse128DR& layer, int32_t* out) {
  for (size_t n = 0; n < kN; ++n) {
    const uint32_t begin = layer.col_offsets[n];
    const uint32_t end = layer.col_offsets[n + 1];
#if defined(__aarch64__) && defined(__ARM_NEON)
    uint16x8_t p_acc = vdupq_n_u16(0);
    uint16x8_t n_acc = vdupq_n_u16(0);
    for (uint32_t idx = begin; idx < end; ++idx) {
      const size_t w = static_cast<size_t>(layer.blk_idx[idx]) * 2;
      const uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
      const uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
      const uint8x16_t bvp =
          vreinterpretq_u8_u64(vld1q_u64(layer.bp.data() + static_cast<size_t>(idx) * 2));
      const uint8x16_t bvn =
          vreinterpretq_u8_u64(vld1q_u64(layer.bn.data() + static_cast<size_t>(idx) * 2));
      p_acc = vpadalq_u8(p_acc, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
      n_acc = vpadalq_u8(n_acc, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
    }
    out[n] = static_cast<int32_t>(vaddlvq_u16(p_acc)) - static_cast<int32_t>(vaddlvq_u16(n_acc));
#else
    int32_t acc = 0;
    for (uint32_t idx = begin; idx < end; ++idx) {
      const size_t w = static_cast<size_t>(layer.blk_idx[idx]) * 2;
      const uint64_t* bp = layer.bp.data() + static_cast<size_t>(idx) * 2;
      const uint64_t* bn = layer.bn.data() + static_cast<size_t>(idx) * 2;
      acc += std::popcount((ap[w] & bp[0]) | (an[w] & bn[0])) -
             std::popcount((ap[w] & bn[0]) | (an[w] & bp[0])) +
             std::popcount((ap[w + 1] & bp[1]) | (an[w + 1] & bn[1])) -
             std::popcount((ap[w + 1] & bn[1]) | (an[w + 1] & bp[1]));
    }
    out[n] = acc;
#endif
  }
}

void matvec_int8_sparse128(const int8_t* act, const LayerSet64::Sparse128Int8& layer,
                           int32_t* out) {
  for (size_t n = 0; n < kN; ++n) {
    const uint32_t begin = layer.col_offsets[n];
    const uint32_t end = layer.col_offsets[n + 1];
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    for (uint32_t idx = begin; idx < end; ++idx) {
      const int8_t* ap = act + static_cast<size_t>(layer.blk_idx[idx]) * 128;
      const int8_t* wp = layer.blocks.data() + static_cast<size_t>(idx) * 128;
      acc0 = vdotq_s32(acc0, vld1q_s8(ap + 0), vld1q_s8(wp + 0));
      acc1 = vdotq_s32(acc1, vld1q_s8(ap + 16), vld1q_s8(wp + 16));
      acc0 = vdotq_s32(acc0, vld1q_s8(ap + 32), vld1q_s8(wp + 32));
      acc1 = vdotq_s32(acc1, vld1q_s8(ap + 48), vld1q_s8(wp + 48));
      acc0 = vdotq_s32(acc0, vld1q_s8(ap + 64), vld1q_s8(wp + 64));
      acc1 = vdotq_s32(acc1, vld1q_s8(ap + 80), vld1q_s8(wp + 80));
      acc0 = vdotq_s32(acc0, vld1q_s8(ap + 96), vld1q_s8(wp + 96));
      acc1 = vdotq_s32(acc1, vld1q_s8(ap + 112), vld1q_s8(wp + 112));
    }
    out[n] = vaddvq_s32(vaddq_s32(acc0, acc1));
#else
    int32_t acc = 0;
    for (uint32_t idx = begin; idx < end; ++idx) {
      const int8_t* ap = act + static_cast<size_t>(layer.blk_idx[idx]) * 128;
      const int8_t* wp = layer.blocks.data() + static_cast<size_t>(idx) * 128;
      for (size_t b = 0; b < 128; ++b) acc += static_cast<int32_t>(ap[b]) * wp[b];
    }
    out[n] = acc;
#endif
  }
}

static void BM_MemBound64_DualRail_Dense(benchmark::State& state) {
  const auto& ls = get_layer_set_64(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_dualrail_dense(ls.act_p.data(), ls.act_n.data(), ls.dr_p[0].data(), ls.dr_n[0].data(),
                        out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("DualRail64_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers64 * kTotalWords * 16;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers64; ++l) {
      matvec_dualrail_dense(ls.act_p.data(), ls.act_n.data(), ls.dr_p[l].data(), ls.dr_n[l].data(),
                            out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound64_DualRail_Dense)->Arg(50)->Arg(75)->Arg(90);

static void BM_MemBound64_SBDR128_SIMD(benchmark::State& state) {
  const auto& ls = get_layer_set_64(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_sbdr128_simd(ls.act_p.data(), ls.act_n.data(), ls.sbdr128_layers[0], out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("SBDR128_SIMD oracle mismatch");
    return;
  }
  size_t total_bytes = 0;
  for (const auto& sl : ls.sbdr128_layers) total_bytes += sl.payload_bytes;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers64; ++l) {
      matvec_sbdr128_simd(ls.act_p.data(), ls.act_n.data(), ls.sbdr128_layers[l], out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound64_SBDR128_SIMD)->Arg(50)->Arg(75)->Arg(90);

static void BM_MemBound64_INT8_Sparse128(benchmark::State& state) {
  const auto& ls = get_layer_set_64(static_cast<int>(state.range(0)));
  std::vector<int32_t> out(kN);
  matvec_int8_sparse128(ls.act_i8.data(), ls.int8_sparse128_layers[0], out.data());
  if (out != ls.oracle_layer0) {
    state.SkipWithError("INT8_Sparse128 oracle mismatch");
    return;
  }
  size_t total_bytes = 0;
  for (const auto& sl : ls.int8_sparse128_layers) total_bytes += sl.payload_bytes;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers64; ++l) {
      matvec_int8_sparse128(ls.act_i8.data(), ls.int8_sparse128_layers[l], out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound64_INT8_Sparse128)->Arg(50)->Arg(75)->Arg(90);

// ============================================================================
// H06-MEM-03: R2 Regime — INT8 Activations [-127, 127] x Ternary Weights
// Compares in-register Dual-Rail unpack + vdotq_s32 (16 MB) vs INT4 (32 MB) vs INT8 (64 MB)
// ============================================================================

const std::vector<int8_t>& get_r2_int8_act() {
  static const std::vector<int8_t> act = [] {
    std::mt19937_64 rng(0x52324143ULL);
    std::uniform_int_distribution<int> dist(-127, 127);
    std::vector<int8_t> v(kK);
    for (size_t k = 0; k < kK; ++k) v[k] = static_cast<int8_t>(dist(rng));
    return v;
  }();
  return act;
}

std::vector<int32_t> compute_r2_oracle(const LayerSet& ls, const std::vector<int8_t>& act) {
  std::vector<int32_t> out(kN, 0);
  for (size_t n = 0; n < kN; ++n) {
    int64_t acc = 0;
    const int8_t* wrow = ls.int8_dense[0].data() + n * kK;
    for (size_t k = 0; k < kK; ++k) acc += static_cast<int64_t>(act[k]) * wrow[k];
    out[n] = static_cast<int32_t>(acc);
  }
  return out;
}

void matvec_r2_int8act_dualrailw(const int8_t* act, const uint64_t* Bp, const uint64_t* Bn,
                                 int32_t* out) {
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  static constexpr uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                              1, 2, 4, 8, 16, 32, 64, 128};
  static constexpr uint8_t kShuf[4][16] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
      {4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5},
      {6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7},
  };
  const uint8x16_t bw = vld1q_u8(kBitWeights);
  const uint8x16_t s0 = vld1q_u8(kShuf[0]);
  const uint8x16_t s1 = vld1q_u8(kShuf[1]);
  const uint8x16_t s2 = vld1q_u8(kShuf[2]);
  const uint8x16_t s3 = vld1q_u8(kShuf[3]);
  for (; n + 2 <= kN; n += 2) {
    const uint64_t* bp0 = Bp + (n + 0) * kW64;
    const uint64_t* bn0 = Bn + (n + 0) * kW64;
    const uint64_t* bp1 = Bp + (n + 1) * kW64;
    const uint64_t* bn1 = Bn + (n + 1) * kW64;
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    for (size_t w = 0; w < kW64; ++w) {
      const int8_t* ap = act + w * 64;
      const int8x16_t a0 = vld1q_s8(ap + 0);
      const int8x16_t a1 = vld1q_s8(ap + 16);
      const int8x16_t a2 = vld1q_s8(ap + 32);
      const int8x16_t a3 = vld1q_s8(ap + 48);
      auto dot64 = [&](uint64_t pw, uint64_t nw, int32x4_t& acc) {
        const uint8x16_t pb = vreinterpretq_u8_u64(vdupq_n_u64(pw));
        const uint8x16_t nb = vreinterpretq_u8_u64(vdupq_n_u64(nw));
        auto unpack16 = [&](uint8x16_t idx) -> int8x16_t {
          // vtstq_u8 returns 0xFF (-1 in s8) where bit is set, 0x00 otherwise.
          // So (nb_mask - pb_mask) == (+1 where p=1, -1 where n=1, 0 where both 0).
          const int8x16_t pm = vreinterpretq_s8_u8(vtstq_u8(vqtbl1q_u8(pb, idx), bw));
          const int8x16_t nm = vreinterpretq_s8_u8(vtstq_u8(vqtbl1q_u8(nb, idx), bw));
          return vsubq_s8(nm, pm);
        };
        acc = vdotq_s32(acc, a0, unpack16(s0));
        acc = vdotq_s32(acc, a1, unpack16(s1));
        acc = vdotq_s32(acc, a2, unpack16(s2));
        acc = vdotq_s32(acc, a3, unpack16(s3));
      };
      dot64(bp0[w], bn0[w], acc0);
      dot64(bp1[w], bn1[w], acc1);
    }
    out[n + 0] = vaddvq_s32(acc0);
    out[n + 1] = vaddvq_s32(acc1);
  }
#endif
  for (; n < kN; ++n) {
    int32_t acc = 0;
    const uint64_t* bp = Bp + n * kW64;
    const uint64_t* bn = Bn + n * kW64;
    for (size_t w = 0; w < kW64; ++w) {
      const uint64_t pw = bp[w], nw = bn[w];
      for (size_t b = 0; b < 64; ++b) {
        const int32_t t =
            static_cast<int32_t>((pw >> b) & 1ULL) - static_cast<int32_t>((nw >> b) & 1ULL);
        acc += static_cast<int32_t>(act[w * 64 + b]) * t;
      }
    }
    out[n] = acc;
  }
}

static void BM_MemBound_R2_INT8Act_INT8W(benchmark::State& state) {
  const auto& ls = get_layer_set(0);
  const auto& act = get_r2_int8_act();
  const auto oracle = compute_r2_oracle(ls, act);
  std::vector<int32_t> out(kN);
  matvec_int8_dense(act.data(), ls.int8_dense[0].data(), out.data());
  if (out != oracle) {
    state.SkipWithError("R2_INT8W oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * kK;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_int8_dense(act.data(), ls.int8_dense[l].data(), out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_R2_INT8Act_INT8W);

static void BM_MemBound_R2_INT8Act_INT4W(benchmark::State& state) {
  const auto& ls = get_layer_set(0);
  const auto& act = get_r2_int8_act();
  const auto oracle = compute_r2_oracle(ls, act);
  std::vector<int32_t> out(kN);
  matvec_int4_dense(act.data(), ls.int4_dense[0].data(), out.data());
  if (out != oracle) {
    state.SkipWithError("R2_INT4W oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * (kK / 2);
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_int4_dense(act.data(), ls.int4_dense[l].data(), out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_R2_INT8Act_INT4W);

static void BM_MemBound_R2_INT8Act_DualRailW(benchmark::State& state) {
  const auto& ls = get_layer_set(0);
  const auto& act = get_r2_int8_act();
  const auto oracle = compute_r2_oracle(ls, act);
  std::vector<int32_t> out(kN);
  matvec_r2_int8act_dualrailw(act.data(), ls.dr_p[0].data(), ls.dr_n[0].data(), out.data());
  if (out != oracle) {
    state.SkipWithError("R2_DualRailW oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kTotalWords * 16;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      matvec_r2_int8act_dualrailw(act.data(), ls.dr_p[l].data(), ls.dr_n[l].data(), out.data());
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_R2_INT8Act_DualRailW);

// ============================================================================
// H06-MEM-04: Multi-Batch M in {1, 4, 16} End-to-End Crossover (R1 vs R2)
// Measures full per-batch latency including runtime activation packing for R1.
// ============================================================================

struct BatchInputs {
  size_t M = 1;
  std::vector<int8_t> act_r1;  // M x K in {-1, 0, +1}
  std::vector<int8_t> act_r2;  // M x K in [-127, 127]
  std::vector<int32_t> oracle_r1;
  std::vector<int32_t> oracle_r2;
};

const BatchInputs& get_batch_inputs(size_t M) {
  static std::vector<BatchInputs> cache;
  for (const auto& b : cache) {
    if (b.M == M) return b;
  }
  const auto& ls = get_layer_set(0);
  BatchInputs bi;
  bi.M = M;
  bi.act_r1.resize(M * kK);
  bi.act_r2.resize(M * kK);
  std::mt19937_64 rng(0x42415443ULL ^ M);
  std::uniform_int_distribution<int> d1(-1, 1);
  std::uniform_int_distribution<int> d2(-127, 127);
  for (size_t i = 0; i < M * kK; ++i) {
    bi.act_r1[i] = static_cast<int8_t>(d1(rng));
    bi.act_r2[i] = static_cast<int8_t>(d2(rng));
  }
  bi.oracle_r1.assign(M * kN, 0);
  bi.oracle_r2.assign(M * kN, 0);
  for (size_t m = 0; m < M; ++m) {
    for (size_t n = 0; n < kN; ++n) {
      int64_t a1 = 0, a2 = 0;
      const int8_t* wrow = ls.int8_dense[0].data() + n * kK;
      for (size_t k = 0; k < kK; ++k) {
        a1 += static_cast<int64_t>(bi.act_r1[m * kK + k]) * wrow[k];
        a2 += static_cast<int64_t>(bi.act_r2[m * kK + k]) * wrow[k];
      }
      bi.oracle_r1[m * kN + n] = static_cast<int32_t>(a1);
      bi.oracle_r2[m * kN + n] = static_cast<int32_t>(a2);
    }
  }
  cache.push_back(std::move(bi));
  return cache.back();
}

static void BM_MemBound_Batch_INT8_Dense(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  for (size_t m = 0; m < M; ++m) {
    matvec_int8_dense(bi.act_r1.data() + m * kK, ls.int8_dense[0].data(), out.data() + m * kN);
  }
  if (out != bi.oracle_r1) {
    state.SkipWithError("Batch_INT8_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * kK;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      for (size_t m = 0; m < M; ++m) {
        matvec_int8_dense(bi.act_r1.data() + m * kK, ls.int8_dense[l].data(), out.data() + m * kN);
      }
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_Batch_INT8_Dense)->Arg(1)->Arg(4)->Arg(16);

static void BM_MemBound_Batch_INT4_Dense(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  for (size_t m = 0; m < M; ++m) {
    matvec_int4_dense(bi.act_r1.data() + m * kK, ls.int4_dense[0].data(), out.data() + m * kN);
  }
  if (out != bi.oracle_r1) {
    state.SkipWithError("Batch_INT4_Dense oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * (kK / 2);
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      for (size_t m = 0; m < M; ++m) {
        matvec_int4_dense(bi.act_r1.data() + m * kK, ls.int4_dense[l].data(), out.data() + m * kN);
      }
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_Batch_INT4_Dense)->Arg(1)->Arg(4)->Arg(16);

static void BM_MemBound_Batch_DualRail_R1(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<uint64_t> ap, an;
  std::vector<int32_t> out(M * kN);
  pack_act_dualrail(bi.act_r1, ap, an);
  for (size_t m = 0; m < M; ++m) {
    matvec_dualrail_dense(ap.data() + m * kW64, an.data() + m * kW64, ls.dr_p[0].data(),
                          ls.dr_n[0].data(), out.data() + m * kN);
  }
  if (out != bi.oracle_r1) {
    state.SkipWithError("Batch_DualRail_R1 oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kTotalWords * 16;
  for (auto _ : state) {
    // Charge full runtime activation packing for all M batch rows inside the timed loop.
    pack_act_dualrail(bi.act_r1, ap, an);
    for (size_t l = 0; l < kM_Layers; ++l) {
      for (size_t m = 0; m < M; ++m) {
        matvec_dualrail_dense(ap.data() + m * kW64, an.data() + m * kW64, ls.dr_p[l].data(),
                              ls.dr_n[l].data(), out.data() + m * kN);
      }
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_Batch_DualRail_R1)->Arg(1)->Arg(4)->Arg(16);

static void BM_MemBound_Batch_DualRail_R2(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  for (size_t m = 0; m < M; ++m) {
    matvec_r2_int8act_dualrailw(bi.act_r2.data() + m * kK, ls.dr_p[0].data(), ls.dr_n[0].data(),
                                out.data() + m * kN);
  }
  if (out != bi.oracle_r2) {
    state.SkipWithError("Batch_DualRail_R2 oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kTotalWords * 16;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) {
      for (size_t m = 0; m < M; ++m) {
        matvec_r2_int8act_dualrailw(bi.act_r2.data() + m * kK, ls.dr_p[l].data(), ls.dr_n[l].data(),
                                    out.data() + m * kN);
      }
      benchmark::DoNotOptimize(out.data());
    }
  }
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_Batch_DualRail_R2)->Arg(1)->Arg(4)->Arg(16);

// ============================================================================
// H06-MEM-05A: Multi-Threaded CPU R2 Scaling (T = 4 threads, M in {1, 4, 16})
// Tests whether parallel CPU cores shift R2 (DualRail -> int8 unpack) from
// compute-bound to DRAM-bound against matched 4-thread INT8W and INT4W.
// ============================================================================

class StaticPool {
public:
  explicit StaticPool(size_t n_threads) : n_threads_(n_threads) {
    workers_.reserve(n_threads_ > 1 ? n_threads_ - 1 : 0);
    for (size_t t = 1; t < n_threads_; ++t) {
      workers_.emplace_back([this, t] {
        uint64_t seen = 0;
        while (true) {
          std::unique_lock<std::mutex> lk(mu_);
          cv_.wait(lk, [&] { return stop_ || epoch_ != seen; });
          if (stop_) return;
          seen = epoch_;
          auto fn = fn_;
          lk.unlock();
          fn(t, n_threads_);
          if (done_.fetch_add(1, std::memory_order_acq_rel) + 1 == n_threads_) {
            std::lock_guard<std::mutex> dlk(mu_);
            done_cv_.notify_one();
          }
        }
      });
    }
  }

  ~StaticPool() {
    {
      std::lock_guard<std::mutex> lk(mu_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) w.join();
  }

  void run(std::function<void(size_t, size_t)> fn) {
    if (n_threads_ <= 1) {
      fn(0, 1);
      return;
    }
    {
      std::lock_guard<std::mutex> lk(mu_);
      fn_ = std::move(fn);
      done_.store(0, std::memory_order_relaxed);
      ++epoch_;
    }
    cv_.notify_all();
    fn_(0, n_threads_);
    if (done_.fetch_add(1, std::memory_order_acq_rel) + 1 < n_threads_) {
      std::unique_lock<std::mutex> lk(mu_);
      done_cv_.wait(lk, [&] { return done_.load(std::memory_order_acquire) == n_threads_; });
    }
  }

private:
  size_t n_threads_;
  std::vector<std::thread> workers_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::condition_variable done_cv_;
  std::function<void(size_t, size_t)> fn_;
  uint64_t epoch_ = 0;
  std::atomic<size_t> done_{0};
  bool stop_ = false;
};

StaticPool& pool4() {
  static StaticPool p(4);
  return p;
}

void matvec_int8_dense_range(const int8_t* act, const int8_t* W, int32_t* out, size_t n_beg,
                             size_t n_end) {
  size_t n = n_beg;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  for (; n + 4 <= n_end; n += 4) {
    const int8_t* w0 = W + (n + 0) * kK;
    const int8_t* w1 = W + (n + 1) * kK;
    const int8_t* w2 = W + (n + 2) * kK;
    const int8_t* w3 = W + (n + 3) * kK;
    int32x4_t a0 = vdupq_n_s32(0), a1 = vdupq_n_s32(0), a2 = vdupq_n_s32(0), a3 = vdupq_n_s32(0);
    for (size_t k = 0; k < kK; k += 16) {
      const int8x16_t av = vld1q_s8(act + k);
      a0 = vdotq_s32(a0, av, vld1q_s8(w0 + k));
      a1 = vdotq_s32(a1, av, vld1q_s8(w1 + k));
      a2 = vdotq_s32(a2, av, vld1q_s8(w2 + k));
      a3 = vdotq_s32(a3, av, vld1q_s8(w3 + k));
    }
    out[n + 0] = vaddvq_s32(a0);
    out[n + 1] = vaddvq_s32(a1);
    out[n + 2] = vaddvq_s32(a2);
    out[n + 3] = vaddvq_s32(a3);
  }
#endif
  for (; n < n_end; ++n) {
    int32_t acc = 0;
    const int8_t* wn = W + n * kK;
    for (size_t k = 0; k < kK; ++k) acc += static_cast<int32_t>(act[k]) * wn[k];
    out[n] = acc;
  }
}

void matvec_int4_dense_range(const int8_t* act, const uint8_t* W4, int32_t* out, size_t n_beg,
                             size_t n_end) {
  size_t n = n_beg;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  const uint8x16_t low_mask = vdupq_n_u8(0x0F);
  for (; n + 2 <= n_end; n += 2) {
    const uint8_t* w0 = W4 + (n + 0) * (kK / 2);
    const uint8_t* w1 = W4 + (n + 1) * (kK / 2);
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    for (size_t k = 0; k < kK; k += 32) {
      const int8x16x2_t av = vld2q_s8(act + k);
      auto dot32 = [&](const uint8_t* wp, int32x4_t& acc) {
        const uint8x16_t packed = vld1q_u8(wp + (k >> 1));
        const int8x16_t lo =
            vshrq_n_s8(vreinterpretq_s8_u8(vshlq_n_u8(vandq_u8(packed, low_mask), 4)), 4);
        const int8x16_t hi = vshrq_n_s8(vreinterpretq_s8_u8(packed), 4);
        acc = vdotq_s32(acc, av.val[0], lo);
        acc = vdotq_s32(acc, av.val[1], hi);
      };
      dot32(w0, acc0);
      dot32(w1, acc1);
    }
    out[n + 0] = vaddvq_s32(acc0);
    out[n + 1] = vaddvq_s32(acc1);
  }
#endif
  for (; n < n_end; ++n) {
    int32_t acc = 0;
    const uint8_t* wn = W4 + n * (kK / 2);
    for (size_t k = 0; k < kK; k += 2) {
      const uint8_t byte = wn[k >> 1];
      const int8_t lo = static_cast<int8_t>(static_cast<int8_t>(byte << 4) >> 4);
      const int8_t hi = static_cast<int8_t>(static_cast<int8_t>(byte) >> 4);
      acc += static_cast<int32_t>(act[k]) * lo + static_cast<int32_t>(act[k + 1]) * hi;
    }
    out[n] = acc;
  }
}

void matvec_r2_dualrail_range(const int8_t* act, const uint64_t* Bp, const uint64_t* Bn,
                              int32_t* out, size_t n_beg, size_t n_end) {
  size_t n = n_beg;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  static constexpr uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                              1, 2, 4, 8, 16, 32, 64, 128};
  static constexpr uint8_t kShuf[4][16] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
      {4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5},
      {6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7},
  };
  const uint8x16_t bw = vld1q_u8(kBitWeights);
  const uint8x16_t s0 = vld1q_u8(kShuf[0]);
  const uint8x16_t s1 = vld1q_u8(kShuf[1]);
  const uint8x16_t s2 = vld1q_u8(kShuf[2]);
  const uint8x16_t s3 = vld1q_u8(kShuf[3]);
  for (; n + 2 <= n_end; n += 2) {
    const uint64_t* bp0 = Bp + (n + 0) * kW64;
    const uint64_t* bn0 = Bn + (n + 0) * kW64;
    const uint64_t* bp1 = Bp + (n + 1) * kW64;
    const uint64_t* bn1 = Bn + (n + 1) * kW64;
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    for (size_t w = 0; w < kW64; ++w) {
      const int8_t* ap = act + w * 64;
      const int8x16_t a0 = vld1q_s8(ap + 0);
      const int8x16_t a1 = vld1q_s8(ap + 16);
      const int8x16_t a2 = vld1q_s8(ap + 32);
      const int8x16_t a3 = vld1q_s8(ap + 48);
      auto dot64 = [&](uint64_t pw, uint64_t nw, int32x4_t& acc) {
        const uint8x16_t pb = vreinterpretq_u8_u64(vdupq_n_u64(pw));
        const uint8x16_t nb = vreinterpretq_u8_u64(vdupq_n_u64(nw));
        auto unpack16 = [&](uint8x16_t idx) -> int8x16_t {
          const int8x16_t pm = vreinterpretq_s8_u8(vtstq_u8(vqtbl1q_u8(pb, idx), bw));
          const int8x16_t nm = vreinterpretq_s8_u8(vtstq_u8(vqtbl1q_u8(nb, idx), bw));
          return vsubq_s8(nm, pm);
        };
        acc = vdotq_s32(acc, a0, unpack16(s0));
        acc = vdotq_s32(acc, a1, unpack16(s1));
        acc = vdotq_s32(acc, a2, unpack16(s2));
        acc = vdotq_s32(acc, a3, unpack16(s3));
      };
      dot64(bp0[w], bn0[w], acc0);
      dot64(bp1[w], bn1[w], acc1);
    }
    out[n + 0] = vaddvq_s32(acc0);
    out[n + 1] = vaddvq_s32(acc1);
  }
#endif
  for (; n < n_end; ++n) {
    int32_t acc = 0;
    const uint64_t* bp = Bp + n * kW64;
    const uint64_t* bn = Bn + n * kW64;
    for (size_t w = 0; w < kW64; ++w) {
      const uint64_t pw = bp[w], nw = bn[w];
      for (size_t b = 0; b < 64; ++b) {
        const int32_t t =
            static_cast<int32_t>((pw >> b) & 1ULL) - static_cast<int32_t>((nw >> b) & 1ULL);
        acc += static_cast<int32_t>(act[w * 64 + b]) * t;
      }
    }
    out[n] = acc;
  }
}

static void BM_MemBound_MT4_R2_INT8W(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  auto run_layer = [&](size_t l) {
    pool4().run([&](size_t tid, size_t nt) {
      const size_t nb = (kN * tid) / nt;
      const size_t ne = (kN * (tid + 1)) / nt;
      for (size_t m = 0; m < M; ++m) {
        matvec_int8_dense_range(bi.act_r2.data() + m * kK, ls.int8_dense[l].data(),
                                out.data() + m * kN, nb, ne);
      }
    });
  };
  run_layer(0);
  if (out != bi.oracle_r2) {
    state.SkipWithError("MT4_R2_INT8W oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * kK;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) run_layer(l);
    benchmark::DoNotOptimize(out.data());
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_MT4_R2_INT8W)->Arg(1)->Arg(4)->Arg(16)->UseRealTime();

static void BM_MemBound_MT4_R2_INT4W(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  auto run_layer = [&](size_t l) {
    pool4().run([&](size_t tid, size_t nt) {
      const size_t nb = (kN * tid) / nt;
      const size_t ne = (kN * (tid + 1)) / nt;
      for (size_t m = 0; m < M; ++m) {
        matvec_int4_dense_range(bi.act_r2.data() + m * kK, ls.int4_dense[l].data(),
                                out.data() + m * kN, nb, ne);
      }
    });
  };
  run_layer(0);
  if (out != bi.oracle_r2) {
    state.SkipWithError("MT4_R2_INT4W oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kN * (kK / 2);
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) run_layer(l);
    benchmark::DoNotOptimize(out.data());
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_MT4_R2_INT4W)->Arg(1)->Arg(4)->Arg(16)->UseRealTime();

static void BM_MemBound_MT4_R2_DualRailW(benchmark::State& state) {
  const size_t M = static_cast<size_t>(state.range(0));
  const auto& ls = get_layer_set(0);
  const auto& bi = get_batch_inputs(M);
  std::vector<int32_t> out(M * kN);
  auto run_layer = [&](size_t l) {
    pool4().run([&](size_t tid, size_t nt) {
      const size_t nb = (kN * tid) / nt;
      const size_t ne = (kN * (tid + 1)) / nt;
      for (size_t m = 0; m < M; ++m) {
        matvec_r2_dualrail_range(bi.act_r2.data() + m * kK, ls.dr_p[l].data(), ls.dr_n[l].data(),
                                 out.data() + m * kN, nb, ne);
      }
    });
  };
  run_layer(0);
  if (out != bi.oracle_r2) {
    state.SkipWithError("MT4_R2_DualRailW oracle mismatch");
    return;
  }
  const size_t total_bytes = kM_Layers * kTotalWords * 16;
  for (auto _ : state) {
    for (size_t l = 0; l < kM_Layers; ++l) run_layer(l);
    benchmark::DoNotOptimize(out.data());
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(total_bytes));
  state.counters["batch_M"] = static_cast<double>(M);
  state.counters["weight_MB"] = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemBound_MT4_R2_DualRailW)->Arg(1)->Arg(4)->Arg(16)->UseRealTime();

}  // namespace
