#include "kernel_main.hpp"

#include <algorithm>

namespace t81::ternaryos::kernel {

namespace {

void record_service_audit_event(KernelRuntimeState& state,
                                KernelAuditEventKind kind,
                                ProcessGroupId process_group_id) {
  const auto subject_tid =
      axion_kernel_primary_tid_for_group(state, process_group_id)
          .value_or(KernelRuntimeState::kKernelTid);
  record_audit_event(state, kind, subject_tid, process_group_id, mmu::MmuFault::None);
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

}  // namespace

KernelServiceActionResult axion_kernel_service_action(
    KernelRuntimeState& state,
    const KernelServiceAction& action) noexcept {
  KernelServiceActionResult result;
  switch (action.kind) {
    case KernelServiceActionKind::AcknowledgeSupervisorFaultGroup: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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
          axion_kernel_primary_tid_for_group(state, *action.requesting_process_group_id);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, action.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_action_rejection(*denied);
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

}  // namespace t81::ternaryos::kernel
