#pragma once

#include "../hal/hal.hpp"
#include "../hal/virtualbox_platform.hpp"
#include "../ipc/canon_message.hpp"
#include "../mmu/page_table.hpp"
#include "../mmu/ternary_page_alloc.hpp"
#include "../sched/scheduler.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace t81::ternaryos::kernel {

using ProcessGroupId = uint32_t;
using SupervisorId = uint32_t;
using ServiceId = uint32_t;

struct KernelFaultRecord {
  std::string platform_id;
  uint64_t tva{0};
  mmu::MmuAccessMode access_mode{mmu::MmuAccessMode::Read};
  mmu::MmuFault fault{mmu::MmuFault::None};
  sched::Tid subject_tid{0};
};

enum class KernelAuditEventKind : uint8_t {
  FaultDelivered = 0,
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
};

struct KernelAuditRecord {
  KernelAuditEventKind kind{KernelAuditEventKind::FaultDelivered};
  sched::Tid subject_tid{0};
  ProcessGroupId process_group_id{0};
  mmu::MmuFault fault{mmu::MmuFault::None};
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

struct KernelRuntimeState {
  struct ThreadRuntimeState {
    sched::Tid tid{0};
    ProcessGroupId process_group_id{0};
    bool quarantined{false};
    std::deque<KernelFaultRecord> fault_inbox;
  };

  struct ProcessGroupAuditCounters {
    uint64_t audit_events{0};
    uint64_t fault_entries{0};
    uint64_t acknowledgements{0};
    uint64_t recoveries{0};
  };

  struct ProcessGroupState {
    ProcessGroupId id{0};
    std::vector<sched::Tid> member_tids;
    bool faulted{false};
    bool blocked{false};
    bool acknowledgement_pending{false};
    std::size_t pending_fault_count{0};
    ProcessGroupAuditCounters counters{};
  };

  struct SupervisorState {
    SupervisorId id{0};
    std::vector<ProcessGroupId> managed_groups;
    std::vector<ServiceId> managed_services;
    std::deque<ProcessGroupId> pending_groups;
    uint64_t fault_notifications{0};
    uint64_t acknowledgements{0};
    uint64_t recovered_groups{0};
    uint64_t service_lifecycle_transitions{0};
    std::optional<ProcessGroupId> last_acknowledged_group{};
    std::optional<ProcessGroupId> last_recovered_group{};
    std::optional<ServiceId> last_service_transition_id{};
    std::optional<KernelAuditEventKind> last_service_transition_kind{};
    std::optional<uint64_t> last_service_transition_sequence{};
  };

  struct ServiceState {
    ServiceId id{0};
    std::string name;
    SupervisorId supervisor_id{0};
    ProcessGroupId process_group_id{0};
    bool blocked{false};
    bool suspended{false};
    bool unhealthy{false};
    bool registered{false};
    uint64_t requests{0};
    uint64_t rejected_requests{0};
    uint64_t state_transitions{0};
    std::optional<KernelAuditEventKind> last_transition_kind{};
    std::optional<uint64_t> last_transition_sequence{};
  };

  struct Counters {
    uint64_t loop_iterations{0};
    uint64_t scheduler_ticks{0};
    uint64_t scheduler_switches{0};
    uint64_t ipc_messages_sent{0};
    uint64_t ipc_messages_received{0};
    uint64_t faults_recorded{0};
    uint64_t faults_delivered{0};
    uint64_t faults_routed_to_threads{0};
    uint64_t thread_quarantines{0};
    uint64_t thread_fault_acknowledgements{0};
    uint64_t thread_fault_recoveries{0};
    uint64_t process_group_fault_entries{0};
    uint64_t process_group_acknowledgements{0};
    uint64_t process_group_recoveries{0};
    uint64_t supervisor_fault_notifications{0};
    uint64_t supervisor_acknowledgements{0};
    uint64_t audit_events_recorded{0};
  };

  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t    total_ternary_pages{0};
  bool        has_writable_memory{false};
  mmu::TernaryPageAllocator allocator;
  mmu::PageTable page_table;
  sched::Scheduler scheduler;
  ipc::MessageBus ipc_bus;
  std::optional<KernelDeviceArbitrationState> device_arbitration;
  std::deque<KernelFaultRecord> fault_log;
  std::deque<KernelFaultRecord> pending_faults;
  std::deque<KernelAuditRecord> audit_log;
  std::unordered_map<sched::Tid, ThreadRuntimeState> thread_runtime;
  std::unordered_map<ProcessGroupId, ProcessGroupState> process_groups;
  std::unordered_map<SupervisorId, SupervisorState> supervisors;
  std::unordered_map<ProcessGroupId, SupervisorId> process_group_supervisors;
  std::unordered_map<ServiceId, ServiceState> services;
  std::unordered_map<ProcessGroupId, ServiceId> process_group_services;
  t81::vm::ThreadContext cpu_context{};
  Counters counters{};
  std::optional<KernelFaultRecord> last_delivered_fault{};
  std::optional<KernelAuditRecord> last_audit_event{};
  ProcessGroupId next_process_group_id{1};
  SupervisorId next_supervisor_id{1};
  ServiceId next_service_id{1};
  uint64_t next_audit_sequence{1};

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
  static constexpr ProcessGroupId kKernelProcessGroup = 0;
  static constexpr SupervisorId kKernelSupervisor = 0;
  static constexpr std::size_t kMaxFaultLog = 27;
  static constexpr std::size_t kMaxAuditLog = 81;

