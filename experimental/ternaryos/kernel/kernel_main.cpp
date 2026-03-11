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

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
