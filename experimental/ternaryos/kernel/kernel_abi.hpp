#pragma once

#include "kernel_base.hpp"
#include "kernel_runtime_support.hpp"
#include "../ipc/canon_message.hpp"

#include <optional>
#include <string>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState;

enum class KernelCapabilityKind : uint8_t {
  Yield = 0,
  ThreadSpawn,
  IpcSend,
  IpcReceive,
  FaultObserve,
  FaultAcknowledge,
};

struct KernelCapabilityRecord {
  CapabilityRecordId record_id{0};
  KernelCapabilityKind kind{KernelCapabilityKind::Yield};
  std::optional<ProcessGroupId> process_group_scope{};
  bool kernel_seeded{false};
  std::optional<ProcessGroupId> delegated_by_process_group_id{};
  std::optional<SupervisorId> delegated_by_supervisor_id{};
};

struct KernelDelegationSummaryEntry {
  ProcessGroupId target_process_group_id{0};
  ProcessGroupId delegated_by_process_group_id{0};
  SupervisorId delegated_by_supervisor_id{0};
  std::size_t delegated_capability_count{0};
};

struct KernelThreadSpawnDescriptor {
  std::size_t pc{0};
  std::size_t sp{0};
  std::int64_t register0{0};
  bool halted{false};
  bool active{true};
  std::string label;
};

struct KernelSupervisorServiceSummaryEntry {
  ServiceId id{0};
  std::string name;
  ProcessGroupId process_group_id{0};
  std::optional<t81::canonfs::CanonRef> object_ref{};
  bool registered{false};
  bool blocked{false};
  bool suspended{false};
  bool unhealthy{false};
  bool has_entry_descriptor{false};
  std::optional<KernelThreadSpawnDescriptor> entry_descriptor{};
};

enum class KernelCallKind : uint8_t {
  Yield = 0,
  SpawnThreadInCallerGroup,
  SpawnThreadUnderSupervisor,
  RegisterThreadEntryDescriptor,
  RegisterExecutableObject,
  PublishExecutableObjectFromTva,
  RegisterExecutableObjectFromTva,
  QueryExecutableObject,
  SpawnThreadFromExecutableObject,
  SpawnThreadFromEntryDescriptor,
  GetThreadIdentity,
  QueryThreadExecutionState,
  ExitThread,
  SendMessage,
  ReceiveMessage,
  ReadFaultInbox,
  AcknowledgeThreadFault,
  AcknowledgeSupervisorFaultGroup,
  QueryProcessGroupMemory,
  SetAddressSpaceBootCritical,
  QueryRuntimeStatus,
  QueryFaultSummary,
  QuerySupervisorStatus,
  QuerySupervisorRecoveryStatus,
  QuerySupervisorServiceStatus,
  QuerySupervisorServiceInventory,
  QuerySupervisorCapabilityInventory,
  QuerySupervisorDelegationSummary,
  QueryCapabilityTransitionHistory,
  QueryCapabilities,
  QueryDelegatedCapabilities,
  QueryCapabilityRecord,
  GrantCapability,
  RevokeCapability,
  RevokeDelegatedCapabilities,
  RegisterService,
  SpawnThreadForService,
  QueryServiceStatus,
  SuspendService,
  ResumeService,
  MarkServiceUnhealthy,
  MarkServiceHealthy,
};

enum class KernelCallStatus : uint8_t {
  Ok = 0,
  InvalidRequest,
  CapabilityDenied,
  FaultedCaller,
  NotFound,
  Conflict,
  RetryLater,
  PolicyDenied,
};

enum class KernelCallRejection : uint8_t {
  None = 0,
  MissingCallerThread,
  MissingCallerProcessGroup,
  FaultedCaller,
  MissingDestinationThread,
  MissingMessage,
  IpcSendFailed,
  IpcReceiveEmpty,
  MissingTargetThread,
  CrossProcessGroupTarget,
  FaultInboxEmpty,
  MissingCapability,
  MissingTargetProcessGroup,
  MissingAddressSpace,
  InvalidAddressSpaceSpan,
  MissingEntryName,
  MissingEntryDescriptor,
  MissingEntryRegistration,
  MissingExecutableRef,
  MissingExecutableImageTva,
  MissingExecutableRegistration,
  InvalidExecutableObject,
  MissingBootCriticalValue,
  InvalidCapabilityRecordId,
  MissingCapabilityTransition,
  MissingDelegationScope,
  MissingSupervisor,
  SupervisorMismatch,
  ForeignSupervisorScope,
  ForeignAddressSpace,
  MissingServiceName,
  MissingService,
  ServiceRequestRejected,
  ServiceActionRejected,
};

