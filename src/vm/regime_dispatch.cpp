#include "vm/regime_dispatch.hpp"

#include <algorithm>
#include <thread>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace t81::vm {

HardwareProfile HardwareProfile::probe_host() noexcept {
  HardwareProfile p;

#if defined(__aarch64__) || defined(_M_ARM64)
  p.arch = ArchKind::Arm64;
  p.has_neon = true;
#elif defined(__x86_64__) || defined(_M_X64)
  p.arch = ArchKind::X86_64;
#if defined(__AVX2__)
  p.has_avx2 = true;
#endif
#else
  p.arch = ArchKind::Other;
#endif

  const unsigned hc = std::thread::hardware_concurrency();
  p.physical_cores = hc > 0 ? static_cast<std::uint32_t>(hc) : 1U;

#if defined(__APPLE__)
  std::uint32_t phys = 0;
  std::size_t phys_sz = sizeof(phys);
  if (sysctlbyname("hw.physicalcpu", &phys, &phys_sz, nullptr, 0) == 0 && phys > 0) {
    p.physical_cores = phys;
  }
  std::uint64_t l2 = 0;
  std::size_t l2_sz = sizeof(l2);
  if (sysctlbyname("hw.l2cachesize", &l2, &l2_sz, nullptr, 0) == 0 && l2 > 0) {
    p.l2_cache_bytes = l2;
  }
  std::uint64_t l3 = 0;
  std::size_t l3_sz = sizeof(l3);
  if (sysctlbyname("hw.l3cachesize", &l3, &l3_sz, nullptr, 0) == 0 && l3 > 0) {
    p.l3_cache_bytes = l3;
  }
  std::uint64_t mem = 0;
  std::size_t mem_sz = sizeof(mem);
  if (sysctlbyname("hw.memsize", &mem, &mem_sz, nullptr, 0) == 0 && mem > 0) {
    p.metal_vram_budget_bytes = mem / 2ULL;
  }
#if defined(__aarch64__)
  p.metal_available = true;
#endif
#else
  // Conservative portable defaults when OS cache sysfs is not queried at zero-alloc time.
  p.l2_cache_bytes = 4ULL * 1024ULL * 1024ULL;
  p.l3_cache_bytes = 32ULL * 1024ULL * 1024ULL;
  p.metal_available = false;
  p.metal_vram_budget_bytes = 0;
#endif

  return p;
}

RegimeSelector::RegimeSelector(HardwareProfile hardware, DispatchPolicy policy) noexcept
    : hardware_(hardware), policy_(policy) {}

KernelKind RegimeSelector::select_kernel(Regime regime, std::uint32_t sparsity_pct,
                                         bool is_r2_int8_act) noexcept {
  if (is_r2_int8_act) {
    return KernelKind::INT8_Sparse128;
  }
  if (regime == Regime::R3_AcceleratorMultiBatch) {
    return sparsity_pct >= 75 ? KernelKind::SBDR1T2D : KernelKind::DualRail_Dense;
  }
  if (sparsity_pct >= 50) {
    return KernelKind::SBDR128_SIMD;
  }
  return KernelKind::DualRail_Dense;
}

std::uint32_t RegimeSelector::compute_worker_count(Regime regime,
                                                   std::uint32_t batch_size) const noexcept {
  const std::uint32_t cores = std::max<std::uint32_t>(1U, hardware_.physical_cores);
  const std::uint32_t b = std::max<std::uint32_t>(1U, batch_size);
  switch (regime) {
    case Regime::R0_CacheResidentInline:
    case Regime::R3_AcceleratorMultiBatch:
      return 1U;
    case Regime::R1_L3ResidentMultiBatch:
      return std::min<std::uint32_t>(cores, b);
    case Regime::R2_OffL3StreamingDRAM:
    case Regime::R4_OversizedOutOfCore: {
      const bool apple_unified = (hardware_.arch == ArchKind::Arm64 && hardware_.metal_available);
      const std::uint32_t auto_cap = apple_unified ? cores : std::min<std::uint32_t>(cores, 4U);
      const std::uint32_t cap =
          policy_.stream_worker_cap > 0 ? policy_.stream_worker_cap : auto_cap;
      return std::min<std::uint32_t>(std::max<std::uint32_t>(1U, cap), b);
    }
  }
  return 1U;
}

