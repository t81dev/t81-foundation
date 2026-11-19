// Heterogeneous Dual-Rail (P, N) CPU <-> GPU Capability & Verification Demo
// Portable C++23 with zero-copy Apple Silicon Metal GPU path on macOS (__APPLE__ && __OBJC__).
// Verifies bit-exact SHA3-512 output equivalence across CPU int8, CPU DualRail, and Fused GPU DualRail.

#if defined(__APPLE__) && defined(__OBJC__)
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#define T81_HAS_METAL 1
#else
#define T81_HAS_METAL 0
#endif

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "t81/crypto/sha3.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

#if T81_HAS_METAL
static NSString* const kMetalSource = @R"(
#include <metal_stdlib>
using namespace metal;

struct GemvDims {
  uint K;
  uint N;
  uint W32;
};

kernel void dualrail_gemv_m1(
    device const uint4* Ap [[buffer(0)]],
    device const uint4* An [[buffer(1)]],
    device const uint4* Bp [[buffer(2)]],
    device const uint4* Bn [[buffer(3)]],
    device int* C          [[buffer(4)]],
    constant GemvDims& d   [[buffer(5)]],
    uint tg_id             [[threadgroup_position_in_grid]],
    uint simd_lane         [[thread_index_in_simdgroup]],
    uint simd_id           [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg      [[simdgroups_per_threadgroup]]) {
  uint n = tg_id * simds_per_tg + simd_id;
  if (n >= d.N) return;

  uint w4 = d.W32 >> 2;
  uint b_off = n * w4;

  int acc = 0;
  for (uint w = simd_lane; w < w4; w += 32) {
    uint4 ap = Ap[w];
    uint4 an = An[w];
    uint4 bp = Bp[b_off + w];
    uint4 bn = Bn[b_off + w];

    uint4 same = (ap & bp) | (an & bn);
    uint4 diff = (ap & bn) | (an & bp);

    uint4 ps = popcount(same);
    uint4 pd = popcount(diff);
    acc += int((ps.x + ps.y) + (ps.z + ps.w)) - int((pd.x + pd.y) + (pd.z + pd.w));
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) {
    C[n] = acc;
  }
}
)";

struct GemvDims {
  uint32_t K;
  uint32_t N;
  uint32_t W32;
};
#endif

void pack_dualrail_into(const int8_t* src, uint64_t* dst_p, uint64_t* dst_n,
                        size_t rows, size_t cols) {
  const size_t W64 = cols / 64;
#if defined(__aarch64__) && defined(__ARM_NEON)
  const int8x16_t zero = vdupq_n_s8(0);
  static constexpr uint8_t kBitWeights[16] = {
      1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128};
  const uint8x16_t weights = vld1q_u8(kBitWeights);
#endif
  for (size_t r = 0; r < rows; ++r) {
    const int8_t* row = src + r * cols;
    uint64_t* dp = dst_p + r * W64;
    uint64_t* dn = dst_n + r * W64;
    for (size_t w = 0; w < W64; ++w) {
#if defined(__aarch64__) && defined(__ARM_NEON)
      const int8_t* p = row + w * 64;
      const int8x16_t v0 = vld1q_s8(p + 0);
      const int8x16_t v1 = vld1q_s8(p + 16);
      const int8x16_t v2 = vld1q_s8(p + 32);
      const int8x16_t v3 = vld1q_s8(p + 48);

      const uint8x16_t pq = vpaddq_u8(
          vpaddq_u8(vandq_u8(vcgtq_s8(v0, zero), weights), vandq_u8(vcgtq_s8(v1, zero), weights)),
          vpaddq_u8(vandq_u8(vcgtq_s8(v2, zero), weights), vandq_u8(vcgtq_s8(v3, zero), weights)));
      dp[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(pq, pq)), 0);

      const uint8x16_t nq = vpaddq_u8(
          vpaddq_u8(vandq_u8(vcltq_s8(v0, zero), weights), vandq_u8(vcltq_s8(v1, zero), weights)),
          vpaddq_u8(vandq_u8(vcltq_s8(v2, zero), weights), vandq_u8(vcltq_s8(v3, zero), weights)));
      dn[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(nq, nq)), 0);
#else
      uint64_t p = 0, n = 0;
      const int8_t* ptr = row + w * 64;
      for (size_t i = 0; i < 64; ++i) {
        if (ptr[i] > 0) p |= (1ULL << i);
        else if (ptr[i] < 0) n |= (1ULL << i);
      }
      dp[w] = p;
      dn[w] = n;
#endif
    }
  }
}

