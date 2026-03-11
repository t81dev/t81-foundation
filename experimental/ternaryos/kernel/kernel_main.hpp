#pragma once

#include "../hal/hal.hpp"
#include "../ipc/canon_message.hpp"
#include "../mmu/page_table.hpp"
#include "../mmu/ternary_page_alloc.hpp"
#include "../sched/scheduler.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace t81::ternaryos::kernel {

struct KernelFaultRecord {
  std::string platform_id;
  uint64_t tva{0};
  mmu::MmuAccessMode access_mode{mmu::MmuAccessMode::Read};
  mmu::MmuFault fault{mmu::MmuFault::None};
};

struct KernelRuntimeState {
  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t    total_ternary_pages{0};
  bool        has_writable_memory{false};
  mmu::TernaryPageAllocator allocator;
  mmu::PageTable page_table;
  sched::Scheduler scheduler;
  ipc::MessageBus ipc_bus;
  std::deque<KernelFaultRecord> fault_log;

  KernelRuntimeState(std::string platform_id_in,
                     std::size_t memory_region_count_in,
                     uint64_t total_ternary_pages_in,
                     bool has_writable_memory_in,
                     mmu::TernaryPageAllocator allocator_in) noexcept
      : platform_id(std::move(platform_id_in)),
        memory_region_count(memory_region_count_in),
        total_ternary_pages(total_ternary_pages_in),
        has_writable_memory(has_writable_memory_in),
        allocator(std::move(allocator_in)) {}

  static constexpr sched::Tid kKernelTid = 0;
  static constexpr std::size_t kMaxFaultLog = 27;

  std::size_t fault_count() const noexcept { return fault_log.size(); }
};

struct KernelAccessReport {
  std::optional<uint64_t> phys_addr{};
  std::optional<KernelFaultRecord> fault{};
};

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept;

KernelAccessReport axion_kernel_check_access(
    KernelRuntimeState& state,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept;

int axion_kernel_main(const hal::BootContext& ctx) noexcept;

}  // namespace t81::ternaryos::kernel
