#include "kernel_main.hpp"

#include <algorithm>

namespace t81::ternaryos::kernel {

namespace {

constexpr std::string_view kVBoxPlatformPrefix = "virtualbox-x86_64:";

std::optional<KernelDeviceArbitrationState> bootstrap_device_arbitration(
    const std::string& platform_id) {
  if (!platform_id.starts_with(kVBoxPlatformPrefix)) {
    return std::nullopt;
  }

  const hal::VBoxProfile profile{};
  const auto profile_summary = hal::virtualbox_profile_summary(profile);
  const std::string expected_platform_id =
      std::string(kVBoxPlatformPrefix) + profile_summary;
  if (platform_id != expected_platform_id) {
    return std::nullopt;
  }

  KernelDeviceArbitrationState state;
  state.profile_summary = profile_summary;
  for (const auto& dev : hal::virtualbox_device_map(profile)) {
    state.devices.push_back(KernelDeviceRecord{
        .name = dev.name,
        .bus = dev.bus,
        .base = dev.base,
        .span_bytes = dev.span_bytes,
        .irq = dev.irq,
    });
    state.has_storage = state.has_storage || state.devices.back().name == "ahci";
    state.has_network = state.has_network || state.devices.back().name == "e1000";
    state.has_display = state.has_display || state.devices.back().name == "vmsvga";
  }
  return state;
}

void record_audit_event(KernelRuntimeState& state,
                        KernelAuditEventKind kind,
                        sched::Tid subject_tid,
                        ProcessGroupId process_group_id,
                        mmu::MmuFault fault = mmu::MmuFault::None) {
  KernelAuditRecord record{
      .kind = kind,
      .subject_tid = subject_tid,
      .process_group_id = process_group_id,
      .fault = fault,
      .sequence = state.next_audit_sequence++,
  };
  if (state.audit_log.size() >= KernelRuntimeState::kMaxAuditLog) {
    state.audit_log.pop_front();
  }
  state.audit_log.push_back(record);
  state.last_audit_event = record;
  ++state.counters.audit_events_recorded;
  if (auto* group = state.find_process_group_mut(process_group_id)) {
    ++group->counters.audit_events;
  }
}

KernelRuntimeState::ProcessGroupState* create_process_group(KernelRuntimeState& state) {
  const ProcessGroupId id = state.next_process_group_id++;
  auto [it, inserted] = state.process_groups.emplace(
      id, KernelRuntimeState::ProcessGroupState{.id = id});
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::SupervisorState* create_supervisor(KernelRuntimeState& state) {
  const SupervisorId id = state.next_supervisor_id++;
  auto [it, inserted] =
      state.supervisors.emplace(id, KernelRuntimeState::SupervisorState{.id = id});
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::AddressSpaceState* create_address_space(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id) {
  const AddressSpaceId id = state.next_address_space_id++;
  auto [it, inserted] = state.address_spaces.emplace(
      id,
      KernelRuntimeState::AddressSpaceState{
          .id = id,
          .process_group_id = process_group_id,
      });
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::ProcessGroupState* assign_thread_to_group(
    KernelRuntimeState& state, sched::Tid tid, ProcessGroupId process_group_id) {
  auto* thread_state = state.find_thread_runtime_mut(tid);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!thread_state || !group_state) {
    return nullptr;
  }
  thread_state->process_group_id = process_group_id;
  group_state->member_tids.push_back(tid);
  return group_state;
}

KernelRuntimeState::SupervisorState* assign_group_to_supervisor(
    KernelRuntimeState& state, ProcessGroupId process_group_id, SupervisorId supervisor_id) {
  auto* supervisor_state = state.find_supervisor_mut(supervisor_id);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!supervisor_state || !group_state) {
    return nullptr;
  }
  supervisor_state->managed_groups.push_back(process_group_id);
  state.process_group_supervisors[process_group_id] = supervisor_id;
  return supervisor_state;
}

KernelRuntimeState::AddressSpaceState* assign_group_to_address_space(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    AddressSpaceId address_space_id) {
  auto* address_space = state.find_address_space_mut(address_space_id);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!address_space || !group_state) {
    return nullptr;
  }
  address_space->process_group_id = process_group_id;
  state.process_group_address_spaces[process_group_id] = address_space_id;
  return address_space;
}

KernelRuntimeState::ServiceState* create_service(KernelRuntimeState& state,
                                                 std::string name,
                                                 ProcessGroupId process_group_id,
                                                 SupervisorId supervisor_id) {
  const ServiceId id = state.next_service_id++;
  auto [it, inserted] = state.services.emplace(
      id,
      KernelRuntimeState::ServiceState{
          .id = id,
          .name = std::move(name),
          .supervisor_id = supervisor_id,
          .process_group_id = process_group_id,
          .registered = true,
      });
  if (!inserted) {
    return nullptr;
  }
  state.process_group_services[process_group_id] = id;
  if (auto* supervisor_state = state.find_supervisor_mut(supervisor_id)) {
    supervisor_state->managed_services.push_back(id);
  }
  return &it->second;
}

void mark_service_blocked(KernelRuntimeState& state,
                          ProcessGroupId process_group_id,
                          bool blocked) {
  const auto service_id = state.find_process_group_service(process_group_id);
  if (!service_id.has_value()) {
    return;
  }
  auto* service_state = state.find_service_mut(*service_id);
  if (!service_state || service_state->blocked == blocked) {
    return;
  }
  service_state->blocked = blocked;
  ++service_state->state_transitions;
}

bool mark_service_suspended(KernelRuntimeState& state,
                            ServiceId service_id,
                            bool suspended) {
  auto* service_state = state.find_service_mut(service_id);
  if (!service_state || !service_state->registered ||
      service_state->suspended == suspended) {
    return false;
  }
  service_state->suspended = suspended;
  ++service_state->state_transitions;
  return true;
}

bool mark_service_unhealthy(KernelRuntimeState& state,
                            ServiceId service_id,
                            bool unhealthy) {
  auto* service_state = state.find_service_mut(service_id);
  if (!service_state || !service_state->registered ||
      service_state->unhealthy == unhealthy) {
    return false;
  }
  service_state->unhealthy = unhealthy;
  ++service_state->state_transitions;
  return true;
}

KernelSupervisorServiceInventoryView build_supervisor_services_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id);

struct LatestServiceTransitionView {
  std::optional<ServiceId> service_id{};
  std::optional<KernelAuditEventKind> kind{};
  std::optional<uint64_t> sequence{};
};

LatestServiceTransitionView latest_service_transition_view(
    const KernelRuntimeState& state) {
  LatestServiceTransitionView latest;
  for (const auto& [_, supervisor_state] : state.supervisors) {
    if (!supervisor_state.last_service_transition_sequence.has_value()) {
      continue;
    }
    if (!latest.sequence.has_value() ||
        *supervisor_state.last_service_transition_sequence > *latest.sequence) {
      latest.service_id = supervisor_state.last_service_transition_id;
      latest.kind = supervisor_state.last_service_transition_kind;
      latest.sequence = supervisor_state.last_service_transition_sequence;
    }
  }
  return latest;
}

struct RuntimeServiceSummary {
  std::size_t managed_service_count{0};
  std::size_t blocked_service_count{0};
  std::size_t suspended_service_count{0};
  std::size_t unhealthy_service_count{0};
  uint64_t service_lifecycle_transitions{0};
};

RuntimeServiceSummary runtime_service_summary(const KernelRuntimeState& state) {
  RuntimeServiceSummary summary;
  for (const auto& [_, service_state] : state.services) {
    if (!service_state.registered) {
      continue;
    }
    ++summary.managed_service_count;
    if (service_state.blocked) {
      ++summary.blocked_service_count;
    }
    if (service_state.suspended) {
      ++summary.suspended_service_count;
    }
    if (service_state.unhealthy) {
      ++summary.unhealthy_service_count;
    }
  }
  for (const auto& [_, supervisor_state] : state.supervisors) {
    summary.service_lifecycle_transitions +=
        supervisor_state.service_lifecycle_transitions;
  }
  return summary;
}

std::size_t count_mapped_pages_for_address_space(const KernelRuntimeState& state,
                                                 AddressSpaceId address_space_id) {
  return std::count_if(
      state.page_table.entries().begin(),
      state.page_table.entries().end(),
      [&](const auto& entry) { return entry.second.owner_pid == address_space_id; });
}

bool is_pager_eligible_fault(mmu::MmuFault fault) {
  return fault == mmu::MmuFault::Unmapped;
}

const KernelRuntimeState::AddressSpaceState* find_group_address_space_state(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) {
  const auto address_space_id = state.find_process_group_address_space(process_group_id);
  if (!address_space_id.has_value()) {
    return nullptr;
  }
  return state.find_address_space(*address_space_id);
}

void record_pager_fault_state(KernelRuntimeState& state,
                              ProcessGroupId process_group_id,
                              const KernelFaultRecord& fault_record) {
  const auto address_space_id = state.find_process_group_address_space(process_group_id);
  if (!address_space_id.has_value() || !is_pager_eligible_fault(fault_record.fault)) {
    ++state.counters.policy_faults;
    return;
  }
  auto* address_space = state.find_address_space_mut(*address_space_id);
  if (!address_space) {
    ++state.counters.policy_faults;
    return;
  }
  address_space->pager_needed = true;
  if (!address_space->pager_handoff_pending) {
    address_space->pager_handoff_pending = true;
    state.pending_pager_handoffs.push_back(*address_space_id);
    ++state.counters.pager_handoffs_queued;
  }
  ++address_space->pending_pager_fault_count;
  ++address_space->pager_faults;
  address_space->last_pager_fault = fault_record;
  address_space->last_pager_fault_sequence = state.counters.faults_delivered;
  ++state.counters.pager_eligible_faults;
}

std::size_t count_pager_needed_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) { return entry.second.pager_needed; });
}