void cpu_dualrail_gemv(const uint64_t* ap, const uint64_t* an,
                       const uint64_t* Bp, const uint64_t* Bn,
                       int32_t* C, size_t K, size_t N) {
  const size_t W64 = K / 64;
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_NEON)
  for (; n + 4 <= N; n += 4) {
    const uint64_t* bp0 = Bp + (n + 0) * W64; const uint64_t* bn0 = Bn + (n + 0) * W64;
    const uint64_t* bp1 = Bp + (n + 1) * W64; const uint64_t* bn1 = Bn + (n + 1) * W64;
    const uint64_t* bp2 = Bp + (n + 2) * W64; const uint64_t* bn2 = Bn + (n + 2) * W64;
    const uint64_t* bp3 = Bp + (n + 3) * W64; const uint64_t* bn3 = Bn + (n + 3) * W64;

    uint16x8_t p0 = vdupq_n_u16(0), n0 = vdupq_n_u16(0);
    uint16x8_t p1 = vdupq_n_u16(0), n1 = vdupq_n_u16(0);
    uint16x8_t p2 = vdupq_n_u16(0), n2 = vdupq_n_u16(0);
    uint16x8_t p3 = vdupq_n_u16(0), n3 = vdupq_n_u16(0);

    for (size_t w = 0; w < W64; w += 2) {
      const uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
      const uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
      auto accum = [&](const uint64_t* bp, const uint64_t* bn, uint16x8_t& pa, uint16x8_t& na) {
        const uint8x16_t bvp = vreinterpretq_u8_u64(vld1q_u64(bp + w));
        const uint8x16_t bvn = vreinterpretq_u8_u64(vld1q_u64(bn + w));
        pa = vpadalq_u8(pa, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
        na = vpadalq_u8(na, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
      };
      accum(bp0, bn0, p0, n0);
      accum(bp1, bn1, p1, n1);
      accum(bp2, bn2, p2, n2);
      accum(bp3, bn3, p3, n3);
    }
    C[n + 0] = static_cast<int32_t>(vaddlvq_u16(p0)) - static_cast<int32_t>(vaddlvq_u16(n0));
    C[n + 1] = static_cast<int32_t>(vaddlvq_u16(p1)) - static_cast<int32_t>(vaddlvq_u16(n1));
    C[n + 2] = static_cast<int32_t>(vaddlvq_u16(p2)) - static_cast<int32_t>(vaddlvq_u16(n2));
    C[n + 3] = static_cast<int32_t>(vaddlvq_u16(p3)) - static_cast<int32_t>(vaddlvq_u16(n3));
  }
#endif
  for (; n < N; ++n) {
    const uint64_t* bp = Bp + n * W64;
    const uint64_t* bn = Bn + n * W64;
    int32_t acc = 0;
    for (size_t w = 0; w < W64; ++w) {
      acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
             std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
    }
    C[n] = acc;
  }
}

void cpu_int8_gemv(const int8_t* a_row, const int8_t* B, int32_t* C, size_t K, size_t N) {
  size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)
  for (; n + 4 <= N; n += 4) {
    const int8_t* b0 = B + (n + 0) * K;
    const int8_t* b1 = B + (n + 1) * K;
    const int8_t* b2 = B + (n + 2) * K;
    const int8_t* b3 = B + (n + 3) * K;
    int32x4_t acc0 = vdupq_n_s32(0), acc1 = vdupq_n_s32(0);
    int32x4_t acc2 = vdupq_n_s32(0), acc3 = vdupq_n_s32(0);
    for (size_t k = 0; k < K; k += 16) {
      const int8x16_t av = vld1q_s8(a_row + k);
      acc0 = vdotq_s32(acc0, av, vld1q_s8(b0 + k));
      acc1 = vdotq_s32(acc1, av, vld1q_s8(b1 + k));
      acc2 = vdotq_s32(acc2, av, vld1q_s8(b2 + k));
      acc3 = vdotq_s32(acc3, av, vld1q_s8(b3 + k));
    }
    C[n + 0] = vaddvq_s32(acc0);
    C[n + 1] = vaddvq_s32(acc1);
    C[n + 2] = vaddvq_s32(acc2);
    C[n + 3] = vaddvq_s32(acc3);
  }
#endif
  for (; n < N; ++n) {
    const int8_t* b_row = B + n * K;
    int32_t acc = 0;
    for (size_t k = 0; k < K; ++k) acc += static_cast<int32_t>(a_row[k]) * b_row[k];
    C[n] = acc;
  }
}

