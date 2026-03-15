#pragma once

#include "kernel_abi.hpp"
#include "kernel_base.hpp"
#include "kernel_runtime_support.hpp"

#include "../sched/scheduler.hpp"
#include "t81/canonfs/canon_driver.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState;

enum class KernelServiceRequestKind : uint8_t {
  RuntimeStatus = 0,
  ProcessGroupStatus,
  SupervisorStatus,
  SupervisorRecoveryStatus,
  ServiceStatus,
  SupervisorServiceStatus,
  SupervisorServiceInventory,
  SupervisorCapabilityInventory,
  SupervisorDelegationSummary,
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
  MissingExecutableRegistration,
};

struct KernelRuntimeStatusView {
  std::string platform_id;
  std::size_t memory_region_count{0};
  uint64_t total_ternary_pages{0};
  std::size_t address_space_count{0};
  std::size_t mapped_pages{0};
  std::size_t boot_critical_address_space_count{0};
  std::size_t boot_critical_pager_needed_count{0};
  std::size_t boot_critical_terminal_count{0};
  bool boot_progress_pending{false};
  bool boot_progress_blocked{false};
  std::size_t pager_needed_address_space_count{0};
  std::size_t pager_terminal_address_space_count{0};
  std::size_t pending_pager_handoff_count{0};
  std::size_t pending_pager_handoff_high_watermark{0};
  std::size_t pager_worker_inbox_count{0};
  std::size_t pager_worker_inbox_high_watermark{0};
  std::size_t pager_worker_ready_backlog_count{0};
  std::size_t pager_worker_ready_backlog_high_watermark{0};
  std::size_t pager_worker_parked_ready_count{0};
  std::size_t pager_worker_parked_ready_high_watermark{0};
  bool pager_worker_busy{false};
  std::optional<AddressSpaceId> pager_worker_active_address_space_id{};
  std::optional<uint64_t> pager_worker_active_handoff_sequence{};
  std::optional<AddressSpaceId> pager_worker_next_queued_address_space_id{};
  std::optional<uint64_t> pager_worker_next_queued_handoff_sequence{};
  uint64_t loop_iterations{0};
  uint64_t scheduler_ticks{0};
  uint64_t ipc_messages_sent{0};
  uint64_t ipc_messages_received{0};
  std::size_t pending_interrupt_count{0};
  std::size_t pending_interrupt_high_watermark{0};
  KernelInterruptSourceCounters pending_interrupt_sources{};
  uint64_t interrupts_recorded{0};
  uint64_t interrupts_delivered{0};
  KernelInterruptSourceCounters interrupt_sources_recorded{};
  KernelInterruptSourceCounters interrupt_sources_delivered{};
  std::optional<KernelInterruptRecord> next_pending_interrupt{};
  std::optional<KernelInterruptRecord> last_pending_interrupt{};
  std::optional<uint64_t> last_recorded_interrupt_audit_sequence{};
  std::optional<uint64_t> last_delivered_interrupt_audit_sequence{};
  std::optional<KernelAuditEventKind> last_interrupt_audit_kind{};
  std::optional<hal::InterruptSource> last_interrupt_audit_source{};
  std::optional<uint64_t> last_interrupt_audit_interrupt_sequence{};
  std::optional<uint64_t> last_interrupt_audit_payload{};
  std::optional<uint64_t> last_interrupt_audit_timestamp_ns{};
  std::optional<uint64_t> last_interrupt_audit_sequence{};
  std::optional<KernelInterruptRecord> last_recorded_interrupt{};
  std::optional<KernelInterruptRecord> last_delivered_interrupt{};
  uint64_t timer_interrupts_handled{0};
  uint64_t timer_preempts{0};
  uint64_t device_interrupts_handled{0};
  std::optional<uint64_t> last_timer_preempt_cycle{};
  std::optional<uint64_t> last_timer_preempt_sequence{};
  uint64_t ipc_blocks{0};             ///< threads that slept on empty inbox
  uint64_t ipc_wakes{0};             ///< threads woken by SendMessage
  std::size_t ipc_blocked_thread_count{0}; ///< threads currently sleeping on IPC inbox
  uint64_t device_wakes{0};          ///< threads woken by a device interrupt (RFC-00B5 §3.3)
  std::size_t device_waiting_thread_count{0}; ///< threads currently waiting on any device source
  uint64_t syscall_trap_dispatches{0}; ///< SVC traps dispatched through wire-TVA path (RFC-00B6 §5.2)
  uint64_t kernel_space_rejections{0}; ///< user AS span attempts into kernel TVA space (RFC-00B1 §3.1)
  uint64_t canonfs_fetch_spawns{0};    ///< spawns resolved from CanonFS without prior in-memory registration (RFC-00B2 §3.1)
  uint64_t pager_service_mappings{0};       ///< page mappings supplied via RequestPageMapping (RFC-00B7 §3.2)
  uint64_t pager_handoff_wakes{0};          ///< PagerService threads woken by a pager handoff dispatch (RFC-00B7 §3.3)
  uint64_t pager_service_resumptions{0};    ///< victim threads un-quarantined via ResumePageFaultedThread (RFC-00B7 §3.4)
  std::size_t pager_handoff_waiting_thread_count{0};  ///< threads currently parked via WaitForPagerHandoff
  uint64_t pager_eligible_faults{0};
  uint64_t policy_faults{0};
  uint64_t pager_handoffs_dispatched{0};
  uint64_t pager_resolutions{0};
  uint64_t pager_faults_coalesced{0};
  uint64_t pager_worker_handoffs_received{0};
  std::optional<AddressSpaceId> pager_worker_last_received_address_space_id{};
  std::optional<uint64_t> pager_worker_last_received_handoff_sequence{};
  uint64_t pager_worker_ready_bypass_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_promoted_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_bypass_cycle{};
  uint64_t pager_worker_ready_bypass_deferrals{0};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_deferred_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_deferred_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_bypass_deferred_cycle{};
  uint64_t pager_worker_parked_cycles{0};
  uint64_t pager_worker_parked_resumptions{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_parked_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_cycle{};
  std::optional<std::size_t> pager_worker_last_parked_ready_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resumed_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resumed_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resumption_cycle{};
  std::optional<std::size_t> pager_worker_last_parked_resumed_ready_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resumed_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resumed_ready_handoff_sequence{};
  uint64_t pager_worker_parked_resolved_heads{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolved_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_resolution_sequence{};
  std::optional<std::size_t> pager_worker_last_parked_resolved_remaining_inbox_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolved_remaining_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_remaining_handoff_sequence{};
  uint64_t pager_worker_parked_resolution_follow_on_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolution_follow_on_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_activation_cycle{};
  uint64_t pager_worker_parked_resolution_follow_on_resolutions{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolution_follow_on_resolved_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_resolution_sequence{};
  uint64_t pager_worker_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_activated_address_space_id{};
  std::optional<uint64_t> pager_worker_last_activation_cycle{};
  uint64_t pager_worker_stall_cycles{0};
  uint64_t pager_worker_backlog_blocked_cycles{0};
  uint64_t pager_worker_ready_backlog_cycles{0};
  uint64_t pager_worker_resolutions_completed{0};
  std::optional<AddressSpaceId> pager_worker_last_completed_address_space_id{};
  std::optional<uint64_t> pager_worker_last_completed_resolution_sequence{};
  std::optional<AddressSpaceId> pager_worker_last_stalled_address_space_id{};
  std::optional<uint64_t> pager_worker_last_stall_cycle{};
  std::optional<AddressSpaceId> pager_worker_last_ready_backlog_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_backlog_cycle{};
  std::optional<std::size_t> pager_worker_last_ready_backlog_count{};
  uint64_t pager_worker_terminal_failures{0};
  std::optional<AddressSpaceId> pager_worker_last_terminal_address_space_id{};
  std::optional<uint64_t> pager_worker_last_terminal_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_terminal_cycle{};
  uint64_t pager_worker_boot_critical_resolutions{0};
  std::optional<AddressSpaceId> pager_worker_last_boot_critical_address_space_id{};
  std::optional<uint64_t> pager_worker_last_boot_critical_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_boot_critical_resolution_sequence{};
  std::size_t managed_service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
  // DPE epoch summary (RFC-DPE-0003 §7) — populated when T81_ENABLE_DPE is active.
  uint64_t epoch_submissions{0};
  uint64_t epoch_commits{0};
  uint64_t epoch_aborts{0};
  uint64_t epoch_task_executions{0};
  std::optional<uint64_t> last_committed_epoch_id{};
  std::optional<t81::hash::CanonHash81> last_committed_epoch_hash{};
};

struct KernelProcessGroupStatusView {
  ProcessGroupId id{0};
  std::optional<AddressSpaceId> address_space_id{};
  std::size_t owned_page_count{0};
  bool pager_needed{false};
  bool pager_handoff_pending{false};
  bool pager_worker_owned{false};
  std::size_t pending_pager_fault_count{0};
  uint64_t pager_faults{0};
  uint64_t pager_handoffs{0};
  uint64_t pager_resolutions{0};
  uint64_t pager_faults_coalesced{0};
  std::optional<KernelFaultRecord> last_pager_fault{};
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
  std::size_t managed_address_space_count{0};
  std::size_t managed_mapped_page_count{0};
  std::size_t pager_needed_address_space_count{0};
  std::size_t pending_pager_fault_count{0};
  std::size_t pending_pager_handoff_count{0};
  uint64_t pager_resolutions{0};
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
  std::size_t managed_address_space_count{0};
  std::size_t managed_mapped_page_count{0};
  std::size_t pager_needed_address_space_count{0};
  std::size_t pending_pager_fault_count{0};
  std::size_t pending_pager_handoff_count{0};
  uint64_t pager_resolutions{0};
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
  std::optional<t81::canonfs::CanonRef> object_ref{};
  bool has_entry_descriptor{false};
  std::optional<KernelThreadSpawnDescriptor> entry_descriptor{};
  std::optional<AddressSpaceId> address_space_id{};
  std::size_t owned_page_count{0};
  bool pager_needed{false};
  bool pager_handoff_pending{false};
  bool pager_worker_owned{false};
  std::size_t pending_pager_fault_count{0};
  uint64_t pager_faults{0};
  uint64_t pager_handoffs{0};
  uint64_t pager_resolutions{0};
  uint64_t pager_faults_coalesced{0};
  std::optional<KernelFaultRecord> last_pager_fault{};
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
  std::optional<t81::canonfs::CanonRef> object_ref{};
  bool has_entry_descriptor{false};
  std::optional<KernelThreadSpawnDescriptor> entry_descriptor{};
  std::optional<AddressSpaceId> address_space_id{};
  std::size_t owned_page_count{0};
  bool pager_needed{false};
  bool pager_handoff_pending{false};
  bool pager_worker_owned{false};
  std::size_t pending_pager_fault_count{0};
  uint64_t pager_faults{0};
  uint64_t pager_handoffs{0};
  uint64_t pager_resolutions{0};
  uint64_t pager_faults_coalesced{0};
  std::optional<KernelFaultRecord> last_pager_fault{};
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

struct KernelSupervisorServiceInventoryView {
  SupervisorId supervisor_id{0};
  std::size_t service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t service_lifecycle_transitions{0};
  uint64_t capability_transitions{0};
  uint64_t total_service_requests{0};
  uint64_t total_service_rejections{0};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
  std::optional<ProcessGroupId> last_capability_transition_group_id{};
  std::optional<CapabilityRecordId> last_capability_transition_record_id{};
  std::optional<KernelAuditEventKind> last_capability_transition_kind{};
  std::optional<uint64_t> last_capability_transition_sequence{};
  std::vector<KernelCapabilityTransitionRecord> recent_capability_transitions;
  std::vector<ServiceId> service_ids;
  std::vector<KernelSupervisorServiceEntryView> services;
};

struct KernelSupervisorCapabilityEntryView {
  ProcessGroupId process_group_id{0};
  std::size_t capability_count{0};
  std::vector<KernelCapabilityRecord> capabilities;
};

struct KernelSupervisorCapabilityInventoryView {
  SupervisorId supervisor_id{0};
  std::size_t process_group_count{0};
  uint64_t capability_transitions{0};
  std::optional<ProcessGroupId> last_capability_transition_group_id{};
  std::optional<CapabilityRecordId> last_capability_transition_record_id{};
  std::optional<KernelAuditEventKind> last_capability_transition_kind{};
  std::optional<uint64_t> last_capability_transition_sequence{};
  std::vector<KernelCapabilityTransitionRecord> recent_capability_transitions;
  std::vector<KernelSupervisorCapabilityEntryView> process_groups;
};

struct KernelSupervisorDelegationSummaryEntryView {
  ProcessGroupId target_process_group_id{0};
  ProcessGroupId delegated_by_process_group_id{0};
  SupervisorId delegated_by_supervisor_id{0};
  std::size_t delegated_capability_count{0};
};

struct KernelSupervisorDelegationSummaryView {
  SupervisorId supervisor_id{0};
  std::size_t process_group_count{0};
  std::size_t delegation_entry_count{0};
  std::size_t delegated_capability_count{0};
  std::vector<KernelSupervisorDelegationSummaryEntryView> entries;
};

struct KernelFaultSummaryView {
  std::size_t recorded_faults{0};
  std::size_t pending_faults{0};
  std::size_t pending_interrupts{0};
  std::size_t pending_interrupt_high_watermark{0};
  KernelInterruptSourceCounters pending_interrupt_sources{};
  std::size_t delivered_faults{0};
  uint64_t interrupts_recorded{0};
  uint64_t interrupts_delivered{0};
  KernelInterruptSourceCounters interrupt_sources_recorded{};
  KernelInterruptSourceCounters interrupt_sources_delivered{};
  std::size_t routed_thread_faults{0};
  std::size_t quarantined_threads{0};
  std::size_t audit_events{0};
  uint64_t pager_eligible_faults{0};
  uint64_t policy_faults{0};
  std::size_t boot_critical_address_spaces{0};
  std::size_t boot_critical_pager_needed_address_spaces{0};
  std::size_t boot_critical_terminal_address_spaces{0};
  bool boot_progress_pending{false};
  bool boot_progress_blocked{false};
  std::size_t pager_needed_address_spaces{0};
  std::size_t pager_terminal_address_spaces{0};
  std::size_t pending_pager_handoffs{0};
  std::size_t pending_pager_handoff_high_watermark{0};
  std::size_t pager_worker_inbox_count{0};
  std::size_t pager_worker_inbox_high_watermark{0};
  std::size_t pager_worker_ready_backlog_count{0};
  std::size_t pager_worker_ready_backlog_high_watermark{0};
  std::size_t pager_worker_parked_ready_count{0};
  std::size_t pager_worker_parked_ready_high_watermark{0};
  bool pager_worker_busy{false};
  std::optional<AddressSpaceId> pager_worker_active_address_space_id{};
  std::optional<uint64_t> pager_worker_active_handoff_sequence{};
  std::optional<AddressSpaceId> pager_worker_next_queued_address_space_id{};
  std::optional<uint64_t> pager_worker_next_queued_handoff_sequence{};
  uint64_t pager_handoffs_dispatched{0};
  uint64_t pager_resolutions{0};
  uint64_t pager_faults_coalesced{0};
  uint64_t pager_worker_handoffs_received{0};
  std::optional<AddressSpaceId> pager_worker_last_received_address_space_id{};
  std::optional<uint64_t> pager_worker_last_received_handoff_sequence{};
  uint64_t pager_worker_ready_bypass_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_promoted_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_bypass_cycle{};
  uint64_t pager_worker_ready_bypass_deferrals{0};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_deferred_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_ready_bypass_deferred_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_bypass_deferred_cycle{};
  uint64_t pager_worker_parked_cycles{0};
  uint64_t pager_worker_parked_resumptions{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_blocked_address_space_id{};
  std::optional<AddressSpaceId> pager_worker_last_parked_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_cycle{};
  std::optional<std::size_t> pager_worker_last_parked_ready_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resumed_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resumed_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resumption_cycle{};
  std::optional<std::size_t> pager_worker_last_parked_resumed_ready_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resumed_ready_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resumed_ready_handoff_sequence{};
  uint64_t pager_worker_parked_resolved_heads{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolved_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_resolution_sequence{};
  std::optional<std::size_t> pager_worker_last_parked_resolved_remaining_inbox_count{};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolved_remaining_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolved_remaining_handoff_sequence{};
  uint64_t pager_worker_parked_resolution_follow_on_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolution_follow_on_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_activation_cycle{};
  uint64_t pager_worker_parked_resolution_follow_on_resolutions{0};
  std::optional<AddressSpaceId> pager_worker_last_parked_resolution_follow_on_resolved_address_space_id{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_parked_resolution_follow_on_resolution_sequence{};
  uint64_t pager_worker_activations{0};
  std::optional<AddressSpaceId> pager_worker_last_activated_address_space_id{};
  std::optional<uint64_t> pager_worker_last_activation_cycle{};
  uint64_t pager_worker_stall_cycles{0};
  uint64_t pager_worker_backlog_blocked_cycles{0};
  uint64_t pager_worker_ready_backlog_cycles{0};
  uint64_t pager_worker_resolutions_completed{0};
  std::optional<AddressSpaceId> pager_worker_last_completed_address_space_id{};
  std::optional<uint64_t> pager_worker_last_completed_resolution_sequence{};
  std::optional<AddressSpaceId> pager_worker_last_stalled_address_space_id{};
  std::optional<uint64_t> pager_worker_last_stall_cycle{};
  std::optional<AddressSpaceId> pager_worker_last_ready_backlog_address_space_id{};
  std::optional<uint64_t> pager_worker_last_ready_backlog_cycle{};
  std::optional<std::size_t> pager_worker_last_ready_backlog_count{};
  uint64_t pager_worker_terminal_failures{0};
  std::optional<AddressSpaceId> pager_worker_last_terminal_address_space_id{};
  std::optional<uint64_t> pager_worker_last_terminal_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_terminal_cycle{};
  uint64_t pager_worker_boot_critical_resolutions{0};
  std::optional<AddressSpaceId> pager_worker_last_boot_critical_address_space_id{};
  std::optional<uint64_t> pager_worker_last_boot_critical_handoff_sequence{};
  std::optional<uint64_t> pager_worker_last_boot_critical_resolution_sequence{};
  uint64_t service_lifecycle_transitions{0};
  std::optional<KernelFaultRecord> last_delivered_fault{};
  std::optional<KernelInterruptRecord> next_pending_interrupt{};
  std::optional<KernelInterruptRecord> last_pending_interrupt{};
  std::optional<uint64_t> last_recorded_interrupt_audit_sequence{};
  std::optional<uint64_t> last_delivered_interrupt_audit_sequence{};
  std::optional<KernelAuditEventKind> last_interrupt_audit_kind{};
  std::optional<hal::InterruptSource> last_interrupt_audit_source{};
  std::optional<uint64_t> last_interrupt_audit_interrupt_sequence{};
  std::optional<uint64_t> last_interrupt_audit_payload{};
  std::optional<uint64_t> last_interrupt_audit_timestamp_ns{};
  std::optional<uint64_t> last_interrupt_audit_sequence{};
  std::optional<KernelInterruptRecord> last_recorded_interrupt{};
  std::optional<KernelInterruptRecord> last_delivered_interrupt{};
  std::optional<AddressSpaceId> last_pager_address_space_id{};
  std::optional<KernelFaultRecord> last_pager_fault{};
  std::optional<KernelPagerHandoffRecord> last_pager_handoff{};
  std::optional<KernelPagerResolutionRecord> last_pager_resolution{};
  std::optional<KernelAuditRecord> last_audit_event{};
  std::optional<ServiceId> last_service_transition_id{};
  std::optional<KernelAuditEventKind> last_service_transition_kind{};
  std::optional<uint64_t> last_service_transition_sequence{};
};

struct KernelAuditSummaryView {
  std::size_t audit_events{0};
  uint64_t fault_deliveries{0};
  std::size_t pending_interrupt_count{0};
  std::size_t pending_interrupt_high_watermark{0};
  KernelInterruptSourceCounters pending_interrupt_sources{};
  uint64_t interrupts_recorded{0};
  uint64_t interrupt_deliveries{0};
  KernelInterruptSourceCounters interrupt_sources_recorded{};
  KernelInterruptSourceCounters interrupt_sources_delivered{};
  std::optional<KernelInterruptRecord> next_pending_interrupt{};
  std::optional<KernelInterruptRecord> last_pending_interrupt{};
  std::optional<uint64_t> last_recorded_interrupt_audit_sequence{};
  std::optional<uint64_t> last_delivered_interrupt_audit_sequence{};
  std::optional<KernelAuditEventKind> last_interrupt_audit_kind{};
  std::optional<hal::InterruptSource> last_interrupt_audit_source{};
  std::optional<uint64_t> last_interrupt_audit_interrupt_sequence{};
  std::optional<uint64_t> last_interrupt_audit_payload{};
  std::optional<uint64_t> last_interrupt_audit_timestamp_ns{};
  uint64_t thread_quarantines{0};
  uint64_t process_group_fault_entries{0};
  uint64_t supervisor_notifications{0};
  uint64_t thread_acknowledgements{0};
  uint64_t process_group_acknowledgements{0};
  uint64_t supervisor_acknowledgements{0};
  uint64_t thread_recoveries{0};
  uint64_t service_lifecycle_transitions{0};
  std::optional<uint64_t> last_interrupt_audit_sequence{};
  std::optional<KernelInterruptRecord> last_recorded_interrupt{};
  std::optional<KernelInterruptRecord> last_delivered_interrupt{};
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
  std::optional<KernelSupervisorCapabilityInventoryView> supervisor_capabilities{};
  std::optional<KernelSupervisorDelegationSummaryView> supervisor_delegations{};
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
  std::optional<t81::canonfs::CanonRef> object_ref{};
  std::optional<KernelThreadSpawnDescriptor> spawn_descriptor{};
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

KernelRuntimeStatusView make_runtime_view(const KernelRuntimeState& state);
KernelProcessGroupStatusView make_process_group_view(const KernelRuntimeState& state,
                                                     ProcessGroupId process_group_id);
KernelSupervisorStatusView make_supervisor_view(const KernelRuntimeState& state,
                                                SupervisorId supervisor_id);
KernelSupervisorRecoveryStatusView make_supervisor_recovery_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id);
KernelServiceStatusView make_service_view(const KernelRuntimeState& state,
                                          ServiceId service_id);
KernelSupervisorServiceInventoryView build_supervisor_services_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id);
KernelSupervisorCapabilityInventoryView build_supervisor_capabilities_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id);
KernelSupervisorDelegationSummaryView build_supervisor_delegation_summary_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id);
KernelFaultSummaryView make_fault_summary_view(const KernelRuntimeState& state);
KernelAuditSummaryView make_audit_summary_view(const KernelRuntimeState& state);
KernelDeviceSummaryView make_device_summary_view(const KernelRuntimeState& state);
KernelServiceResult axion_kernel_service_request(
    const KernelRuntimeState& state,
    const KernelServiceRequest& request) noexcept;
KernelServiceActionResult axion_kernel_service_action(
    KernelRuntimeState& state,
    const KernelServiceAction& action) noexcept;

}  // namespace t81::ternaryos::kernel