std::uint32_t RegimeSelector::compute_tile_batch(const WorkloadQuery& query) const noexcept {
  const std::uint32_t b = std::max<std::uint32_t>(1U, query.batch_size);
  const std::uint64_t llc = hardware_.effective_llc_bytes();
  if (llc == 0 || query.working_set_bytes == 0) {
    return b;
  }
  const std::uint64_t max_slice =
      static_cast<std::uint64_t>(static_cast<double>(llc) * policy_.out_of_core_tile_ratio);
  if (query.working_set_bytes <= max_slice || max_slice == 0) {
    return b;
  }
  const std::uint64_t per_item = std::max<std::uint64_t>(1ULL, query.working_set_bytes / b);
  const std::uint64_t tile = max_slice / per_item;
  return static_cast<std::uint32_t>(std::clamp<std::uint64_t>(tile, 1ULL, b));
}

double RegimeSelector::estimate_throughput_score(Regime regime,
                                                 const WorkloadQuery& query) const noexcept {
  const double b = static_cast<double>(std::max<std::uint32_t>(1U, query.batch_size));
  const double cores = static_cast<double>(std::max<std::uint32_t>(1U, hardware_.physical_cores));
  const double llc =
      static_cast<double>(std::max<std::uint64_t>(1ULL, hardware_.effective_llc_bytes()));
  const double ws = static_cast<double>(query.working_set_bytes);

  switch (regime) {
    case Regime::R0_CacheResidentInline:
      // Low launch latency; scales linearly with batch up to r0_max_batch, then saturates 1 core.
      return (ws <= llc * policy_.l3_resident_ratio) ? std::min(b, 4.0) * 1.10 : 1.0;
    case Regime::R1_L3ResidentMultiBatch: {
      // Thread-pool wakeup overhead hurts tiny batches; shines for medium batches inside LLC.
      const double wakeup_penalty = (b <= static_cast<double>(policy_.r0_max_batch)) ? 0.55 : 1.0;
      const double parallel = std::min(b, cores) * 0.85;
      const double cache_factor = (ws <= llc * policy_.l3_resident_ratio) ? 1.0 : 0.70;
      return parallel * wakeup_penalty * cache_factor;
    }
    case Regime::R2_OffL3StreamingDRAM: {
      const double stream_w = static_cast<double>(
          compute_worker_count(Regime::R2_OffL3StreamingDRAM, query.batch_size));
      return (ws > llc * policy_.l3_resident_ratio) ? stream_w * 1.25 : stream_w * 0.75;
    }
    case Regime::R3_AcceleratorMultiBatch: {
      if (!hardware_.metal_available || query.metal_queue_depth > policy_.metal_max_queue_depth) {
        return 0.0;
      }
      const bool work_ok = (policy_.metal_min_work_ops == 0 || query.work_ops == 0 ||
                            query.work_ops >= policy_.metal_min_work_ops);
      if (query.batch_size < policy_.metal_min_batch ||
          query.working_set_bytes < policy_.metal_min_working_set_bytes || !work_ok) {
        return 0.50;
      }
      return std::min(b, 64.0) * 0.65 + 4.0;
    }
    case Regime::R4_OversizedOutOfCore:
      return (ws > llc * 2.0)
                 ? static_cast<double>(compute_worker_count(regime, query.batch_size)) * 1.45
                 : 0.60;
  }
  return 1.0;
}