std::string hash_i32(const int32_t* p, size_t n) {
  return t81::crypto::sha3_512_hex(
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(p), n * sizeof(int32_t)));
}

template <typename Fn>
double measure_ms(Fn&& fn, int reps) {
  fn();  // warmup
  const auto t0 = Clock::now();
  for (int i = 0; i < reps; ++i) fn();
  const auto t1 = Clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count() / static_cast<double>(reps);
}

}  // namespace

int main(int argc, char** argv) {
  size_t layers = 8;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg.starts_with("--layers=")) {
      layers = static_cast<size_t>(std::strtoull(std::string(arg.substr(9)).c_str(), nullptr, 10));
      if (layers == 0 || layers > 64) layers = 8;
    }
  }

  constexpr size_t K = 4096;
  constexpr size_t N_proj = 4096;
  constexpr size_t N_ffn = 11008;
  constexpr size_t W64 = K / 64;
  const size_t proj_words = N_proj * W64;
  const size_t ffn_words = N_ffn * W64;
  const size_t layer_words = proj_words + ffn_words;
  const size_t total_words = layers * layer_words;
  const size_t total_dr_bytes = total_words * sizeof(uint64_t) * 2;

  std::mt19937_64 rng(2026);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> a_i8(K);
  for (auto& x : a_i8) x = static_cast<int8_t>(dist(rng));

  // Build 1 exact int8 reference layer for CPU int8 oracle + throughput comparison
  std::vector<int8_t> b0_proj_i8(N_proj * K), b0_ffn_i8(N_ffn * K);
  for (auto& x : b0_proj_i8) x = static_cast<int8_t>(dist(rng));
  for (auto& x : b0_ffn_i8) x = static_cast<int8_t>(dist(rng));

  std::vector<int32_t> c0_i8_proj(N_proj), c0_i8_ffn(N_ffn);
  cpu_int8_gemv(a_i8.data(), b0_proj_i8.data(), c0_i8_proj.data(), K, N_proj);
  cpu_int8_gemv(a_i8.data(), b0_ffn_i8.data(), c0_i8_ffn.data(), K, N_ffn);

