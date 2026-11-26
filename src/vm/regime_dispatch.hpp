#pragma once

// src/vm/regime_dispatch.hpp
//
// H06-MEM-04 deterministic multi-batch regime dispatch selector.
// Staged under src/vm/ to preserve docs/governance/PUBLIC_API_LOCK.json (v2.2.0)
// and frozen TISC v1.9.0 ISA contracts until promoted in a SemVer minor release.

#include <cstdint>
#include <optional>
#include <string_view>

namespace t81::vm {

enum class Regime : std::uint8_t {
  R0_CacheResidentInline = 0,   // cpu_simd_1t — inline NEON/AVX2, zero thread-pool wakeup
  R1_L3ResidentMultiBatch = 1,  // cpu_simd_mt — physical-core capped batch partitioning
  R2_OffL3StreamingDRAM = 2,    // cpu_stream_mt — bandwidth-capped streaming workers (>0.85*L3)
  R3_AcceleratorMultiBatch =
      3,                      // metal_gpu on Apple Silicon (queue_depth <= 2; fallback cpu_simd_mt)
  R4_OversizedOutOfCore = 4,  // cpu_stream_mt with B-tiling keeping active slice <= 0.80*L3
};

enum class BackendKind : std::uint8_t {
  CpuSimd1T = 0,
  CpuSimdMT = 1,
  CpuStreamMT = 2,
  MetalGpu = 3,
};

enum class KernelKind : std::uint8_t {
  DualRail_Dense = 0,
  SBDR128_SIMD = 1,
  INT8_Sparse128 = 2,
  SBDR1T2D = 3,
};

enum class ArchKind : std::uint8_t {
  Arm64 = 0,
  X86_64 = 1,
  Other = 2,
};

struct HardwareProfile {
  ArchKind arch{ArchKind::Other};
  bool has_neon{false};
  bool has_avx2{false};
  std::uint32_t physical_cores{1};
  std::uint64_t l2_cache_bytes{4ULL * 1024ULL * 1024ULL};
  std::uint64_t l3_cache_bytes{0};
  bool metal_available{false};
  std::uint64_t metal_vram_budget_bytes{0};

  // Detect host hardware topology deterministically (no heap allocation).
  [[nodiscard]] static HardwareProfile probe_host() noexcept;

  // Deterministic reference profiles for cross-platform testing.
  [[nodiscard]] static constexpr HardwareProfile apple_m2_reference() noexcept {
    HardwareProfile p;
    p.arch = ArchKind::Arm64;
    p.has_neon = true;
    p.has_avx2 = false;
    p.physical_cores = 8;
    p.l2_cache_bytes = 16ULL * 1024ULL * 1024ULL;
    p.l3_cache_bytes = 0;
    p.metal_available = true;
    p.metal_vram_budget_bytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
    return p;
  }

  [[nodiscard]] static constexpr HardwareProfile xeon_6975p_reference() noexcept {
    HardwareProfile p;
    p.arch = ArchKind::X86_64;
    p.has_neon = false;
    p.has_avx2 = true;
    p.physical_cores = 4;
    p.l2_cache_bytes = 4ULL * 1024ULL * 1024ULL;
    p.l3_cache_bytes = 480ULL * 1024ULL * 1024ULL;
    p.metal_available = false;
    p.metal_vram_budget_bytes = 0;
    return p;
  }

  // Effective last-level cache capacity (L3 if present, else L2).
  [[nodiscard]] constexpr std::uint64_t effective_llc_bytes() const noexcept {
    return l3_cache_bytes > 0 ? l3_cache_bytes : l2_cache_bytes;
  }
};

struct DispatchPolicy {
  std::uint32_t r0_max_batch{4};
  std::uint32_t r1_max_batch{32};
  double l2_resident_ratio{0.75};
  double l3_resident_ratio{0.85};
  double out_of_core_tile_ratio{0.80};
  std::uint32_t metal_min_batch{16};
  std::uint64_t metal_min_working_set_bytes{64ULL * 1024ULL * 1024ULL};
  std::uint64_t metal_min_work_ops{32ULL * 1024ULL *
                                   1024ULL};  // 2*B*M*K gate to amortize GPU launch
  std::uint32_t metal_max_queue_depth{2};
  double hysteresis_gain_ratio{1.15};
  std::uint32_t stream_worker_cap{
      0};  // 0 = auto (physical_cores on Apple unified mem, else min(cores, 4))
};

struct WorkloadQuery {
  std::uint32_t batch_size{1};
  std::uint64_t working_set_bytes{0};
  std::uint64_t work_ops{0};      // 0 = unspecified; otherwise total arithmetic ops (2*B*M*K)
  std::uint32_t sparsity_pct{0};  // 0..100
  std::uint32_t metal_queue_depth{0};
  bool worker_pool_healthy{true};
  bool is_r2_int8_act{false};
};

struct DispatchDecision {
  Regime regime{Regime::R0_CacheResidentInline};
  BackendKind backend{BackendKind::CpuSimd1T};
  KernelKind kernel{KernelKind::DualRail_Dense};
  std::uint32_t worker_threads{1};
  std::uint32_t tile_batch_size{1};
  double estimated_gain_ratio{1.0};
  bool switched_from_active{false};
  bool fallback_triggered{false};
};

// Zero-allocation deterministic regime selector with hysteresis & fallback guards.
class RegimeSelector {
public:
  explicit RegimeSelector(HardwareProfile hardware, DispatchPolicy policy = {}) noexcept;