std::size_t count_pending_pager_handoff_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) {
                         return entry.second.pager_handoff_pending;
                       });
}

std::size_t count_managed_pager_needed_address_spaces(
    const KernelRuntimeState& state,
    const KernelRuntimeState::SupervisorState& supervisor) {
  std::size_t count = 0;
  for (auto group_id : supervisor.managed_groups) {
    const auto* address_space = find_group_address_space_state(state, group_id);
    if (address_space && address_space->pager_needed) {
      ++count;
    }
  }
  return count;
}

std::size_t count_managed_pending_pager_faults(
    const KernelRuntimeState& state,
    const KernelRuntimeState::SupervisorState& supervisor) {
  std::size_t count = 0;
  for (auto group_id : supervisor.managed_groups) {
    const auto* address_space = find_group_address_space_state(state, group_id);
    if (address_space) {
      count += address_space->pending_pager_fault_count;
    }
  }
  return count;
}

std::size_t count_managed_pending_pager_handoffs(
    const KernelRuntimeState& state,
    const KernelRuntimeState::SupervisorState& supervisor) {
  std::size_t count = 0;
  for (auto group_id : supervisor.managed_groups) {
    const auto* address_space = find_group_address_space_state(state, group_id);
    if (address_space && address_space->pager_handoff_pending) {
      ++count;
    }
  }
  return count;
}

struct LatestPagerFaultView {
  std::optional<AddressSpaceId> address_space_id{};
  std::optional<KernelFaultRecord> fault{};
};

LatestPagerFaultView latest_pager_fault_view(const KernelRuntimeState& state) {
  LatestPagerFaultView latest;
  std::optional<uint64_t> latest_sequence{};
  for (const auto& [address_space_id, address_space] : state.address_spaces) {
    if (!address_space.last_pager_fault.has_value() ||
        !address_space.last_pager_fault_sequence.has_value()) {
      continue;
    }
    if (!latest_sequence.has_value() ||
        *address_space.last_pager_fault_sequence > *latest_sequence) {
      latest.address_space_id = address_space_id;
      latest.fault = address_space.last_pager_fault;
      latest_sequence = address_space.last_pager_fault_sequence;
    }
  }
  return latest;
}

std::size_t count_managed_address_spaces(const KernelRuntimeState& state,
                                         const KernelRuntimeState::SupervisorState& supervisor) {
  std::size_t count = 0;
  for (auto group_id : supervisor.managed_groups) {
    if (state.find_process_group_address_space(group_id).has_value()) {
      ++count;
    }
  }
  return count;
}

std::size_t count_managed_mapped_pages(const KernelRuntimeState& state,
                                       const KernelRuntimeState::SupervisorState& supervisor) {
  std::size_t total = 0;
  for (auto group_id : supervisor.managed_groups) {
    const auto address_space_id = state.find_process_group_address_space(group_id);
    if (!address_space_id.has_value()) {
      continue;
    }
    total += count_mapped_pages_for_address_space(state, *address_space_id);
  }
  return total;
}

bool queue_supervisor_pending_group(KernelRuntimeState& state,
                                    ProcessGroupId process_group_id,
                                    sched::Tid subject_tid,
                                    mmu::MmuFault fault) {
  const auto supervisor_id = state.find_process_group_supervisor(process_group_id);
  if (!supervisor_id.has_value()) {
    return false;
  }
  auto* supervisor_state = state.find_supervisor_mut(*supervisor_id);
  if (!supervisor_state) {
    return false;
  }
  bool already_pending = false;
  for (auto pending_group_id : supervisor_state->pending_groups) {
    if (pending_group_id == process_group_id) {
      already_pending = true;
      break;
    }
  }
  if (!already_pending) {
    supervisor_state->pending_groups.push_back(process_group_id);
  }
  ++supervisor_state->fault_notifications;
  ++state.counters.supervisor_fault_notifications;
  record_audit_event(state,
                     KernelAuditEventKind::SupervisorFaultNotified,
                     subject_tid,
                     process_group_id,
                     fault);
  return true;
}

void record_fault(KernelRuntimeState& state,
                  uint64_t tva,
                  mmu::MmuAccessMode mode,
                  mmu::MmuFault fault) {
  const sched::Tid subject_tid = state.scheduler.current_tid();
  KernelFaultRecord record{
      .platform_id = state.platform_id,
      .tva = tva,
      .access_mode = mode,
      .fault = fault,
      .subject_tid = subject_tid,
  };
  if (state.fault_log.size() >= KernelRuntimeState::kMaxFaultLog) {
    state.fault_log.pop_front();
  }
  state.fault_log.push_back(record);
  state.pending_faults.push_back(record);
  ++state.counters.faults_recorded;
}

bool maybe_recover_thread(KernelRuntimeState& state,
                          KernelRuntimeState::ThreadRuntimeState& thread_state) {
  if (!thread_state.quarantined || !thread_state.fault_inbox.empty()) {
    return false;
  }
  auto* group_state = state.find_process_group_mut(thread_state.process_group_id);
  if (!group_state || group_state->acknowledgement_pending ||
      group_state->pending_fault_count != 0) {
    return false;
  }
  thread_state.quarantined = false;
  if (!state.scheduler.wake(thread_state.tid)) {
    return false;
  }
  ++state.counters.thread_fault_recoveries;
  ++state.counters.process_group_recoveries;
  ++group_state->counters.recoveries;
  mark_service_blocked(state, group_state->id, false);
  record_audit_event(state,
                     KernelAuditEventKind::ThreadRecovered,
                     thread_state.tid,
                     group_state->id);
  return true;
}

KernelDeviceRecord* find_device(KernelRuntimeState& state, std::string_view device_name) {
  if (!state.device_arbitration) {
    return nullptr;
  }
  for (auto& device : state.device_arbitration->devices) {
    if (device.name == device_name) {
      return &device;
    }
  }
  return nullptr;
}

std::size_t count_quarantined_threads(const KernelRuntimeState& state,
                                      ProcessGroupId process_group_id) {
  std::size_t count = 0;
  for (const auto& [tid, thread_state] : state.thread_runtime) {
    (void)tid;
    if (thread_state.process_group_id == process_group_id && thread_state.quarantined) {
      ++count;
    }
  }
  return count;
}

std::size_t count_faulted_groups(const KernelRuntimeState& state,
                                 const KernelRuntimeState::SupervisorState& supervisor_state) {
  std::size_t count = 0;
  for (auto process_group_id : supervisor_state.managed_groups) {
    const auto* group_state = state.find_process_group(process_group_id);
    if (group_state && group_state->faulted) {
      ++count;
    }
  }
  return count;
}