DispatchDecision RegimeSelector::evaluate_candidate(const WorkloadQuery& query) const noexcept {
  DispatchDecision d;
  const std::uint32_t b = std::max<std::uint32_t>(1U, query.batch_size);
  const std::uint64_t l2 = hardware_.l2_cache_bytes;
  const std::uint64_t llc = hardware_.effective_llc_bytes();
  const std::uint64_t l2_thresh =
      static_cast<std::uint64_t>(static_cast<double>(l2) * policy_.l2_resident_ratio);
  const std::uint64_t llc_thresh =
      static_cast<std::uint64_t>(static_cast<double>(llc) * policy_.l3_resident_ratio);
  const std::uint64_t out_of_core_thresh = llc * 2ULL;

  // Deterministic worker-pool failure fallback -> always R0 inline single-thread.
  if (!query.worker_pool_healthy) {
    d.regime = Regime::R0_CacheResidentInline;
    d.backend = BackendKind::CpuSimd1T;
    d.worker_threads = 1U;
    d.tile_batch_size = b;
    d.fallback_triggered = true;
    d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
    return d;
  }

  // Check accelerator eligibility (R3) first when batch, working set & work_ops meet Metal
  // thresholds.
  const bool work_ok = (policy_.metal_min_work_ops == 0 || query.work_ops == 0 ||
                        query.work_ops >= policy_.metal_min_work_ops);
  const bool metal_eligible = hardware_.metal_available && b >= policy_.metal_min_batch &&
                              query.working_set_bytes >= policy_.metal_min_working_set_bytes &&
                              work_ok &&
                              (hardware_.metal_vram_budget_bytes == 0 ||
                               query.working_set_bytes <= hardware_.metal_vram_budget_bytes);

  if (metal_eligible) {
    if (query.metal_queue_depth <= policy_.metal_max_queue_depth) {
      d.regime = Regime::R3_AcceleratorMultiBatch;
      d.backend = BackendKind::MetalGpu;
      d.worker_threads = 1U;
      d.tile_batch_size = b;
      d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
      return d;
    }
    // Queue depth exceeded -> deterministic fallback to cpu_simd_mt.
    d.regime = Regime::R1_L3ResidentMultiBatch;
    d.backend = BackendKind::CpuSimdMT;
    d.worker_threads = compute_worker_count(d.regime, b);
    d.tile_batch_size = b;
    d.fallback_triggered = true;
    d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
    return d;
  }

  // R4: Oversized out-of-core (> 2x LLC or > r1_max_batch and > LLC threshold)
  if (llc > 0 && (query.working_set_bytes > out_of_core_thresh ||
                  (b > policy_.r1_max_batch && query.working_set_bytes > llc_thresh))) {
    d.regime = Regime::R4_OversizedOutOfCore;
    d.backend = BackendKind::CpuStreamMT;
    d.worker_threads = compute_worker_count(d.regime, b);
    d.tile_batch_size = compute_tile_batch(query);
    d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
    return d;
  }

  // R2: Off-L3 streaming DRAM (> 0.85 * LLC)
  if (llc > 0 && query.working_set_bytes > llc_thresh) {
    d.regime = Regime::R2_OffL3StreamingDRAM;
    d.backend = BackendKind::CpuStreamMT;
    d.worker_threads = compute_worker_count(d.regime, b);
    d.tile_batch_size = b;
    d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
    return d;
  }

  // R0: Cache-resident inline (small batch <= r0_max_batch and fits within L2/LLC)
  if (b <= policy_.r0_max_batch &&
      (l2 == 0 || query.working_set_bytes <= std::max(l2_thresh, llc_thresh))) {
    d.regime = Regime::R0_CacheResidentInline;
    d.backend = BackendKind::CpuSimd1T;
    d.worker_threads = 1U;
    d.tile_batch_size = b;
    d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
    return d;
  }

  // R1: L3-resident multi-batch
  d.regime = Regime::R1_L3ResidentMultiBatch;
  d.backend = BackendKind::CpuSimdMT;
  d.worker_threads = compute_worker_count(d.regime, b);
  d.tile_batch_size = b;
  d.kernel = select_kernel(d.regime, query.sparsity_pct, query.is_r2_int8_act);
  return d;
}

DispatchDecision RegimeSelector::select(const WorkloadQuery& query) noexcept {
  DispatchDecision candidate = evaluate_candidate(query);

  if (!active_regime_.has_value()) {
    active_regime_ = candidate.regime;
    candidate.switched_from_active = false;
    candidate.estimated_gain_ratio = 1.0;
    return candidate;
  }

  const Regime prev = *active_regime_;
  if (candidate.regime == prev) {
    candidate.switched_from_active = false;
    candidate.estimated_gain_ratio = 1.0;
    return candidate;
  }

  // Mandatory safety transitions (fallback or out-of-core tiling) bypass hysteresis.
  if (candidate.fallback_triggered || candidate.regime == Regime::R4_OversizedOutOfCore) {
    active_regime_ = candidate.regime;
    candidate.switched_from_active = true;
    candidate.estimated_gain_ratio = policy_.hysteresis_gain_ratio;
    return candidate;
  }

  const double prev_score = std::max(0.01, estimate_throughput_score(prev, query));
  const double cand_score = estimate_throughput_score(candidate.regime, query);
  const double gain = cand_score / prev_score;

  if (gain >= policy_.hysteresis_gain_ratio) {
    active_regime_ = candidate.regime;
    candidate.switched_from_active = true;
    candidate.estimated_gain_ratio = gain;
    return candidate;
  }

  // Hold active regime to prevent boundary thrashing.
  DispatchDecision held;
  held.regime = prev;
  held.backend =
      (prev == Regime::R0_CacheResidentInline)
          ? BackendKind::CpuSimd1T
          : (prev == Regime::R1_L3ResidentMultiBatch
                 ? BackendKind::CpuSimdMT
                 : (prev == Regime::R3_AcceleratorMultiBatch ? BackendKind::MetalGpu
                                                             : BackendKind::CpuStreamMT));
  held.worker_threads = compute_worker_count(prev, query.batch_size);
  held.tile_batch_size =
      (prev == Regime::R4_OversizedOutOfCore) ? compute_tile_batch(query) : query.batch_size;
  held.kernel = select_kernel(prev, query.sparsity_pct, query.is_r2_int8_act);
  held.estimated_gain_ratio = gain;
  held.switched_from_active = false;
  held.fallback_triggered = false;
  return held;
}

}  // namespace t81::vm
