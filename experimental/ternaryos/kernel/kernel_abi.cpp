#include "kernel_abi.hpp"

#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

namespace {

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

}  // namespace

KernelCallResult axion_kernel_call(KernelRuntimeState& state,
                                   const KernelCallRequest& request) noexcept {
  CallerContext caller;
  if (auto invalid = validate_caller(state, request, caller); invalid.has_value()) {
    return *invalid;
  }

  KernelCallResult result = init_result(caller);
  switch (request.kind) {
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
      if (!axion_kernel_grant_process_group_capability(
              state, target_group_state->id, *request.capability)) {
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
      if (!axion_kernel_revoke_process_group_capability(
              state,
              target_group_state->id,
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
  }

  result.status = KernelCallStatus::InvalidRequest;
  return result;
}

}  // namespace t81::ternaryos::kernel
