#include "kernel_abi.hpp"

#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

namespace {

sched::TiscContext make_spawn_context(
    const std::optional<KernelThreadSpawnDescriptor>& descriptor) {
  sched::TiscContext ctx{};
  if (!descriptor.has_value()) {
    return ctx;
  }
  ctx.pc = descriptor->pc;
  ctx.sp = descriptor->sp;
  ctx.registers[0] = descriptor->register0;
  ctx.halted = descriptor->halted;
  ctx.active = descriptor->active;
  ctx.label = descriptor->label;
  return ctx;
}

struct CallerContext {
  sched::Tid tid{0};
  ProcessGroupId process_group_id{0};
  KernelRuntimeState::ThreadRuntimeState* thread_state{nullptr};
  KernelRuntimeState::ProcessGroupState* group_state{nullptr};
};

std::optional<CallerContext> find_caller_context(KernelRuntimeState& state) {
  const auto caller_tid = state.scheduler.current_tid();
  auto* thread_state = state.find_thread_runtime_mut(caller_tid);
  if (!thread_state) {
    return std::nullopt;
  }
  auto* group_state = state.find_process_group_mut(thread_state->process_group_id);
  if (!group_state) {
    return std::nullopt;
  }
  return CallerContext{
      .tid = caller_tid,
      .process_group_id = thread_state->process_group_id,
      .thread_state = thread_state,
      .group_state = group_state,
  };
}

bool request_allowed_while_faulted(KernelCallKind kind) {
  return kind == KernelCallKind::ReadFaultInbox ||
         kind == KernelCallKind::AcknowledgeThreadFault ||
         kind == KernelCallKind::QueryProcessGroupMemory ||
         kind == KernelCallKind::QueryCapabilities;
}

bool has_capability(const KernelRuntimeState::ProcessGroupState& group_state,
                    KernelCapabilityKind kind,
                    std::optional<ProcessGroupId> process_group_scope = std::nullopt) {
  for (const auto& capability : group_state.capabilities) {
    if (capability.kind != kind) {
      continue;
    }
    if (!process_group_scope.has_value() ||
        !capability.process_group_scope.has_value() ||
        capability.process_group_scope == process_group_scope) {
      return true;
    }
  }
  return false;
}

KernelCallResult make_missing_caller_result() {
  KernelCallResult result;
  result.status = KernelCallStatus::NotFound;
  result.rejection = KernelCallRejection::MissingCallerThread;
  return result;
}

KernelCallResult make_missing_group_result(sched::Tid caller_tid) {
  KernelCallResult result;
  result.status = KernelCallStatus::NotFound;
  result.rejection = KernelCallRejection::MissingCallerProcessGroup;
  result.caller_tid = caller_tid;
  return result;
}

std::optional<KernelCallResult> validate_caller(KernelRuntimeState& state,
                                                const KernelCallRequest& request,
                                                CallerContext& caller) {
  const auto caller_ctx = find_caller_context(state);
  if (!caller_ctx.has_value()) {
    return make_missing_caller_result();
  }
  caller = *caller_ctx;
  if (!caller.group_state) {
    return make_missing_group_result(caller.tid);
  }
  if (caller.group_state->faulted && !request_allowed_while_faulted(request.kind)) {
    KernelCallResult result;
    result.status = KernelCallStatus::FaultedCaller;
    result.rejection = KernelCallRejection::FaultedCaller;
    result.caller_tid = caller.tid;
    result.caller_process_group_id = caller.process_group_id;
    return result;
  }
  return std::nullopt;
}

KernelCallResult init_result(const CallerContext& caller) {
  KernelCallResult result;
  result.caller_tid = caller.tid;
  result.caller_process_group_id = caller.process_group_id;
  return result;
}

std::optional<KernelCallResult> require_capability(
    const CallerContext& caller,
    KernelCapabilityKind kind,
    std::optional<ProcessGroupId> process_group_scope = std::nullopt) {
  if (has_capability(*caller.group_state, kind, process_group_scope)) {
    return std::nullopt;
  }
  KernelCallResult result = init_result(caller);
  result.status = KernelCallStatus::CapabilityDenied;
  result.rejection = KernelCallRejection::MissingCapability;
  return result;
}

std::optional<KernelCallResult> resolve_fault_target(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::ThreadRuntimeState*& target_thread_state) {
  const auto target_tid = request.target_tid.value_or(caller.tid);
  target_thread_state = state.find_thread_runtime_mut(target_tid);
  if (!target_thread_state) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingTargetThread;
    return result;
  }
  if (target_thread_state->process_group_id != caller.process_group_id) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::CapabilityDenied;
    result.rejection = KernelCallRejection::CrossProcessGroupTarget;
    return result;
  }
  return std::nullopt;
}