  // Select optimal regime/backend/kernel for `query`, applying hysteresis against
  // the currently active regime across consecutive calls.
  [[nodiscard]] DispatchDecision select(const WorkloadQuery& query) noexcept;

  // Pure kernel routing hint for a given regime and sparsity profile.
  [[nodiscard]] static KernelKind select_kernel(Regime regime, std::uint32_t sparsity_pct,
                                                bool is_r2_int8_act) noexcept;

  void reset_hysteresis() noexcept { active_regime_.reset(); }
  [[nodiscard]] std::optional<Regime> active_regime() const noexcept { return active_regime_; }
  [[nodiscard]] const HardwareProfile& hardware() const noexcept { return hardware_; }
  [[nodiscard]] const DispatchPolicy& policy() const noexcept { return policy_; }

private:
  [[nodiscard]] DispatchDecision evaluate_candidate(const WorkloadQuery& query) const noexcept;
  [[nodiscard]] double estimate_throughput_score(Regime regime,
                                                 const WorkloadQuery& query) const noexcept;
  [[nodiscard]] std::uint32_t compute_worker_count(Regime regime,
                                                   std::uint32_t batch_size) const noexcept;
  [[nodiscard]] std::uint32_t compute_tile_batch(const WorkloadQuery& query) const noexcept;

  HardwareProfile hardware_;
  DispatchPolicy policy_;
  std::optional<Regime> active_regime_;
};

[[nodiscard]] constexpr std::string_view regime_name(Regime r) noexcept {
  switch (r) {
    case Regime::R0_CacheResidentInline:
      return "R0_CacheResidentInline";
    case Regime::R1_L3ResidentMultiBatch:
      return "R1_L3ResidentMultiBatch";
    case Regime::R2_OffL3StreamingDRAM:
      return "R2_OffL3StreamingDRAM";
    case Regime::R3_AcceleratorMultiBatch:
      return "R3_AcceleratorMultiBatch";
    case Regime::R4_OversizedOutOfCore:
      return "R4_OversizedOutOfCore";
  }
  return "R0_CacheResidentInline";
}

[[nodiscard]] constexpr std::string_view backend_name(BackendKind b) noexcept {
  switch (b) {
    case BackendKind::CpuSimd1T:
      return "cpu_simd_1t";
    case BackendKind::CpuSimdMT:
      return "cpu_simd_mt";
    case BackendKind::CpuStreamMT:
      return "cpu_stream_mt";
    case BackendKind::MetalGpu:
      return "metal_gpu";
  }
  return "cpu_simd_1t";
}

[[nodiscard]] constexpr std::string_view kernel_name(KernelKind k) noexcept {
  switch (k) {
    case KernelKind::DualRail_Dense:
      return "DualRail_Dense";
    case KernelKind::SBDR128_SIMD:
      return "SBDR128_SIMD";
    case KernelKind::INT8_Sparse128:
      return "INT8_Sparse128";
    case KernelKind::SBDR1T2D:
      return "SBDR1T2D";
  }
  return "DualRail_Dense";
}

[[nodiscard]] constexpr std::string_view arch_name(ArchKind a) noexcept {
  switch (a) {
    case ArchKind::Arm64:
      return "arm64";
    case ArchKind::X86_64:
      return "x86_64";
    case ArchKind::Other:
      return "other";
  }
  return "other";
}

}  // namespace t81::vm
