#include "kernel_abi.hpp"

#include "kernel_executable.hpp"
#include "kernel_loader.hpp"
#include "kernel_main.hpp"
#include "../dev/canon_store.hpp"

#ifdef T81_ENABLE_DPE
#include "kernel_epoch.hpp"
#endif

namespace t81::ternaryos::kernel {

namespace {

std::string executable_key(const t81::canonfs::CanonRef& ref) {
  return std::string(reinterpret_cast<const char*>(ref.hash.h.bytes.data()),
                     ref.hash.h.bytes.size());
}

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

std::optional<KernelCallResult> resolve_executable_record(
    const CallerContext& caller,
    const KernelCallRequest& request,
    const KernelRuntimeState::ExecutableRecord*& executable_record) {
  auto result = init_result(caller);
  if (!request.object_ref.has_value()) {
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingExecutableRef;
    return result;
  }
  const auto it =
      caller.group_state->executable_records.find(executable_key(*request.object_ref));
  if (it == caller.group_state->executable_records.end()) {
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingExecutableRegistration;
    return result;
  }
  executable_record = &it->second;
  return std::nullopt;
}

std::optional<KernelCallResult> load_executable_descriptor(
    const CallerContext& caller,
    const KernelRuntimeState::ExecutableRecord& executable_record,
    KernelThreadSpawnDescriptor& descriptor) {
  const auto decoded =
      axion_kernel_decode_executable_block(executable_record.image_block);
  if (!decoded.has_value()) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::InvalidExecutableObject;
    return result;
  }
  descriptor = *decoded;
  return std::nullopt;
}

std::optional<t81::canonfs::CanonBlock> load_published_executable_block(
    KernelRuntimeState& state,
    const t81::canonfs::CanonRef& object_ref) {
  if (state.published_executable_canonfs) {
    auto read_result = state.published_executable_canonfs->read_object_bytes(object_ref);
    if (read_result.has_value()) {
      return t81::canonfs::CanonBlock::from_bytes(
          std::span<const std::byte>(read_result->data(), read_result->size()));
    }
  }
  if (!state.published_executable_store_device) {
    return std::nullopt;
  }
  t81::ternaryos::dev::CanonStore store(*state.published_executable_store_device);
  store.rebuild_index();
  return store.get(object_ref);
}

bool publish_executable_block(KernelRuntimeState& state,
                              const t81::canonfs::CanonRef& object_ref,
                              const t81::canonfs::CanonBlock& block) {
  if (state.published_executable_canonfs) {
    const auto block_bytes = block.to_bytes();
    auto write_result = state.published_executable_canonfs->write_object(
        t81::canonfs::ObjectType::CanonExec,
        std::span<const std::byte>(block_bytes.data(), block_bytes.size()));
    if (!write_result.has_value() || write_result->hash != object_ref.hash) {
      return false;
    }
    return true;
  }
  if (!state.published_executable_store_device) {
    return false;
  }
  t81::ternaryos::dev::CanonStore store(*state.published_executable_store_device);
  store.rebuild_index();
  const auto stored_ref = store.put(block);
  if (!stored_ref.has_value() || stored_ref->hash != object_ref.hash) {
    return false;
  }
  return store.flush();
}

std::optional<KernelCallResult> load_executable_block_from_tva(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    t81::canonfs::CanonBlock& block,
    KernelThreadSpawnDescriptor& descriptor) {
  if (!request.object_tva.has_value()) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingExecutableImageTva;
    return result;
  }
  const auto address_space_id =
      state.find_process_group_address_space(caller.process_group_id);
  if (!address_space_id.has_value()) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingAddressSpace;
    return result;
  }
  if (!axion_kernel_validate_address_space_span(state,
                                                *address_space_id,
                                                *request.object_tva,
                                                t81::canonfs::CanonBlock::kTryteCount,
                                                mmu::MmuAccessMode::Read)) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::InvalidAddressSpaceSpan;
    return result;
  }
  std::array<std::byte, t81::canonfs::CanonBlock::kTryteCount> raw{};
  if (!axion_kernel_read_address_space_bytes(state,
                                             *address_space_id,
                                             *request.object_tva,
                                             raw.data(),
                                             raw.size())) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::InvalidAddressSpaceSpan;
    return result;
  }
  const auto decoded_block = t81::canonfs::CanonBlock::from_bytes(raw);
  if (!decoded_block.has_value()) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::InvalidExecutableObject;
    return result;
  }
  const auto decoded_descriptor =
      axion_kernel_decode_executable_block(*decoded_block);
  if (!decoded_descriptor.has_value()) {
    auto result = init_result(caller);
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::InvalidExecutableObject;
    return result;
  }
  block = *decoded_block;
  descriptor = *decoded_descriptor;
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
  const auto caller_supervisor_id =
      state.find_process_group_supervisor(caller.process_group_id);
  const auto target_supervisor_id =
      request.supervisor_id.value_or(caller_supervisor_id.value_or(0));
  if (target_supervisor_id == 0 || !state.find_supervisor(target_supervisor_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!axion_kernel_supervisor_matches_process_group(
          state, target_supervisor_id, caller.process_group_id)) {
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
  const auto caller_supervisor_id =
      state.find_process_group_supervisor(caller.process_group_id);
  const auto target_supervisor_id =
      request.supervisor_id.value_or(caller_supervisor_id.value_or(0));
  if (target_supervisor_id == 0 || !state.find_supervisor(target_supervisor_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingSupervisor;
    return result;
  }
  if (!axion_kernel_supervisor_matches_process_group(
          state, target_supervisor_id, caller.process_group_id)) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ForeignSupervisorScope;
    return result;
  }
  return std::nullopt;
}

std::optional<SupervisorId> resolve_effective_supervisor_id(
    const KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request) {
  return request.supervisor_id.value_or(
      state.find_process_group_supervisor(caller.process_group_id).value_or(0));
}

bool spawn_descriptors_match(const KernelThreadSpawnDescriptor& lhs,
                             const KernelThreadSpawnDescriptor& rhs) noexcept {
  return lhs.pc == rhs.pc &&
         lhs.sp == rhs.sp &&
         lhs.register0 == rhs.register0 &&
         lhs.halted == rhs.halted &&
         lhs.active == rhs.active &&
         lhs.label == rhs.label;
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
  const auto caller_address_space_id =
      state.find_process_group_address_space(caller.process_group_id);
  const auto target_address_space_id =
      request.address_space_id.value_or(caller_address_space_id.value_or(0));
  if (target_address_space_id == 0) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingAddressSpace;
    return result;
  }
  address_space = state.find_address_space_mut(target_address_space_id);
  if (!address_space) {
    KernelCallResult result = init_result(caller);
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingAddressSpace;
    return result;
  }
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
  std::optional<KernelThreadSpawnDescriptor> effective_spawn_descriptor =
      request.spawn_descriptor;
  if (request.object_ref.has_value()) {
    const KernelRuntimeState::ExecutableRecord* executable_record = nullptr;
    if (auto invalid = resolve_executable_record(caller, request, executable_record);
        invalid.has_value()) {
      return *invalid;
    }
    if (effective_spawn_descriptor.has_value() &&
        !spawn_descriptors_match(*effective_spawn_descriptor,
                                 executable_record->entry_descriptor)) {
      result.status = KernelCallStatus::InvalidRequest;
      result.rejection = KernelCallRejection::ServiceActionRejected;
      return result;
    }
    effective_spawn_descriptor = executable_record->entry_descriptor;
  }
  return map_service_action_result(
      caller,
      axion_kernel_service_action(
          state,
          KernelServiceAction{
              .kind = KernelServiceActionKind::RegisterService,
              .requesting_process_group_id = caller.process_group_id,
              .service_name = *request.service_name,
              .object_ref = request.object_ref,
              .spawn_descriptor = effective_spawn_descriptor,
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

std::optional<KernelCallResult> resolve_owned_service(
    KernelRuntimeState& state,
    const CallerContext& caller,
    const KernelCallRequest& request,
    KernelRuntimeState::ServiceState*& service_state) {
  auto result = init_result(caller);
  if (!request.service_id.has_value()) {
    result.status = KernelCallStatus::InvalidRequest;
    result.rejection = KernelCallRejection::MissingService;
    return result;
  }
  service_state = state.find_service_mut(*request.service_id);
  if (!service_state || !service_state->registered) {
    result.status = KernelCallStatus::NotFound;
    result.rejection = KernelCallRejection::MissingService;
    return result;
  }
  if (service_state->process_group_id != caller.process_group_id) {
    result.status = KernelCallStatus::PolicyDenied;
    result.rejection = KernelCallRejection::ServiceActionRejected;
    return result;
  }
  return std::nullopt;
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
              .supervisor_id = request.supervisor_id.value_or(
                  state.find_process_group_supervisor(caller.process_group_id).value_or(0)),
              .service_id = request.service_id.value_or(0),
          }));
}

KernelCallResult map_service_action_result(const CallerContext& caller,
                                           const KernelServiceActionResult& action_result) {
  KernelCallResult result = init_result(caller);
  result.action_performed = action_result.action_performed;
  result.service_registered =
      action_result.service.has_value() && action_result.service->registered;
  result.service_has_entry_descriptor =
      action_result.service.has_value() &&
      action_result.service->has_entry_descriptor;
  result.service_suspended =
      action_result.service.has_value() && action_result.service->suspended;
  result.service_unhealthy =
      action_result.service.has_value() && action_result.service->unhealthy;
  result.service_blocked =
      action_result.service.has_value() && action_result.service->blocked;
  if (action_result.service.has_value()) {
    result.service_id = action_result.service->id;
    result.service_name = action_result.service->name;
    result.object_ref = action_result.service->object_ref;
    result.service_entry_descriptor = action_result.service->entry_descriptor;
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
      result.rejection =
          action_result.rejection == KernelServiceActionRejection::MissingServiceName
              ? KernelCallRejection::MissingServiceName
          : action_result.rejection ==
                    KernelServiceActionRejection::MissingExecutableRegistration
              ? KernelCallRejection::MissingExecutableRegistration
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
    result.object_ref = request_result.service->object_ref;
    result.service_registered = request_result.service->registered;
    result.service_has_entry_descriptor =
        request_result.service->has_entry_descriptor;
    result.service_entry_descriptor = request_result.service->entry_descriptor;
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
  if (request_result.supervisor_services.has_value()) {
    result.supervisor_id = request_result.supervisor_services->supervisor_id;
    result.supervisor_service_count =
        request_result.supervisor_services->service_count;
    result.supervisor_blocked_service_count =
        request_result.supervisor_services->blocked_service_count;
    result.supervisor_suspended_service_count =
        request_result.supervisor_services->suspended_service_count;
    result.supervisor_unhealthy_service_count =
        request_result.supervisor_services->unhealthy_service_count;
    result.supervisor_service_lifecycle_transitions =
        request_result.supervisor_services->service_lifecycle_transitions;
    result.supervisor_services.clear();
    result.supervisor_services.reserve(
        request_result.supervisor_services->services.size());
    for (const auto& service :
         request_result.supervisor_services->services) {
      result.supervisor_services.push_back(KernelSupervisorServiceSummaryEntry{
          .id = service.id,
          .name = service.name,
          .process_group_id = service.process_group_id,
          .object_ref = service.object_ref,
          .registered = service.registered,
          .blocked = service.blocked,
          .suspended = service.suspended,
          .unhealthy = service.unhealthy,
          .has_entry_descriptor = service.has_entry_descriptor,
          .entry_descriptor = service.entry_descriptor,
      });
    }
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
    case KernelCallKind::RegisterExecutableObject: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.object_ref.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingExecutableRef;
        return result;
      }
      t81::canonfs::CanonBlock executable_block{};
      KernelThreadSpawnDescriptor loaded_descriptor;
      if (request.spawn_descriptor.has_value()) {
        const auto encoded_block =
            axion_kernel_encode_executable_block(*request.spawn_descriptor);
        if (!encoded_block.has_value()) {
          result.status = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::InvalidExecutableObject;
          return result;
        }
        const t81::canonfs::CanonRef encoded_ref{encoded_block->hash()};
        if (encoded_ref.hash != request.object_ref->hash) {
          result.status = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::InvalidExecutableObject;
          return result;
        }
        const auto decoded_descriptor =
            axion_kernel_decode_executable_block(*encoded_block);
        if (!decoded_descriptor.has_value()) {
          result.status = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::InvalidExecutableObject;
          return result;
        }
        executable_block = *encoded_block;
        loaded_descriptor = *decoded_descriptor;
      } else {
        const auto published_block =
            load_published_executable_block(state, *request.object_ref);
        if (!published_block.has_value()) {
          result.status = KernelCallStatus::NotFound;
          result.rejection = KernelCallRejection::MissingExecutableRegistration;
          return result;
        }
        const auto decoded_descriptor =
            axion_kernel_decode_executable_block(*published_block);
        if (!decoded_descriptor.has_value()) {
          result.status = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::InvalidExecutableObject;
          return result;
        }
        executable_block = *published_block;
        loaded_descriptor = *decoded_descriptor;
      }
      caller.group_state->executable_records[executable_key(*request.object_ref)] =
          KernelRuntimeState::ExecutableRecord{
              .object_ref = *request.object_ref,
              .image_block = executable_block,
              .entry_descriptor = loaded_descriptor,
          };
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.executable_registered = true;
      result.object_ref = request.object_ref;
      result.executable_entry_descriptor = loaded_descriptor;
      return result;
    }
    case KernelCallKind::PublishExecutableObjectFromTva: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.object_ref.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingExecutableRef;
        return result;
      }
      t81::canonfs::CanonBlock executable_block{};
      KernelThreadSpawnDescriptor loaded_descriptor;
      if (auto invalid = load_executable_block_from_tva(
              state, caller, request, executable_block, loaded_descriptor);
          invalid.has_value()) {
        return *invalid;
      }
      const t81::canonfs::CanonRef encoded_ref{executable_block.hash()};
      if (encoded_ref.hash != request.object_ref->hash) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::InvalidExecutableObject;
        return result;
      }
      if (!publish_executable_block(state, *request.object_ref, executable_block)) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::InvalidExecutableObject;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.executable_published = true;
      result.object_ref = request.object_ref;
      result.executable_entry_descriptor = loaded_descriptor;
      return result;
    }
    case KernelCallKind::RegisterExecutableObjectFromTva: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.object_ref.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingExecutableRef;
        return result;
      }
      t81::canonfs::CanonBlock executable_block{};
      KernelThreadSpawnDescriptor loaded_descriptor;
      if (auto invalid = load_executable_block_from_tva(
              state, caller, request, executable_block, loaded_descriptor);
          invalid.has_value()) {
        return *invalid;
      }
      const t81::canonfs::CanonRef encoded_ref{executable_block.hash()};
      if (encoded_ref.hash != request.object_ref->hash) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::InvalidExecutableObject;
        return result;
      }
      caller.group_state->executable_records[executable_key(*request.object_ref)] =
          KernelRuntimeState::ExecutableRecord{
              .object_ref = *request.object_ref,
              .image_block = executable_block,
              .entry_descriptor = loaded_descriptor,
          };
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.executable_registered = true;
      result.object_ref = request.object_ref;
      result.executable_entry_descriptor = loaded_descriptor;
      return result;
    }
    case KernelCallKind::QueryExecutableObject: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      const KernelRuntimeState::ExecutableRecord* executable_record = nullptr;
      if (auto invalid = resolve_executable_record(caller, request, executable_record);
          invalid.has_value()) {
        return *invalid;
      }
      KernelThreadSpawnDescriptor loaded_descriptor;
      if (auto invalid =
              load_executable_descriptor(caller, *executable_record, loaded_descriptor);
          invalid.has_value()) {
        return *invalid;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.executable_registered = true;
      result.object_ref = executable_record->object_ref;
      result.executable_entry_descriptor = loaded_descriptor;
      return result;
    }
    case KernelCallKind::SpawnThreadFromExecutableObject: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      const KernelRuntimeState::ExecutableRecord* executable_record = nullptr;
      // Temporary storage for a record fetched directly from CanonFS (Slice 7).
      // Lives for the duration of this case so executable_record can point into it.
      std::optional<KernelRuntimeState::ExecutableRecord> canonfs_record{};
      if (auto invalid = resolve_executable_record(caller, request, executable_record);
          invalid.has_value()) {
        // On a registry miss, attempt a direct CanonFS fetch before giving up.
        // This is the Slice 7 path: SpawnThreadFromExecutableObject no longer
        // requires a prior RegisterExecutableObject call when a bound CanonFS
        // driver holds the CanonExec block (RFC-00B2 §3.1).
        if (invalid->rejection == KernelCallRejection::MissingExecutableRegistration &&
            request.object_ref.has_value()) {
          auto fetched_block =
              load_published_executable_block(state, *request.object_ref);
          if (fetched_block.has_value()) {
            canonfs_record = KernelRuntimeState::ExecutableRecord{
                .object_ref  = *request.object_ref,
                .image_block = *fetched_block,
            };
            executable_record = &*canonfs_record;
            state.counters.canonfs_fetch_spawns++;
          } else {
            return *invalid;
          }
        } else {
          return *invalid;
        }
      }
      KernelThreadSpawnDescriptor loaded_descriptor;
      if (auto invalid =
              load_executable_descriptor(caller, *executable_record, loaded_descriptor);
          invalid.has_value()) {
        return *invalid;
      }
      // Map CanonExec sections into the address space before spawning.
      // On OOM the load returns nullopt; on re-spawn the already-mapped page
      // is reused and entry_tva is returned unchanged.
      const auto load_result = load_canon_exec_sections(
          state,
          caller.process_group_id,
          executable_record->image_block,
          static_cast<uint64_t>(loaded_descriptor.pc));
      if (!load_result.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      loaded_descriptor.pc = static_cast<std::size_t>(load_result->entry_tva);
      const auto spawned_tid = axion_kernel_spawn_thread_in_group(
          state,
          make_spawn_context(loaded_descriptor),
          caller.process_group_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.executable_registered = true;
      result.object_ref = executable_record->object_ref;
      result.executable_entry_descriptor = loaded_descriptor;
      result.spawned_tid = *spawned_tid;
      return result;
    }
    case KernelCallKind::RegisterThreadEntryDescriptor: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.service_name.has_value() || request.service_name->empty()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingEntryName;
        return result;
      }
      if (!request.spawn_descriptor.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingEntryDescriptor;
        return result;
      }
      caller.group_state->entry_descriptors[*request.service_name] = *request.spawn_descriptor;
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.service_name = request.service_name;
      return result;
    }
    case KernelCallKind::SpawnThreadFromEntryDescriptor: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      if (!request.service_name.has_value() || request.service_name->empty()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingEntryName;
        return result;
      }
      const auto it = caller.group_state->entry_descriptors.find(*request.service_name);
      if (it == caller.group_state->entry_descriptors.end()) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingEntryRegistration;
        return result;
      }
      const auto spawned_tid = axion_kernel_spawn_thread_in_group(
          state, make_spawn_context(it->second), caller.process_group_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.spawned_tid = *spawned_tid;
      result.service_name = request.service_name;
      return result;
    }
    case KernelCallKind::SpawnThreadForService: {
      if (auto denied = require_capability(caller, KernelCapabilityKind::ThreadSpawn);
          denied.has_value()) {
        return *denied;
      }
      KernelRuntimeState::ServiceState* service_state = nullptr;
      if (auto invalid = resolve_owned_service(state, caller, request, service_state);
          invalid.has_value()) {
        return *invalid;
      }
      if (!service_state->entry_descriptor.has_value()) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingEntryRegistration;
        return result;
      }
      const auto spawned_tid = axion_kernel_spawn_thread_in_group(
          state, make_spawn_context(service_state->entry_descriptor), caller.process_group_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.spawned_tid = *spawned_tid;
      result.service_id = service_state->id;
      result.service_name = service_state->name;
      result.object_ref = service_state->object_ref;
      return result;
    }
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
      const auto caller_supervisor_id =
          state.find_process_group_supervisor(caller.process_group_id);
      const auto target_supervisor_id =
          request.supervisor_id.value_or(caller_supervisor_id.value_or(0));
      if (target_supervisor_id == 0 || !state.find_supervisor(target_supervisor_id)) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingSupervisor;
        return result;
      }
      if (!axion_kernel_supervisor_matches_process_group(
              state, target_supervisor_id, caller.process_group_id)) {
        result.status = KernelCallStatus::PolicyDenied;
        result.rejection = KernelCallRejection::SupervisorMismatch;
        return result;
      }
      const auto spawned_tid = axion_kernel_spawn_thread_under_supervisor(
          state, make_spawn_context(request.spawn_descriptor), target_supervisor_id);
      if (!spawned_tid.has_value()) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::ServiceActionRejected;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.spawned_tid = *spawned_tid;
      result.supervisor_id = target_supervisor_id;
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
      const sched::Tid dst_tid = *request.ipc_dst;
      if (!axion_kernel_ipc_send(state, dst_tid, std::move(msg))) {
        result.status = KernelCallStatus::Conflict;
        result.rejection = KernelCallRejection::IpcSendFailed;
        return result;
      }
      // RFC-00B5 §3.6 / RFC-00B6 §5.3.2: if the destination is sleeping on an
      // empty inbox (BlockOnIpcReceive parked it), wake it now.
      if (state.ipc_blocked_tids.count(dst_tid) > 0) {
        state.scheduler.wake(dst_tid);
        state.ipc_blocked_tids.erase(dst_tid);
        ++state.counters.ipc_wakes;
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
    case KernelCallKind::BlockOnIpcReceive: {
      // RFC-00B6 §5.3.2 blocking receive / RFC-00B5 §3.6 continuation model.
      //
      // Fast path: a message is already waiting — return it immediately.
      // Slow path: inbox is empty — park the calling thread via the scheduler
      // sleep/wake API so the CPU is yielded to the next Ready thread.  A
      // future SendMessage to this TID will wake the thread and deliver the
      // message to its inbox, after which the caller must issue ReceiveMessage
      // to consume it.
      if (auto denied = require_capability(caller, KernelCapabilityKind::IpcReceive);
          denied.has_value()) {
        return *denied;
      }
      // Fast path: message already present.
      auto msg = axion_kernel_ipc_recv(state, caller.tid);
      if (msg.has_value()) {
        result.status = KernelCallStatus::Ok;
        result.rejection = KernelCallRejection::None;
        result.action_performed = true;
        result.message = std::move(msg);
        return result;
      }
      // Slow path: park the calling thread.
      if (!state.scheduler.sleep(caller.tid, state.cpu_context)) {
        // Fallback: sleep() failed (should not happen for a valid running thread).
        result.status = KernelCallStatus::RetryLater;
        result.rejection = KernelCallRejection::IpcReceiveEmpty;
        return result;
      }
      state.ipc_blocked_tids.insert(caller.tid);
      ++state.counters.ipc_blocks;
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.thread_sleeping = true;
      return result;
    }
    case KernelCallKind::WaitForDevice: {
      // RFC-00B5 §3.3 — device-wake: park the calling thread in the per-source
      // wait set.  When a matching Storage or Network interrupt is delivered,
      // the kernel will send a synthetic IPC message and call scheduler.wake().
      if (!request.device_source.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::None;
        return result;
      }
      const uint8_t src_key = static_cast<uint8_t>(*request.device_source);
      // Park the calling thread.
      if (!state.scheduler.sleep(caller.tid, state.cpu_context)) {
        result.status = KernelCallStatus::RetryLater;
        return result;
      }
      state.device_waiting_tids[src_key].insert(caller.tid);
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.thread_sleeping = true;
      return result;
    }
    case KernelCallKind::RequestPageMapping: {
      // RFC-00B7 §3.2 — pager service supplies a page mapping for a pager-needed address space.
      // Caller must hold PagerService capability.  The mapping is installed at
      // last_pager_fault->tva with permissions derived from the fault access mode.
      // The pager worker detects the mapping on its next tick via is_pager_work_item_ready()
      // and calls resolve_completed_pager_work() to clear pager_needed.
      if (auto denied =
              require_capability(caller, KernelCapabilityKind::PagerService);
          denied.has_value()) {
        return *denied;
      }
      if (!request.address_space_id.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingAddressSpace;
        return result;
      }
      const auto as_id = *request.address_space_id;
      auto* as_state = state.find_address_space_mut(as_id);
      if (!as_state) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingAddressSpace;
        return result;
      }
      if (!as_state->pager_needed) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::AddressSpaceNotPagerNeeded;
        return result;
      }
      if (!as_state->last_pager_fault.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingPagerFault;
        return result;
      }
      const mmu::PagePermissions perms = [&]() -> mmu::PagePermissions {
        switch (as_state->last_pager_fault->access_mode) {
          case mmu::MmuAccessMode::Read:
            return {.readable = true, .writable = false, .executable = false};
          case mmu::MmuAccessMode::Write:
            return {.readable = true, .writable = true, .executable = false};
          case mmu::MmuAccessMode::Execute:
            return {.readable = true, .writable = false, .executable = true};
        }
        return {};
      }();
      mmu::mmu_map(state.page_table, state.allocator,
                   as_state->last_pager_fault->tva, as_id, perms);
      ++state.counters.pager_service_mappings;
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.pager_mapping_supplied = true;
      result.address_space_id = as_id;
      return result;
    }
    case KernelCallKind::WaitForPagerHandoff: {
      // RFC-00B7 §3.3 — park the calling PagerService thread until the next pager
      // handoff is dispatched.  dispatch_pending_pager_handoff() (kernel_pager.cpp)
      // sends a synthetic IPC message carrying the address_space_id as payload and
      // calls scheduler.wake() for every thread in pager_handoff_waiting_tids.
      if (auto denied =
              require_capability(caller, KernelCapabilityKind::PagerService);
          denied.has_value()) {
        return *denied;
      }
      if (!state.scheduler.sleep(caller.tid, state.cpu_context)) {
        result.status = KernelCallStatus::RetryLater;
        return result;
      }
      state.pager_handoff_waiting_tids.insert(caller.tid);
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.thread_sleeping = true;
      return result;
    }
    case KernelCallKind::ResumePageFaultedThread: {
      // RFC-00B7 §3.4 — complete the fault→handoff→service→resume lifecycle.
      // Caller must hold PagerService capability.  request.target_tid must identify
      // a quarantined thread whose front fault is Unmapped and whose fault TVA is
      // now present in the page table (i.e. RequestPageMapping was already called).
      if (auto denied =
              require_capability(caller, KernelCapabilityKind::PagerService);
          denied.has_value()) {
        return *denied;
      }
      if (!request.target_tid.has_value()) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingTargetThread;
        return result;
      }
      const auto target_tid = *request.target_tid;
      const auto* target_thread_state = state.find_thread_runtime(target_tid);
      if (!target_thread_state) {
        result.status = KernelCallStatus::NotFound;
        result.rejection = KernelCallRejection::MissingTargetThread;
        return result;
      }
      if (!target_thread_state->quarantined ||
          target_thread_state->fault_inbox.empty() ||
          target_thread_state->fault_inbox.front().fault != mmu::MmuFault::Unmapped) {
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::TargetNotQuarantined;
        return result;
      }
      if (!axion_kernel_resume_pager_faulted_thread(state, target_tid)) {
        // resume_pager_faulted_thread can fail if the TVA is not yet mapped.
        result.status = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::PagerFaultNotResolved;
        return result;
      }
      result.status = KernelCallStatus::Ok;
      result.rejection = KernelCallRejection::None;
      result.action_performed = true;
      result.pager_thread_resumed = true;
      return result;
    }
