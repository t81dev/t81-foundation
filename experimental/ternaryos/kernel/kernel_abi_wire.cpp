#include "kernel_abi_wire.hpp"

#include "kernel_main.hpp"

#include <algorithm>
#include <cstring>
#include <string_view>

namespace t81::ternaryos::kernel {

namespace {

void encode_fixed_string(std::string_view input, char* out, std::size_t size) noexcept {
  std::memset(out, 0, size);
  const auto copy_size = std::min(size - 1, input.size());
  std::memcpy(out, input.data(), copy_size);
}

std::string decode_fixed_string(const char* input, std::size_t size) {
  const auto length = std::find(input, input + size, '\0') - input;
  return std::string(input, length);
}

uint64_t encode_capability_flags(const KernelCapabilityRecord& capability) noexcept {
  uint64_t flags = 0;
  if (capability.process_group_scope.has_value()) {
    flags |= kWireCapabilityHasProcessGroupScope;
  }
  if (capability.kernel_seeded) {
    flags |= kWireCapabilityKernelSeeded;
  }
  if (capability.delegated_by_process_group_id.has_value()) {
    flags |= kWireCapabilityHasDelegatedByProcessGroupId;
  }
  if (capability.delegated_by_supervisor_id.has_value()) {
    flags |= kWireCapabilityHasDelegatedBySupervisorId;
  }
  return flags;
}

KernelCallWireCapabilityRecord encode_capability(
    const KernelCapabilityRecord& capability) noexcept {
  return KernelCallWireCapabilityRecord{
      .flags = encode_capability_flags(capability),
      .record_id = capability.record_id,
      .kind = static_cast<uint32_t>(capability.kind),
      .process_group_scope = capability.process_group_scope.value_or(0),
      .delegated_by_process_group_id =
          capability.delegated_by_process_group_id.value_or(0),
      .delegated_by_supervisor_id =
          capability.delegated_by_supervisor_id.value_or(0),
  };
}

KernelCapabilityRecord decode_capability(
    const KernelCallWireCapabilityRecord& capability) noexcept {
  return KernelCapabilityRecord{
      .record_id = capability.record_id,
      .kind = static_cast<KernelCapabilityKind>(capability.kind),
      .process_group_scope =
          (capability.flags & kWireCapabilityHasProcessGroupScope)
              ? std::optional<ProcessGroupId>{capability.process_group_scope}
              : std::nullopt,
      .kernel_seeded = (capability.flags & kWireCapabilityKernelSeeded) != 0,
      .delegated_by_process_group_id =
          (capability.flags & kWireCapabilityHasDelegatedByProcessGroupId)
              ? std::optional<ProcessGroupId>{capability.delegated_by_process_group_id}
              : std::nullopt,
      .delegated_by_supervisor_id =
          (capability.flags & kWireCapabilityHasDelegatedBySupervisorId)
              ? std::optional<SupervisorId>{capability.delegated_by_supervisor_id}
              : std::nullopt,
  };
}

KernelCallWireMessage encode_message(const ipc::CanonMessage& message) noexcept {
  KernelCallWireMessage wire{
      .sender = message.sender,
      .payload = message.payload,
  };
  encode_fixed_string(message.tag, wire.tag.data(), wire.tag.size());
  std::copy(message.ref.hash.h.bytes.begin(),
            message.ref.hash.h.bytes.end(),
            wire.ref.begin());
  return wire;
}

ipc::CanonMessage decode_message(const KernelCallWireMessage& message) {
  t81::canonfs::CanonRef ref;
  std::copy(message.ref.begin(), message.ref.end(), ref.hash.h.bytes.begin());
  return ipc::CanonMessage{
      .sender = message.sender,
      .ref = ref,
      .payload = message.payload,
      .tag = decode_fixed_string(message.tag.data(), message.tag.size()),
  };
}

KernelCallWireFault encode_fault(const KernelFaultRecord& fault) noexcept {
  KernelCallWireFault wire{
      .tva = fault.tva,
      .access_mode = static_cast<uint32_t>(fault.access_mode),
      .fault = static_cast<uint32_t>(fault.fault),
      .subject_tid = fault.subject_tid,
  };
  encode_fixed_string(fault.platform_id, wire.platform_id.data(), wire.platform_id.size());
  return wire;
}

KernelFaultRecord decode_fault(const KernelCallWireFault& fault) {
  return KernelFaultRecord{
      .platform_id = decode_fixed_string(fault.platform_id.data(), fault.platform_id.size()),
      .tva = fault.tva,
      .access_mode = static_cast<mmu::MmuAccessMode>(fault.access_mode),
      .fault = static_cast<mmu::MmuFault>(fault.fault),
      .subject_tid = fault.subject_tid,
  };
}

}  // namespace

bool axion_kernel_validate_wire_request_block(
    const KernelCallWireRequestBlock& block) noexcept {
  return block.header.magic == kKernelAbiWireRequestMagic &&
         block.header.version == kKernelAbiWireVersion &&
         block.header.bytes == sizeof(KernelCallWireRequestBlock);
}

bool axion_kernel_validate_wire_response_block(
    const KernelCallWireResponseBlock& block) noexcept {
  return block.header.magic == kKernelAbiWireResponseMagic &&
         block.header.version == kKernelAbiWireVersion &&
         block.header.bytes == sizeof(KernelCallWireResponseBlock);
}

KernelCallWireRequestBlock axion_kernel_encode_wire_request(
    const KernelCallRequest& request) noexcept {
  KernelCallWireRequestBlock block;
  block.kind = static_cast<uint32_t>(request.kind);
  if (request.target_tid.has_value()) {
    block.flags |= kWireHasTargetTid;
    block.target_tid = *request.target_tid;
  }
  if (request.process_group_id.has_value()) {
    block.flags |= kWireHasProcessGroupId;
    block.process_group_id = *request.process_group_id;
  }
  if (request.supervisor_id.has_value()) {
    block.flags |= kWireHasSupervisorId;
    block.supervisor_id = *request.supervisor_id;
  }
  if (request.address_space_id.has_value()) {
    block.flags |= kWireHasAddressSpaceId;
    block.address_space_id = *request.address_space_id;
  }
  if (request.capability_transition_sequence.has_value()) {
    block.flags |= kWireHasCapabilityTransitionSequence;
    block.capability_transition_sequence = *request.capability_transition_sequence;
  }
  if (request.boot_critical.has_value()) {
    block.flags |= kWireHasBootCritical;
    block.boot_critical = *request.boot_critical ? 1 : 0;
  }
  if (request.ipc_dst.has_value()) {
    block.flags |= kWireHasIpcDst;
    block.ipc_dst = *request.ipc_dst;
  }
  if (request.message.has_value()) {
    block.flags |= kWireHasMessage;
    if (!request.message->tag.empty()) {
      block.flags |= kWireMessageHasTag;
    }
    block.message = encode_message(*request.message);
  }
  if (request.capability.has_value()) {
    block.flags |= kWireHasCapability;
    block.capability = encode_capability(*request.capability);
  }
  if (request.spawn_descriptor.has_value()) {
    block.flags |= kWireHasSpawnDescriptor;
    block.spawn_pc = request.spawn_descriptor->pc;
    block.spawn_sp = request.spawn_descriptor->sp;
    block.spawn_register0 = request.spawn_descriptor->register0;
    block.spawn_halted = request.spawn_descriptor->halted ? 1 : 0;
    block.spawn_active = request.spawn_descriptor->active ? 1 : 0;
    encode_fixed_string(request.spawn_descriptor->label,
                        block.spawn_label.data(),
                        block.spawn_label.size());
  }
  if (request.service_id.has_value()) {
    block.flags |= kWireHasServiceId;
    block.service_id = *request.service_id;
  }
  if (request.service_name.has_value()) {
    block.flags |= kWireHasServiceName;
    encode_fixed_string(
        *request.service_name, block.service_name.data(), block.service_name.size());
  }
  return block;
}

std::optional<KernelCallRequest> axion_kernel_decode_wire_request(
    const KernelCallWireRequestBlock& block) noexcept {
  if (!axion_kernel_validate_wire_request_block(block)) {
    return std::nullopt;
  }
  KernelCallRequest request{
      .kind = static_cast<KernelCallKind>(block.kind),
  };
  if (block.flags & kWireHasTargetTid) {
    request.target_tid = block.target_tid;
  }
  if (block.flags & kWireHasProcessGroupId) {
    request.process_group_id = block.process_group_id;
  }
  if (block.flags & kWireHasSupervisorId) {
    request.supervisor_id = block.supervisor_id;
  }
  if (block.flags & kWireHasAddressSpaceId) {
    request.address_space_id = block.address_space_id;
  }
  if (block.flags & kWireHasCapabilityTransitionSequence) {
    request.capability_transition_sequence = block.capability_transition_sequence;
  }
  if (block.flags & kWireHasBootCritical) {
    request.boot_critical = block.boot_critical != 0;
  }
  if (block.flags & kWireHasIpcDst) {
    request.ipc_dst = block.ipc_dst;
  }
  if (block.flags & kWireHasMessage) {
    request.message = decode_message(block.message);
  }
  if (block.flags & kWireHasCapability) {
    request.capability = decode_capability(block.capability);
  }
  if (block.flags & kWireHasSpawnDescriptor) {
    request.spawn_descriptor = KernelThreadSpawnDescriptor{
        .pc = static_cast<std::size_t>(block.spawn_pc),
        .sp = static_cast<std::size_t>(block.spawn_sp),
        .register0 = block.spawn_register0,
        .halted = block.spawn_halted != 0,
        .active = block.spawn_active != 0,
        .label = decode_fixed_string(block.spawn_label.data(), block.spawn_label.size()),
    };
  }
  if (block.flags & kWireHasServiceId) {
    request.service_id = block.service_id;
  }
  if (block.flags & kWireHasServiceName) {
    request.service_name =
        decode_fixed_string(block.service_name.data(), block.service_name.size());
  }
  return request;
}

KernelCallWireResponseBlock axion_kernel_encode_wire_response(
    const KernelCallResult& result) noexcept {
  KernelCallWireResponseBlock block;
  block.status = static_cast<uint32_t>(result.status);
  block.rejection = static_cast<uint32_t>(result.rejection);
  block.action_performed = result.action_performed ? 1 : 0;
  block.yielded = result.yielded ? 1 : 0;
  if (result.thread_exited) {
    block.flags |= kWireResponseThreadExited;
  }
  if (result.spawned_tid.has_value()) {
    block.flags |= kWireResponseHasSpawnedTid;
    block.spawned_tid = *result.spawned_tid;
  }
  if (result.queried_tid.has_value()) {
    block.flags |= kWireResponseHasQueriedTid;
    block.queried_tid = *result.queried_tid;
  }
  block.service_registered = result.service_registered ? 1 : 0;
  block.service_suspended = result.service_suspended ? 1 : 0;
  block.service_unhealthy = result.service_unhealthy ? 1 : 0;
  block.service_blocked = result.service_blocked ? 1 : 0;
  block.thread_state_bits =
      (result.thread_active ? 0x1u : 0u) |
      (result.thread_halted ? 0x2u : 0u) |
      (result.thread_running ? 0x4u : 0u);
  block.thread_pc = result.thread_pc;
  block.thread_sp = result.thread_sp;
  block.thread_register0 = result.thread_register0;
  if (result.caller_tid.has_value()) {
    block.flags |= kWireResponseHasCallerTid;
    block.caller_tid = *result.caller_tid;
  }
  if (result.caller_process_group_id.has_value()) {
    block.flags |= kWireResponseHasCallerProcessGroupId;
    block.caller_process_group_id = *result.caller_process_group_id;
  }
  if (result.message.has_value()) {
    block.flags |= kWireResponseHasMessage;
    block.message = encode_message(*result.message);
  }
  if (result.fault.has_value()) {
    block.flags |= kWireResponseHasFault;
    block.fault = encode_fault(*result.fault);
  }
  if (result.service_id.has_value()) {
    block.flags |= kWireResponseHasServiceId;
    block.service_id = *result.service_id;
  }
  if (result.service_name.has_value()) {
    block.flags |= kWireResponseHasServiceName;
    encode_fixed_string(
        *result.service_name, block.service_name.data(), block.service_name.size());
  }
  if (!result.thread_label.empty()) {
    encode_fixed_string(
        result.thread_label, block.thread_label.data(), block.thread_label.size());
  }
  if (result.supervisor_id.has_value()) {
    block.flags |= kWireResponseHasSupervisorId;
    block.supervisor_id = *result.supervisor_id;
  }
  if (result.address_space_id.has_value()) {
    block.flags |= kWireResponseHasAddressSpaceId;
    block.address_space_id = *result.address_space_id;
  }
  if (result.target_process_group_id.has_value()) {
    block.flags |= kWireResponseHasTargetProcessGroupId;
    block.target_process_group_id = *result.target_process_group_id;
  }
  if (result.supervisor_last_pending_group.has_value()) {
    block.flags |= kWireResponseHasSupervisorLastPendingGroup;
    block.supervisor_last_pending_group = *result.supervisor_last_pending_group;
  }
  if (result.supervisor_last_acknowledged_group.has_value()) {
    block.flags |= kWireResponseHasSupervisorLastAcknowledgedGroup;
    block.supervisor_last_acknowledged_group = *result.supervisor_last_acknowledged_group;
  }
  if (result.supervisor_last_recovered_group.has_value()) {
    block.flags |= kWireResponseHasSupervisorLastRecoveredGroup;
    block.supervisor_last_recovered_group = *result.supervisor_last_recovered_group;
  }
  block.process_group_owned_page_count = result.process_group_owned_page_count;
  block.process_group_pending_fault_count = result.process_group_pending_fault_count;
  block.runtime_mapped_pages = result.runtime_mapped_pages;
  block.runtime_boot_critical_address_space_count =
      result.runtime_boot_critical_address_space_count;
  block.fault_summary_recorded_faults = result.fault_summary_recorded_faults;
  block.fault_summary_pending_faults = result.fault_summary_pending_faults;
  block.fault_summary_delivered_faults = result.fault_summary_delivered_faults;
  block.fault_summary_routed_thread_faults = result.fault_summary_routed_thread_faults;
  block.fault_summary_quarantined_threads = result.fault_summary_quarantined_threads;
  block.supervisor_fault_notifications = result.supervisor_fault_notifications;
  block.supervisor_acknowledgements = result.supervisor_acknowledgements;
  block.supervisor_recovered_groups = result.supervisor_recovered_groups;
  block.supervisor_capability_transitions = result.supervisor_capability_transitions;
  block.supervisor_delegated_capability_count = result.supervisor_delegated_capability_count;
  block.supervisor_managed_group_count = result.supervisor_managed_group_count;
  block.supervisor_managed_faulted_group_count =
      result.supervisor_managed_faulted_group_count;
  block.supervisor_pending_group_count = result.supervisor_pending_group_count;
  block.supervisor_capability_process_group_count =
      result.supervisor_capability_process_group_count;
  block.supervisor_delegation_process_group_count =
      result.supervisor_delegation_process_group_count;
  block.supervisor_delegation_entry_count = result.supervisor_delegation_entry_count;
  block.supervisor_last_capability_transition_group_id =
      result.supervisor_last_capability_transition_group_id.value_or(0);
  block.supervisor_last_capability_transition_record_id =
      result.supervisor_last_capability_transition_record_id.value_or(0);
  block.capability_count = static_cast<uint32_t>(
      std::min(result.capabilities.size(), block.capabilities.size()));
  for (std::size_t i = 0; i < block.capability_count; ++i) {
    block.capabilities[i] = encode_capability(result.capabilities[i]);
  }
  block.delegation_entry_count = static_cast<uint32_t>(
      std::min(result.supervisor_delegation_entries.size(),
               static_cast<std::size_t>(kKernelAbiWireCapabilitySlots)));
  return block;
}

std::optional<KernelCallResult> axion_kernel_decode_wire_response(
    const KernelCallWireResponseBlock& block) noexcept {
  if (!axion_kernel_validate_wire_response_block(block)) {
    return std::nullopt;
  }
  KernelCallResult result{
      .status = static_cast<KernelCallStatus>(block.status),
      .rejection = static_cast<KernelCallRejection>(block.rejection),
      .action_performed = block.action_performed != 0,
      .yielded = block.yielded != 0,
      .service_registered = block.service_registered != 0,
      .service_suspended = block.service_suspended != 0,
      .service_unhealthy = block.service_unhealthy != 0,
      .service_blocked = block.service_blocked != 0,
      .process_group_owned_page_count = block.process_group_owned_page_count,
      .process_group_pending_fault_count = block.process_group_pending_fault_count,
      .runtime_mapped_pages = block.runtime_mapped_pages,
      .runtime_boot_critical_address_space_count =
          block.runtime_boot_critical_address_space_count,
      .fault_summary_recorded_faults = block.fault_summary_recorded_faults,
      .fault_summary_pending_faults = block.fault_summary_pending_faults,
      .fault_summary_delivered_faults = block.fault_summary_delivered_faults,
      .fault_summary_routed_thread_faults = block.fault_summary_routed_thread_faults,
      .fault_summary_quarantined_threads = block.fault_summary_quarantined_threads,
      .supervisor_fault_notifications = block.supervisor_fault_notifications,
      .supervisor_acknowledgements = block.supervisor_acknowledgements,
      .supervisor_recovered_groups = block.supervisor_recovered_groups,
      .supervisor_capability_transitions = block.supervisor_capability_transitions,
      .supervisor_managed_group_count = block.supervisor_managed_group_count,
      .supervisor_managed_faulted_group_count =
          block.supervisor_managed_faulted_group_count,
      .supervisor_pending_group_count = block.supervisor_pending_group_count,
      .supervisor_capability_process_group_count =
          block.supervisor_capability_process_group_count,
      .supervisor_delegation_process_group_count =
          block.supervisor_delegation_process_group_count,
      .supervisor_delegation_entry_count = block.supervisor_delegation_entry_count,
      .supervisor_delegated_capability_count = block.supervisor_delegated_capability_count,
  };
  result.thread_exited = (block.flags & kWireResponseThreadExited) != 0;
  if (block.flags & kWireResponseHasSpawnedTid) {
    result.spawned_tid = block.spawned_tid;
  }
  if (block.flags & kWireResponseHasQueriedTid) {
    result.queried_tid = block.queried_tid;
  }
  if (block.flags & kWireResponseHasCallerTid) {
    result.caller_tid = block.caller_tid;
  }
  if (block.flags & kWireResponseHasCallerProcessGroupId) {
    result.caller_process_group_id = block.caller_process_group_id;
  }
  if (block.flags & kWireResponseHasMessage) {
    result.message = decode_message(block.message);
  }
  if (block.flags & kWireResponseHasFault) {
    result.fault = decode_fault(block.fault);
  }
  if (block.flags & kWireResponseHasServiceId) {
    result.service_id = block.service_id;
  }
  if (block.flags & kWireResponseHasServiceName) {
    result.service_name =
        decode_fixed_string(block.service_name.data(), block.service_name.size());
  }
  if (block.flags & kWireResponseHasSupervisorId) {
    result.supervisor_id = block.supervisor_id;
  }
  if (block.flags & kWireResponseHasAddressSpaceId) {
    result.address_space_id = block.address_space_id;
  }
  if (block.flags & kWireResponseHasTargetProcessGroupId) {
    result.target_process_group_id = block.target_process_group_id;
  }
  result.thread_pc = static_cast<std::size_t>(block.thread_pc);
  result.thread_sp = static_cast<std::size_t>(block.thread_sp);
  result.thread_register0 = block.thread_register0;
  result.thread_active = (block.thread_state_bits & 0x1u) != 0;
  result.thread_halted = (block.thread_state_bits & 0x2u) != 0;
  result.thread_running = (block.thread_state_bits & 0x4u) != 0;
  result.thread_label =
      decode_fixed_string(block.thread_label.data(), block.thread_label.size());
  if (block.flags & kWireResponseHasSupervisorLastPendingGroup) {
    result.supervisor_last_pending_group = block.supervisor_last_pending_group;
  }
  if (block.flags & kWireResponseHasSupervisorLastAcknowledgedGroup) {
    result.supervisor_last_acknowledged_group = block.supervisor_last_acknowledged_group;
  }
  if (block.flags & kWireResponseHasSupervisorLastRecoveredGroup) {
    result.supervisor_last_recovered_group = block.supervisor_last_recovered_group;
  }
  if (block.supervisor_last_capability_transition_group_id != 0) {
    result.supervisor_last_capability_transition_group_id =
        block.supervisor_last_capability_transition_group_id;
  }
  if (block.supervisor_last_capability_transition_record_id != 0) {
    result.supervisor_last_capability_transition_record_id =
        block.supervisor_last_capability_transition_record_id;
  }
  for (std::size_t i = 0; i < block.capability_count && i < block.capabilities.size(); ++i) {
    result.capabilities.push_back(decode_capability(block.capabilities[i]));
  }
  return result;
}

namespace {

KernelCallWireResponseBlock make_invalid_wire_response() noexcept {
  return axion_kernel_encode_wire_response(KernelCallResult{
      .status = KernelCallStatus::InvalidRequest,
      .rejection = KernelCallRejection::None,
  });
}

}  // namespace

bool axion_kernel_call_wire(KernelRuntimeState& state,
                            const KernelCallWireRequestBlock* request_block,
                            KernelCallWireResponseBlock* response_block) noexcept {
  if (!response_block) {
    return false;
  }

  if (!request_block || !axion_kernel_validate_wire_request_block(*request_block)) {
    *response_block = make_invalid_wire_response();
    return true;
  }

  const auto request = axion_kernel_decode_wire_request(*request_block);
  if (!request.has_value()) {
    *response_block = make_invalid_wire_response();
    return true;
  }

  *response_block = axion_kernel_encode_wire_response(
      axion_kernel_call(state, *request));
  return true;
}

bool axion_kernel_call_wire_bytes(KernelRuntimeState& state,
                                  const void* request_bytes,
                                  std::size_t request_size,
                                  void* response_bytes,
                                  std::size_t response_size) noexcept {
  if (!response_bytes || response_size < sizeof(KernelCallWireResponseBlock)) {
    return false;
  }

  auto* response_block =
      static_cast<KernelCallWireResponseBlock*>(response_bytes);
  if (!request_bytes || request_size < sizeof(KernelCallWireRequestBlock)) {
    *response_block = make_invalid_wire_response();
    return true;
  }

  KernelCallWireRequestBlock request_block;
  std::memcpy(&request_block, request_bytes, sizeof(request_block));
  return axion_kernel_call_wire(state, &request_block, response_block);
}

}  // namespace t81::ternaryos::kernel