std::size_t count_claimed_devices(const KernelRuntimeState& state) {
  if (!state.device_arbitration.has_value()) {
    return 0;
  }
  std::size_t claimed = 0;
  for (const auto& device : state.device_arbitration->devices) {
    if (device.owner_tid.has_value()) {
      ++claimed;
    }
  }
  return claimed;
}

std::optional<sched::Tid> primary_tid_for_group(const KernelRuntimeState& state,
                                                ProcessGroupId process_group_id) {
  const auto* group_state = state.find_process_group(process_group_id);
  if (!group_state || group_state->member_tids.empty()) {
    return std::nullopt;
  }
  return *std::min_element(group_state->member_tids.begin(),
                           group_state->member_tids.end());
}

void record_service_audit_event(KernelRuntimeState& state,
                                KernelAuditEventKind kind,
                                ProcessGroupId process_group_id) {
  const auto subject_tid =
      primary_tid_for_group(state, process_group_id).value_or(KernelRuntimeState::kKernelTid);
  record_audit_event(
      state, kind, subject_tid, process_group_id, mmu::MmuFault::None);
}

void record_supervisor_service_transition(KernelRuntimeState& state,
                                          ServiceId service_id,
                                          SupervisorId supervisor_id,
                                          ProcessGroupId process_group_id,
                                          KernelAuditEventKind kind) {
  record_service_audit_event(state, kind, process_group_id);
  auto* service_state = state.find_service_mut(service_id);
  if (service_state && state.last_audit_event.has_value()) {
    service_state->last_transition_kind = kind;
    service_state->last_transition_sequence = state.last_audit_event->sequence;
  }
  auto* supervisor_state = state.find_supervisor_mut(supervisor_id);
  if (!supervisor_state || !state.last_audit_event.has_value()) {
    return;
  }
  ++supervisor_state->service_lifecycle_transitions;
  supervisor_state->last_service_transition_id = service_id;
  supervisor_state->last_service_transition_kind = kind;
  supervisor_state->last_service_transition_sequence = state.last_audit_event->sequence;
}

bool group_has_pending_thread_faults(const KernelRuntimeState& state,
                                     ProcessGroupId process_group_id) {
  const auto* group_state = state.find_process_group(process_group_id);
  if (!group_state) {
    return false;
  }
  for (const auto tid : group_state->member_tids) {
    const auto* thread_state = state.find_thread_runtime(tid);
    if (thread_state && !thread_state->fault_inbox.empty()) {
      return true;
    }
  }
  return false;
}

KernelProcessGroupStatusView make_process_group_view(const KernelRuntimeState& state,
                                                     ProcessGroupId process_group_id) {
  const auto* group_state = state.find_process_group(process_group_id);
  const auto address_space_id = state.find_process_group_address_space(process_group_id);
  const auto* address_space = address_space_id.has_value()
                                  ? state.find_address_space(*address_space_id)
                                  : nullptr;
  return KernelProcessGroupStatusView{
      .id = group_state ? group_state->id : process_group_id,
      .address_space_id = address_space_id,
      .owned_page_count = address_space_id.has_value()
                              ? count_mapped_pages_for_address_space(state, *address_space_id)
                              : 0,
      .pager_needed = address_space ? address_space->pager_needed : false,
      .pager_handoff_pending =
          address_space ? address_space->pager_handoff_pending : false,
      .pending_pager_fault_count =
          address_space ? address_space->pending_pager_fault_count : 0,
      .pager_faults = address_space ? address_space->pager_faults : 0,
      .pager_handoffs = address_space ? address_space->pager_handoffs : 0,
      .last_pager_fault =
          address_space ? address_space->last_pager_fault : std::nullopt,
      .member_count = group_state ? group_state->member_tids.size() : 0,
      .quarantined_thread_count =
          group_state ? count_quarantined_threads(state, process_group_id) : 0,
      .faulted = group_state ? group_state->faulted : false,
      .blocked = group_state ? group_state->blocked : false,
      .acknowledgement_pending =
          group_state ? group_state->acknowledgement_pending : false,
      .pending_fault_count = group_state ? group_state->pending_fault_count : 0,
      .audit_events = group_state ? group_state->counters.audit_events : 0,
      .fault_entries = group_state ? group_state->counters.fault_entries : 0,
      .acknowledgements = group_state ? group_state->counters.acknowledgements : 0,
      .recoveries = group_state ? group_state->counters.recoveries : 0,
      .supervisor_id = state.find_process_group_supervisor(process_group_id),
  };
}

KernelSupervisorStatusView make_supervisor_view(const KernelRuntimeState& state,
                                                SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  const auto service_inventory = build_supervisor_services_view(state, supervisor_id);
  return KernelSupervisorStatusView{
      .id = supervisor_state ? supervisor_state->id : supervisor_id,
      .managed_group_count =
          supervisor_state ? supervisor_state->managed_groups.size() : 0,
      .managed_address_space_count =
          supervisor_state ? count_managed_address_spaces(state, *supervisor_state) : 0,
      .managed_mapped_page_count =
          supervisor_state ? count_managed_mapped_pages(state, *supervisor_state) : 0,
      .pager_needed_address_space_count =
          supervisor_state
              ? count_managed_pager_needed_address_spaces(state, *supervisor_state)
              : 0,
      .pending_pager_fault_count =
          supervisor_state ? count_managed_pending_pager_faults(state, *supervisor_state)
                           : 0,
      .pending_pager_handoff_count =
          supervisor_state ? count_managed_pending_pager_handoffs(state, *supervisor_state)
                           : 0,
      .managed_faulted_group_count =
          supervisor_state ? count_faulted_groups(state, *supervisor_state) : 0,
      .managed_service_count = service_inventory.service_count,
      .blocked_service_count = service_inventory.blocked_service_count,
      .suspended_service_count = service_inventory.suspended_service_count,
      .unhealthy_service_count = service_inventory.unhealthy_service_count,
      .pending_group_count =
          supervisor_state ? supervisor_state->pending_groups.size() : 0,
      .fault_notifications =
          supervisor_state ? supervisor_state->fault_notifications : 0,
      .acknowledgements = supervisor_state ? supervisor_state->acknowledgements : 0,
      .service_lifecycle_transitions =
          supervisor_state ? supervisor_state->service_lifecycle_transitions : 0,
      .last_pending_group =
          (!supervisor_state || supervisor_state->pending_groups.empty())
              ? std::nullopt
              : std::optional<ProcessGroupId>{supervisor_state->pending_groups.back()},
      .last_service_transition_id = service_inventory.last_service_transition_id,
      .last_service_transition_kind = service_inventory.last_service_transition_kind,
      .last_service_transition_sequence =
          service_inventory.last_service_transition_sequence,
  };
}

KernelSupervisorRecoveryStatusView make_supervisor_recovery_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  const auto service_inventory = build_supervisor_services_view(state, supervisor_id);
  return KernelSupervisorRecoveryStatusView{
      .id = supervisor_state ? supervisor_state->id : supervisor_id,
      .pending_group_count =
          supervisor_state ? supervisor_state->pending_groups.size() : 0,
      .managed_address_space_count =
          supervisor_state ? count_managed_address_spaces(state, *supervisor_state) : 0,
      .managed_mapped_page_count =
          supervisor_state ? count_managed_mapped_pages(state, *supervisor_state) : 0,
      .pager_needed_address_space_count =
          supervisor_state
              ? count_managed_pager_needed_address_spaces(state, *supervisor_state)
              : 0,
      .pending_pager_fault_count =
          supervisor_state ? count_managed_pending_pager_faults(state, *supervisor_state)
                           : 0,
      .pending_pager_handoff_count =
          supervisor_state ? count_managed_pending_pager_handoffs(state, *supervisor_state)
                           : 0,
      .managed_service_count = service_inventory.service_count,
      .blocked_service_count = service_inventory.blocked_service_count,
      .suspended_service_count = service_inventory.suspended_service_count,
      .unhealthy_service_count = service_inventory.unhealthy_service_count,
      .acknowledgements = supervisor_state ? supervisor_state->acknowledgements : 0,
      .recovered_groups = supervisor_state ? supervisor_state->recovered_groups : 0,
      .service_lifecycle_transitions =
          supervisor_state ? supervisor_state->service_lifecycle_transitions : 0,
      .pending_group_ids = supervisor_state
                               ? std::vector<ProcessGroupId>(supervisor_state->pending_groups.begin(),
                                                             supervisor_state->pending_groups.end())
                               : std::vector<ProcessGroupId>{},
      .last_acknowledged_group =
          supervisor_state ? supervisor_state->last_acknowledged_group : std::nullopt,
      .last_recovered_group =
          supervisor_state ? supervisor_state->last_recovered_group : std::nullopt,
      .last_service_transition_id = service_inventory.last_service_transition_id,
      .last_service_transition_kind = service_inventory.last_service_transition_kind,
      .last_service_transition_sequence =
          service_inventory.last_service_transition_sequence,
  };
}

