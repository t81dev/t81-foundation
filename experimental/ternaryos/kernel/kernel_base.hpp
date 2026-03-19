#pragma once

#include "../mmu/page_table.hpp"
#include "../sched/scheduler.hpp"

#include <cstdint>
#include <string>

namespace t81::ternaryos::kernel {

using ProcessGroupId = uint32_t;
using SupervisorId = uint32_t;
using ServiceId = uint32_t;
using AddressSpaceId = uint32_t;
using CapabilityRecordId = uint64_t;

struct KernelFaultRecord {
  std::string platform_id;
  uint64_t tva{0};
  mmu::MmuAccessMode access_mode{mmu::MmuAccessMode::Read};
  mmu::MmuFault fault{mmu::MmuFault::None};
  sched::Tid subject_tid{0};
};

}  // namespace t81::ternaryos::kernel