#ifdef T81_ENABLE_DPE
    case KernelCallKind::SubmitEpoch: {
      // RFC-DPE-0003 §10 / RFC-DPE-0006 §4 — submit a DPE epoch graph for
      // deterministic parallel execution through the kernel ABI boundary.
      //
      // No capability check: any caller may submit an epoch (policy enforcement
      // is the responsibility of the KernelEpochPolicyGate, not a capability).
      if (!request.epoch_graph.has_value()) {
        result.status    = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingEpochGraph;
        return result;
      }
      if (!request.epoch_programs.has_value()) {
        result.status    = KernelCallStatus::InvalidRequest;
        result.rejection = KernelCallRejection::MissingEpochPrograms;
        return result;
      }

      const auto epoch_result = axion_kernel_submit_epoch(
          state,
          *request.epoch_graph,
          *request.epoch_programs);

      switch (epoch_result.status) {
        case KernelEpochStatus::Ok:
          result.status          = KernelCallStatus::Ok;
          result.rejection       = KernelCallRejection::None;
          result.action_performed = true;
          result.epoch_committed = true;
          result.epoch_hash      = epoch_result.epoch_hash;
          break;
        case KernelEpochStatus::Rejected_AcceptFailed:
          result.status    = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::EpochAcceptFailed;
          break;
        case KernelEpochStatus::Aborted_TaskFault:
          result.status    = KernelCallStatus::InvalidRequest;
          result.rejection = KernelCallRejection::EpochTaskFault;
          break;
        case KernelEpochStatus::Aborted_ExclusiveConflict:
          result.status    = KernelCallStatus::Conflict;
          result.rejection = KernelCallRejection::EpochExclusiveConflict;
          break;
        case KernelEpochStatus::Aborted_PolicyFault:
          result.status    = KernelCallStatus::PolicyDenied;
          result.rejection = KernelCallRejection::EpochPolicyFault;
          break;
      }
      return result;
    }