  std::size_t fault_count() const noexcept { return fault_log.size(); }
  std::size_t pending_fault_count() const noexcept { return pending_faults.size(); }
  std::size_t audit_count() const noexcept { return audit_log.size(); }
  std::size_t process_group_count() const noexcept { return process_groups.size(); }
  std::size_t supervisor_count() const noexcept { return supervisors.size(); }
  std::size_t service_count() const noexcept { return services.size(); }
  bool has_device_arbitration() const noexcept { return device_arbitration.has_value(); }

  const ThreadRuntimeState* find_thread_runtime(sched::Tid tid) const noexcept {
    auto it = thread_runtime.find(tid);
    return it == thread_runtime.end() ? nullptr : &it->second;
  }

  ThreadRuntimeState* find_thread_runtime_mut(sched::Tid tid) noexcept {
    auto it = thread_runtime.find(tid);
    return it == thread_runtime.end() ? nullptr : &it->second;
  }

  const ProcessGroupState* find_process_group(ProcessGroupId id) const noexcept {
    auto it = process_groups.find(id);
    return it == process_groups.end() ? nullptr : &it->second;
  }

  ProcessGroupState* find_process_group_mut(ProcessGroupId id) noexcept {
    auto it = process_groups.find(id);
    return it == process_groups.end() ? nullptr : &it->second;
  }

  const SupervisorState* find_supervisor(SupervisorId id) const noexcept {
    auto it = supervisors.find(id);
    return it == supervisors.end() ? nullptr : &it->second;
  }

  SupervisorState* find_supervisor_mut(SupervisorId id) noexcept {
    auto it = supervisors.find(id);
    return it == supervisors.end() ? nullptr : &it->second;
  }

  const ServiceState* find_service(ServiceId id) const noexcept {
    auto it = services.find(id);
    return it == services.end() ? nullptr : &it->second;
  }

  ServiceState* find_service_mut(ServiceId id) noexcept {
    auto it = services.find(id);
    return it == services.end() ? nullptr : &it->second;
  }

