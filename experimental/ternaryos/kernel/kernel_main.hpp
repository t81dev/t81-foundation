#pragma once

#include "../hal/hal.hpp"
#include "../mmu/page_table.hpp"

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

struct KernelFaultRecord {
  std::string platform_id;
  uint64_t tva{0};
  mmu::MmuAccessMode access_mode{mmu::MmuAccessMode::Read};
  mmu::MmuFault fault{mmu::MmuFault::None};
};

struct KernelAccessReport {
  std::optional<uint64_t> phys_addr{};
  std::optional<KernelFaultRecord> fault{};
};

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept;

KernelAccessReport axion_kernel_check_access(
    const KernelRuntimeState& state,
    const mmu::PageTable& page_table,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept;

int axion_kernel_main(const hal::BootContext& ctx) noexcept;

}  // namespace t81::ternaryos::kernel
