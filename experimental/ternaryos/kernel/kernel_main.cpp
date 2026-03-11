#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept {
  if (ctx.memory_map.empty()) {
    return std::nullopt;
  }

  KernelRuntimeState state;
  state.platform_id = ctx.platform_id;
  state.memory_region_count = ctx.memory_map.size();

  for (const auto& region : ctx.memory_map) {
    state.total_ternary_pages += region.ternary_page_count();
    state.has_writable_memory = state.has_writable_memory || region.writable;
  }

  if (!state.has_writable_memory) {
    return std::nullopt;
  }

  return state;
}

KernelAccessReport axion_kernel_check_access(
    const KernelRuntimeState& state,
    const mmu::PageTable& page_table,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept {
  const auto result = mmu::mmu_translate_checked(page_table, tva, mode);
  if (result.fault == mmu::MmuFault::None) {
    return {.phys_addr = result.phys_addr, .fault = std::nullopt};
  }

  return {
      .phys_addr = std::nullopt,
      .fault = KernelFaultRecord{
          .platform_id = state.platform_id,
          .tva = tva,
          .access_mode = mode,
          .fault = result.fault,
      },
  };
}

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
