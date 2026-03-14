#pragma once

#include "kernel_abi.hpp"
#include "kernel_base.hpp"
#include "kernel_runtime_support.hpp"

#include "../dev/block_device.hpp"
#include "../dev/hosted_block_dev.hpp"
#include "../ipc/canon_message.hpp"
#include "../mmu/page_table.hpp"
#include "../mmu/ternary_page_alloc.hpp"
#include "../sched/scheduler.hpp"
#include "t81/canonfs/canon_driver.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState {
  struct ExecutableRecord {
    t81::canonfs::CanonRef object_ref{};
    t81::canonfs::CanonBlock image_block{};
    KernelThreadSpawnDescriptor entry_descriptor{};
  };

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
    std::vector<KernelCapabilityRecord> capabilities;
    std::unordered_map<std::string, KernelThreadSpawnDescriptor> entry_descriptors;
    std::unordered_map<std::string, ExecutableRecord> executable_records;
    bool faulted{false};
    bool blocked{false};
    bool acknowledgement_pending{false};
    std::size_t pending_fault_count{0};
    ProcessGroupAuditCounters counters{};
  };

  struct SupervisorState {
    static constexpr std::size_t kMaxCapabilityTransitionHistory = 8;

    SupervisorId id{0};
    std::vector<ProcessGroupId> managed_groups;
    std::vector<ServiceId> managed_services;
    std::deque<ProcessGroupId> pending_groups;
    std::deque<KernelCapabilityTransitionRecord> recent_capability_transitions;
    uint64_t fault_notifications{0};
    uint64_t acknowledgements{0};
    uint64_t recovered_groups{0};
    uint64_t service_lifecycle_transitions{0};
    uint64_t capability_transitions{0};
    std::optional<ProcessGroupId> last_acknowledged_group{};
    std::optional<ProcessGroupId> last_recovered_group{};
    std::optional<ServiceId> last_service_transition_id{};
    std::optional<KernelAuditEventKind> last_service_transition_kind{};
    std::optional<uint64_t> last_service_transition_sequence{};
    std::optional<ProcessGroupId> last_capability_transition_group_id{};
    std::optional<CapabilityRecordId> last_capability_transition_record_id{};
    std::optional<KernelAuditEventKind> last_capability_transition_kind{};
    std::optional<uint64_t> last_capability_transition_sequence{};
  };

  struct AddressSpaceState {
    AddressSpaceId id{0};
    ProcessGroupId process_group_id{0};
    /// True for the kernel address space (id==kKernelAddressSpace).
    /// Kernel-owned spaces may map TVAs anywhere in the 3^30 virtual space.
    /// User-owned spaces are restricted to VPN < kKernelSpaceVpnBase (RFC-00B1 §3.1).
    bool kernel_owned{false};
    bool boot_critical{false};
    bool pager_needed{false};
    bool pager_handoff_pending{false};
    bool pager_worker_owned{false};
    bool pager_terminal{false};
    std::size_t pending_pager_fault_count{0};
    uint64_t pager_faults{0};
    uint64_t pager_handoffs{0};
    uint64_t pager_resolutions{0};
    uint64_t pager_faults_coalesced{0};
    uint64_t pager_terminal_failures{0};
    uint64_t pager_boot_critical_resolutions{0};
    std::optional<KernelFaultRecord> last_pager_fault{};
    std::optional<uint64_t> last_pager_fault_sequence{};
    std::optional<uint64_t> last_pager_handoff_sequence{};
    std::optional<uint64_t> last_pager_resolution_sequence{};
    std::optional<uint64_t> last_pager_terminal_sequence{};
  };

  struct ServiceState {
    ServiceId id{0};
    std::string name;
    SupervisorId supervisor_id{0};
    ProcessGroupId process_group_id{0};
    std::optional<t81::canonfs::CanonRef> object_ref{};
    std::optional<KernelThreadSpawnDescriptor> entry_descriptor{};
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

  struct PagerWorkerState {
    std::deque<KernelPagerWorkItem> inbox;
    std::optional<KernelPagerWorkItem> active_work{};
    std::size_t inbox_high_watermark{0};
    std::size_t ready_backlog_high_watermark{0};
    uint64_t handoffs_received{0};
    std::optional<AddressSpaceId> last_received_address_space_id{};
    std::optional<uint64_t> last_received_handoff_sequence{};
    uint64_t ready_bypass_activations{0};
    std::optional<AddressSpaceId> last_ready_bypass_blocked_address_space_id{};
    std::optional<AddressSpaceId> last_ready_bypass_promoted_address_space_id{};
    std::optional<uint64_t> last_ready_bypass_cycle{};
    uint64_t ready_bypass_deferrals{0};
    std::optional<AddressSpaceId> last_ready_bypass_deferred_blocked_address_space_id{};
    std::optional<AddressSpaceId> last_ready_bypass_deferred_ready_address_space_id{};
    std::optional<uint64_t> last_ready_bypass_deferred_cycle{};
    uint64_t parked_cycles{0};
    uint64_t parked_resumptions{0};
    std::size_t parked_ready_high_watermark{0};
    std::optional<AddressSpaceId> parked_blocked_address_space_id{};
    std::optional<AddressSpaceId> last_parked_blocked_address_space_id{};
    std::optional<AddressSpaceId> last_parked_ready_address_space_id{};
    std::optional<uint64_t> last_parked_cycle{};
    std::optional<std::size_t> last_parked_ready_count{};
    std::optional<AddressSpaceId> last_parked_resumed_address_space_id{};
    std::optional<uint64_t> last_parked_resumed_handoff_sequence{};
    std::optional<uint64_t> last_parked_resumption_cycle{};
    std::optional<std::size_t> last_parked_resumed_ready_count{};
    std::optional<AddressSpaceId> last_parked_resumed_ready_address_space_id{};
    std::optional<uint64_t> last_parked_resumed_ready_handoff_sequence{};
    uint64_t parked_resolved_heads{0};
    std::optional<AddressSpaceId> last_parked_resolved_address_space_id{};
    std::optional<uint64_t> last_parked_resolved_handoff_sequence{};
    std::optional<uint64_t> last_parked_resolved_resolution_sequence{};
    std::optional<std::size_t> last_parked_resolved_remaining_inbox_count{};
    std::optional<AddressSpaceId> last_parked_resolved_remaining_address_space_id{};
    std::optional<uint64_t> last_parked_resolved_remaining_handoff_sequence{};
    uint64_t parked_resolution_follow_on_activations{0};
    std::optional<AddressSpaceId> last_parked_resolution_follow_on_address_space_id{};
    std::optional<uint64_t> last_parked_resolution_follow_on_handoff_sequence{};
    std::optional<uint64_t> last_parked_resolution_follow_on_activation_cycle{};
    uint64_t parked_resolution_follow_on_resolutions{0};
    std::optional<AddressSpaceId> last_parked_resolution_follow_on_resolved_address_space_id{};
    std::optional<uint64_t> last_parked_resolution_follow_on_resolved_handoff_sequence{};
    std::optional<uint64_t> last_parked_resolution_follow_on_resolution_sequence{};
    uint64_t activations{0};
    std::optional<AddressSpaceId> last_activated_address_space_id{};
    std::optional<uint64_t> last_activation_cycle{};
    uint64_t stall_cycles{0};
    uint64_t backlog_blocked_cycles{0};
    uint64_t ready_backlog_cycles{0};
    uint64_t resolutions_completed{0};
    std::optional<AddressSpaceId> last_completed_address_space_id{};
    std::optional<uint64_t> last_completed_resolution_sequence{};
    std::optional<AddressSpaceId> last_stalled_address_space_id{};
    std::optional<uint64_t> last_stall_cycle{};
    std::optional<AddressSpaceId> last_ready_backlog_address_space_id{};
    std::optional<uint64_t> last_ready_backlog_cycle{};
    std::optional<std::size_t> last_ready_backlog_count{};
    uint64_t terminal_failures{0};
    std::optional<AddressSpaceId> last_terminal_address_space_id{};
    std::optional<uint64_t> last_terminal_handoff_sequence{};
    std::optional<uint64_t> last_terminal_cycle{};
    uint64_t boot_critical_resolutions{0};
    std::optional<AddressSpaceId> last_boot_critical_address_space_id{};
    std::optional<uint64_t> last_boot_critical_handoff_sequence{};
    std::optional<uint64_t> last_boot_critical_resolution_sequence{};
  };

  struct Counters {
    uint64_t loop_iterations{0};
    uint64_t scheduler_ticks{0};
    uint64_t scheduler_switches{0};
    uint64_t ipc_messages_sent{0};
    uint64_t ipc_messages_received{0};
    uint64_t interrupts_recorded{0};
    uint64_t interrupts_delivered{0};
    KernelInterruptSourceCounters interrupt_sources_recorded{};
    KernelInterruptSourceCounters interrupt_sources_delivered{};
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
    uint64_t pager_eligible_faults{0};
    uint64_t policy_faults{0};
    uint64_t pager_handoffs_queued{0};
    uint64_t pager_handoffs_dispatched{0};
    uint64_t pager_resolutions{0};
    uint64_t pager_faults_coalesced{0};
    uint64_t pager_worker_activations{0};
    uint64_t pager_worker_ready_bypass_activations{0};
    uint64_t pager_worker_ready_bypass_deferrals{0};
    uint64_t pager_worker_parked_cycles{0};
    uint64_t pager_worker_parked_resumptions{0};
    uint64_t pager_worker_parked_resolved_heads{0};
    uint64_t pager_worker_parked_resolution_follow_on_activations{0};
    uint64_t pager_worker_parked_resolution_follow_on_resolutions{0};
    uint64_t pager_worker_stall_cycles{0};
    uint64_t pager_worker_backlog_blocked_cycles{0};
    uint64_t pager_worker_ready_backlog_cycles{0};
    uint64_t pager_worker_terminal_failures{0};
    uint64_t pager_worker_boot_critical_resolutions{0};
    uint64_t audit_events_recorded{0};
    uint64_t timer_interrupts_handled{0};
    uint64_t timer_preempts{0};
    uint64_t device_interrupts_handled{0};
    uint64_t ipc_blocks{0};    ///< threads that slept on empty inbox (RFC-00B6 §5.3.2)
    uint64_t ipc_wakes{0};     ///< threads woken by a successful SendMessage
    uint64_t device_wakes{0};  ///< threads woken by a device interrupt (RFC-00B5 §3.3)
    uint64_t syscall_trap_dispatches{0};  ///< SVC traps dispatched through axion_kernel_call_wire_tva() (RFC-00B6 §5.2)
    uint64_t kernel_space_rejections{0};  ///< user AS span attempts into kernel TVA space (RFC-00B1 §3.1 / RFC-00B6 §5.7)
    uint64_t canonfs_fetch_spawns{0};     ///< spawns that fetched CanonExec directly from CanonFS without prior registration (RFC-00B2 §3.1)
    uint64_t pager_service_mappings{0};   ///< page mappings supplied via RequestPageMapping by a PagerService-capable thread (RFC-00B7 §3.2)
  };

  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t total_ternary_pages{0};
  bool has_writable_memory{false};
  mmu::TernaryPageAllocator allocator;
  mmu::PageTable page_table;
  sched::Scheduler scheduler;
  ipc::MessageBus ipc_bus;
  std::optional<KernelDeviceArbitrationState> device_arbitration;
  std::deque<KernelFaultRecord> fault_log;
  std::deque<KernelFaultRecord> pending_faults;
  std::deque<KernelInterruptRecord> pending_interrupts;
  std::size_t pending_interrupt_high_watermark{0};
  std::deque<AddressSpaceId> pending_pager_handoffs;
  std::size_t pending_pager_handoff_high_watermark{0};
  std::deque<KernelAuditRecord> audit_log;
  std::unordered_map<sched::Tid, ThreadRuntimeState> thread_runtime;
  std::unordered_map<ProcessGroupId, ProcessGroupState> process_groups;
  std::unordered_map<SupervisorId, SupervisorState> supervisors;
  std::unordered_map<AddressSpaceId, AddressSpaceState> address_spaces;
  std::unordered_map<uint64_t, std::vector<std::byte>> physical_page_storage;
  std::unordered_map<ProcessGroupId, SupervisorId> process_group_supervisors;
  std::unordered_map<ProcessGroupId, AddressSpaceId> process_group_address_spaces;
  std::unordered_map<ServiceId, ServiceState> services;
  std::unordered_map<ProcessGroupId, ServiceId> process_group_services;
  /// Threads currently sleeping on an empty IPC inbox (RFC-00B6 §5.3.2 blocking receive).
  std::unordered_set<sched::Tid> ipc_blocked_tids;
  /// Threads parked waiting for a device interrupt (RFC-00B5 §3.3), keyed by source.
  std::unordered_map<uint8_t, std::unordered_set<sched::Tid>> device_waiting_tids;

  std::unique_ptr<t81::ternaryos::dev::IBlockDevice> published_executable_store_device;
  std::unique_ptr<t81::canonfs::Driver> published_executable_canonfs;
  PagerWorkerState pager_worker{};
  t81::vm::ThreadContext cpu_context{};
  Counters counters{};
  std::optional<KernelFaultRecord> last_delivered_fault{};
  std::optional<KernelInterruptRecord> last_recorded_interrupt{};
  std::optional<KernelInterruptRecord> last_delivered_interrupt{};
  std::optional<uint64_t> last_recorded_interrupt_audit_sequence{};
  std::optional<uint64_t> last_delivered_interrupt_audit_sequence{};
  std::optional<KernelAuditEventKind> last_interrupt_audit_kind{};
  std::optional<hal::InterruptSource> last_interrupt_audit_source{};
  std::optional<uint64_t> last_interrupt_audit_interrupt_sequence{};
  std::optional<uint64_t> last_interrupt_audit_payload{};
  std::optional<uint64_t> last_interrupt_audit_timestamp_ns{};
  std::optional<uint64_t> last_interrupt_audit_sequence{};
  std::optional<uint64_t> last_timer_preempt_cycle{};
  std::optional<uint64_t> last_timer_preempt_sequence{};
  std::optional<KernelPagerHandoffRecord> last_pager_handoff{};
  std::optional<KernelPagerResolutionRecord> last_pager_resolution{};
  std::optional<KernelAuditRecord> last_audit_event{};
  ProcessGroupId next_process_group_id{1};
  SupervisorId next_supervisor_id{1};
  ServiceId next_service_id{1};
  AddressSpaceId next_address_space_id{1};
  CapabilityRecordId next_capability_record_id{1};
  uint64_t next_interrupt_sequence{1};
  uint64_t next_pager_handoff_sequence{1};
  uint64_t next_pager_resolution_sequence{1};
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
        allocator(std::move(allocator_in)),
        published_executable_store_device(
            std::make_unique<t81::ternaryos::dev::HostedBlockDev>(
                256, "kernel-exec-store")) {}

  static constexpr sched::Tid kKernelTid = 0;
  static constexpr ProcessGroupId kKernelProcessGroup = 0;
  static constexpr SupervisorId kKernelSupervisor = 0;
  static constexpr AddressSpaceId kKernelAddressSpace = 0;
  static constexpr std::size_t kMaxFaultLog = 27;
  static constexpr std::size_t kMaxAuditLog = 81;
  static constexpr uint64_t kPagerWorkerParkedCycleLimit = 3;

  std::size_t fault_count() const noexcept { return fault_log.size(); }
  std::size_t pending_fault_count() const noexcept { return pending_faults.size(); }
  std::size_t pending_interrupt_count() const noexcept {
    return pending_interrupts.size();
  }
  std::size_t audit_count() const noexcept { return audit_log.size(); }
  std::size_t process_group_count() const noexcept { return process_groups.size(); }
  std::size_t supervisor_count() const noexcept { return supervisors.size(); }
  std::size_t address_space_count() const noexcept { return address_spaces.size(); }
  std::size_t pending_pager_handoff_count() const noexcept {
    return pending_pager_handoffs.size();
  }
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

  const AddressSpaceState* find_address_space(AddressSpaceId id) const noexcept {
    auto it = address_spaces.find(id);
    return it == address_spaces.end() ? nullptr : &it->second;
  }

  AddressSpaceState* find_address_space_mut(AddressSpaceId id) noexcept {
    auto it = address_spaces.find(id);
    return it == address_spaces.end() ? nullptr : &it->second;
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

  std::optional<AddressSpaceId> find_process_group_address_space(
      ProcessGroupId process_group_id) const noexcept {
    auto it = process_group_address_spaces.find(process_group_id);
    if (it == process_group_address_spaces.end()) {
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

}  // namespace t81::ternaryos::kernel
