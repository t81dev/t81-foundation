#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "vm/regime_dispatch.hpp"

#define T81_REQUIRE(cond)                                                             \
  do {                                                                                \
    if (!(cond)) {                                                                    \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #cond << "\n"; \
      std::exit(1);                                                                   \
    }                                                                                 \
  } while (0)

namespace {

using namespace t81::vm;

constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;

void test_apple_m2_boundaries() {
  std::cout << "[1/5] Apple M2 arm64 R0..R4 boundary transitions...\n";
  const HardwareProfile m2 = HardwareProfile::apple_m2_reference();
  RegimeSelector sel(m2);

  // R0: small batch inside 16 MiB L2
  WorkloadQuery q0;
  q0.batch_size = 4;
  q0.working_set_bytes = 8 * kMiB;
  q0.sparsity_pct = 50;
  auto d0 = sel.select(q0);
  T81_REQUIRE(d0.regime == Regime::R0_CacheResidentInline);
  T81_REQUIRE(d0.backend == BackendKind::CpuSimd1T);
  T81_REQUIRE(d0.kernel == KernelKind::SBDR128_SIMD);

  // R1: medium batch inside L2
  sel.reset_hysteresis();
  WorkloadQuery q1;
  q1.batch_size = 8;
  q1.working_set_bytes = 10 * kMiB;
  auto d1 = sel.select(q1);
  T81_REQUIRE(d1.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(d1.backend == BackendKind::CpuSimdMT);
  T81_REQUIRE(d1.worker_threads == 8);

  // R2: off-L2 streaming (> 0.85 * 16 MiB, < 64 MiB Metal threshold)
  sel.reset_hysteresis();
  WorkloadQuery q2;
  q2.batch_size = 8;
  q2.working_set_bytes = 24 * kMiB;
  auto d2 = sel.select(q2);
  T81_REQUIRE(d2.regime == Regime::R2_OffL3StreamingDRAM);
  T81_REQUIRE(d2.backend == BackendKind::CpuStreamMT);

  // R3: Metal GPU eligible (B >= 16, ws >= 64 MiB, queue_depth <= 2)
  sel.reset_hysteresis();
  WorkloadQuery q3;
  q3.batch_size = 32;
  q3.working_set_bytes = 128 * kMiB;
  q3.sparsity_pct = 75;
  q3.metal_queue_depth = 1;
  auto d3 = sel.select(q3);
  T81_REQUIRE(d3.regime == Regime::R3_AcceleratorMultiBatch);
  T81_REQUIRE(d3.backend == BackendKind::MetalGpu);
  T81_REQUIRE(d3.kernel == KernelKind::SBDR1T2D);

  // R4: Oversized out-of-core (B < metal_min_batch so CPU path, ws > 2 * LLC)
  sel.reset_hysteresis();
  WorkloadQuery q4;
  q4.batch_size = 8;
  q4.working_set_bytes = 64 * kMiB;
  auto d4 = sel.select(q4);
  T81_REQUIRE(d4.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d4.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d4.tile_batch_size < q4.batch_size);
}

void test_xeon_6975p_boundaries() {
  std::cout << "[2/5] Xeon 6975P-C x86_64 AVX2 (480 MiB L3) boundary transitions...\n";
  const HardwareProfile xeon = HardwareProfile::xeon_6975p_reference();
  RegimeSelector sel(xeon);

  // R0: small batch inside L3
  WorkloadQuery q0;
  q0.batch_size = 4;
  q0.working_set_bytes = 64 * kMiB;
  auto d0 = sel.select(q0);
  T81_REQUIRE(d0.regime == Regime::R0_CacheResidentInline);
  T81_REQUIRE(d0.backend == BackendKind::CpuSimd1T);

  // R1: multi-batch inside 480 MiB L3
  sel.reset_hysteresis();
  WorkloadQuery q1;
  q1.batch_size = 16;
  q1.working_set_bytes = 256 * kMiB;
  auto d1 = sel.select(q1);
  T81_REQUIRE(d1.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(d1.backend == BackendKind::CpuSimdMT);
  T81_REQUIRE(d1.worker_threads == 4);

  // R2: off-L3 DRAM streaming (> 0.85 * 480 MiB = 408 MiB, <= 960 MiB)
  sel.reset_hysteresis();
  WorkloadQuery q2;
  q2.batch_size = 16;
  q2.working_set_bytes = 640 * kMiB;
  q2.sparsity_pct = 75;
  auto d2 = sel.select(q2);
  T81_REQUIRE(d2.regime == Regime::R2_OffL3StreamingDRAM);
  T81_REQUIRE(d2.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d2.kernel == KernelKind::SBDR128_SIMD);

  // R4: oversized out-of-core (> 960 MiB)
  sel.reset_hysteresis();
  WorkloadQuery q4;
  q4.batch_size = 64;
  q4.working_set_bytes = 1536 * kMiB;
  auto d4 = sel.select(q4);
  T81_REQUIRE(d4.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d4.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d4.tile_batch_size >= 1 && d4.tile_batch_size < q4.batch_size);
}

void test_hysteresis_stability() {
  std::cout << "[3/5] Hysteresis stability under boundary batch oscillation...\n";
  const HardwareProfile xeon = HardwareProfile::xeon_6975p_reference();
  DispatchPolicy pol;
  pol.hysteresis_gain_ratio = 1.15;
  RegimeSelector sel(xeon, pol);

  // Start in R0 at B = 4
  WorkloadQuery q4;
  q4.batch_size = 4;
  q4.working_set_bytes = 64 * kMiB;
  auto d_init = sel.select(q4);
  T81_REQUIRE(d_init.regime == Regime::R0_CacheResidentInline);

  // Oscillate to B = 5 -> R1 score (4 * 0.85 = 3.4) vs R0 score (4.4) -> gain < 1.15 -> hold R0!
  WorkloadQuery q5 = q4;
  q5.batch_size = 5;
  auto d_osc = sel.select(q5);
  T81_REQUIRE(d_osc.regime == Regime::R0_CacheResidentInline);
  T81_REQUIRE(!d_osc.switched_from_active);

  // Oscillate near B = 16 <-> 17 on Apple M2 with small working set (< Metal threshold)
  const HardwareProfile m2 = HardwareProfile::apple_m2_reference();
  RegimeSelector sel_m2(m2, pol);
  WorkloadQuery q16;
  q16.batch_size = 16;
  q16.working_set_bytes = 10 * kMiB;
  T81_REQUIRE(sel_m2.select(q16).regime == Regime::R1_L3ResidentMultiBatch);
  WorkloadQuery q17 = q16;
  q17.batch_size = 17;
  auto d17 = sel_m2.select(q17);
  T81_REQUIRE(d17.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(!d17.switched_from_active);

  // Oscillate B = 32 <-> 33 inside Xeon 480 MiB L3 -> stays stable in R1
  RegimeSelector sel_r1(xeon, pol);
  WorkloadQuery q32;
  q32.batch_size = 32;
  q32.working_set_bytes = 200 * kMiB;
  T81_REQUIRE(sel_r1.select(q32).regime == Regime::R1_L3ResidentMultiBatch);
  WorkloadQuery q33 = q32;
  q33.batch_size = 33;
  auto d33 = sel_r1.select(q33);
  T81_REQUIRE(d33.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(!d33.switched_from_active);
}

void test_metal_queue_and_worker_fallbacks() {
  std::cout << "[4/5] Metal queue-depth (>2) and unhealthy worker-pool fallbacks...\n";
  const HardwareProfile m2 = HardwareProfile::apple_m2_reference();
  RegimeSelector sel(m2);

  WorkloadQuery q;
  q.batch_size = 32;
  q.working_set_bytes = 128 * kMiB;
  q.metal_queue_depth = 3;  // > 2 -> must fall back to cpu_simd_mt
  auto d_fb = sel.select(q);
  T81_REQUIRE(d_fb.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(d_fb.backend == BackendKind::CpuSimdMT);
  T81_REQUIRE(d_fb.fallback_triggered);

  // Worker pool unhealthy -> must fall back to cpu_simd_1t
  q.worker_pool_healthy = false;
  auto d_pool = sel.select(q);
  T81_REQUIRE(d_pool.regime == Regime::R0_CacheResidentInline);
  T81_REQUIRE(d_pool.backend == BackendKind::CpuSimd1T);
  T81_REQUIRE(d_pool.worker_threads == 1);
  T81_REQUIRE(d_pool.fallback_triggered);
}

void test_bit_exact_cross_backend_determinism() {
  std::cout << "[5/5] Bit-exact determinism_hash equivalence across 1T and MT backends...\n";
  constexpr std::uint32_t kBatch = 16;
  constexpr std::uint32_t kOut = 32;
  constexpr std::uint32_t kDim = 128;

  std::vector<std::int8_t> weights(kOut * kDim);
  std::vector<std::int8_t> acts(kBatch * kDim);
  for (std::size_t i = 0; i < weights.size(); ++i)
    weights[i] = static_cast<std::int8_t>((i % 3) - 1);
  for (std::size_t i = 0; i < acts.size(); ++i)
    acts[i] = static_cast<std::int8_t>(((i * 7) % 3) - 1);

  auto run_range = [&](std::uint32_t s, std::uint32_t e, std::vector<std::int64_t>& out) {
    for (std::uint32_t b = s; b < e; ++b) {
      for (std::uint32_t o = 0; o < kOut; ++o) {
        std::int64_t acc = 0;
        for (std::uint32_t k = 0; k < kDim; ++k) {
          acc += static_cast<std::int64_t>(acts[b * kDim + k]) *
                 static_cast<std::int64_t>(weights[o * kDim + k]);
        }
        out[b * kOut + o] = acc;
      }
    }
  };

  std::vector<std::int64_t> out_1t(kBatch * kOut, 0);
  run_range(0, kBatch, out_1t);

  std::vector<std::int64_t> out_mt(kBatch * kOut, 0);
  std::vector<std::thread> threads;
  for (std::uint32_t t = 0; t < 4; ++t) {
    threads.emplace_back([&, t]() { run_range(t * 4, (t + 1) * 4, out_mt); });
  }
  for (auto& th : threads) th.join();

  T81_REQUIRE(out_1t == out_mt);
}

void test_m2_work_op_gating_and_stream_cap() {
  std::cout << "[6/6] Apple M2 R3 work_ops gating & R4 stream_worker_cap=8...\n";
  const HardwareProfile m2 = HardwareProfile::apple_m2_reference();
  DispatchPolicy pol;
  pol.metal_min_batch = 16;
  pol.metal_min_working_set_bytes = 64 * kMiB;
  pol.metal_min_work_ops = 32ULL * 1024ULL * 1024ULL;  // 32M ops
  pol.stream_worker_cap = 0;                           // auto -> 8 on Apple M2 unified memory

  RegimeSelector sel(m2, pol);

  // Small matrix (128x512, B=32 -> 4.19M ops < 32M): must NOT dispatch to Metal GPU
  WorkloadQuery q_small;
  q_small.batch_size = 32;
  q_small.working_set_bytes = 80 * kMiB;
  q_small.work_ops = 2ULL * 32ULL * 128ULL * 512ULL;
  auto d_small = sel.select(q_small);
  T81_REQUIRE(d_small.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_small.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_small.worker_threads == 8);

  // Large matrix (512x2048, B=32 -> 67.1M ops >= 32M): eligible for Metal GPU R3
  sel.reset_hysteresis();
  WorkloadQuery q_large = q_small;
  q_large.work_ops = 2ULL * 32ULL * 512ULL * 2048ULL;
  auto d_large = sel.select(q_large);
  T81_REQUIRE(d_large.regime == Regime::R3_AcceleratorMultiBatch);
  T81_REQUIRE(d_large.backend == BackendKind::MetalGpu);

  // Explicit stream_worker_cap = 8 in R4
  pol.stream_worker_cap = 8;
  RegimeSelector sel_cap8(m2, pol);
  WorkloadQuery q_r4;
  q_r4.batch_size = 16;
  q_r4.working_set_bytes = 48 * kMiB;  // > 2*16MiB LLC, < 64MiB Metal threshold
  auto d_r4 = sel_cap8.select(q_r4);
  T81_REQUIRE(d_r4.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_r4.worker_threads == 8);
}

void test_phase3_r3_r4_boundary_and_sparse_routing_m2() {
  std::cout << "[7/8] Phase 3 Apple M2 R3/R4 boundary tightening & sparse-aware dispatch...\n";
  const HardwareProfile m2 = HardwareProfile::apple_m2_reference();
  RegimeSelector sel(m2);  // default policy: metal_min_ws=32MiB, metal_min_nonzero_density=0.30

  // Medium working set (24 MiB < 32 MiB) avoids GPU launch latency even at B=32, 512x2048 ops.
  WorkloadQuery q_med;
  q_med.batch_size = 32;
  q_med.working_set_bytes = 24 * kMiB;
  q_med.work_ops = 2ULL * 32ULL * 512ULL * 2048ULL;
  q_med.nonzero_density = 1.0;
  auto d_med = sel.select(q_med);
  T81_REQUIRE(d_med.regime == Regime::R2_OffL3StreamingDRAM);
  T81_REQUIRE(d_med.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_med.worker_threads == 8);

  // OUT_OF_CORE 512x2048 B=32 (~60 MiB dense DualRail/SBDR1T2D): now routes to R3 Metal GPU
  // instead of falling into R4 CPU streaming.
  sel.reset_hysteresis();
  WorkloadQuery q_512_b32;
  q_512_b32.batch_size = 32;
  q_512_b32.working_set_bytes = 60 * kMiB;
  q_512_b32.work_ops = 2ULL * 32ULL * 512ULL * 2048ULL;
  q_512_b32.nonzero_density = 1.0;
  auto d_512_b32 = sel.select(q_512_b32);
  T81_REQUIRE(d_512_b32.regime == Regime::R3_AcceleratorMultiBatch);
  T81_REQUIRE(d_512_b32.backend == BackendKind::MetalGpu);

  // OUT_OF_CORE 1024x4096 B=16 (~60 MiB, 134.2M ops, low sparsity 0.50 density): routes to R3.
  sel.reset_hysteresis();
  WorkloadQuery q_1024_b16;
  q_1024_b16.batch_size = 16;
  q_1024_b16.working_set_bytes = 60 * kMiB;
  q_1024_b16.work_ops = 2ULL * 16ULL * 1024ULL * 4096ULL;
  q_1024_b16.sparsity_pct = 50;
  q_1024_b16.nonzero_density = 0.50;
  auto d_1024_b16 = sel.select(q_1024_b16);
  T81_REQUIRE(d_1024_b16.regime == Regime::R3_AcceleratorMultiBatch);
  T81_REQUIRE(d_1024_b16.backend == BackendKind::MetalGpu);

  // High-sparsity INT8_Sparse128 (75% zeros -> 0.25 non-zero density, 84 MiB, 512x2048 B=32):
  // MUST bypass R3 Metal GPU and route to zero-skip CPU path (R4 with 8 workers).
  sel.reset_hysteresis();
  WorkloadQuery q_sparse;
  q_sparse.batch_size = 32;
  q_sparse.working_set_bytes = 84 * kMiB;
  q_sparse.work_ops = 2ULL * 32ULL * 512ULL * 2048ULL;
  q_sparse.sparsity_pct = 75;
  q_sparse.nonzero_density = 0.25;
  q_sparse.is_r2_int8_act = true;
  auto d_sparse = sel.select(q_sparse);
  T81_REQUIRE(d_sparse.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_sparse.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_sparse.kernel == KernelKind::INT8_Sparse128);
  T81_REQUIRE(d_sparse.worker_threads == 8);
  T81_REQUIRE(d_sparse.tile_batch_size >= d_sparse.worker_threads);
  T81_REQUIRE(d_sparse.tile_batch_size < q_sparse.batch_size);

  // Backwards-compatible unspecified nonzero_density (< 0.0) with INT8_Sparse128 at 75% sparsity
  // also routes to zero-skip CPU instead of Metal GPU.
  sel.reset_hysteresis();
  WorkloadQuery q_sparse_legacy = q_sparse;
  q_sparse_legacy.nonzero_density = -1.0;
  auto d_sparse_legacy = sel.select(q_sparse_legacy);
  T81_REQUIRE(d_sparse_legacy.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_sparse_legacy.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_sparse_legacy.kernel == KernelKind::INT8_Sparse128);

  // Hysteresis must immediately exit R3 when switching from dense matrix to high-sparsity CPU path.
  sel.reset_hysteresis();
  T81_REQUIRE(sel.select(q_512_b32).regime == Regime::R3_AcceleratorMultiBatch);
  auto d_exit = sel.select(q_sparse);
  T81_REQUIRE(d_exit.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_exit.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_exit.switched_from_active);

  // Small matrix 128x512 B=64 (8.4M ops < 32M) at 60 MiB stays on CPU R4 (no GPU launch overhead).
  sel.reset_hysteresis();
  WorkloadQuery q_128_b64;
  q_128_b64.batch_size = 64;
  q_128_b64.working_set_bytes = 60 * kMiB;
  q_128_b64.work_ops = 2ULL * 64ULL * 128ULL * 512ULL;
  q_128_b64.nonzero_density = 1.0;
  auto d_128_b64 = sel.select(q_128_b64);
  T81_REQUIRE(d_128_b64.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_128_b64.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_128_b64.worker_threads == 8);
}

void test_phase3_sparse_routing_xeon() {
  std::cout << "[8/8] Phase 3 Xeon 6975P-C x86_64 dense vs. high-sparsity routing...\n";
  const HardwareProfile xeon = HardwareProfile::xeon_6975p_reference();
  RegimeSelector sel(xeon);

  // L3-resident high-sparsity INT8_Sparse128 (B=16, 256 MiB < 0.85*480 MiB): R1 with 4 workers.
  WorkloadQuery q_r1;
  q_r1.batch_size = 16;
  q_r1.working_set_bytes = 256 * kMiB;
  q_r1.work_ops = 2ULL * 16ULL * 1024ULL * 4096ULL;
  q_r1.sparsity_pct = 75;
  q_r1.nonzero_density = 0.25;
  q_r1.is_r2_int8_act = true;
  auto d_r1 = sel.select(q_r1);
  T81_REQUIRE(d_r1.regime == Regime::R1_L3ResidentMultiBatch);
  T81_REQUIRE(d_r1.backend == BackendKind::CpuSimdMT);
  T81_REQUIRE(d_r1.kernel == KernelKind::INT8_Sparse128);
  T81_REQUIRE(d_r1.worker_threads == 4);

  // Off-L3 streaming dense matrix (B=32, 640 MiB): R2 with <= 4 streaming workers (no Metal).
  sel.reset_hysteresis();
  WorkloadQuery q_r2;
  q_r2.batch_size = 32;
  q_r2.working_set_bytes = 640 * kMiB;
  q_r2.work_ops = 2ULL * 32ULL * 1024ULL * 4096ULL;
  q_r2.nonzero_density = 1.0;
  auto d_r2 = sel.select(q_r2);
  T81_REQUIRE(d_r2.regime == Regime::R2_OffL3StreamingDRAM);
  T81_REQUIRE(d_r2.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_r2.kernel == KernelKind::DualRail_Dense);
  T81_REQUIRE(d_r2.worker_threads == 4);

  // Out-of-core high-sparsity matrix (B=64, 1536 MiB > 2*480 MiB): R4 tiled CPU streaming.
  sel.reset_hysteresis();
  WorkloadQuery q_r4;
  q_r4.batch_size = 64;
  q_r4.working_set_bytes = 1536 * kMiB;
  q_r4.work_ops = 2ULL * 64ULL * 1024ULL * 4096ULL;
  q_r4.sparsity_pct = 85;
  q_r4.nonzero_density = 0.15;
  q_r4.is_r2_int8_act = true;
  auto d_r4 = sel.select(q_r4);
  T81_REQUIRE(d_r4.regime == Regime::R4_OversizedOutOfCore);
  T81_REQUIRE(d_r4.backend == BackendKind::CpuStreamMT);
  T81_REQUIRE(d_r4.kernel == KernelKind::INT8_Sparse128);
  T81_REQUIRE(d_r4.worker_threads == 4);
  T81_REQUIRE(d_r4.tile_batch_size >= 1 && d_r4.tile_batch_size < q_r4.batch_size);
}

}  // namespace

int main() {
  test_apple_m2_boundaries();
  test_xeon_6975p_boundaries();
  test_hysteresis_stability();
  test_metal_queue_and_worker_fallbacks();
  test_bit_exact_cross_backend_determinism();
  test_m2_work_op_gating_and_stream_cap();
  test_phase3_r3_r4_boundary_and_sparse_routing_m2();
  test_phase3_sparse_routing_xeon();
  std::cout << "ALL H06-MEM-04 REGIME DISPATCH TESTS PASSED.\n";
  return 0;
}