  std::optional<SupervisorId> find_process_group_supervisor(
      ProcessGroupId process_group_id) const noexcept {
    auto it = process_group_supervisors.find(process_group_id);
    if (it == process_group_supervisors.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  std::optional<ServiceId> find_process_group_service(
      ProcessGroupId process_group_id) const noexcept {
    auto it = process_group_services.find(process_group_id);
    if (it == process_group_services.end()) {
      return std::nullopt;
    }
    return it->second;
  }
};

struct KernelAccessReport {
  std::optional<uint64_t> phys_addr{};
  std::optional<KernelFaultRecord> fault{};
};

enum class KernelServiceRequestKind : uint8_t {
  RuntimeStatus = 0,
  ProcessGroupStatus,
  SupervisorStatus,
  SupervisorRecoveryStatus,
  ServiceStatus,
  SupervisorServiceInventory,
  FaultSummary,
  AuditSummary,
  DeviceSummary,
};

enum class KernelServiceStatus : uint8_t {
  Ok = 0,
  InvalidRequest,
  NotFound,
  FaultedGroup,
  ServiceUnavailable,
  NoDeviceArbitration,
};

enum class KernelServiceRequestRejection : uint8_t {
  None = 0,
  MissingRequestingGroup,
  MissingProcessGroup,
  MissingSupervisor,
  MissingService,
  FaultedRequestingGroup,
  UnhealthyService,
  MissingDeviceArbitration,
};

enum class KernelServiceActionKind : uint8_t {
  AcknowledgeSupervisorFaultGroup = 0,
  ClaimDevice,
  ReleaseDevice,
  RegisterService,
  UnregisterService,
  SuspendService,
  ResumeService,
  MarkServiceUnhealthy,
  MarkServiceHealthy,
};

enum class KernelServiceActionRejection : uint8_t {
  None = 0,
  MissingRequestingGroup,
  MissingProcessGroup,
  MissingSupervisor,
  MissingService,
  DuplicateService,
  ServiceSupervisorMismatch,
  ServiceProcessGroupMismatch,
  ServiceAlreadySuspended,
  ServiceNotSuspended,
  ServiceAlreadyUnhealthy,
  ServiceAlreadyHealthy,
  MissingDeviceName,
  MissingServiceName,
  MissingDeviceArbitration,
  FaultedRequestingGroup,
  NoPrimaryThread,
  DeviceConflict,
  DeviceNotOwned,
  SupervisorGatePendingThreadFault,
  SupervisorGroupNotPending,
};

struct KernelRuntimeStatusView {
  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t total_ternary_pages{0};
  uint64_t loop_iterations{0};
  uint64_t scheduler_ticks{0};
  uint64_t ipc_messages_sent{0};
  uint64_t ipc_messages_received{0};
  std::size_t managed_service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
};

struct KernelProcessGroupStatusView {
  ProcessGroupId id{0};
  std::size_t member_count{0};
  std::size_t quarantined_thread_count{0};
  bool faulted{false};
  bool blocked{false};
  bool acknowledgement_pending{false};
  std::size_t pending_fault_count{0};
  uint64_t audit_events{0};
  uint64_t fault_entries{0};
  uint64_t acknowledgements{0};
  uint64_t recoveries{0};
  std::optional<SupervisorId> supervisor_id{};
};

struct KernelSupervisorStatusView {
  SupervisorId id{0};
  std::size_t managed_group_count{0};
  std::size_t managed_faulted_group_count{0};
  std::size_t managed_service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  std::size_t pending_group_count{0};
  uint64_t fault_notifications{0};
  uint64_t acknowledgements{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<ProcessGroupId> last_pending_group{};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
};

struct KernelSupervisorRecoveryStatusView {
  SupervisorId id{0};
  std::size_t pending_group_count{0};
  std::size_t managed_service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t acknowledgements{0};
  uint64_t recovered_groups{0};
  uint64_t service_lifecycle_transitions{0};
  std::vector<ProcessGroupId> pending_group_ids;
  std::optional<ProcessGroupId> last_acknowledged_group{};
  std::optional<ProcessGroupId> last_recovered_group{};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
};

struct KernelServiceStatusView {
  ServiceId id{0};
  std::string name;
  SupervisorId supervisor_id{0};
  ProcessGroupId process_group_id{0};
  std::optional<sched::Tid> primary_tid{};
  bool blocked{false};
  bool suspended{false};
  bool unhealthy{false};
  bool registered{false};
  bool faulted_group{false};
  std::size_t quarantined_thread_count{0};
  std::size_t pending_fault_count{0};
  uint64_t requests{0};
  uint64_t rejected_requests{0};
  uint64_t state_transitions{0};
  std::optional<KernelAuditEventKind> last_transition_kind{};
  std::optional<uint64_t> last_transition_sequence{};
};

struct KernelSupervisorServiceEntryView {
  ServiceId id{0};
  std::string name;
  ProcessGroupId process_group_id{0};
  bool blocked{false};
  bool suspended{false};
  bool unhealthy{false};
  bool registered{false};
  uint64_t requests{0};
  uint64_t rejected_requests{0};
};

struct KernelSupervisorServiceInventoryView {
  SupervisorId supervisor_id{0};
  std::size_t service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t service_lifecycle_transitions{0};
  uint64_t total_service_requests{0};
  uint64_t total_service_rejections{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
  std::vector<ServiceId> service_ids;
  std::vector<KernelSupervisorServiceEntryView> services;
};

struct KernelFaultSummaryView {
  std::size_t recorded_faults{0};
  std::size_t pending_faults{0};
  std::size_t delivered_faults{0};
  std::size_t routed_thread_faults{0};
  std::size_t quarantined_threads{0};
  std::size_t audit_events{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<KernelFaultRecord> last_delivered_fault{};
  std::optional<KernelAuditRecord> last_audit_event{};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
};

struct KernelAuditSummaryView {
  std::size_t audit_events{0};
  uint64_t fault_deliveries{0};
  uint64_t thread_quarantines{0};
  uint64_t process_group_fault_entries{0};
  uint64_t supervisor_notifications{0};
  uint64_t thread_acknowledgements{0};
  uint64_t process_group_acknowledgements{0};
  uint64_t supervisor_acknowledgements{0};
  uint64_t thread_recoveries{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
  std::vector<KernelAuditRecord> recent_events;
};

struct KernelDeviceOwnershipView {
  std::string name;
  bool claimed{false};
  std::optional<sched::Tid> owner_tid{};
  uint8_t irq{0};
};

struct KernelDeviceSummaryView {
  bool has_device_arbitration{false};
  std::size_t device_count{0};
  std::size_t claimed_device_count{0};
  bool has_storage{false};
  bool has_network{false};
  bool has_display{false};
  uint64_t service_lifecycle_transitions{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
  std::vector<KernelDeviceOwnershipView> devices;
};

struct KernelServiceRequest {
  KernelServiceRequestKind kind{KernelServiceRequestKind::RuntimeStatus};
  std::optional<ProcessGroupId> requesting_process_group_id{};
  std::optional<ProcessGroupId> process_group_id{};
  std::optional<SupervisorId> supervisor_id{};
  std::optional<ServiceId> service_id{};
};

struct KernelServiceResult {
  KernelServiceStatus status{KernelServiceStatus::InvalidRequest};
  KernelServiceRequestRejection rejection{KernelServiceRequestRejection::None};
  std::optional<KernelRuntimeStatusView> runtime{};
  std::optional<KernelProcessGroupStatusView> process_group{};
  std::optional<KernelSupervisorStatusView> supervisor{};
  std::optional<KernelSupervisorRecoveryStatusView> supervisor_recovery{};
  std::optional<KernelServiceStatusView> service{};
  std::optional<KernelSupervisorServiceInventoryView> supervisor_services{};
  std::optional<KernelFaultSummaryView> fault_summary{};
  std::optional<KernelAuditSummaryView> audit_summary{};
  std::optional<KernelDeviceSummaryView> device_summary{};
};

struct KernelServiceAction {
  KernelServiceActionKind kind{
      KernelServiceActionKind::AcknowledgeSupervisorFaultGroup};
  std::optional<ProcessGroupId> requesting_process_group_id{};
  std::optional<ProcessGroupId> process_group_id{};
  std::optional<SupervisorId> supervisor_id{};
  std::optional<ServiceId> service_id{};
  std::optional<std::string> service_name{};
  std::optional<std::string> device_name{};
};

struct KernelServiceActionResult {
  KernelServiceStatus status{KernelServiceStatus::InvalidRequest};
  KernelServiceActionRejection rejection{KernelServiceActionRejection::None};
  bool action_performed{false};
  std::optional<KernelProcessGroupStatusView> process_group{};
  std::optional<KernelSupervisorStatusView> supervisor{};
  std::optional<KernelSupervisorRecoveryStatusView> supervisor_recovery{};
  std::optional<KernelServiceStatusView> service{};
  std::optional<KernelSupervisorServiceInventoryView> supervisor_services{};
  std::optional<KernelFaultSummaryView> fault_summary{};
  std::optional<KernelAuditSummaryView> audit_summary{};
  std::optional<KernelDeviceSummaryView> device_summary{};
};

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept;

KernelAccessReport axion_kernel_check_access(
    KernelRuntimeState& state,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread(
    KernelRuntimeState& state,
    sched::TiscContext ctx) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread_in_group(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    ProcessGroupId process_group_id) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread_under_supervisor(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    SupervisorId supervisor_id) noexcept;

bool axion_kernel_tick(KernelRuntimeState& state) noexcept;

bool axion_kernel_step(KernelRuntimeState& state) noexcept;

bool axion_kernel_ipc_send(KernelRuntimeState& state,
                           sched::Tid dst,
                           ipc::CanonMessage msg) noexcept;

std::optional<ipc::CanonMessage> axion_kernel_ipc_recv(
    KernelRuntimeState& state,
    sched::Tid tid) noexcept;

KernelServiceResult axion_kernel_service_request(
    const KernelRuntimeState& state,
    const KernelServiceRequest& request) noexcept;

KernelServiceActionResult axion_kernel_service_action(
    KernelRuntimeState& state,
    const KernelServiceAction& action) noexcept;

bool axion_kernel_claim_device(KernelRuntimeState& state,
                               std::string_view device_name,
                               sched::Tid owner) noexcept;

bool axion_kernel_release_device(KernelRuntimeState& state,
                                 std::string_view device_name,
                                 sched::Tid owner) noexcept;

bool axion_kernel_ack_thread_fault(KernelRuntimeState& state,
                                   sched::Tid tid) noexcept;

bool axion_kernel_ack_process_group_fault(KernelRuntimeState& state,
                                          ProcessGroupId process_group_id) noexcept;

bool axion_kernel_ack_supervisor_group_fault(KernelRuntimeState& state,
                                             SupervisorId supervisor_id,
                                             ProcessGroupId process_group_id) noexcept;

int axion_kernel_main(const hal::BootContext& ctx) noexcept;

}  // namespace t81::ternaryos::kernel