#endif
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
      auto mapped = map_service_request_result(
          caller,
          axion_kernel_service_request(
              state,
              KernelServiceRequest{
                  .kind = KernelServiceRequestKind::RuntimeStatus,
                  .requesting_process_group_id = caller.process_group_id,
              }));
      if (caller.process_group_id != KernelRuntimeState::kKernelProcessGroup) {
        mapped.supervisor_id = resolve_effective_supervisor_id(state, caller, request);
      }
      return mapped;
    }
    case KernelCallKind::QueryFaultSummary: {
      if (auto invalid = validate_global_summary_query(state, caller, request);
          invalid.has_value()) {
        return *invalid;
      }
      auto mapped = map_service_request_result(
          caller,
          axion_kernel_service_request(
              state,
              KernelServiceRequest{
                  .kind = KernelServiceRequestKind::FaultSummary,
                  .requesting_process_group_id = caller.process_group_id,
              }));
      if (caller.process_group_id != KernelRuntimeState::kKernelProcessGroup) {
        mapped.supervisor_id = resolve_effective_supervisor_id(state, caller, request);
      }
      return mapped;
    }
    case KernelCallKind::QuerySupervisorStatus: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorStatus);
    }
    case KernelCallKind::QuerySupervisorRecoveryStatus: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorRecoveryStatus);
    }
    case KernelCallKind::QuerySupervisorServiceStatus: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorServiceStatus);
    }
    case KernelCallKind::QuerySupervisorServiceInventory: {
      return dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorServiceInventory);
    }
    case KernelCallKind::QuerySupervisorCapabilityInventory: {
      auto mapped = dispatch_supervisor_request(
          caller, state, request, KernelServiceRequestKind::SupervisorCapabilityInventory);
      if (mapped.status != KernelCallStatus::Ok || !request.process_group_id.has_value()) {
        return mapped;
      }
      const auto target_supervisor_id = resolve_effective_supervisor_id(state, caller, request);
      if (!axion_kernel_supervisor_matches_process_group(
              state, *target_supervisor_id, *request.process_group_id)) {
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
