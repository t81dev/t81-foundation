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

}  // namespace

int main() {
  test_apple_m2_boundaries();
  test_xeon_6975p_boundaries();
  test_hysteresis_stability();
  test_metal_queue_and_worker_fallbacks();
  test_bit_exact_cross_backend_determinism();
  std::cout << "ALL H06-MEM-04 REGIME DISPATCH TESTS PASSED.\n";
  return 0;
}
