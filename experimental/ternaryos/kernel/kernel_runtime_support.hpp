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

/// RFC-00B5 §3.7 — verdict returned by the interrupt policy gate (Slice 27).
enum class InterruptPolicyVerdict : uint8_t {
  Allow,       ///< interrupt passes; dispatch normally
  Quarantine,  ///< rate limit exceeded; dispatch this interrupt but quarantine the source
  Deny,        ///< source is already quarantined; drop this interrupt
};

/// Per-source rate-limit configuration (Slice 27).
/// max_per_window == 0 means no rate limit for this source.
struct KernelInterruptRateConfig {
  uint32_t max_per_window{0};  ///< max deliveries allowed per window; 0 = unlimited
  uint32_t window_size{0};     ///< window length in loop_iterations; 0 = unlimited
};

/// Per-source policy runtime state maintained by the kernel (Slice 27).
struct KernelInterruptPolicySourceState {
  bool quarantined{false};          ///< source is quarantined; all further interrupts denied
  uint64_t recent_count{0};         ///< deliveries seen in the current rate-limit window
  uint64_t window_start_cycle{0};   ///< loop_iteration when the current window began
  KernelInterruptRateConfig config{}; ///< active rate-limit configuration
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
  EpochAbortedPolicyFault,  ///< a task's program was denied by the epoch policy gate (RFC-DPE-0003 §6.1)
  // RFC-DPE-0008: full epoch lifecycle audit trail
  EpochSubmitted,   ///< epoch accepted by accept_epoch() and dispatched for execution (RFC-DPE-0008 §3.1)
  EpochCommitted,   ///< epoch committed successfully via commit_epoch() (RFC-DPE-0008 §3.2)
  EpochAborted,     ///< epoch aborted (TaskFault, ExclusiveConflict, or Timeout) (RFC-DPE-0008 §3.3)
  // RFC-00B5 §3.7 — interrupt policy gate (Slice 27)
  InterruptPolicyAllow,      ///< interrupt passed the policy gate; dispatch proceeds normally
  InterruptPolicyQuarantine, ///< rate limit exceeded; source quarantined; this interrupt dropped
  InterruptPolicyDeny,       ///< source is quarantined; interrupt dropped without dispatch
  // RFC-00B5 §3.5 — unhandled interrupt governance (Slice 28)
  UnhandledInterruptDropped, ///< HAL dispatched an interrupt with no registered handler; dropped and audited
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
  bool kernel_seeded{false};
  std::optional<ProcessGroupId> delegated_by_process_group_id{};
  std::optional<SupervisorId> delegated_by_supervisor_id{};
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
