// H06-MEM-05B & H06-MEM-06: Apple Silicon Metal GPU R2/R1 + SIMD Host Pack Benchmark
//
// Compares zero-copy MTLResourceStorageModeShared fused GPU kernels across L = 32 distinct
// 4096x1024 layers (134.2M weights: 128 MB INT8, 64 MB INT4, 32 MB DualRail) for M in {1, 4, 16}:
//   - GPU_R2_INT8W        : char4 int8 activations x char4 int8 weights (128 MB)
//   - GPU_R2_INT4W        : char4 int8 activations x packed 4-bit signed weights (64 MB)
//   - GPU_R2_DualRailW    : scalar bit-extract Dual-Rail (32 MB, H06-MEM-05B baseline)
//   - GPU_R2_DualRail_LUT : uint4 coalesced + 16-entry char4 LUT Dual-Rail (32 MB, H06-MEM-06)
//   - GPU_R1_DualRail_Pop : uint4 popcount Dual-Rail for R1 ternary activations (32 MB)
//
// Also measures 32-layer weight packing (scalar vs 1-thread NEON/AVX2 vs 4-thread NEON/AVX2)
// and computes exact K-token break-even K* against 4-thread CPU MT4_R2_INT4W (1.031 ms / 16L).

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

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

#include "t81/crypto/sha3.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr size_t kL = 32;
constexpr size_t kK = 4096;
constexpr size_t kN = 1024;
constexpr size_t kW32 = kK / 32;  // 128 uint32 words per column
constexpr size_t kW64 = kK / 64;  // 64 uint64 words per column

std::string sha3_512_hex(const int32_t* data, size_t count) {
  std::vector<uint8_t> bytes(count * sizeof(int32_t));
  std::memcpy(bytes.data(), data, bytes.size());
  const auto d = t81::crypto::sha3_512(bytes);
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.reserve(d.size() * 2);
  for (uint8_t b : d) {
    out.push_back(kHex[(b >> 4) & 0xF]);
    out.push_back(kHex[b & 0xF]);
  }
  return out;
}

void pack_row_simd(const int8_t* src, uint64_t* dst_p, uint64_t* dst_n, size_t len) {
  const size_t words = len / 64;
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
    dst_p[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(pq, pq)), 0);
    const uint8x16_t nq = vpaddq_u8(
        vpaddq_u8(vandq_u8(vcltq_s8(v0, zero), weights), vandq_u8(vcltq_s8(v1, zero), weights)),
        vpaddq_u8(vandq_u8(vcltq_s8(v2, zero), weights), vandq_u8(vcltq_s8(v3, zero), weights)));
    dst_n[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(nq, nq)), 0);
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
    dst_p[w] = static_cast<uint64_t>(plo) | (static_cast<uint64_t>(phi) << 32);
    dst_n[w] = static_cast<uint64_t>(nlo) | (static_cast<uint64_t>(nhi) << 32);
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
    dst_p[w] = p;
    dst_n[w] = n;
  }
#endif
}

#if T81_HAS_METAL
static NSString* const kMetalSrc = @R"(
#include <metal_stdlib>
using namespace metal;

struct R2Dims {
  uint K;
  uint N;
  uint M;
  uint W32;
};

constant char4 kLut4[16] = {
  char4(0,0,0,0), char4(1,0,0,0), char4(0,1,0,0), char4(1,1,0,0),
  char4(0,0,1,0), char4(1,0,1,0), char4(0,1,1,0), char4(1,1,1,0),
  char4(0,0,0,1), char4(1,0,0,1), char4(0,1,0,1), char4(1,1,0,1),
  char4(0,0,1,1), char4(1,0,1,1), char4(0,1,1,1), char4(1,1,1,1),
};