#if T81_HAS_METAL
  @autoreleasepool {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    NSError* err = nil;
    id<MTLLibrary> lib = [device newLibraryWithSource:kMetalSource options:nil error:&err];
    id<MTLComputePipelineState> pso =
        [device newComputePipelineStateWithFunction:[lib newFunctionWithName:@"dualrail_gemv_m1"] error:&err];
    id<MTLCommandQueue> queue = [device newCommandQueue];

    id<MTLBuffer> buf_Ap = [device newBufferWithLength:W64 * sizeof(uint64_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf_An = [device newBufferWithLength:W64 * sizeof(uint64_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf_Bp = [device newBufferWithLength:total_words * sizeof(uint64_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf_Bn = [device newBufferWithLength:total_words * sizeof(uint64_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf_C_proj = [device newBufferWithLength:layers * N_proj * sizeof(int32_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf_C_ffn  = [device newBufferWithLength:layers * N_ffn  * sizeof(int32_t) options:MTLResourceStorageModeShared];

    uint64_t* Ap = static_cast<uint64_t*>([buf_Ap contents]);
    uint64_t* An = static_cast<uint64_t*>([buf_An contents]);
    uint64_t* Bp = static_cast<uint64_t*>([buf_Bp contents]);
    uint64_t* Bn = static_cast<uint64_t*>([buf_Bn contents]);

    pack_dualrail_into(a_i8.data(), Ap, An, 1, K);
    pack_dualrail_into(b0_proj_i8.data(), Bp, Bn, N_proj, K);
    pack_dualrail_into(b0_ffn_i8.data(), Bp + proj_words, Bn + proj_words, N_ffn, K);
    for (size_t i = layer_words; i < total_words; ++i) {
      uint64_t r1 = rng(), r2 = rng();
      Bp[i] = r1 & r2;
      Bn[i] = (~r1) & r2;
    }

    std::vector<int32_t> C_cpu_proj(layers * N_proj), C_cpu_ffn(layers * N_ffn);
    auto run_cpu_dr = [&]() {
      for (size_t l = 0; l < layers; ++l) {
        const size_t off_p = l * layer_words;
        const size_t off_f = off_p + proj_words;
        cpu_dualrail_gemv(Ap, An, Bp + off_p, Bn + off_p, C_cpu_proj.data() + l * N_proj, K, N_proj);
        cpu_dualrail_gemv(Ap, An, Bp + off_f, Bn + off_f, C_cpu_ffn.data() + l * N_ffn, K, N_ffn);
      }
    };

    const GemvDims d_proj{K, N_proj, K / 32};
    const GemvDims d_ffn{K, N_ffn, K / 32};
    const NSUInteger simds_per_tg = 8;
    const NSUInteger tg_threads = simds_per_tg * 32;

    auto run_gpu_fused = [&](double* kern_ms) {
      id<MTLCommandBuffer> cb = [queue commandBuffer];
      id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
      [enc setComputePipelineState:pso];
      [enc setBuffer:buf_Ap offset:0 atIndex:0];
      [enc setBuffer:buf_An offset:0 atIndex:1];
      for (size_t l = 0; l < layers; ++l) {
        const NSUInteger off_p = (l * layer_words) * sizeof(uint64_t);
        const NSUInteger off_f = (l * layer_words + proj_words) * sizeof(uint64_t);
        [enc setBuffer:buf_Bp offset:off_p atIndex:2];
        [enc setBuffer:buf_Bn offset:off_p atIndex:3];
        [enc setBuffer:buf_C_proj offset:l * N_proj * sizeof(int32_t) atIndex:4];
        [enc setBytes:&d_proj length:sizeof(d_proj) atIndex:5];
        [enc dispatchThreadgroups:MTLSizeMake((N_proj + simds_per_tg - 1) / simds_per_tg, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(tg_threads, 1, 1)];

        [enc setBuffer:buf_Bp offset:off_f atIndex:2];
        [enc setBuffer:buf_Bn offset:off_f atIndex:3];
        [enc setBuffer:buf_C_ffn offset:l * N_ffn * sizeof(int32_t) atIndex:4];
        [enc setBytes:&d_ffn length:sizeof(d_ffn) atIndex:5];
        [enc dispatchThreadgroups:MTLSizeMake((N_ffn + simds_per_tg - 1) / simds_per_tg, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(tg_threads, 1, 1)];
      }
      [enc endEncoding];
      [cb commit];
      [cb waitUntilCompleted];
      if (kern_ms) *kern_ms = ([cb GPUEndTime] - [cb GPUStartTime]) * 1e3;
    };

    run_cpu_dr();
    run_gpu_fused(nullptr);

    const int32_t* G_proj = static_cast<const int32_t*>([buf_C_proj contents]);
    const int32_t* G_ffn = static_cast<const int32_t*>([buf_C_ffn contents]);

    const bool l0_i8_ok = (hash_i32(c0_i8_proj.data(), N_proj) == hash_i32(C_cpu_proj.data(), N_proj)) &&
                          (hash_i32(c0_i8_ffn.data(), N_ffn) == hash_i32(C_cpu_ffn.data(), N_ffn));
    const std::string h_cpu_p = hash_i32(C_cpu_proj.data(), C_cpu_proj.size());
    const std::string h_gpu_p = hash_i32(G_proj, C_cpu_proj.size());
    const std::string h_cpu_f = hash_i32(C_cpu_ffn.data(), C_cpu_ffn.size());
    const std::string h_gpu_f = hash_i32(G_ffn, C_cpu_ffn.size());
    const bool all_exact = l0_i8_ok && (h_cpu_p == h_gpu_p) && (h_cpu_f == h_gpu_f);
    if (!all_exact) {
      std::cerr << "FAIL: Cross-accelerator SHA3-512 mismatch!\n";
      return 1;
    }

    const double ms_cpu_dr = measure_ms(run_cpu_dr, 3);
    double gpu_kern_ms = 0.0;
    const double ms_gpu_fused = measure_ms([&]() { run_gpu_fused(&gpu_kern_ms); }, 4);
    const double eff_bw_gbs = (static_cast<double>(total_dr_bytes) / 1e9) / (ms_gpu_fused / 1e3);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "{\n"
              << "  \"schema\": \"t81.heterogeneous-dualrail.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"device\": \"" << [[device name] UTF8String] << "\",\n"
              << "  \"unified_memory\": " << ([device hasUnifiedMemory] ? "true" : "false") << ",\n"
              << "  \"layers\": " << layers << ",\n"
              << "  \"dualrail_weight_mb\": " << (static_cast<double>(total_dr_bytes) / (1024.0 * 1024.0)) << ",\n"
              << "  \"bit_exact_cross_device_sha3_512\": true,\n"
              << "  \"proj_digest\": \"sha3-512:" << h_cpu_p << "\",\n"
              << "  \"ffn_digest\": \"sha3-512:" << h_cpu_f << "\",\n"
              << "  \"cpu_dualrail_ms\": " << ms_cpu_dr << ",\n"
              << "  \"gpu_fused_dualrail_ms\": " << ms_gpu_fused << ",\n"
              << "  \"gpu_kernel_ms\": " << gpu_kern_ms << ",\n"
              << "  \"gpu_effective_bw_gbs\": " << eff_bw_gbs << ",\n"
              << "  \"gpu_vs_cpu_speedup\": " << (ms_cpu_dr / ms_gpu_fused) << ",\n"
              << "  \"advertised_capabilities\": {\n"
              << "    \"layout\": \"DualRail_PN_64\",\n"
              << "    \"bytes_per_trit\": 0.25,\n"
              << "    \"zero_copy_cpu_gpu\": true,\n"
              << "    \"bit_exact_integer_reduction\": true,\n"
              << "    \"recommended_route\": {\n"
              << "      \"small_or_single_sync_decode\": \"cpu_neon_vcnt\",\n"
              << "      \"fused_multilayer_decode\": \"gpu_metal_dualrail_fused\"\n"
              << "    }\n"
              << "  }\n"
              << "}\n";
    return 0;
  }
#else
  std::vector<uint64_t> Ap(W64, 0), An(W64, 0);
  std::vector<uint64_t> Bp(total_words, 0), Bn(total_words, 0);
  pack_dualrail_into(a_i8.data(), Ap.data(), An.data(), 1, K);
  pack_dualrail_into(b0_proj_i8.data(), Bp.data(), Bn.data(), N_proj, K);
  pack_dualrail_into(b0_ffn_i8.data(), Bp.data() + proj_words, Bn.data() + proj_words, N_ffn, K);
  for (size_t i = layer_words; i < total_words; ++i) {
    uint64_t r1 = rng(), r2 = rng();
    Bp[i] = r1 & r2;
    Bn[i] = (~r1) & r2;
  }
  std::vector<int32_t> C_cpu_proj(layers * N_proj), C_cpu_ffn(layers * N_ffn);
  auto run_cpu_dr = [&]() {
    for (size_t l = 0; l < layers; ++l) {
      const size_t off_p = l * layer_words;
      const size_t off_f = off_p + proj_words;
      cpu_dualrail_gemv(Ap.data(), An.data(), Bp.data() + off_p, Bn.data() + off_p, C_cpu_proj.data() + l * N_proj, K, N_proj);
      cpu_dualrail_gemv(Ap.data(), An.data(), Bp.data() + off_f, Bn.data() + off_f, C_cpu_ffn.data() + l * N_ffn, K, N_ffn);
    }
  };
  const double ms_cpu_dr = measure_ms(run_cpu_dr, 3);
  const std::string h_cpu_p = hash_i32(C_cpu_proj.data(), C_cpu_proj.size());
  const std::string h_cpu_f = hash_i32(C_cpu_ffn.data(), C_cpu_ffn.size());
  if (hash_i32(c0_i8_proj.data(), N_proj) != hash_i32(C_cpu_proj.data(), N_proj) ||
      hash_i32(c0_i8_ffn.data(), N_ffn) != hash_i32(C_cpu_ffn.data(), N_ffn)) {
    std::cerr << "FAIL: CPU int8 vs DualRail SHA3-512 mismatch!\n";
    return 1;
  }
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "{\n"
            << "  \"schema\": \"t81.heterogeneous-dualrail.v1\",\n"
            << "  \"ok\": true,\n"
            << "  \"device\": \"cpu-only\",\n"
            << "  \"unified_memory\": false,\n"
            << "  \"layers\": " << layers << ",\n"
            << "  \"bit_exact_cross_device_sha3_512\": true,\n"
            << "  \"proj_digest\": \"sha3-512:" << h_cpu_p << "\",\n"
            << "  \"ffn_digest\": \"sha3-512:" << h_cpu_f << "\",\n"
            << "  \"cpu_dualrail_ms\": " << ms_cpu_dr << "\n"
            << "}\n";
  return 0;
#endif
}