KernelServiceStatusView make_service_view(const KernelRuntimeState& state,
                                         ServiceId service_id) {
  const auto* service_state = state.find_service(service_id);
  const auto* group_state =
      service_state ? state.find_process_group(service_state->process_group_id) : nullptr;
  const auto address_space_id = service_state
                                    ? state.find_process_group_address_space(
                                          service_state->process_group_id)
                                    : std::nullopt;
  const auto* address_space = address_space_id.has_value()
                                  ? state.find_address_space(*address_space_id)
                                  : nullptr;
  return KernelServiceStatusView{
      .id = service_state ? service_state->id : service_id,
      .name = service_state ? service_state->name : std::string{},
      .supervisor_id = service_state ? service_state->supervisor_id : 0,
      .process_group_id = service_state ? service_state->process_group_id : 0,
      .address_space_id = address_space_id,
      .owned_page_count = address_space_id.has_value()
                              ? count_mapped_pages_for_address_space(state, *address_space_id)
                              : 0,
      .pager_needed = address_space ? address_space->pager_needed : false,
      .pager_handoff_pending =
          address_space ? address_space->pager_handoff_pending : false,
      .pending_pager_fault_count =
          address_space ? address_space->pending_pager_fault_count : 0,
      .pager_faults = address_space ? address_space->pager_faults : 0,
      .pager_handoffs = address_space ? address_space->pager_handoffs : 0,
      .last_pager_fault =
          address_space ? address_space->last_pager_fault : std::nullopt,
      .primary_tid =
          service_state ? primary_tid_for_group(state, service_state->process_group_id)
                        : std::nullopt,
      .blocked = service_state ? service_state->blocked : false,
      .suspended = service_state ? service_state->suspended : false,
      .unhealthy = service_state ? service_state->unhealthy : false,
      .registered = service_state ? service_state->registered : false,
      .faulted_group = group_state ? group_state->faulted : false,
      .quarantined_thread_count =
          group_state ? count_quarantined_threads(state, group_state->id) : 0,
      .pending_fault_count = group_state ? group_state->pending_fault_count : 0,
      .requests = service_state ? service_state->requests : 0,
      .rejected_requests = service_state ? service_state->rejected_requests : 0,
      .state_transitions = service_state ? service_state->state_transitions : 0,
      .last_transition_kind =
          service_state ? service_state->last_transition_kind : std::nullopt,
      .last_transition_sequence =
          service_state ? service_state->last_transition_sequence : std::nullopt,
  };
}

KernelSupervisorServiceInventoryView make_supervisor_services_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  return KernelSupervisorServiceInventoryView{
      .supervisor_id = supervisor_state ? supervisor_state->id : supervisor_id,
      .service_count = supervisor_state ? supervisor_state->managed_services.size() : 0,
      .service_lifecycle_transitions =
          supervisor_state ? supervisor_state->service_lifecycle_transitions : 0,
      .last_service_transition_id =
          supervisor_state ? supervisor_state->last_service_transition_id : std::nullopt,
      .last_service_transition_kind =
          supervisor_state ? supervisor_state->last_service_transition_kind : std::nullopt,
      .last_service_transition_sequence = supervisor_state
                                              ? supervisor_state->last_service_transition_sequence
                                              : std::nullopt,
      .service_ids = {},
      .services = {},
  };
}

KernelSupervisorServiceInventoryView build_supervisor_services_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  auto view = make_supervisor_services_view(state, supervisor_id);
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  if (!supervisor_state) {
    return view;
  }
  for (auto service_id : supervisor_state->managed_services) {
    const auto* service_state = state.find_service(service_id);
    if (!service_state || !service_state->registered) {
      continue;
    }
    view.service_ids.push_back(service_id);
    view.services.push_back(KernelSupervisorServiceEntryView{
        .id = service_state->id,
        .name = service_state->name,
        .process_group_id = service_state->process_group_id,
        .address_space_id =
            state.find_process_group_address_space(service_state->process_group_id),
        .owned_page_count = [&]() -> std::size_t {
          const auto address_space_id =
              state.find_process_group_address_space(service_state->process_group_id);
          return address_space_id.has_value()
                     ? count_mapped_pages_for_address_space(state, *address_space_id)
                     : 0;
        }(),
        .pager_needed = [&]() -> bool {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_needed : false;
        }(),
        .pager_handoff_pending = [&]() -> bool {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_handoff_pending : false;
        }(),
        .pending_pager_fault_count = [&]() -> std::size_t {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pending_pager_fault_count : 0;
        }(),
        .pager_faults = [&]() -> uint64_t {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_faults : 0;
        }(),
        .pager_handoffs = [&]() -> uint64_t {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_handoffs : 0;
        }(),
        .last_pager_fault = [&]() -> std::optional<KernelFaultRecord> {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->last_pager_fault : std::nullopt;
        }(),
        .blocked = service_state->blocked,
        .suspended = service_state->suspended,
        .unhealthy = service_state->unhealthy,
        .registered = service_state->registered,
        .requests = service_state->requests,
        .rejected_requests = service_state->rejected_requests,
        .state_transitions = service_state->state_transitions,
        .last_transition_kind = service_state->last_transition_kind,
        .last_transition_sequence = service_state->last_transition_sequence,
    });
    if (service_state->blocked) {
      ++view.blocked_service_count;
    }
    if (service_state->suspended) {
      ++view.suspended_service_count;
    }
    if (service_state->unhealthy) {
      ++view.unhealthy_service_count;
    }
    view.total_service_requests += service_state->requests;
    view.total_service_rejections += service_state->rejected_requests;
  }
  view.service_count = view.service_ids.size();
  return view;
}

KernelFaultSummaryView make_fault_summary_view(const KernelRuntimeState& state) {
  const auto latest_service_transition = latest_service_transition_view(state);
  const auto latest_pager_fault = latest_pager_fault_view(state);
  return KernelFaultSummaryView{
      .recorded_faults = state.fault_count(),
      .pending_faults = state.pending_fault_count(),
      .delivered_faults = static_cast<std::size_t>(state.counters.faults_delivered),
      .routed_thread_faults =
          static_cast<std::size_t>(state.counters.faults_routed_to_threads),
      .quarantined_threads =
          static_cast<std::size_t>(state.counters.thread_quarantines),
      .audit_events = state.audit_count(),
      .pager_eligible_faults = state.counters.pager_eligible_faults,
      .policy_faults = state.counters.policy_faults,
      .pager_needed_address_spaces = count_pager_needed_address_spaces(state),
      .pending_pager_handoffs = count_pending_pager_handoff_address_spaces(state),
      .pager_handoffs_dispatched = state.counters.pager_handoffs_dispatched,
      .service_lifecycle_transitions =
          [&state]() {
            uint64_t total = 0;
            for (const auto& [_, supervisor_state] : state.supervisors) {
              total += supervisor_state.service_lifecycle_transitions;
            }
            return total;
          }(),
      .last_delivered_fault = state.last_delivered_fault,
      .last_pager_address_space_id = latest_pager_fault.address_space_id,
      .last_pager_fault = latest_pager_fault.fault,
      .last_pager_handoff = state.last_pager_handoff,
      .last_audit_event = state.last_audit_event,
      .last_service_transition_id = latest_service_transition.service_id,
      .last_service_transition_kind = latest_service_transition.kind,
      .last_service_transition_sequence = latest_service_transition.sequence,
  };
}

