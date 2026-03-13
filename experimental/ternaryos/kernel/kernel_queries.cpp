#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

namespace {

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
    if (!axion_kernel_supervisor_matches_process_group(
            state,
            service_state->supervisor_id,
            *request.requesting_process_group_id)) {
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

const KernelRuntimeState::SupervisorState* resolve_supervisor_request(
    const KernelRuntimeState& state,
    KernelServiceResult& result,
    const KernelServiceRequest& request,
    bool enforce_requesting_group_match) {
  if (!request.supervisor_id.has_value()) {
    result.status = KernelServiceStatus::InvalidRequest;
    result.rejection = KernelServiceRequestRejection::MissingSupervisor;
    return nullptr;
  }
  const auto* supervisor_state = state.find_supervisor(*request.supervisor_id);
  if (!supervisor_state) {
    result.status = KernelServiceStatus::NotFound;
    result.rejection = KernelServiceRequestRejection::MissingSupervisor;
    return nullptr;
  }
  if (enforce_requesting_group_match && request.requesting_process_group_id.has_value() &&
      !axion_kernel_supervisor_matches_process_group(
          state,
          supervisor_state->id,
          *request.requesting_process_group_id)) {
    result.status = KernelServiceStatus::InvalidRequest;
    result.rejection = KernelServiceRequestRejection::MissingSupervisor;
    return nullptr;
  }
  return supervisor_state;
}

}  // namespace

KernelServiceResult axion_kernel_service_request(
    const KernelRuntimeState& state,
    const KernelServiceRequest& request) noexcept {
  KernelServiceResult result;
  switch (request.kind) {
    case KernelServiceRequestKind::RuntimeStatus: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.runtime = make_runtime_view(state);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      const auto* supervisor_state =
          resolve_supervisor_request(state, result, request, false);
      if (!supervisor_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor = make_supervisor_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorRecoveryStatus: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      const auto* supervisor_state =
          resolve_supervisor_request(state, result, request, false);
      if (!supervisor_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_recovery =
          make_supervisor_recovery_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::ServiceStatus: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
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
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      const auto* supervisor_state =
          resolve_supervisor_request(state, result, request, true);
      if (!supervisor_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_services =
          build_supervisor_services_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorCapabilityInventory: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      const auto* supervisor_state =
          resolve_supervisor_request(state, result, request, true);
      if (!supervisor_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_capabilities =
          build_supervisor_capabilities_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::SupervisorDelegationSummary: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      const auto* supervisor_state =
          resolve_supervisor_request(state, result, request, true);
      if (!supervisor_state) {
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.supervisor_delegations =
          build_supervisor_delegation_summary_view(state, supervisor_state->id);
      return result;
    }
    case KernelServiceRequestKind::FaultSummary: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.fault_summary = make_fault_summary_view(state);
      return result;
    }
    case KernelServiceRequestKind::AuditSummary: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.rejection = KernelServiceRequestRejection::None;
      result.audit_summary = make_audit_summary_view(state);
      return result;
    }
    case KernelServiceRequestKind::DeviceSummary: {
      if (auto denied = axion_kernel_validate_requesting_group(
              state, request.requesting_process_group_id);
          denied.has_value()) {
        result.status = *denied;
        result.rejection = axion_kernel_requesting_group_request_rejection(*denied);
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

}  // namespace t81::ternaryos::kernel