struct KernelCallRequest {
  KernelCallKind kind{KernelCallKind::Yield};
  std::optional<sched::Tid> target_tid{};
  std::optional<ProcessGroupId> process_group_id{};
  std::optional<SupervisorId> supervisor_id{};
  std::optional<AddressSpaceId> address_space_id{};
  std::optional<uint64_t> object_tva{};
  std::optional<uint64_t> capability_transition_sequence{};
  std::optional<bool> boot_critical{};
  std::optional<sched::Tid> ipc_dst{};
  std::optional<ipc::CanonMessage> message{};
  std::optional<t81::canonfs::CanonRef> object_ref{};
  std::optional<KernelCapabilityRecord> capability{};
  std::optional<KernelThreadSpawnDescriptor> spawn_descriptor{};
  std::optional<ServiceId> service_id{};
  std::optional<std::string> service_name{};
};

struct KernelCallResult {
  KernelCallStatus status{KernelCallStatus::InvalidRequest};
  KernelCallRejection rejection{KernelCallRejection::None};
  bool action_performed{false};
  bool yielded{false};
  bool thread_exited{false};
  std::optional<sched::Tid> spawned_tid{};
  std::optional<sched::Tid> queried_tid{};
  std::optional<sched::Tid> caller_tid{};
  std::optional<ProcessGroupId> caller_process_group_id{};
  std::optional<ipc::CanonMessage> message{};
  std::optional<KernelFaultRecord> fault{};
  std::optional<t81::canonfs::CanonRef> object_ref{};
  std::vector<KernelCapabilityRecord> capabilities;
  bool executable_registered{false};
  bool executable_published{false};
  std::optional<KernelThreadSpawnDescriptor> executable_entry_descriptor{};
  std::optional<ServiceId> service_id{};
  std::optional<std::string> service_name{};
  bool service_registered{false};
  bool service_has_entry_descriptor{false};
  bool service_suspended{false};
  bool service_unhealthy{false};
  bool service_blocked{false};
  std::optional<KernelThreadSpawnDescriptor> service_entry_descriptor{};
  std::optional<SupervisorId> supervisor_id{};
  std::optional<AddressSpaceId> address_space_id{};
  std::optional<ProcessGroupId> target_process_group_id{};
  std::size_t thread_pc{0};
  std::size_t thread_sp{0};
  std::int64_t thread_register0{0};
  bool thread_active{false};
  bool thread_halted{false};
  bool thread_running{false};
  std::string thread_label;
  std::size_t process_group_owned_page_count{0};
  std::size_t process_group_pending_fault_count{0};
  bool process_group_pager_needed{false};
  bool process_group_faulted{false};
  bool process_group_blocked{false};
  bool process_group_acknowledgement_pending{false};
  bool address_space_boot_critical{false};
  std::size_t runtime_boot_critical_address_space_count{0};
  std::size_t runtime_boot_critical_pager_needed_count{0};
  std::size_t runtime_boot_critical_terminal_count{0};
  std::size_t runtime_mapped_pages{0};
  std::size_t fault_summary_recorded_faults{0};
  std::size_t fault_summary_pending_faults{0};
  std::size_t fault_summary_delivered_faults{0};
  std::size_t fault_summary_routed_thread_faults{0};
  std::size_t fault_summary_quarantined_threads{0};
  std::size_t supervisor_managed_group_count{0};
  std::size_t supervisor_managed_faulted_group_count{0};
  std::size_t supervisor_pending_group_count{0};
  uint64_t supervisor_fault_notifications{0};
  uint64_t supervisor_acknowledgements{0};
  uint64_t supervisor_recovered_groups{0};
  std::optional<ProcessGroupId> supervisor_last_pending_group{};
  std::optional<ProcessGroupId> supervisor_last_acknowledged_group{};
  std::optional<ProcessGroupId> supervisor_last_recovered_group{};
  std::size_t supervisor_capability_process_group_count{0};
  std::size_t supervisor_service_count{0};
  std::size_t supervisor_blocked_service_count{0};
  std::size_t supervisor_suspended_service_count{0};
  std::size_t supervisor_unhealthy_service_count{0};
  uint64_t supervisor_capability_transitions{0};
  uint64_t supervisor_service_lifecycle_transitions{0};
  std::optional<ProcessGroupId> supervisor_last_capability_transition_group_id{};
  std::optional<CapabilityRecordId> supervisor_last_capability_transition_record_id{};
  std::vector<KernelCapabilityTransitionRecord> supervisor_capability_transition_history;
  std::vector<KernelSupervisorServiceSummaryEntry> supervisor_services;
  std::size_t supervisor_delegation_process_group_count{0};
  std::size_t supervisor_delegation_entry_count{0};
  std::size_t supervisor_delegated_capability_count{0};
  std::vector<KernelDelegationSummaryEntry> supervisor_delegation_entries;
};

KernelCallResult axion_kernel_call(KernelRuntimeState& state,
                                   const KernelCallRequest& request) noexcept;

}  // namespace t81::ternaryos::kernel