KernelAuditSummaryView make_audit_summary_view(const KernelRuntimeState& state) {
  const auto latest_service_transition = latest_service_transition_view(state);
  KernelAuditSummaryView view{
      .audit_events = state.audit_count(),
      .fault_deliveries = state.counters.faults_delivered,
      .thread_quarantines = state.counters.thread_quarantines,
      .process_group_fault_entries = state.counters.process_group_fault_entries,
      .supervisor_notifications = state.counters.supervisor_fault_notifications,
      .thread_acknowledgements = state.counters.thread_fault_acknowledgements,
      .process_group_acknowledgements = state.counters.process_group_acknowledgements,
      .supervisor_acknowledgements = state.counters.supervisor_acknowledgements,
      .thread_recoveries = state.counters.thread_fault_recoveries,
      .service_lifecycle_transitions = runtime_service_summary(state).service_lifecycle_transitions,
      .last_service_transition_id = latest_service_transition.service_id,
      .last_service_transition_kind = latest_service_transition.kind,
      .last_service_transition_sequence = latest_service_transition.sequence,
  };
  for (const auto& record : state.audit_log) {
    view.recent_events.push_back(record);
  }
  return view;
}

KernelDeviceSummaryView make_device_summary_view(const KernelRuntimeState& state) {
  const auto latest_service_transition = latest_service_transition_view(state);
  KernelDeviceSummaryView view{
      .has_device_arbitration = state.device_arbitration.has_value(),
      .device_count = state.device_arbitration ? state.device_arbitration->devices.size() : 0,
      .claimed_device_count = count_claimed_devices(state),
      .has_storage = state.device_arbitration ? state.device_arbitration->has_storage : false,
      .has_network = state.device_arbitration ? state.device_arbitration->has_network : false,
      .has_display = state.device_arbitration ? state.device_arbitration->has_display : false,
      .service_lifecycle_transitions = runtime_service_summary(state).service_lifecycle_transitions,
      .last_service_transition_id = latest_service_transition.service_id,
      .last_service_transition_kind = latest_service_transition.kind,
      .last_service_transition_sequence = latest_service_transition.sequence,
  };
  if (state.device_arbitration) {
    for (const auto& device : state.device_arbitration->devices) {
      view.devices.push_back(KernelDeviceOwnershipView{
          .name = device.name,
          .claimed = device.owner_tid.has_value(),
          .owner_tid = device.owner_tid,
          .irq = device.irq,
      });
    }
  }
  return view;
}

std::optional<KernelServiceStatus> validate_requesting_group(
    const KernelRuntimeState& state,
    const KernelServiceRequest& request) {
  if (!request.requesting_process_group_id.has_value()) {
    return std::nullopt;
  }
  const auto* requesting_group = state.find_process_group(*request.requesting_process_group_id);
  if (!requesting_group) {
    return KernelServiceStatus::NotFound;
  }
  if (requesting_group->faulted) {
    return KernelServiceStatus::FaultedGroup;
  }
  return std::nullopt;
}

std::optional<KernelServiceStatus> validate_requesting_group(
    const KernelRuntimeState& state,
    const KernelServiceAction& action) {
  if (!action.requesting_process_group_id.has_value()) {
    return std::nullopt;
  }
  const auto* requesting_group = state.find_process_group(*action.requesting_process_group_id);
  if (!requesting_group) {
    return KernelServiceStatus::NotFound;
  }
  if (requesting_group->faulted) {
    return KernelServiceStatus::FaultedGroup;
  }
  return std::nullopt;
}

const KernelRuntimeState::ServiceState* validate_service_request(
    KernelRuntimeState& state,
    KernelServiceResult& result,
    const KernelServiceRequest& request) {
  if (!request.service_id.has_value()) {
    result.status = KernelServiceStatus::InvalidRequest;
    result.rejection = KernelServiceRequestRejection::MissingService;
    return nullptr;
  }
  auto* service_state = state.find_service_mut(*request.service_id);
  if (!service_state || !service_state->registered) {
    result.status = KernelServiceStatus::NotFound;
    result.rejection = KernelServiceRequestRejection::MissingService;
    return nullptr;
  }
  ++service_state->requests;
  if (request.requesting_process_group_id.has_value()) {
    const auto supervisor_id =
        state.find_process_group_supervisor(*request.requesting_process_group_id);
    if (!supervisor_id.has_value() || *supervisor_id != service_state->supervisor_id) {
      ++service_state->rejected_requests;
      result.status = KernelServiceStatus::InvalidRequest;
      result.rejection = KernelServiceRequestRejection::MissingSupervisor;
      return nullptr;
    }
  }
  if (service_state->blocked) {
    ++service_state->rejected_requests;
    result.status = KernelServiceStatus::FaultedGroup;
    result.rejection = KernelServiceRequestRejection::FaultedRequestingGroup;
    return nullptr;
  }
  if (service_state->unhealthy) {
    ++service_state->rejected_requests;
    result.status = KernelServiceStatus::ServiceUnavailable;
    result.rejection = KernelServiceRequestRejection::UnhealthyService;
    return nullptr;
  }
  return service_state;
}

}  // namespace

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept {
  if (ctx.memory_map.empty()) {
    return std::nullopt;
  }

  std::size_t memory_region_count = ctx.memory_map.size();
  uint64_t total_ternary_pages = 0;
  bool has_writable_memory = false;

  for (const auto& region : ctx.memory_map) {
    total_ternary_pages += region.ternary_page_count();
    has_writable_memory = has_writable_memory || region.writable;
  }

  if (!has_writable_memory) {
    return std::nullopt;
  }

  KernelRuntimeState state{
      ctx.platform_id,
      memory_region_count,
      total_ternary_pages,
      has_writable_memory,
      mmu::TernaryPageAllocator(ctx.memory_map),
  };
  state.ipc_bus.register_thread(KernelRuntimeState::kKernelTid);
  state.thread_runtime.emplace(
      KernelRuntimeState::kKernelTid,
      KernelRuntimeState::ThreadRuntimeState{
          .tid = KernelRuntimeState::kKernelTid,
          .process_group_id = KernelRuntimeState::kKernelProcessGroup,
      });
  state.process_groups.emplace(
      KernelRuntimeState::kKernelProcessGroup,
      KernelRuntimeState::ProcessGroupState{
          .id = KernelRuntimeState::kKernelProcessGroup,
          .member_tids = {KernelRuntimeState::kKernelTid},
      });
  state.address_spaces.emplace(
      KernelRuntimeState::kKernelAddressSpace,
      KernelRuntimeState::AddressSpaceState{
          .id = KernelRuntimeState::kKernelAddressSpace,
          .process_group_id = KernelRuntimeState::kKernelProcessGroup,
      });
  state.supervisors.emplace(
      KernelRuntimeState::kKernelSupervisor,
      KernelRuntimeState::SupervisorState{
          .id = KernelRuntimeState::kKernelSupervisor,
          .managed_groups = {KernelRuntimeState::kKernelProcessGroup},
      });
  state.process_group_supervisors.emplace(KernelRuntimeState::kKernelProcessGroup,
                                          KernelRuntimeState::kKernelSupervisor);
  state.process_group_address_spaces.emplace(KernelRuntimeState::kKernelProcessGroup,
                                             KernelRuntimeState::kKernelAddressSpace);
  state.device_arbitration = bootstrap_device_arbitration(ctx.platform_id);
  return state;
}

