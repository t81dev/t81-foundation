#include "kernel_main.hpp"

#include <algorithm>

namespace t81::ternaryos::kernel {

namespace {

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

bool capability_matches(const KernelCapabilityRecord& capability,
                        KernelCapabilityKind capability_kind,
                        std::optional<ProcessGroupId> process_group_scope) {
  return capability.kind == capability_kind &&
         capability.process_group_scope == process_group_scope;
}

KernelCapabilityRecord issued_capability_record(
    KernelRuntimeState& state,
    const KernelCapabilityRecord& capability) {
  auto issued = capability;
  if (issued.record_id == 0) {
    issued.record_id = state.next_capability_record_id++;
  }
  return issued;
}

void record_capability_transition(KernelRuntimeState& state,
                                  ProcessGroupId process_group_id,
                                  KernelAuditEventKind kind,
                                  const KernelCapabilityRecord& capability) {
  const auto subject_tid =
      axion_kernel_primary_tid_for_group(state, process_group_id)
          .value_or(KernelRuntimeState::kKernelTid);
  record_audit_event(state, kind, subject_tid, process_group_id, mmu::MmuFault::None);
  const auto supervisor_id = state.find_process_group_supervisor(process_group_id);
  if (!supervisor_id.has_value() || !state.last_audit_event.has_value()) {
    return;
  }
  auto* supervisor_state = state.find_supervisor_mut(*supervisor_id);
  if (!supervisor_state) {
    return;
  }
  ++supervisor_state->capability_transitions;
  supervisor_state->last_capability_transition_group_id = process_group_id;
  supervisor_state->last_capability_transition_record_id = capability.record_id;
  supervisor_state->last_capability_transition_kind = kind;
  supervisor_state->last_capability_transition_sequence =
      state.last_audit_event->sequence;
  if (supervisor_state->recent_capability_transitions.size() >=
      KernelRuntimeState::SupervisorState::kMaxCapabilityTransitionHistory) {
    supervisor_state->recent_capability_transitions.pop_front();
  }
  supervisor_state->recent_capability_transitions.push_back(
      KernelCapabilityTransitionRecord{
          .process_group_id = process_group_id,
          .record_id = capability.record_id,
          .kind = kind,
          .sequence = state.last_audit_event->sequence,
          .kernel_seeded = capability.kernel_seeded,
          .delegated_by_process_group_id = capability.delegated_by_process_group_id,
          .delegated_by_supervisor_id = capability.delegated_by_supervisor_id,
      });
}

}  // namespace

std::optional<sched::Tid> axion_kernel_primary_tid_for_group(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) noexcept {
  const auto* group = state.find_process_group(process_group_id);
  if (!group || group->member_tids.empty()) {
    return std::nullopt;
  }
  return *std::min_element(group->member_tids.begin(), group->member_tids.end());
}

void record_audit_event(KernelRuntimeState& state,
                        KernelAuditEventKind kind,
                        sched::Tid subject_tid,
                        ProcessGroupId process_group_id,
                        mmu::MmuFault fault) {
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

std::optional<KernelServiceStatus> axion_kernel_validate_requesting_group(
    const KernelRuntimeState& state,
    std::optional<ProcessGroupId> requesting_process_group_id) noexcept {
  if (!requesting_process_group_id.has_value()) {
    return std::nullopt;
  }
  const auto* requesting_group = state.find_process_group(*requesting_process_group_id);
  if (!requesting_group) {
    return KernelServiceStatus::NotFound;
  }
  if (requesting_group->faulted) {
    return KernelServiceStatus::FaultedGroup;
  }
  return std::nullopt;
}

bool axion_kernel_terminate_thread(KernelRuntimeState& state,
                                   sched::Tid tid) noexcept {
  auto* thread_state = state.find_thread_runtime_mut(tid);
  if (!thread_state) {
    return false;
  }

  const bool was_current = state.scheduler.current_tid() == tid;
  if (was_current && state.scheduler.thread_count() > 1) {
    (void)axion_kernel_tick(state);
  }

  if (!state.scheduler.terminate(tid)) {
    return false;
  }
  state.ipc_bus.deregister_thread(tid);

  if (auto* group_state = state.find_process_group_mut(thread_state->process_group_id)) {
    group_state->member_tids.erase(
        std::remove(group_state->member_tids.begin(), group_state->member_tids.end(), tid),
        group_state->member_tids.end());
  }

  state.thread_runtime.erase(tid);
  return true;
}

KernelServiceRequestRejection axion_kernel_requesting_group_request_rejection(
    KernelServiceStatus status) noexcept {
  return status == KernelServiceStatus::NotFound
             ? KernelServiceRequestRejection::MissingRequestingGroup
             : KernelServiceRequestRejection::FaultedRequestingGroup;
}

KernelServiceActionRejection axion_kernel_requesting_group_action_rejection(
    KernelServiceStatus status) noexcept {
  return status == KernelServiceStatus::NotFound
             ? KernelServiceActionRejection::MissingRequestingGroup
             : KernelServiceActionRejection::FaultedRequestingGroup;
}

bool axion_kernel_process_groups_share_supervisor(
    const KernelRuntimeState& state,
    ProcessGroupId lhs_process_group_id,
    ProcessGroupId rhs_process_group_id) noexcept {
  const auto lhs_supervisor = state.find_process_group_supervisor(lhs_process_group_id);
  const auto rhs_supervisor = state.find_process_group_supervisor(rhs_process_group_id);
  return lhs_supervisor.has_value() && rhs_supervisor.has_value() &&
         *lhs_supervisor == *rhs_supervisor;
}

bool axion_kernel_supervisor_matches_process_group(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id,
    ProcessGroupId process_group_id) noexcept {
  const auto process_group_supervisor =
      state.find_process_group_supervisor(process_group_id);
  return process_group_supervisor.has_value() &&
         *process_group_supervisor == supervisor_id;
}

bool axion_kernel_grant_process_group_capability(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<ProcessGroupId> delegated_by_process_group_id,
    std::optional<SupervisorId> delegated_by_supervisor_id,
    const KernelCapabilityRecord& capability) noexcept {
  auto* group = state.find_process_group_mut(process_group_id);
  if (!group) {
    return false;
  }
  for (const auto& existing : group->capabilities) {
    if (capability_matches(existing, capability.kind, capability.process_group_scope)) {
      return true;
    }
  }
  auto granted = capability;
  granted.kernel_seeded = false;
  granted.delegated_by_process_group_id = delegated_by_process_group_id;
  granted.delegated_by_supervisor_id = delegated_by_supervisor_id;
  const auto issued = issued_capability_record(state, granted);
  group->capabilities.push_back(issued);
  record_capability_transition(
      state,
      process_group_id,
      KernelAuditEventKind::CapabilityGranted,
      issued);
  return true;
}

bool axion_kernel_revoke_process_group_capability(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<CapabilityRecordId> capability_record_id,
    KernelCapabilityKind capability_kind,
    std::optional<ProcessGroupId> process_group_scope) noexcept {
  auto* group = state.find_process_group_mut(process_group_id);
  if (!group) {
    return false;
  }
  std::vector<KernelCapabilityRecord> removed_capabilities;
  const auto old_size = group->capabilities.size();
  group->capabilities.erase(
      std::remove_if(group->capabilities.begin(),
                     group->capabilities.end(),
                     [&](const auto& capability) {
                       bool matches = false;
                       if (capability_record_id.has_value()) {
                         matches = capability.record_id == *capability_record_id;
                       } else {
                         matches = capability_matches(capability,
                                                     capability_kind,
                                                     process_group_scope);
                       }
                       if (matches) {
                         removed_capabilities.push_back(capability);
                       }
                       return matches;
                     }),
      group->capabilities.end());
  const bool changed = group->capabilities.size() != old_size;
  if (changed) {
    KernelCapabilityRecord transition_capability{
        .record_id = capability_record_id.value_or(0),
        .kind = capability_kind,
        .process_group_scope = process_group_scope,
    };
    if (capability_record_id.has_value()) {
      const auto removed = std::find_if(
          removed_capabilities.begin(),
          removed_capabilities.end(),
          [&](const auto& capability) {
            return capability.record_id == *capability_record_id;
          });
      if (removed != removed_capabilities.end()) {
        transition_capability = *removed;
      }
    }
    record_capability_transition(
        state,
        process_group_id,
        KernelAuditEventKind::CapabilityRevoked,
        transition_capability);
  }
  return changed;
}

bool axion_kernel_revoke_delegated_process_group_capabilities(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<ProcessGroupId> delegated_by_process_group_id,
    std::optional<SupervisorId> delegated_by_supervisor_id) noexcept {
  auto* group = state.find_process_group_mut(process_group_id);
  if (!group) {
    return false;
  }
  std::vector<KernelCapabilityRecord> removed_capabilities;
  const auto old_size = group->capabilities.size();
  group->capabilities.erase(
      std::remove_if(group->capabilities.begin(),
                     group->capabilities.end(),
                     [&](const auto& capability) {
                       if (capability.kernel_seeded) {
                         return false;
                       }
                       if (delegated_by_process_group_id.has_value() &&
                           capability.delegated_by_process_group_id !=
                               delegated_by_process_group_id) {
                         return false;
                       }
                       if (delegated_by_supervisor_id.has_value() &&
                           capability.delegated_by_supervisor_id !=
                               delegated_by_supervisor_id) {
                         return false;
                       }
                       removed_capabilities.push_back(capability);
                       return true;
                     }),
      group->capabilities.end());
  if (group->capabilities.size() == old_size) {
    return false;
  }
  for (const auto& capability : removed_capabilities) {
    record_capability_transition(
        state,
        process_group_id,
        KernelAuditEventKind::CapabilityRevoked,
        capability);
  }
  return true;
}

std::vector<KernelCapabilityRecord> axion_kernel_list_process_group_capabilities(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) noexcept {
  const auto* group = state.find_process_group(process_group_id);
  if (!group) {
    return {};
  }
  return group->capabilities;
}

}  // namespace t81::ternaryos::kernel
