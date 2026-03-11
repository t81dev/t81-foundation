#pragma once

#include "../hal/hal.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState {
  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t    total_ternary_pages{0};
  bool        has_writable_memory{false};
};

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept;

int axion_kernel_main(const hal::BootContext& ctx) noexcept;

}  // namespace t81::ternaryos::kernel