KernelAccessReport axion_kernel_check_access(
    KernelRuntimeState& state,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept {
  const auto result = mmu::mmu_translate_checked(state.page_table, tva, mode);
  if (result.fault == mmu::MmuFault::None) {
    return {.phys_addr = result.phys_addr, .fault = std::nullopt};
  }

  record_fault(state, tva, mode, result.fault);
  return {
      .phys_addr = std::nullopt,
      .fault = KernelFaultRecord{
          .platform_id = state.platform_id,
          .tva = tva,
          .access_mode = mode,
      .fault = result.fault,
          .subject_tid = state.scheduler.current_tid(),
      },
  };
}

std::optional<sched::Tid> axion_kernel_spawn_thread(
    KernelRuntimeState& state,
    sched::TiscContext ctx) noexcept {
  auto* group = create_process_group(state);
  auto* supervisor = create_supervisor(state);
  auto* address_space = group ? create_address_space(state, group->id) : nullptr;
  if (!group || !supervisor || !address_space) {
    if (group) {
      state.process_groups.erase(group->id);
    }
    if (supervisor) {
      state.supervisors.erase(supervisor->id);
    }
    if (address_space) {
      state.address_spaces.erase(address_space->id);
    }
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (tid.has_value()) {
    state.ipc_bus.register_thread(*tid);
    state.thread_runtime.emplace(*tid,
                                 KernelRuntimeState::ThreadRuntimeState{
                                     .tid = *tid,
                                     .process_group_id = group->id,
                                 });
    assign_thread_to_group(state, *tid, group->id);
    assign_group_to_supervisor(state, group->id, supervisor->id);
    assign_group_to_address_space(state, group->id, address_space->id);
  } else {
    state.process_groups.erase(group->id);
    state.supervisors.erase(supervisor->id);
    state.address_spaces.erase(address_space->id);
  }
  return tid;
}

std::optional<sched::Tid> axion_kernel_spawn_thread_in_group(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    ProcessGroupId process_group_id) noexcept {
  if (!state.find_process_group(process_group_id)) {
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (tid.has_value()) {
    state.ipc_bus.register_thread(*tid);
    state.thread_runtime.emplace(*tid,
                                 KernelRuntimeState::ThreadRuntimeState{
                                     .tid = *tid,
                                     .process_group_id = process_group_id,
                                 });
    assign_thread_to_group(state, *tid, process_group_id);
  }
  return tid;
}

std::optional<sched::Tid> axion_kernel_spawn_thread_under_supervisor(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    SupervisorId supervisor_id) noexcept {
  if (!state.find_supervisor(supervisor_id)) {
    return std::nullopt;
  }
  auto* group = create_process_group(state);
  auto* address_space = group ? create_address_space(state, group->id) : nullptr;
  if (!group || !address_space) {
    if (group) {
      state.process_groups.erase(group->id);
    }
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (!tid.has_value()) {
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  state.ipc_bus.register_thread(*tid);
  state.thread_runtime.emplace(*tid,
                               KernelRuntimeState::ThreadRuntimeState{
                                   .tid = *tid,
                                   .process_group_id = group->id,
                               });
  assign_thread_to_group(state, *tid, group->id);
  if (!assign_group_to_supervisor(state, group->id, supervisor_id)) {
    state.thread_runtime.erase(*tid);
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  if (!assign_group_to_address_space(state, group->id, address_space->id)) {
    state.thread_runtime.erase(*tid);
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  return tid;
}

bool axion_kernel_tick(KernelRuntimeState& state) noexcept {
  ++state.counters.scheduler_ticks;
  const bool switched = state.scheduler.tick(state.cpu_context);
  if (switched) {
    ++state.counters.scheduler_switches;
  }
  return switched;
}

bool axion_kernel_step(KernelRuntimeState& state) noexcept {
  ++state.counters.loop_iterations;
  bool handled_by_policy = false;
  if (!state.pending_faults.empty()) {
    state.last_delivered_fault = state.pending_faults.front();
    state.pending_faults.pop_front();
    ++state.counters.faults_delivered;

    const sched::Tid subject_tid = state.last_delivered_fault->subject_tid;
    auto* thread_state = [&]() -> KernelRuntimeState::ThreadRuntimeState* {
      auto it = state.thread_runtime.find(subject_tid);
      return it == state.thread_runtime.end() ? nullptr : &it->second;
    }();
    if (thread_state) {
      thread_state->fault_inbox.push_back(*state.last_delivered_fault);
      ++state.counters.faults_routed_to_threads;
      record_audit_event(state,
                         KernelAuditEventKind::FaultDelivered,
                         subject_tid,
                         thread_state->process_group_id,
                         state.last_delivered_fault->fault);

      auto* group_state = state.find_process_group_mut(thread_state->process_group_id);
      if (group_state) {
        record_pager_fault_state(state, group_state->id, *state.last_delivered_fault);
        const bool entering_fault_state = !group_state->faulted;
        group_state->faulted = true;
        group_state->blocked = true;
        group_state->acknowledgement_pending = true;
        ++group_state->pending_fault_count;
        ++group_state->counters.fault_entries;
        ++state.counters.process_group_fault_entries;
        mark_service_blocked(state, group_state->id, true);
        if (entering_fault_state) {
          record_audit_event(state,
                             KernelAuditEventKind::ProcessGroupFaultEntered,
                             subject_tid,
                             group_state->id,
                             state.last_delivered_fault->fault);
        }
        queue_supervisor_pending_group(
            state, group_state->id, subject_tid, state.last_delivered_fault->fault);
      }

      if (subject_tid != KernelRuntimeState::kKernelTid && !thread_state->quarantined) {
        thread_state->quarantined = true;
        ++state.counters.thread_quarantines;
        record_audit_event(state,
                           KernelAuditEventKind::ThreadQuarantined,
                           subject_tid,
                           thread_state->process_group_id,
                           state.last_delivered_fault->fault);
        const bool was_running = state.scheduler.current_tid() == subject_tid;
        if (state.scheduler.sleep(subject_tid, state.cpu_context)) {
          handled_by_policy = was_running;
          ++state.counters.scheduler_ticks;
          if (was_running) {
            ++state.counters.scheduler_switches;
          }
        }
      }
    }
  } else {
    state.last_delivered_fault.reset();
    if (!state.pending_pager_handoffs.empty()) {
      const auto address_space_id = state.pending_pager_handoffs.front();
      state.pending_pager_handoffs.pop_front();
      auto* address_space = state.find_address_space_mut(address_space_id);
      if (address_space && address_space->last_pager_fault.has_value()) {
        address_space->pager_handoff_pending = false;
        ++address_space->pager_handoffs;
        address_space->last_pager_handoff_sequence =
            state.next_pager_handoff_sequence++;
        state.last_pager_handoff = KernelPagerHandoffRecord{
            .address_space_id = address_space_id,
            .process_group_id = address_space->process_group_id,
            .fault = *address_space->last_pager_fault,
            .sequence = *address_space->last_pager_handoff_sequence,
        };
        ++state.counters.pager_handoffs_dispatched;
      }
    }
  }
  if (handled_by_policy) {
    return true;
  }
  return axion_kernel_tick(state);
}

bool axion_kernel_ipc_send(KernelRuntimeState& state,
                           sched::Tid dst,
                           ipc::CanonMessage msg) noexcept {
  const bool sent = state.ipc_bus.ipc_send(dst, std::move(msg));
  if (sent) {
    ++state.counters.ipc_messages_sent;
  }
  return sent;
}

std::optional<ipc::CanonMessage> axion_kernel_ipc_recv(
    KernelRuntimeState& state,
    sched::Tid tid) noexcept {
  auto msg = state.ipc_bus.ipc_recv(tid);
  if (msg.has_value()) {
    ++state.counters.ipc_messages_received;
  }
  return msg;
}

KernelServiceResult axion_kernel_service_request(
    const KernelRuntimeState& state,
    const KernelServiceRequest& request) noexcept {
  KernelServiceResult result;
  switch (request.kind) {
    case KernelServiceRequestKind::RuntimeStatus: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      const auto service_summary = runtime_service_summary(state);
      const auto latest_service_transition = latest_service_transition_view(state);
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.runtime = KernelRuntimeStatusView{
          .platform_id = state.platform_id,
          .memory_region_count = state.memory_region_count,
          .total_ternary_pages = state.total_ternary_pages,
          .address_space_count = state.address_space_count(),
          .mapped_pages = state.page_table.size(),
          .pager_needed_address_space_count = count_pager_needed_address_spaces(state),
          .pending_pager_handoff_count = count_pending_pager_handoff_address_spaces(state),
          .loop_iterations = state.counters.loop_iterations,
          .scheduler_ticks = state.counters.scheduler_ticks,
          .ipc_messages_sent = state.counters.ipc_messages_sent,
          .ipc_messages_received = state.counters.ipc_messages_received,
          .pager_eligible_faults = state.counters.pager_eligible_faults,
          .policy_faults = state.counters.policy_faults,
          .pager_handoffs_dispatched = state.counters.pager_handoffs_dispatched,
          .managed_service_count = service_summary.managed_service_count,
          .blocked_service_count = service_summary.blocked_service_count,
          .suspended_service_count = service_summary.suspended_service_count,
          .unhealthy_service_count = service_summary.unhealthy_service_count,
          .service_lifecycle_transitions = service_summary.service_lifecycle_transitions,
          .last_service_transition_id = latest_service_transition.service_id,
          .last_service_transition_kind = latest_service_transition.kind,
          .last_service_transition_sequence = latest_service_transition.sequence,
      };
      return result;
    }
    case KernelServiceRequestKind::ProcessGroupStatus: {
      if (!request.process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceRequestRejection::MissingProcessGroup;
        return result;
      }
      const auto* group_state = state.find_process_group(*request.process_group_id);
      if (!group_state) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceRequestRejection::MissingProcessGroup;
        return result;
      }
      result.status = group_state->faulted ? KernelServiceStatus::FaultedGroup
                                           : KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.process_group = make_process_group_view(state, group_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorStatus: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      if (!request.supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      const auto* supervisor_state = state.find_supervisor(*request.supervisor_id);
      if (!supervisor_state) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor = make_supervisor_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorRecoveryStatus: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      if (!request.supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      const auto* supervisor_state = state.find_supervisor(*request.supervisor_id);
      if (!supervisor_state) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_recovery =
          make_supervisor_recovery_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::ServiceStatus: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      const auto* service_state = validate_service_request(
          const_cast<KernelRuntimeState&>(state), result, request);
      if (!service_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.service = make_service_view(state, service_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorServiceInventory: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      if (!request.supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      const auto* supervisor_state = state.find_supervisor(*request.supervisor_id);
      if (!supervisor_state) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceRequestRejection::MissingSupervisor;
        return result;
      }
      if (request.requesting_process_group_id.has_value()) {
        const auto requesting_supervisor =
            state.find_process_group_supervisor(*request.requesting_process_group_id);
        if (!requesting_supervisor.has_value() ||
            *requesting_supervisor != supervisor_state->id) {
          result.status = KernelServiceStatus::InvalidRequest;
          result.rejection = KernelServiceRequestRejection::MissingSupervisor;
          return result;
        }
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_services =
          build_supervisor_services_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::FaultSummary: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.fault_summary = make_fault_summary_view(state);
      return result;
    }
    case KernelServiceRequestKind::AuditSummary: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.audit_summary = make_audit_summary_view(state);
      return result;
    }
    case KernelServiceRequestKind::DeviceSummary: {
      if (auto denied = validate_requesting_group(state, request); denied.has_value()) {
        result.status = *denied;
        result.rejection =
            denied == KernelServiceStatus::NotFound
                ? KernelServiceRequestRejection::MissingRequestingGroup
                : KernelServiceRequestRejection::FaultedRequestingGroup;
        return result;
      }
      if (!state.device_arbitration.has_value()) {
        result.status = KernelServiceStatus::NoDeviceArbitration;
        result.rejection = KernelServiceRequestRejection::MissingDeviceArbitration;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.device_summary = make_device_summary_view(state);
      return result;
    }
  }
  result.status = KernelServiceStatus::InvalidRequest;
  return result;
}

KernelServiceActionResult axion_kernel_service_action(
    KernelRuntimeState& state,
    const KernelServiceAction& action) noexcept {
  KernelServiceActionResult result;
  switch (action.kind) {
    case KernelServiceActionKind::AcknowledgeSupervisorFaultGroup: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!action.supervisor_id.has_value() || !action.process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = !action.supervisor_id.has_value()
                               ? KernelServiceActionRejection::MissingSupervisor
                               : KernelServiceActionRejection::MissingProcessGroup;
        return result;
      }
      if (!state.find_supervisor(*action.supervisor_id)) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingSupervisor;
        return result;
      }
      if (!state.find_process_group(*action.process_group_id)) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingProcessGroup;
        return result;
      }
      if (!axion_kernel_ack_supervisor_group_fault(
              state, *action.supervisor_id, *action.process_group_id)) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = group_has_pending_thread_faults(
                               state, *action.process_group_id)
                               ? KernelServiceActionRejection::SupervisorGatePendingThreadFault
                               : KernelServiceActionRejection::SupervisorGroupNotPending;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.process_group_id);
      result.supervisor = make_supervisor_view(state, *action.supervisor_id);
      result.supervisor_recovery =
          make_supervisor_recovery_view(state, *action.supervisor_id);
      result.fault_summary = make_fault_summary_view(state);
      return result;
    }
    case KernelServiceActionKind::ClaimDevice:
    case KernelServiceActionKind::ReleaseDevice: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!state.device_arbitration.has_value()) {
        result.status = KernelServiceStatus::NoDeviceArbitration;
        result.rejection = KernelServiceActionRejection::MissingDeviceArbitration;
        return result;
      }
      if (!action.requesting_process_group_id.has_value() ||
          !action.device_name.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = !action.requesting_process_group_id.has_value()
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::MissingDeviceName;
        return result;
      }
      const auto* group_state =
          state.find_process_group(*action.requesting_process_group_id);
      if (!group_state) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingRequestingGroup;
        return result;
      }
      const auto owner_tid =
          primary_tid_for_group(state, *action.requesting_process_group_id);
      if (!owner_tid.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::NoPrimaryThread;
        return result;
      }

      const bool ok =
          action.kind == KernelServiceActionKind::ClaimDevice
              ? axion_kernel_claim_device(state, *action.device_name, *owner_tid)
              : axion_kernel_release_device(state, *action.device_name, *owner_tid);
      if (!ok) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection =
            action.kind == KernelServiceActionKind::ClaimDevice
                ? KernelServiceActionRejection::DeviceConflict
                : KernelServiceActionRejection::DeviceNotOwned;
        return result;
      }

      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.requesting_process_group_id);
      result.device_summary = make_device_summary_view(state);
      return result;
    }
    case KernelServiceActionKind::RegisterService: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!action.requesting_process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingRequestingGroup;
        return result;
      }
      if (!action.service_name.has_value() || action.service_name->empty()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingServiceName;
        return result;
      }
      const auto supervisor_id =
          state.find_process_group_supervisor(*action.requesting_process_group_id);
      if (!supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingSupervisor;
        return result;
      }
      if (action.supervisor_id.has_value() && *action.supervisor_id != *supervisor_id) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::ServiceSupervisorMismatch;
        return result;
      }
      if (state.find_process_group_service(*action.requesting_process_group_id).has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::DuplicateService;
        return result;
      }
      auto* service_state = create_service(
          state, *action.service_name, *action.requesting_process_group_id, *supervisor_id);
      if (!service_state) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::DuplicateService;
        return result;
      }
      record_supervisor_service_transition(state,
                                           service_state->id,
                                           *supervisor_id,
                                           service_state->process_group_id,
                                           KernelAuditEventKind::ServiceRegistered);
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.requesting_process_group_id);
      result.service = make_service_view(state, service_state->id);
      result.supervisor_services =
          build_supervisor_services_view(state, *supervisor_id);
      return result;
    }
    case KernelServiceActionKind::UnregisterService: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!action.requesting_process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingRequestingGroup;
        return result;
      }
      if (!action.service_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      auto* service_state = state.find_service_mut(*action.service_id);
      if (!service_state || !service_state->registered) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      if (service_state->process_group_id != *action.requesting_process_group_id) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::ServiceProcessGroupMismatch;
        return result;
      }
      const auto supervisor_id =
          state.find_process_group_supervisor(*action.requesting_process_group_id);
      if (!supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingSupervisor;
        return result;
      }
      if (service_state->supervisor_id != *supervisor_id) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::ServiceSupervisorMismatch;
        return result;
      }
      service_state->registered = false;
      service_state->blocked = false;
      service_state->suspended = false;
      ++service_state->state_transitions;
      state.process_group_services.erase(service_state->process_group_id);
      if (auto* supervisor_state = state.find_supervisor_mut(*supervisor_id)) {
        supervisor_state->managed_services.erase(
            std::remove(supervisor_state->managed_services.begin(),
                        supervisor_state->managed_services.end(),
                        service_state->id),
            supervisor_state->managed_services.end());
      }
      record_supervisor_service_transition(state,
                                           service_state->id,
                                           *supervisor_id,
                                           service_state->process_group_id,
                                           KernelAuditEventKind::ServiceUnregistered);
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.requesting_process_group_id);
      result.service = make_service_view(state, service_state->id);
      result.supervisor_services =
          build_supervisor_services_view(state, *supervisor_id);
      return result;
    }
    case KernelServiceActionKind::SuspendService:
    case KernelServiceActionKind::ResumeService: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!action.requesting_process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingRequestingGroup;
        return result;
      }
      if (!action.service_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      auto* service_state = state.find_service_mut(*action.service_id);
      if (!service_state || !service_state->registered) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      const auto requester_supervisor_id =
          state.find_process_group_supervisor(*action.requesting_process_group_id);
      if (!requester_supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingSupervisor;
        return result;
      }
      const auto supervisor_id =
          state.find_process_group_supervisor(service_state->process_group_id);
      if (!supervisor_id.has_value() ||
          service_state->supervisor_id != *supervisor_id ||
          *requester_supervisor_id != *supervisor_id) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::ServiceSupervisorMismatch;
        return result;
      }
      const bool suspend = action.kind == KernelServiceActionKind::SuspendService;
      if (!mark_service_suspended(state, *action.service_id, suspend)) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = suspend
                               ? KernelServiceActionRejection::ServiceAlreadySuspended
                               : KernelServiceActionRejection::ServiceNotSuspended;
        return result;
      }
      record_supervisor_service_transition(state,
                                           service_state->id,
                                           *supervisor_id,
                                           service_state->process_group_id,
                                           suspend ? KernelAuditEventKind::ServiceSuspended
                                                   : KernelAuditEventKind::ServiceResumed);
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.requesting_process_group_id);
      result.service = make_service_view(state, *action.service_id);
      result.supervisor_services =
          build_supervisor_services_view(state, *supervisor_id);
      return result;
    }
    case KernelServiceActionKind::MarkServiceUnhealthy:
    case KernelServiceActionKind::MarkServiceHealthy: {
      if (auto denied = validate_requesting_group(state, action); denied.has_value()) {
        result.status = *denied;
        result.rejection = denied == KernelServiceStatus::NotFound
                               ? KernelServiceActionRejection::MissingRequestingGroup
                               : KernelServiceActionRejection::FaultedRequestingGroup;
        return result;
      }
      if (!action.requesting_process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingRequestingGroup;
        return result;
      }
      if (!action.service_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      auto* service_state = state.find_service_mut(*action.service_id);
      if (!service_state || !service_state->registered) {
        result.status = KernelServiceStatus::NotFound;
        result.rejection = KernelServiceActionRejection::MissingService;
        return result;
      }
      const auto requester_supervisor_id =
          state.find_process_group_supervisor(*action.requesting_process_group_id);
      if (!requester_supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::MissingSupervisor;
        return result;
      }
      const auto supervisor_id =
          state.find_process_group_supervisor(service_state->process_group_id);
      if (!supervisor_id.has_value() ||
          service_state->supervisor_id != *supervisor_id ||
          *requester_supervisor_id != *supervisor_id) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = KernelServiceActionRejection::ServiceSupervisorMismatch;
        return result;
      }
      const bool unhealthy = action.kind == KernelServiceActionKind::MarkServiceUnhealthy;
      if (!mark_service_unhealthy(state, *action.service_id, unhealthy)) {
        result.status = KernelServiceStatus::InvalidRequest;
        result.rejection = unhealthy
                               ? KernelServiceActionRejection::ServiceAlreadyUnhealthy
                               : KernelServiceActionRejection::ServiceAlreadyHealthy;
        return result;
      }
      record_supervisor_service_transition(state,
                                           service_state->id,
                                           *supervisor_id,
                                           service_state->process_group_id,
                                           unhealthy ? KernelAuditEventKind::ServiceMarkedUnhealthy
                                                     : KernelAuditEventKind::ServiceMarkedHealthy);
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceActionRejection::None;
      result.action_performed = true;
      result.process_group =
          make_process_group_view(state, *action.requesting_process_group_id);
      result.service = make_service_view(state, *action.service_id);
      result.supervisor_services =
          build_supervisor_services_view(state, *supervisor_id);
      return result;
    }
  }
  result.status = KernelServiceStatus::InvalidRequest;
  return result;
}