std::optional<KernelCallResult> resolve_thread_query_target(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::ThreadRuntimeState*& target_thread_state,
    const sched::TiscContext*& target_context) {
  const auto target_tid = request.target_tid.value_or(caller.tid);
  target_thread_state = state.find_thread_runtime_mut(target_tid);
  target_context = state.scheduler.run_queue().find(target_tid);
  if (!target_thread_state || !target_context) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingTargetThread;
    return result;
  }
  if (target_thread_state->process_group_id != caller.process_group_id &&
      !axion_kernel_process_groups_share_supervisor(
          state, caller.process_group_id, target_thread_state->process_group_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignSupervisorScope;
    return result;
  }
  return std::nullopt;
}

std::optional<KernelCallResult> resolve_capability_target_group(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::ProcessGroupState*& target_group_state) {
  if (!request.process_group_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingTargetProcessGroup;
    return result;
  }
  target_group_state = state.find_process_group_mut(*request.process_group_id);
  if (!target_group_state) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingTargetProcessGroup;
    return result;
  }
  if (!axion_kernel_process_groups_share_supervisor(state,
                                                    caller.process_group_id,
                                                    target_group_state->id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::SupervisorMismatch;
    return result;
  }
  return std::nullopt;
}

const KernelCapabilityRecord* find_capability_record(
    const KernelRuntimeState::ProcessGroupState& group_state,
    CapabilityRecordId record_id) {
  for (const auto& capability : group_state.capabilities) {
    if (capability.record_id == record_id) {
      return &capability;
    }
  }
  return nullptr;
}

const KernelCapabilityTransitionRecord* find_capability_transition(
    const KernelRuntimeState::SupervisorState& supervisor_state,
    uint64_t sequence) {
  for (const auto& transition : supervisor_state.recent_capability_transitions) {
    if (transition.sequence == sequence) {
      return &transition;
    }
  }
  return nullptr;
}

std::optional<KernelCallResult> resolve_memory_target_group(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    const KernelRuntimeState::ProcessGroupState*& target_group_state) {
  const auto target_group_id = request.process_group_id.value_or(caller.process_group_id);
  target_group_state = state.find_process_group(target_group_id);
  if (!target_group_state) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingTargetProcessGroup;
    return result;
  }
  if (target_group_id != caller.process_group_id &&
      !axion_kernel_process_groups_share_supervisor(
          state, caller.process_group_id, target_group_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignSupervisorScope;
    return result;
  }
  return std::nullopt;
}

