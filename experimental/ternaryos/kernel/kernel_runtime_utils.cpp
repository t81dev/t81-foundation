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

void record_capability_transition(KernelRuntimeState& state,
                                  ProcessGroupId process_group_id,
                                  KernelAuditEventKind kind) {
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
  supervisor_state->last_capability_transition_kind = kind;
  supervisor_state->last_capability_transition_sequence =
      state.last_audit_event->sequence;
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
  group->capabilities.push_back(capability);
  record_capability_transition(
      state, process_group_id, KernelAuditEventKind::CapabilityGranted);
  return true;
}

bool axion_kernel_revoke_process_group_capability(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    KernelCapabilityKind capability_kind,
    std::optional<ProcessGroupId> process_group_scope) noexcept {
  auto* group = state.find_process_group_mut(process_group_id);
  if (!group) {
    return false;
  }
  const auto old_size = group->capabilities.size();
  group->capabilities.erase(
      std::remove_if(group->capabilities.begin(),
                     group->capabilities.end(),
                     [&](const auto& capability) {
                       return capability_matches(capability,
                                                 capability_kind,
                                                 process_group_scope);
                     }),
      group->capabilities.end());
  const bool changed = group->capabilities.size() != old_size;
  if (changed) {
    record_capability_transition(
        state, process_group_id, KernelAuditEventKind::CapabilityRevoked);
  }
  return changed;
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