bool axion_kernel_claim_device(KernelRuntimeState& state,
                               std::string_view device_name,
                               sched::Tid owner) noexcept {
  auto* device = find_device(state, device_name);
  if (!device) {
    return false;
  }
  if (device->owner_tid.has_value() && *device->owner_tid != owner) {
    return false;
  }
  device->owner_tid = owner;
  return true;
}

bool axion_kernel_release_device(KernelRuntimeState& state,
                                 std::string_view device_name,
                                 sched::Tid owner) noexcept {
  auto* device = find_device(state, device_name);
  if (!device || !device->owner_tid.has_value() || *device->owner_tid != owner) {
    return false;
  }
  device->owner_tid.reset();
  return true;
}

bool axion_kernel_ack_thread_fault(KernelRuntimeState& state,
                                   sched::Tid tid) noexcept {
  auto* thread_state = state.find_thread_runtime_mut(tid);
  if (!thread_state || thread_state->fault_inbox.empty()) {
    return false;
  }

  thread_state->fault_inbox.pop_front();
  ++state.counters.thread_fault_acknowledgements;
  record_audit_event(state,
                     KernelAuditEventKind::ThreadFaultAcknowledged,
                     tid,
                     thread_state->process_group_id);

  if (thread_state->fault_inbox.empty()) {
    auto* group_state = state.find_process_group_mut(thread_state->process_group_id);
    if (group_state && group_state->pending_fault_count > 0) {
      --group_state->pending_fault_count;
      if (group_state->pending_fault_count == 0 && !group_state->acknowledgement_pending) {
        group_state->faulted = false;
        group_state->blocked = false;
      }
    }
    maybe_recover_thread(state, *thread_state);
  }
  return true;
}

