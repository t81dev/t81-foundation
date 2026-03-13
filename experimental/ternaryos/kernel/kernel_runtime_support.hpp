#pragma once

#include "kernel_base.hpp"

#include "../hal/hal.hpp"
#include "../hal/virtualbox_platform.hpp"
#include "../mmu/page_table.hpp"
#include "../sched/scheduler.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::kernel {

struct KernelInterruptRecord {
  hal::InterruptSource source{hal::InterruptSource::Unknown};
  uint64_t timestamp_ns{0};
  uint64_t payload{0};
  uint64_t sequence{0};
  uint64_t recorded_audit_sequence{0};
  uint64_t delivered_audit_sequence{0};
};

struct KernelInterruptSourceCounters {
  uint64_t timer{0};
  uint64_t storage{0};
  uint64_t network{0};
  uint64_t keyboard{0};
  uint64_t unknown{0};
};

struct KernelPagerHandoffRecord {
  AddressSpaceId address_space_id{0};
  ProcessGroupId process_group_id{0};
  KernelFaultRecord fault{};
  uint64_t sequence{0};
};

struct KernelPagerResolutionRecord {
  AddressSpaceId address_space_id{0};
  ProcessGroupId process_group_id{0};
  KernelFaultRecord fault{};
  uint64_t sequence{0};
};

struct KernelPagerWorkItem {
  KernelPagerHandoffRecord handoff{};
  uint64_t ready_bypass_count{0};
  uint64_t parked_cycle_count{0};
  bool resumed_from_parked{false};
  bool follow_on_from_parked_resolution{false};
};

enum class KernelAuditEventKind : uint8_t {
  FaultDelivered = 0,
  InterruptRecorded,
  InterruptDelivered,
  ThreadQuarantined,
  ProcessGroupFaultEntered,
  SupervisorFaultNotified,
  ThreadFaultAcknowledged,
  ProcessGroupAcknowledged,
  SupervisorGroupAcknowledged,
  ThreadRecovered,
  ServiceRegistered,
  ServiceUnregistered,
  ServiceSuspended,
  ServiceResumed,
  ServiceMarkedUnhealthy,
  ServiceMarkedHealthy,
  CapabilityGranted,
  CapabilityRevoked,
};

struct KernelAuditRecord {
  KernelAuditEventKind kind{KernelAuditEventKind::FaultDelivered};
  sched::Tid subject_tid{0};
  ProcessGroupId process_group_id{0};
  mmu::MmuFault fault{mmu::MmuFault::None};
  uint64_t sequence{0};
};

struct KernelCapabilityTransitionRecord {
  ProcessGroupId process_group_id{0};
  CapabilityRecordId record_id{0};
  KernelAuditEventKind kind{KernelAuditEventKind::CapabilityGranted};
  uint64_t sequence{0};
};

struct KernelDeviceRecord {
  std::string name;
  hal::VBoxBusKind bus{hal::VBoxBusKind::Mmio};
  uint64_t base{0};
  uint64_t span_bytes{0};
  uint8_t irq{0};
  std::optional<sched::Tid> owner_tid{};
};

struct KernelDeviceArbitrationState {
  std::string profile_summary;
  std::vector<KernelDeviceRecord> devices;
  bool has_storage{false};
  bool has_network{false};
  bool has_display{false};
};

}  // namespace t81::ternaryos::kernel