std::optional<KernelCallResult> resolve_supervisor_fault_target(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::ProcessGroupState*& target_group_state) {
  if (!request.supervisor_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!request.process_group_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingTargetProcessGroup;
    return result;
  }
  if (!state.find_supervisor(*request.supervisor_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  target_group_state = state.find_process_group_mut(*request.process_group_id);
  if (!target_group_state) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingTargetProcessGroup;
    return result;
  }
  if (!axion_kernel_supervisor_matches_process_group(
          state, *request.supervisor_id, target_group_state->id) ||
      !axion_kernel_process_groups_share_supervisor(
          state, caller.process_group_id, target_group_state->id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::SupervisorMismatch;
    return result;
  }
  return std::nullopt;
}

std::optional<KernelCallResult> validate_supervisor_query(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request) {
  if (!request.supervisor_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!state.find_supervisor(*request.supervisor_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!axion_kernel_supervisor_matches_process_group(
          state, *request.supervisor_id, caller.process_group_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignSupervisorScope;
    return result;
  }
  return std::nullopt;
}

std::optional<KernelCallResult> validate_global_summary_query(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request) {
  if (caller.process_group_id == KernelRuntimeState::kKernelProcessGroup) {
    return std::nullopt;
  }
  if (!request.supervisor_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!state.find_supervisor(*request.supervisor_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!axion_kernel_supervisor_matches_process_group(
          state, *request.supervisor_id, caller.process_group_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignSupervisorScope;
    return result;
  }
  return std::nullopt;
}

KernelCallResult map_service_action_result(const CallerContext& caller,
                                           const KernelServiceActionResult& action_result);

KernelCallResult map_service_request_result(const CallerContext& caller,
                                            const KernelServiceResult& request_result);

void populate_process_group_result(KernelCallResult& result,
                                   const KernelProcessGroupStatusView& process_group) {
  result.target_process_group_id = process_group.id;
  result.address_space_id = process_group.address_space_id;
  result.process_group_owned_page_count = process_group.owned_page_count;
  result.process_group_pending_fault_count = process_group.pending_fault_count;
  result.process_group_pager_needed = process_group.pager_needed;
  result.process_group_faulted = process_group.faulted;
  result.process_group_blocked = process_group.blocked;
  result.process_group_acknowledgement_pending =
      process_group.acknowledgement_pending;
}

std::optional<KernelCallResult> resolve_owned_address_space(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::AddressSpaceState*& address_space) {
  if (!request.address_space_id.has_value()) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingAddressSpace;
    return result;
  }
  address_space = state.find_address_space_mut(*request.address_space_id);
  if (!address_space) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingAddressSpace;
    return result;
  }
  const auto caller_address_space_id =
      state.find_process_group_address_space(caller.process_group_id);
  if (!caller_address_space_id.has_value() || *caller_address_space_id != address_space->id) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignAddressSpace;
    return result;
  }
  return std::nullopt;
}

KernelCallResult dispatch_service_register(const CallerContext& caller,
                                           KernelRuntimeState& state,
                                           const KernelCallRequest& request) {
  auto result = init_result(caller);
  if (!request.service_name.has_value() || request.service_name->empty()) {
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingServiceName;
    return result;
  }
  return map_service_action_result(
      caller,
      axion_kernel_service_action(
          state,
          KernelServiceAction{
              .kind = KernelServiceActionKind::RegisterService,
              .requesting_process_group_id = caller.process_group_id,
              .service_name = *request.service_name,
          }));
}

KernelCallResult dispatch_service_request(const CallerContext& caller,
                                          KernelRuntimeState& state,
                                          ServiceId service_id) {
  return map_service_request_result(
      caller,
      axion_kernel_service_request(
          state,
          KernelServiceRequest{
              .kind = KernelServiceRequestKind::ServiceStatus,
              .requesting_process_group_id = caller.process_group_id,
              .service_id = service_id,
          }));
}

KernelCallResult dispatch_service_action_with_id(const CallerContext& caller,
                                                 KernelRuntimeState& state,
                                                 const KernelCallRequest& request,
                                                 KernelServiceActionKind kind) {
  auto result = init_result(caller);
  if (!request.service_id.has_value()) {
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingService;
    return result;
  }
  return map_service_action_result(
      caller,
      axion_kernel_service_action(
          state,
          KernelServiceAction{
              .kind = kind,
              .requesting_process_group_id = caller.process_group_id,
              .service_id = *request.service_id,
          }));
}

KernelCallResult dispatch_supervisor_request(const CallerContext& caller,
                                             KernelRuntimeState& state,
                                             const KernelCallRequest& request,
                                             KernelServiceRequestKind kind) {
  if (auto invalid = validate_supervisor_query(state, caller, request);
      invalid.has_value()) {
    return *invalid;
  }
  return map_service_request_result(
      caller,
      axion_kernel_service_request(
          state,
          KernelServiceRequest{
              .kind = kind,
              .requesting_process_group_id = caller.process_group_id,
              .supervisor_id = *request.supervisor_id,
          }));
}

KernelCallResult map_service_action_result(const CallerContext& caller,
                                           const KernelServiceActionResult& action_result) {
  KernelCallResult result = init_result(caller);
  result.action_performed = action_result.action_performed;
  result.service_registered =
      action_result.service.has_value() && action_result.service->registered;
  result.service_suspended =
      action_result.service.has_value() && action_result.service->suspended;
  result.service_unhealthy =
      action_result.service.has_value() && action_result.service->unhealthy;
  result.service_blocked =
      action_result.service.has_value() && action_result.service->blocked;
  if (action_result.service.has_value()) {
    result.service_id = action_result.service->id;
    result.service_name = action_result.service->name;
  }
  switch (action_result.status) {
    case KernelServiceStatus::Ok:
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      break;
    case KernelServiceStatus::NotFound:
      result.status = KernelCallStatus::NotFound;
      result.rejection = KernelCallRejection::MissingService;
      break;
    case KernelServiceStatus::InvalidRequest:
      result.status = KernelCallStatus::InvalidRequest;
      result.rejection = action_result.rejection ==
                                 KernelServiceActionRejection::MissingServiceName
                             ? KernelCallRejection::MissingServiceName
                             : KernelCallRejection::ServiceActionRejected;
      break;
    case KernelServiceStatus::FaultedGroup:
      result.status = KernelCallStatus::FaultedCaller;
      result.rejection = KernelCallRejection::FaultedCaller;
      break;
    case KernelServiceStatus::ServiceUnavailable:
      result.status = KernelCallStatus::RetryLater;
      result.rejection = KernelCallRejection::ServiceActionRejected;
      break;
    case KernelServiceStatus::NoDeviceArbitration:
      result.status = KernelCallStatus::PolicyDenied;
      result.rejection = KernelCallRejection::ServiceActionRejected;
      break;
  }
  return result;
}

KernelCallResult map_service_request_result(const CallerContext& caller,
                                            const KernelServiceResult& request_result) {
  KernelCallResult result = init_result(caller);
  if (request_result.runtime.has_value()) {
    result.runtime_mapped_pages = request_result.runtime->mapped_pages;
    result.runtime_boot_critical_address_space_count =
        request_result.runtime->boot_critical_address_space_count;
    result.runtime_boot_critical_pager_needed_count =
        request_result.runtime->boot_critical_pager_needed_count;
    result.runtime_boot_critical_terminal_count =
        request_result.runtime->boot_critical_terminal_count;
  }
  if (request_result.service.has_value()) {
    result.service_id = request_result.service->id;
    result.service_name = request_result.service->name;
    result.service_registered = request_result.service->registered;
    result.service_suspended = request_result.service->suspended;
    result.service_unhealthy = request_result.service->unhealthy;
    result.service_blocked = request_result.service->blocked;
  }
  if (request_result.supervisor_recovery.has_value()) {
    result.supervisor_id = request_result.supervisor_recovery->id;
    result.supervisor_pending_group_count =
        request_result.supervisor_recovery->pending_group_count;
    result.supervisor_acknowledgements =
        request_result.supervisor_recovery->acknowledgements;
    result.supervisor_recovered_groups =
        request_result.supervisor_recovery->recovered_groups;
    result.supervisor_last_acknowledged_group =
        request_result.supervisor_recovery->last_acknowledged_group;
    result.supervisor_last_recovered_group =
        request_result.supervisor_recovery->last_recovered_group;
  }
  if (request_result.supervisor.has_value()) {
    result.supervisor_id = request_result.supervisor->id;
    result.supervisor_managed_group_count =
        request_result.supervisor->managed_group_count;
    result.supervisor_managed_faulted_group_count =
        request_result.supervisor->managed_faulted_group_count;
    result.supervisor_pending_group_count =
        request_result.supervisor->pending_group_count;
    result.supervisor_fault_notifications =
        request_result.supervisor->fault_notifications;
    result.supervisor_acknowledgements =
        request_result.supervisor->acknowledgements;
    result.supervisor_last_pending_group =
        request_result.supervisor->last_pending_group;
  }
  if (request_result.supervisor_capabilities.has_value()) {
    result.supervisor_id = request_result.supervisor_capabilities->supervisor_id;
    result.supervisor_capability_process_group_count =
        request_result.supervisor_capabilities->process_group_count;
    result.supervisor_capability_transitions =
        request_result.supervisor_capabilities->capability_transitions;
    result.supervisor_last_capability_transition_group_id =
        request_result.supervisor_capabilities->last_capability_transition_group_id;
    result.supervisor_last_capability_transition_record_id =
        request_result.supervisor_capabilities->last_capability_transition_record_id;
    result.supervisor_capability_transition_history =
        request_result.supervisor_capabilities->recent_capability_transitions;
  }
  if (request_result.supervisor_delegations.has_value()) {
    result.supervisor_id = request_result.supervisor_delegations->supervisor_id;
    result.supervisor_delegation_process_group_count =
        request_result.supervisor_delegations->process_group_count;
    result.supervisor_delegation_entry_count =
        request_result.supervisor_delegations->delegation_entry_count;
    result.supervisor_delegated_capability_count =
        request_result.supervisor_delegations->delegated_capability_count;
    for (const auto& entry : request_result.supervisor_delegations->entries) {
      result.supervisor_delegation_entries.push_back(KernelDelegationSummaryEntry{
          .target_process_group_id = entry.target_process_group_id,
          .delegated_by_process_group_id = entry.delegated_by_process_group_id,
          .delegated_by_supervisor_id = entry.delegated_by_supervisor_id,
          .delegated_capability_count = entry.delegated_capability_count,
      });
    }
  }
  if (request_result.fault_summary.has_value()) {
    result.fault_summary_recorded_faults =
        request_result.fault_summary->recorded_faults;
    result.fault_summary_pending_faults =
        request_result.fault_summary->pending_faults;
    result.fault_summary_delivered_faults =
        request_result.fault_summary->delivered_faults;
    result.fault_summary_routed_thread_faults =
        request_result.fault_summary->routed_thread_faults;
    result.fault_summary_quarantined_threads =
        request_result.fault_summary->quarantined_threads;
  }
  switch (request_result.status) {
    case KernelServiceStatus::Ok:
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      break;
    case KernelServiceStatus::NotFound:
      result.status = KernelCallStatus::NotFound;
      result.rejection = KernelCallRejection::MissingService;
      break;
    case KernelServiceStatus::InvalidRequest:
      result.status = KernelCallStatus::InvalidRequest;
      result.rejection = request_result.rejection ==
                                 KernelServiceRequestRejection::MissingService
                             ? KernelCallRejection::MissingService
                             : KernelCallRejection::ServiceRequestRejected;
      break;
    case KernelServiceStatus::FaultedGroup:
      result.status = KernelCallStatus::FaultedCaller;
      result.rejection = KernelCallRejection::FaultedCaller;
      break;
    case KernelServiceStatus::ServiceUnavailable:
      result.status = KernelCallStatus::RetryLater;
      result.rejection = KernelCallRejection::ServiceRequestRejected;
      break;
    case KernelServiceStatus::NoDeviceArbitration:
      result.status = KernelCallStatus::PolicyDenied;
      result.rejection = KernelCallRejection::ServiceRequestRejected;
      break;
  }
  return result;
}

}  // namespace

KernelCallResult axion_kernel_call(KernelRuntimeState& state,
                                   const KernelCallRequest& request) noexcept {
  CallerContext caller;
  if (auto invalid = validate_caller(state, request, caller); invalid.has_value()) {
    return *invalid;
  }

  KernelCallResult result = init_result(caller);
  switch (request.kind) {
    case KernelCallKind::SpawnThreadInCallerGroup: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      const auto spawned_tid = axion_kernel_spawn_thread_in_group(
          state, make_spawn_context(request.spawn_descriptor), caller.process_group_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.spawned_tid = *spawned_tid;
      return result;
    }
    case KernelCallKind::SpawnThreadUnderSupervisor: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.supervisor_id.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingSupervisor;
        return result;
      }
      if (!state.find_supervisor(*request.supervisor_id)) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingSupervisor;
        return result;
      }
      if (!axion_kernel_supervisor_matches_process_group(
              state, *request.supervisor_id, caller.process_group_id)) {
        result.status = KernelCallStatus::PolicyDenied;
        result.rejection = KernelCallRejection::SupervisorMismatch;
        return result;
      }
      const auto spawned_tid = axion_kernel_spawn_thread_under_supervisor(
          state, make_spawn_context(request.spawn_descriptor), *request.supervisor_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.spawned_tid = *spawned_tid;
      result.supervisor_id = *request.supervisor_id;
      return result;
    }
    case KernelCallKind::GetThreadIdentity: {
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.supervisor_id = state.find_process_group_supervisor(caller.process_group_id);
      result.address_space_id = state.find_process_group_address_space(caller.process_group_id);
      return result;
    }
    case KernelCallKind::QueryThreadExecutionState: {
      KernelRuntimeState::ThreadRuntimeState* target_thread_state = nullptr;
      const sched::TiscContext* target_context = nullptr;
      if (auto denied = resolve_thread_query_target(
              state, caller, request, target_thread_state, target_context);
          denied.has_value()) {
        return *denied;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.queried_tid = target_context->tid;
      result.target_process_group_id = target_thread_state->process_group_id;
      result.supervisor_id =
          state.find_process_group_supervisor(target_thread_state->process_group_id);
      result.address_space_id =
          state.find_process_group_address_space(target_thread_state->process_group_id);
      result.thread_pc = target_context->pc;
      result.thread_sp = target_context->sp;
      result.thread_register0 = target_context->registers[0];
      result.thread_active = target_context->active;
      result.thread_halted = target_context->halted;
      result.thread_running = target_context->state == sched::ThreadState::Running;
      result.thread_label = target_context->label;
      return result;
    }
    case KernelCallKind::ExitThread: {
      if (!axion_kernel_terminate_thread(state, caller.tid)) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::MissingCallerThread;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.thread_exited = true;
      return result;
    }
    case KernelCallKind::Yield: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::Yield);
          denied.has_value()) {
        return *denied;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.yielded = axion_kernel_tick(state);
      result.action_performed = result.yielded;
      return result;
    }
    case KernelCallKind::SendMessage: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::IpcSend);
          denied.has_value()) {
        return *denied;
      }
      if (!request.ipc_dst.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingDestinationThread;
        return result;
      }
      if (!request.message.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingMessage;
        return result;
      }
      auto msg = *request.message;
      msg.sender = caller.tid;
      if (!axion_kernel_ipc_send(state, *request.ipc_dst, std::move(msg))) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::IpcSendFailed;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      return result;
    }
    case KernelCallKind::ReceiveMessage: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::IpcReceive);
          denied.has_value()) {
        return *denied;
      }
      auto msg = axion_kernel_ipc_recv(state, caller.tid);
      if (!msg.has_value()) {
        result.status = KernelCallStatus::RetryLater;
        result.rejection = KernelCallRejection::IpcReceiveEmpty;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.message = std::move(msg);
      return result;
    }
    case KernelCallKind::ReadFaultInbox: {
      KernelRuntimeState::ThreadRuntimeState* target_thread_state = nullptr;
      if (auto invalid =
              resolve_fault_target(state, caller, request, target_thread_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (auto denied = require_capability(caller,
                                           KernelCapabilityKind::FaultObserve,
                                           target_thread_state->process_group_id);
          denied.has_value()) {
        return *denied;
      }
      if (target_thread_state->fault_inbox.empty()) {
        result.status = KernelCallStatus::RetryLater;
        result.rejection = KernelCallRejection::FaultInboxEmpty;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.fault = target_thread_state->fault_inbox.front();
      return result;
    }
    case KernelCallKind::AcknowledgeThreadFault: {
      KernelRuntimeState::ThreadRuntimeState* target_thread_state = nullptr;
      if (auto invalid =
              resolve_fault_target(state, caller, request, target_thread_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (auto denied = require_capability(caller,
                                           KernelCapabilityKind::FaultAcknowledge,
                                           target_thread_state->process_group_id);
          denied.has_value()) {
        return *denied;
      }
      if (target_thread_state->fault_inbox.empty()) {
        result.status = KernelCallStatus::RetryLater;
        result.rejection = KernelCallRejection::FaultInboxEmpty;
        return result;
      }
      if (!axion_kernel_ack_thread_fault(state, target_thread_state->tid)) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::FaultInboxEmpty;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      return result;
    }
    case KernelCallKind::AcknowledgeSupervisorFaultGroup: {
      KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
      if (auto invalid =
              resolve_supervisor_fault_target(state, caller, request, target_group_state);
          invalid.has_value()) {
        return *invalid;
      }
      return map_service_action_result(
          caller,
          axion_kernel_service_action(
              state,
              KernelServiceAction{
                  .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
                  .requesting_process_group_id = caller.process_group_id,
                  .supervisor_id = *request.supervisor_id,
                  .process_group_id = target_group_state->id,
              }));
    }
    case KernelCallKind::QueryProcessGroupMemory: {
      const KernelRuntimeState::ProcessGroupState* group_state = nullptr;
      if (auto invalid =
              resolve_memory_target_group(state, caller, request, group_state);
          invalid.has_value()) {
        return *invalid;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      populate_process_group_result(result, make_process_group_view(state, group_state->id));
      if (result.address_space_id.has_value()) {
        if (const auto* address_space = state.find_address_space(*result.address_space_id)) {
          result.address_space_boot_critical = address_space->boot_critical;
        }
      }
      return result;
    }
    case KernelCallKind::SetAddressSpaceBootCritical: {
      KernelRuntimeState::AddressSpaceState* address_space = nullptr;
      if (auto invalid = resolve_owned_address_space(state, caller, request, address_space);
          invalid.has_value()) {
        return *invalid;
      }
      if (!request.boot_critical.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingBootCriticalValue;
        return result;
      }
      if (!axion_kernel_set_address_space_boot_critical(
              state, address_space->id, *request.boot_critical)) {
        result.status = KernelCallStatus::Conflict;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.address_space_id = address_space->id;
      result.address_space_boot_critical = address_space->boot_critical;
      return result;
    }
    case KernelCallKind::QueryRuntimeStatus: {
      if (auto invalid = validate_global_summary_query(state, caller, request);
          invalid.has_value()) {
        return *invalid;
      }
      return map_service_request_result(
          caller,
          axion_kernel_service_request(
              state,
              KernelServiceRequest{
                  .kind = KernelServiceRequestKind::RuntimeStatus,
                  .requesting_process_group_id = caller.process_group_id,
              }));
    }
    case KernelCallKind::QueryFaultSummary: {
      if (auto invalid = validate_global_summary_query(state, caller, request);
          invalid.has_value()) {
        return *invalid;
      }
      return map_service_request_result(
          caller,
          axion_kernel_service_request(
              state,
              KernelServiceRequest{
                  .kind = KernelServiceRequestKind::FaultSummary,
                  .requesting_process_group_id = caller.process_group_id,
              }));
    }
    case KernelCallKind::QuerySupervisorStatus: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorStatus);
    }
    case KernelCallKind::QuerySupervisorRecoveryStatus: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorRecoveryStatus);
    }
    case KernelCallKind::QuerySupervisorCapabilityInventory: {
      auto mapped = dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorCapabilityInventory);
      if (mapped.status != KernelCallStatus::Ok || !request.process_group_id.has_value()) {
        return mapped;
      }
      if (!axion_kernel_supervisor_matches_process_group(
              state, *request.supervisor_id, *request.process_group_id)) {
        mapped.status = KernelCallStatus::PolicyDenied;
        mapped.rejection = KernelCallRejection::SupervisorMismatch;
        mapped.capabilities.clear();
        return mapped;
      }
      mapped.capabilities =
          axion_kernel_list_process_group_capabilities(state, *request.process_group_id);
      return mapped;
    }
    case KernelCallKind::QuerySupervisorDelegationSummary: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorDelegationSummary);
    }
    case KernelCallKind::QueryCapabilityTransitionHistory: {
      auto mapped = dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorCapabilityInventory);
      if (mapped.status != KernelCallStatus::Ok) {
        return mapped;
      }
      if (!request.process_group_id.has_value()) {
        return mapped;
      }
      if (!axion_kernel_supervisor_matches_process_group(
              state, *request.supervisor_id, *request.process_group_id)) {
        mapped.status = KernelCallStatus::PolicyDenied;
        mapped.rejection = KernelCallRejection::SupervisorMismatch;
        mapped.supervisor_capability_transition_history.clear();
        return mapped;
      }
      std::erase_if(
          mapped.supervisor_capability_transition_history,
          [&](const auto& transition) {
            return transition.process_group_id != *request.process_group_id;
          });
      if (request.capability.has_value() && request.capability->record_id != 0) {
        std::erase_if(
            mapped.supervisor_capability_transition_history,
            [&](const auto& transition) {
              return transition.record_id != request.capability->record_id;
            });
      }
      return mapped;
    }
    case KernelCallKind::QueryCapabilities: {
      if (request.process_group_id.has_value() &&
          *request.process_group_id != caller.process_group_id) {
        KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
        if (auto invalid = resolve_capability_target_group(
                state, caller, request, target_group_state);
            invalid.has_value()) {
          return *invalid;
        }
        result.status = KernelCallStatus::Ok;
        result.rejection = KernelCallRejection::None;
        result.capabilities =
            axion_kernel_list_process_group_capabilities(state, target_group_state->id);
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.capabilities =
          axion_kernel_list_process_group_capabilities(state, caller.process_group_id);
      return result;
    }
    case KernelCallKind::QueryDelegatedCapabilities: {
      if (!request.capability.has_value() ||
          (!request.capability->delegated_by_process_group_id.has_value() &&
           !request.capability->delegated_by_supervisor_id.has_value())) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingDelegationScope;
        return result;
      }
      const KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
      if (request.process_group_id.has_value() &&
          *request.process_group_id != caller.process_group_id) {
        KernelRuntimeState::ProcessGroupState* target_group_state_mut = nullptr;
        if (auto invalid = resolve_capability_target_group(
                state, caller, request, target_group_state_mut);
            invalid.has_value()) {
          return *invalid;
        }
        target_group_state = target_group_state_mut;
      } else {
        target_group_state = state.find_process_group(caller.process_group_id);
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      if (!target_group_state) {
        return result;
      }
      const auto capabilities =
          axion_kernel_list_process_group_capabilities(state, target_group_state->id);
      for (const auto& capability : capabilities) {
        if (capability.kernel_seeded) {
          continue;
        }
        if (request.capability->delegated_by_process_group_id.has_value() &&
            capability.delegated_by_process_group_id !=
                request.capability->delegated_by_process_group_id) {
          continue;
        }
        if (request.capability->delegated_by_supervisor_id.has_value() &&
            capability.delegated_by_supervisor_id !=
                request.capability->delegated_by_supervisor_id) {
          continue;
        }
        result.capabilities.push_back(capability);
      }
      return result;
    }
    case KernelCallKind::QueryCapabilityRecord: {
      if (!request.capability.has_value() || request.capability->record_id == 0) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::InvalidCapabilityRecordId;
        return result;
      }
      if (request.process_group_id.has_value() &&
          *request.process_group_id != caller.process_group_id) {
        KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
        if (auto invalid = resolve_capability_target_group(
                state, caller, request, target_group_state);
            invalid.has_value()) {
          return *invalid;
        }
        const auto* capability =
            find_capability_record(*target_group_state, request.capability->record_id);
        if (!capability) {
          result.status = KernelCallStatus::NotFound;
          result.rejection = KernelCallRejection::MissingCapability;
          return result;
        }
        result.status = KernelCallStatus::Ok;
        result.rejection = KernelCallRejection::None;
        result.capabilities = {*capability};
        return result;
      }
      const auto* caller_group = state.find_process_group(caller.process_group_id);
      const auto* capability =
          caller_group ? find_capability_record(*caller_group, request.capability->record_id)
                       : nullptr;
      if (!capability) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingCapability;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.capabilities = {*capability};
      return result;
    }
    case KernelCallKind::GrantCapability: {
      KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
      if (auto invalid = resolve_capability_target_group(
              state, caller, request, target_group_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (!request.capability.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        return result;
      }
      if (request.capability->record_id != 0) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::InvalidCapabilityRecordId;
        return result;
      }
      if (!axion_kernel_grant_process_group_capability(
              state,
              target_group_state->id,
              caller.process_group_id,
              state.find_process_group_supervisor(caller.process_group_id),
              *request.capability)) {
        result.status = KernelCallStatus::Conflict;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.capabilities =
          axion_kernel_list_process_group_capabilities(state, target_group_state->id);
      return result;
    }
    case KernelCallKind::RevokeCapability: {
      KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
      if (auto invalid = resolve_capability_target_group(
              state, caller, request, target_group_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (!request.capability.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        return result;
      }
      auto capability_record_id =
          request.capability->record_id == 0
              ? std::optional<CapabilityRecordId>{}
              : std::optional<CapabilityRecordId>{request.capability->record_id};
      if (!capability_record_id.has_value() &&
          request.capability_transition_sequence.has_value()) {
        const auto supervisor_id =
            state.find_process_group_supervisor(target_group_state->id);
        const auto* supervisor_state =
            supervisor_id ? state.find_supervisor(*supervisor_id) : nullptr;
        const auto* transition =
            supervisor_state
                ? find_capability_transition(*supervisor_state,
                                             *request.capability_transition_sequence)
                : nullptr;
        if (!transition ||
            transition->process_group_id != target_group_state->id) {
          result.status = KernelCallStatus::NotFound;
          result.rejection = KernelCallRejection::MissingCapabilityTransition;
          return result;
        }
        capability_record_id = transition->record_id;
      }
      if (!axion_kernel_revoke_process_group_capability(
              state,
              target_group_state->id,
              capability_record_id,
              request.capability->kind,
              request.capability->process_group_scope)) {
        result.status = KernelCallStatus::Conflict;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.capabilities =
          axion_kernel_list_process_group_capabilities(state, target_group_state->id);
      return result;
    }
    case KernelCallKind::RevokeDelegatedCapabilities: {
      KernelRuntimeState::ProcessGroupState* target_group_state = nullptr;
      if (auto invalid = resolve_capability_target_group(
              state, caller, request, target_group_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (!request.capability.has_value() ||
          (!request.capability->delegated_by_process_group_id.has_value() &&
           !request.capability->delegated_by_supervisor_id.has_value())) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingDelegationScope;
        return result;
      }
      if (!axion_kernel_revoke_delegated_process_group_capabilities(
              state,
              target_group_state->id,
              request.capability->delegated_by_process_group_id,
              request.capability->delegated_by_supervisor_id)) {
        result.status = KernelCallStatus::Conflict;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.capabilities =
          axion_kernel_list_process_group_capabilities(state, target_group_state->id);
      return result;
    }
    case KernelCallKind::RegisterService: {
      return dispatch_service_register(caller, state, request);
    }
    case KernelCallKind::QueryServiceStatus: {
      if (!request.service_id.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingService;
        return result;
      }
      return dispatch_service_request(caller, state, *request.service_id);
    }
    case KernelCallKind::SuspendService:
    case KernelCallKind::ResumeService: {
      return dispatch_service_action_with_id(
          caller,
          state,
          request,
          request.kind == KernelCallKind::SuspendService
              ? KernelServiceActionKind::SuspendService
              : KernelServiceActionKind::ResumeService);
    }
    case KernelCallKind::MarkServiceUnhealthy:
    case KernelCallKind::MarkServiceHealthy: {
      return dispatch_service_action_with_id(
          caller,
          state,
          request,
          request.kind == KernelCallKind::MarkServiceUnhealthy
              ? KernelServiceActionKind::MarkServiceUnhealthy
              : KernelServiceActionKind::MarkServiceHealthy);
    }
  }

  result.status = KernelCallStatus::InvalidRequest;
  return result;
}

}  // namespace t81::ternaryos::kernel