bool axion_kernel_ack_process_group_fault(KernelRuntimeState& state,
                                          ProcessGroupId process_group_id) noexcept {
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!group_state || !group_state->faulted || !group_state->acknowledgement_pending ||
      group_state->pending_fault_count != 0) {
    return false;
  }

  group_state->acknowledgement_pending = false;
  ++state.counters.process_group_acknowledgements;
  ++group_state->counters.acknowledgements;
  record_audit_event(state,
                     KernelAuditEventKind::ProcessGroupAcknowledged,
                     KernelRuntimeState::kKernelTid,
                     process_group_id);

  if (group_state->pending_fault_count == 0) {
    group_state->faulted = false;
    group_state->blocked = false;
  }

  bool recovered_any = false;
  for (auto tid : group_state->member_tids) {
    auto* thread_state = state.find_thread_runtime_mut(tid);
    if (thread_state) {
      recovered_any = maybe_recover_thread(state, *thread_state) || recovered_any;
    }
  }
  return true;
}

bool axion_kernel_ack_supervisor_group_fault(KernelRuntimeState& state,
                                             SupervisorId supervisor_id,
                                             ProcessGroupId process_group_id) noexcept {
  auto* supervisor_state = state.find_supervisor_mut(supervisor_id);
  if (!supervisor_state) {
    return false;
  }
  const auto mapped_supervisor = state.find_process_group_supervisor(process_group_id);
  if (!mapped_supervisor.has_value() || *mapped_supervisor != supervisor_id) {
    return false;
  }
  auto pending_it = supervisor_state->pending_groups.end();
  for (auto it = supervisor_state->pending_groups.begin();
       it != supervisor_state->pending_groups.end();
       ++it) {
    if (*it == process_group_id) {
      pending_it = it;
      break;
    }
  }
  if (pending_it == supervisor_state->pending_groups.end()) {
    return false;
  }
  ++supervisor_state->acknowledgements;
  supervisor_state->last_acknowledged_group = process_group_id;
  ++state.counters.supervisor_acknowledgements;
  record_audit_event(state,
                     KernelAuditEventKind::SupervisorGroupAcknowledged,
                     KernelRuntimeState::kKernelTid,
                     process_group_id);
  if (!axion_kernel_ack_process_group_fault(state, process_group_id)) {
    --supervisor_state->acknowledgements;
    supervisor_state->last_acknowledged_group.reset();
    --state.counters.supervisor_acknowledgements;
    state.audit_log.pop_back();
    state.last_audit_event =
        state.audit_log.empty() ? std::nullopt : std::optional<KernelAuditRecord>(state.audit_log.back());
    --state.counters.audit_events_recorded;
    return false;
  }
  supervisor_state = state.find_supervisor_mut(supervisor_id);
  if (!supervisor_state) {
    return false;
  }
  supervisor_state->pending_groups.erase(pending_it);
  ++supervisor_state->recovered_groups;
  supervisor_state->last_recovered_group = process_group_id;
  return true;
}

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