kernel void r2_int8_gemm(
    device const char4* A   [[buffer(0)]],
    device const char4* W   [[buffer(1)]],
    device int* C           [[buffer(2)]],
    constant R2Dims& d      [[buffer(3)]],
    uint2 tg_id             [[threadgroup_position_in_grid]],
    uint simd_lane          [[thread_index_in_simdgroup]],
    uint simd_id            [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg       [[simdgroups_per_threadgroup]]) {
  uint n = tg_id.x * simds_per_tg + simd_id;
  uint m = tg_id.y;
  if (n >= d.N || m >= d.M) return;

  uint k4 = d.K >> 2;
  device const char4* a_row = A + m * k4;
  device const char4* w_col = W + n * k4;

  int acc = 0;
  for (uint i = simd_lane; i < k4; i += 32) {
    char4 a = a_row[i];
    char4 w = w_col[i];
    acc += int(a.x) * int(w.x) + int(a.y) * int(w.y) + int(a.z) * int(w.z) + int(a.w) * int(w.w);
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) C[m * d.N + n] = acc;
}

kernel void r2_int4_gemm(
    device const char4* A    [[buffer(0)]],
    device const ushort* W4  [[buffer(1)]],
    device int* C            [[buffer(2)]],
    constant R2Dims& d       [[buffer(3)]],
    uint2 tg_id              [[threadgroup_position_in_grid]],
    uint simd_lane           [[thread_index_in_simdgroup]],
    uint simd_id             [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg        [[simdgroups_per_threadgroup]]) {
  uint n = tg_id.x * simds_per_tg + simd_id;
  uint m = tg_id.y;
  if (n >= d.N || m >= d.M) return;

  uint k4 = d.K >> 2;
  device const char4* a_row = A + m * k4;
  device const ushort* w_col = W4 + n * k4;

  int acc = 0;
  for (uint i = simd_lane; i < k4; i += 32) {
    char4 a = a_row[i];
    ushort p = w_col[i];
    int w0 = int(char((p & 0xFu) << 4)) >> 4;
    int w1 = int(char(((p >> 4) & 0xFu) << 4)) >> 4;
    int w2 = int(char(((p >> 8) & 0xFu) << 4)) >> 4;
    int w3 = int(char(((p >> 12) & 0xFu) << 4)) >> 4;
    acc += int(a.x) * w0 + int(a.y) * w1 + int(a.z) * w2 + int(a.w) * w3;
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) C[m * d.N + n] = acc;
}

inline int dot_u32_lut(device const char4* a_base, uint pw, uint nw) {
  int acc = 0;
  #pragma unroll
  for (uint g = 0; g < 8; ++g) {
    char4 a = a_base[g];
    char4 w = kLut4[(pw >> (g * 4)) & 0xFu] - kLut4[(nw >> (g * 4)) & 0xFu];
    acc += int(a.x) * int(w.x) + int(a.y) * int(w.y) + int(a.z) * int(w.z) + int(a.w) * int(w.w);
  }
  return acc;
}

kernel void r2_dualrail_lut_gemm(
    device const char4* A   [[buffer(0)]],
    device const uint4* Bp  [[buffer(1)]],
    device const uint4* Bn  [[buffer(2)]],
    device int* C           [[buffer(3)]],
    constant R2Dims& d      [[buffer(4)]],
    uint2 tg_id             [[threadgroup_position_in_grid]],
    uint simd_lane          [[thread_index_in_simdgroup]],
    uint simd_id            [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg       [[simdgroups_per_threadgroup]]) {
  uint n = tg_id.x * simds_per_tg + simd_id;
  uint m = tg_id.y;
  if (n >= d.N || m >= d.M) return;

  uint k4 = d.K >> 2;
  uint w4 = d.W32 >> 2;  // 128-bit coalesced uint4 loads
  device const char4* a_row = A + m * k4;
  device const uint4* bp_col = Bp + n * w4;
  device const uint4* bn_col = Bn + n * w4;

  int acc = 0;
  for (uint w = simd_lane; w < w4; w += 32) {
    uint4 pw = bp_col[w];
    uint4 nw = bn_col[w];
    uint base4 = w << 5;  // 128 elements = 32 char4s
    acc += dot_u32_lut(a_row + base4 + 0, pw.x, nw.x);
    acc += dot_u32_lut(a_row + base4 + 8, pw.y, nw.y);
    acc += dot_u32_lut(a_row + base4 + 16, pw.z, nw.z);
    acc += dot_u32_lut(a_row + base4 + 24, pw.w, nw.w);
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) C[m * d.N + n] = acc;
}

kernel void r1_dualrail_pop_gemm(
    device const uint4* Ap  [[buffer(0)]],
    device const uint4* An  [[buffer(1)]],
    device const uint4* Bp  [[buffer(2)]],
    device const uint4* Bn  [[buffer(3)]],
    device int* C           [[buffer(4)]],
    constant R2Dims& d      [[buffer(5)]],
    uint2 tg_id             [[threadgroup_position_in_grid]],
    uint simd_lane          [[thread_index_in_simdgroup]],
    uint simd_id            [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg       [[simdgroups_per_threadgroup]]) {
  uint n = tg_id.x * simds_per_tg + simd_id;
  uint m = tg_id.y;
  if (n >= d.N || m >= d.M) return;

  uint w4 = d.W32 >> 2;
  device const uint4* ap_row = Ap + m * w4;
  device const uint4* an_row = An + m * w4;
  device const uint4* bp_col = Bp + n * w4;
  device const uint4* bn_col = Bn + n * w4;

  int acc = 0;
  for (uint w = simd_lane; w < w4; w += 32) {
    uint4 ap = ap_row[w], an = an_row[w];
    uint4 bp = bp_col[w], bn = bn_col[w];
    uint4 ps = popcount((ap & bp) | (an & bn));
    uint4 pd = popcount((ap & bn) | (an & bp));
    acc += int((ps.x + ps.y) + (ps.z + ps.w)) - int((pd.x + pd.y) + (pd.z + pd.w));
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) C[m * d.N + n] = acc;
}

kernel void r1_sbdr128_pop_gemm(
    device const uint4* Ap      [[buffer(0)]],
    device const uint4* An      [[buffer(1)]],
    device const uint* Offsets  [[buffer(2)]],
    device const ushort* Idx    [[buffer(3)]],
    device const uint4* Bp      [[buffer(4)]],
    device const uint4* Bn      [[buffer(5)]],
    device int* C               [[buffer(6)]],
    constant R2Dims& d          [[buffer(7)]],
    uint2 tg_id                 [[threadgroup_position_in_grid]],
    uint simd_lane              [[thread_index_in_simdgroup]],
    uint simd_id                [[simdgroup_index_in_threadgroup]],
    uint simds_per_tg           [[simdgroups_per_threadgroup]]) {
  uint n = tg_id.x * simds_per_tg + simd_id;
  uint m = tg_id.y;
  if (n >= d.N || m >= d.M) return;

  uint w4 = d.W32 >> 2;
  device const uint4* ap_row = Ap + m * w4;
  device const uint4* an_row = An + m * w4;
  uint start = Offsets[n];
  uint end = Offsets[n + 1];

  int acc = 0;
  for (uint j = start + simd_lane; j < end; j += 32) {
    uint w = uint(Idx[j]);
    uint4 ap = ap_row[w], an = an_row[w];
    uint4 bp = Bp[j], bn = Bn[j];
    uint4 ps = popcount((ap & bp) | (an & bn));
    uint4 pd = popcount((ap & bn) | (an & bp));
    acc += int((ps.x + ps.y) + (ps.z + ps.w)) - int((pd.x + pd.y) + (pd.z + pd.w));
  }
  acc = simd_sum(acc);
  if (simd_lane == 0) C[m * d.N + n] = acc;
}
)";

struct R2Dims {
  uint32_t K;
  uint32_t N;
  uint32_t M;
  uint32_t W32;
};
#endif

}  // namespace

int main(int argc, char** argv) {
  size_t num_layers = kL;
  bool run_sparse = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--layers=", 9) == 0) {
      const long v = std::strtol(argv[i] + 9, nullptr, 10);
      if (v > 0 && v <= 128) num_layers = static_cast<size_t>(v);
    } else if (std::strcmp(argv[i], "--sparse") == 0) {
      run_sparse = true;
    }
  }
#if !T81_HAS_METAL
  (void)num_layers;
  (void)run_sparse;
  std::cout << "{\"metal_available\":false}\n";
  return 0;
#else
  @autoreleasepool {
    const size_t L = num_layers;
    id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
    if (!dev) return 1;
    NSError* err = nil;
    id<MTLLibrary> lib = [dev newLibraryWithSource:kMetalSrc options:nil error:&err];
    if (!lib) {
      std::cerr << "Metal compile error: " << [[err localizedDescription] UTF8String] << "\n";
      return 1;
    }
    auto make_pso = [&](NSString* name) -> id<MTLComputePipelineState> {
      id<MTLFunction> fn = [lib newFunctionWithName:name];
      NSError* perr = nil;
      id<MTLComputePipelineState> pso = [dev newComputePipelineStateWithFunction:fn error:&perr];
      if (!pso) std::exit(1);
      return pso;
    };
    id<MTLComputePipelineState> pso_i8 = make_pso(@"r2_int8_gemm");
    id<MTLComputePipelineState> pso_i4 = make_pso(@"r2_int4_gemm");
    id<MTLComputePipelineState> pso_lut = make_pso(@"r2_dualrail_lut_gemm");
    id<MTLComputePipelineState> pso_r1 = make_pso(@"r1_dualrail_pop_gemm");
    id<MTLComputePipelineState> pso_sbdr = make_pso(@"r1_sbdr128_pop_gemm");
    id<MTLCommandQueue> queue = [dev newCommandQueue];

    std::mt19937_64 rng(0x48303636ULL);
    std::uniform_int_distribution<int> trit_dist(-1, 1);
    std::uniform_int_distribution<int> act_dist(-127, 127);

    const size_t i8_layer_bytes = kN * kK;
    const size_t i4_layer_bytes = kN * (kK / 2);
    const size_t dr_plane_bytes = kN * kW32 * sizeof(uint32_t);

    std::vector<id<MTLBuffer>> buf_i8(L), buf_i4(L), buf_bp(L), buf_bn(L);
    for (size_t l = 0; l < L; ++l) {
      buf_i8[l] = [dev newBufferWithLength:i8_layer_bytes options:MTLResourceStorageModeShared];
      buf_i4[l] = [dev newBufferWithLength:i4_layer_bytes options:MTLResourceStorageModeShared];
      buf_bp[l] = [dev newBufferWithLength:dr_plane_bytes options:MTLResourceStorageModeShared];
      buf_bn[l] = [dev newBufferWithLength:dr_plane_bytes options:MTLResourceStorageModeShared];
      int8_t* w8 = static_cast<int8_t*>([buf_i8[l] contents]);
      uint8_t* w4 = static_cast<uint8_t*>([buf_i4[l] contents]);
      for (size_t i = 0; i < kN * kK; ++i) w8[i] = static_cast<int8_t>(trit_dist(rng));
      for (size_t n = 0; n < kN; ++n) {
        pack_row_simd(w8 + n * kK, static_cast<uint64_t*>([buf_bp[l] contents]) + n * kW64,
                      static_cast<uint64_t*>([buf_bn[l] contents]) + n * kW64, kK);
        for (size_t k = 0; k < kK; k += 2) {
          const uint8_t lo = static_cast<uint8_t>(w8[n * kK + k]) & 0x0Fu;
          const uint8_t hi = static_cast<uint8_t>(w8[n * kK + k + 1]) & 0x0Fu;
          w4[n * (kK / 2) + (k >> 1)] = static_cast<uint8_t>(lo | (hi << 4));
        }
      }
    }

    // Measure 1-thread SIMD vs 4-thread SIMD host packing of all L layers
    auto t_s1 = Clock::now();
    for (size_t l = 0; l < L; ++l) {
      const int8_t* w8 = static_cast<const int8_t*>([buf_i8[l] contents]);
      uint64_t* bp = static_cast<uint64_t*>([buf_bp[l] contents]);
      uint64_t* bn = static_cast<uint64_t*>([buf_bn[l] contents]);
      pack_row_simd(w8, bp, bn, kN * kK);
    }
    const double simd_pack_1t_ms =
        std::chrono::duration<double, std::milli>(Clock::now() - t_s1).count();

    auto t_s4 = Clock::now();
    std::vector<std::thread> threads;
    for (size_t t = 0; t < 4; ++t) {
      threads.emplace_back([&, t] {
        for (size_t l = t; l < L; l += 4) {
          const int8_t* w8 = static_cast<const int8_t*>([buf_i8[l] contents]);
          uint64_t* bp = static_cast<uint64_t*>([buf_bp[l] contents]);
          uint64_t* bn = static_cast<uint64_t*>([buf_bn[l] contents]);
          pack_row_simd(w8, bp, bn, kN * kK);
        }
      });
    }
    for (auto& th : threads) th.join();
    const double simd_pack_4t_ms =
        std::chrono::duration<double, std::milli>(Clock::now() - t_s4).count();

    std::cout << "Metal Device: " << [[dev name] UTF8String] << " | Layers=" << L
              << " (4096x1024)\n"
              << "HostPack" << L << "L: SIMD_1T=" << std::fixed << std::setprecision(2)
              << simd_pack_1t_ms << "ms | SIMD_4T=" << simd_pack_4t_ms << "ms\n";

    constexpr size_t kSimdsPerTg = 4;
    const MTLSize tg_size = MTLSizeMake(32 * kSimdsPerTg, 1, 1);

    for (uint32_t M : {1u, 4u, 16u}) {
      id<MTLBuffer> buf_a_r2 =
          [dev newBufferWithLength:M * kK options:MTLResourceStorageModeShared];
      id<MTLBuffer> buf_ap_r1 = [dev newBufferWithLength:M * kW32 * sizeof(uint32_t)
                                                 options:MTLResourceStorageModeShared];
      id<MTLBuffer> buf_an_r1 = [dev newBufferWithLength:M * kW32 * sizeof(uint32_t)
                                                 options:MTLResourceStorageModeShared];
      id<MTLBuffer> buf_c =
          [dev newBufferWithLength:M * kN * sizeof(int32_t) options:MTLResourceStorageModeShared];

      int8_t* a_r2 = static_cast<int8_t*>([buf_a_r2 contents]);
      std::vector<int8_t> a_r1(M * kK);
      for (size_t i = 0; i < M * kK; ++i) {
        a_r2[i] = static_cast<int8_t>(act_dist(rng));
        a_r1[i] = static_cast<int8_t>(trit_dist(rng));
      }
      pack_row_simd(a_r1.data(), static_cast<uint64_t*>([buf_ap_r1 contents]),
                    static_cast<uint64_t*>([buf_an_r1 contents]), M * kK);

      std::vector<int32_t> oracle_r2(M * kN, 0), oracle_r1(M * kN, 0);
      const int8_t* w_last = static_cast<const int8_t*>([buf_i8[L - 1] contents]);
      for (uint32_t m = 0; m < M; ++m) {
        for (size_t n = 0; n < kN; ++n) {
          int64_t a2 = 0, a1 = 0;
          for (size_t k = 0; k < kK; ++k) {
            a2 += static_cast<int64_t>(a_r2[m * kK + k]) * w_last[n * kK + k];
            a1 += static_cast<int64_t>(a_r1[m * kK + k]) * w_last[n * kK + k];
          }
          oracle_r2[m * kN + n] = static_cast<int32_t>(a2);
          oracle_r1[m * kN + n] = static_cast<int32_t>(a1);
        }
      }
      const std::string sha_r2 = sha3_512_hex(oracle_r2.data(), oracle_r2.size());
      const std::string sha_r1 = sha3_512_hex(oracle_r1.data(), oracle_r1.size());

      const R2Dims dims{static_cast<uint32_t>(kK), static_cast<uint32_t>(kN), M,
                        static_cast<uint32_t>(kW32)};
      const MTLSize grid = MTLSizeMake((kN + kSimdsPerTg - 1) / kSimdsPerTg, M, 1);

      auto bench_gpu = [&](const char* label, id<MTLComputePipelineState> pso, int mode,
                           double weight_gb, const std::string& exp_sha) -> double {
        auto dispatch_once = [&]() {
          id<MTLCommandBuffer> cb = [queue commandBuffer];
          id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
          [enc setComputePipelineState:pso];
          for (size_t l = 0; l < L; ++l) {
            if (mode == 0) {
              [enc setBuffer:buf_a_r2 offset:0 atIndex:0];
              [enc setBuffer:buf_i8[l] offset:0 atIndex:1];
              [enc setBuffer:buf_c offset:0 atIndex:2];
              [enc setBytes:&dims length:sizeof(dims) atIndex:3];
            } else if (mode == 1) {
              [enc setBuffer:buf_a_r2 offset:0 atIndex:0];
              [enc setBuffer:buf_i4[l] offset:0 atIndex:1];
              [enc setBuffer:buf_c offset:0 atIndex:2];
              [enc setBytes:&dims length:sizeof(dims) atIndex:3];
            } else if (mode == 2) {
              [enc setBuffer:buf_a_r2 offset:0 atIndex:0];
              [enc setBuffer:buf_bp[l] offset:0 atIndex:1];
              [enc setBuffer:buf_bn[l] offset:0 atIndex:2];
              [enc setBuffer:buf_c offset:0 atIndex:3];
              [enc setBytes:&dims length:sizeof(dims) atIndex:4];
            } else {
              [enc setBuffer:buf_ap_r1 offset:0 atIndex:0];
              [enc setBuffer:buf_an_r1 offset:0 atIndex:1];
              [enc setBuffer:buf_bp[l] offset:0 atIndex:2];
              [enc setBuffer:buf_bn[l] offset:0 atIndex:3];
              [enc setBuffer:buf_c offset:0 atIndex:4];
              [enc setBytes:&dims length:sizeof(dims) atIndex:5];
            }
            [enc dispatchThreadgroups:grid threadsPerThreadgroup:tg_size];
          }
          [enc endEncoding];
          [cb commit];
          [cb waitUntilCompleted];
        };

        dispatch_once();
        const std::string got_sha =
            sha3_512_hex(static_cast<const int32_t*>([buf_c contents]), M * kN);
        if (got_sha != exp_sha) {
          std::cerr << "ORACLE MISMATCH on " << label << " M=" << M << "\n";
          std::exit(2);
        }
        constexpr int kIters = 10;
        const auto t0 = Clock::now();
        for (int it = 0; it < kIters; ++it) dispatch_once();
        const double ms =
            std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / kIters;
        const double gbs = weight_gb / (ms / 1e3);
        std::cout << "M=" << std::setw(2) << M << " | " << std::left << std::setw(20) << label
                  << std::right << " | real=" << std::setw(7) << std::setprecision(3) << ms
                  << " ms | per_tok=" << std::setw(7) << (ms / M) << " ms | bw=" << std::setw(6)
                  << std::setprecision(2) << gbs << " GB/s | sha3=OK\n";
        return ms;
      };

      const double t_i8 = bench_gpu("GPU_R2_INT8W", pso_i8, 0, (L * i8_layer_bytes) / 1e9, sha_r2);
      const double t_i4 = bench_gpu("GPU_R2_INT4W", pso_i4, 1, (L * i4_layer_bytes) / 1e9, sha_r2);
      const double t_lut =
          bench_gpu("GPU_R2_DualRail_LUT", pso_lut, 2, (L * 2 * dr_plane_bytes) / 1e9, sha_r2);
      const double t_r1 =
          bench_gpu("GPU_R1_DualRail_Pop", pso_r1, 3, (L * 2 * dr_plane_bytes) / 1e9, sha_r1);

      if (M == 1) {
        const double cpu_i4_ms = 1.031 * (static_cast<double>(L) / 16.0);
        const double kstar_r2_vs_i4 =
            (t_i4 > t_lut) ? std::ceil(simd_pack_4t_ms / (t_i4 - t_lut)) : -1.0;
        const double kstar_r1_vs_i4 =
            (t_i4 > t_r1) ? std::ceil(simd_pack_4t_ms / (t_i4 - t_r1)) : -1.0;
        const double kstar_r1_vs_cpu_i4 =
            (cpu_i4_ms > t_r1) ? std::ceil(simd_pack_4t_ms / (cpu_i4_ms - t_r1)) : -1.0;
        std::cout << "  -> M=1 Break-Even (SIMD_4T pack=" << std::setprecision(2) << simd_pack_4t_ms
                  << "ms): K*(R2_LUT vs GPU_INT4)=" << std::setprecision(0) << kstar_r2_vs_i4
                  << " | K*(R1_Pop vs GPU_INT4)=" << kstar_r1_vs_i4
                  << " | K*(R1_Pop vs 4T_CPU_INT4)=" << kstar_r1_vs_cpu_i4
                  << " | R2_LUT vs 4T_CPU_INT4=" << std::setprecision(2) << (cpu_i4_ms / t_lut)
                  << "x | R1_Pop vs 4T_CPU_INT4=" << (cpu_i4_ms / t_r1) << "x\n";
      }
      (void)t_i8;
    }

    if (run_sparse) {
      std::cout << "\n--- H06-MEM-10: Metal GPU_R1_SBDR128_Pop (L=" << L << ", 4096x1024) ---\n";
      const size_t kBlocksPerCol = kK / 128;  // 32 128-trit blocks per column
      for (int sp_pct : {50, 75, 90}) {
        std::vector<id<MTLBuffer>> s_off(L), s_idx(L), s_bp(L), s_bn(L);
        std::vector<std::vector<int8_t>> sparse_w_last(kN, std::vector<int8_t>(kK, 0));
        size_t total_bytes = 0;
        for (size_t l = 0; l < L; ++l) {
          std::vector<uint32_t> offs(kN + 1, 0);
          std::vector<uint16_t> idxs;
          std::vector<uint64_t> bp_v, bn_v;
          idxs.reserve((kN * kBlocksPerCol * (100 - sp_pct)) / 100 + kN);
          bp_v.reserve(idxs.capacity() * 2);
          bn_v.reserve(idxs.capacity() * 2);
          std::uniform_int_distribution<int> pct(0, 99);
          for (size_t n = 0; n < kN; ++n) {
            offs[n] = static_cast<uint32_t>(idxs.size());
            for (size_t b = 0; b < kBlocksPerCol; ++b) {
              if (pct(rng) < sp_pct) continue;
              int8_t blk[128];
              for (int i = 0; i < 128; ++i) {
                blk[i] = static_cast<int8_t>(trit_dist(rng));
                if (l == L - 1) sparse_w_last[n][b * 128 + i] = blk[i];
              }
              uint64_t p[2], q[2];
              pack_row_simd(blk, p, q, 128);
              idxs.push_back(static_cast<uint16_t>(b));
              bp_v.push_back(p[0]);
              bp_v.push_back(p[1]);
              bn_v.push_back(q[0]);
              bn_v.push_back(q[1]);
            }
          }
          offs[kN] = static_cast<uint32_t>(idxs.size());
          const size_t nnz = std::max<size_t>(1, idxs.size());
          s_off[l] = [dev newBufferWithBytes:offs.data()
                                      length:offs.size() * sizeof(uint32_t)
                                     options:MTLResourceStorageModeShared];
          s_idx[l] = [dev newBufferWithBytes:(idxs.empty() ? nullptr : idxs.data())
                                      length:nnz * sizeof(uint16_t)
                                     options:MTLResourceStorageModeShared];
          s_bp[l] = [dev newBufferWithBytes:(bp_v.empty() ? nullptr : bp_v.data())
                                     length:nnz * 2 * sizeof(uint64_t)
                                    options:MTLResourceStorageModeShared];
          s_bn[l] = [dev newBufferWithBytes:(bn_v.empty() ? nullptr : bn_v.data())
                                     length:nnz * 2 * sizeof(uint64_t)
                                    options:MTLResourceStorageModeShared];
          total_bytes += offs.size() * sizeof(uint32_t) + idxs.size() * sizeof(uint16_t) +
                         idxs.size() * 4 * sizeof(uint64_t);
        }
        for (uint32_t M : {1u, 4u, 16u}) {
          id<MTLBuffer> buf_ap = [dev newBufferWithLength:M * kW32 * sizeof(uint32_t)
                                                  options:MTLResourceStorageModeShared];
          id<MTLBuffer> buf_an = [dev newBufferWithLength:M * kW32 * sizeof(uint32_t)
                                                  options:MTLResourceStorageModeShared];
          id<MTLBuffer> buf_c = [dev newBufferWithLength:M * kN * sizeof(int32_t)
                                                 options:MTLResourceStorageModeShared];
          std::vector<int8_t> a_r1(M * kK);
          for (size_t i = 0; i < M * kK; ++i) a_r1[i] = static_cast<int8_t>(trit_dist(rng));
          pack_row_simd(a_r1.data(), static_cast<uint64_t*>([buf_ap contents]),
                        static_cast<uint64_t*>([buf_an contents]), M * kK);
          std::vector<int32_t> oracle(M * kN, 0);
          for (uint32_t m = 0; m < M; ++m) {
            for (size_t n = 0; n < kN; ++n) {
              int64_t acc = 0;
              for (size_t k = 0; k < kK; ++k) {
                acc += static_cast<int64_t>(a_r1[m * kK + k]) * sparse_w_last[n][k];
              }
              oracle[m * kN + n] = static_cast<int32_t>(acc);
            }
          }
          const std::string exp_sha = sha3_512_hex(oracle.data(), oracle.size());
          const R2Dims dims{static_cast<uint32_t>(kK), static_cast<uint32_t>(kN), M,
                            static_cast<uint32_t>(kW32)};
          const MTLSize grid = MTLSizeMake((kN + kSimdsPerTg - 1) / kSimdsPerTg, M, 1);
          auto run_once = [&]() {
            id<MTLCommandBuffer> cb = [queue commandBuffer];
            id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
            [enc setComputePipelineState:pso_sbdr];
            for (size_t l = 0; l < L; ++l) {
              [enc setBuffer:buf_ap offset:0 atIndex:0];
              [enc setBuffer:buf_an offset:0 atIndex:1];
              [enc setBuffer:s_off[l] offset:0 atIndex:2];
              [enc setBuffer:s_idx[l] offset:0 atIndex:3];
              [enc setBuffer:s_bp[l] offset:0 atIndex:4];
              [enc setBuffer:s_bn[l] offset:0 atIndex:5];
              [enc setBuffer:buf_c offset:0 atIndex:6];
              [enc setBytes:&dims length:sizeof(dims) atIndex:7];
              [enc dispatchThreadgroups:grid threadsPerThreadgroup:tg_size];
            }
            [enc endEncoding];
            [cb commit];
            [cb waitUntilCompleted];
          };
          run_once();
          const std::string got_sha =
              sha3_512_hex(static_cast<const int32_t*>([buf_c contents]), M * kN);
          if (got_sha != exp_sha) {
            std::cerr << "ORACLE MISMATCH on GPU_R1_SBDR128_Pop sp=" << sp_pct << " M=" << M
                      << "\n";
            std::exit(2);
          }
          constexpr int kIters = 10;
          const auto t0 = Clock::now();
          for (int it = 0; it < kIters; ++it) run_once();
          const double ms =
              std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / kIters;
          const double gbs = (static_cast<double>(total_bytes) / 1e9) / (ms / 1e3);
          std::cout << "s=" << sp_pct << "% | M=" << std::setw(2) << M
                    << " | GPU_R1_SBDR128_Pop | size=" << std::setw(5) << std::setprecision(2)
                    << (static_cast<double>(total_bytes) / (1024.0 * 1024.0))
                    << " MB | real=" << std::setw(7) << std::setprecision(3) << ms
                    << " ms | per_tok=" << std::setw(7) << (ms / M) << " ms | bw=" << std::setw(6)
                    << std::setprecision(2) << gbs << " GB/s | sha3=OK\n";
        }
      }
    }
  }
  return 0;
#endif
}
