#include "kernel_main.hpp"

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
  state.supervisors.emplace(
      KernelRuntimeState::kKernelSupervisor,
      KernelRuntimeState::SupervisorState{
          .id = KernelRuntimeState::kKernelSupervisor,
          .managed_groups = {KernelRuntimeState::kKernelProcessGroup},
      });
  state.process_group_supervisors.emplace(KernelRuntimeState::kKernelProcessGroup,
                                          KernelRuntimeState::kKernelSupervisor);
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
  if (!group || !supervisor) {
    if (group) {
      state.process_groups.erase(group->id);
    }
    if (supervisor) {
      state.supervisors.erase(supervisor->id);
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
  } else {
    state.process_groups.erase(group->id);
    state.supervisors.erase(supervisor->id);
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
        const bool entering_fault_state = !group_state->faulted;
        group_state->faulted = true;
        group_state->blocked = true;
        group_state->acknowledgement_pending = true;
        ++group_state->pending_fault_count;
        ++group_state->counters.fault_entries;
        ++state.counters.process_group_fault_entries;
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
      result.status = KernelServiceStatus::Ok;
      result.runtime = KernelRuntimeStatusView{
          .platform_id = state.platform_id,
          .memory_region_count = state.memory_region_count,
          .total_ternary_pages = state.total_ternary_pages,
          .loop_iterations = state.counters.loop_iterations,
          .scheduler_ticks = state.counters.scheduler_ticks,
          .ipc_messages_sent = state.counters.ipc_messages_sent,
          .ipc_messages_received = state.counters.ipc_messages_received,
      };
      return result;
    }
    case KernelServiceRequestKind::ProcessGroupStatus: {
      if (!request.process_group_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        return result;
      }
      const auto* group_state = state.find_process_group(*request.process_group_id);
      if (!group_state) {
        result.status = KernelServiceStatus::NotFound;
        return result;
      }
      result.status = group_state->faulted ? KernelServiceStatus::FaultedGroup
                                           : KernelServiceStatus::Ok;
      result.process_group = KernelProcessGroupStatusView{
          .id = group_state->id,
          .member_count = group_state->member_tids.size(),
          .faulted = group_state->faulted,
          .blocked = group_state->blocked,
          .acknowledgement_pending = group_state->acknowledgement_pending,
          .pending_fault_count = group_state->pending_fault_count,
          .supervisor_id = state.find_process_group_supervisor(group_state->id),
      };
      return result;
    }
    case KernelServiceRequestKind::SupervisorStatus: {
      if (!request.supervisor_id.has_value()) {
        result.status = KernelServiceStatus::InvalidRequest;
        return result;
      }
      const auto* supervisor_state = state.find_supervisor(*request.supervisor_id);
      if (!supervisor_state) {
        result.status = KernelServiceStatus::NotFound;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.supervisor = KernelSupervisorStatusView{
          .id = supervisor_state->id,
          .managed_group_count = supervisor_state->managed_groups.size(),
          .pending_group_count = supervisor_state->pending_groups.size(),
          .fault_notifications = supervisor_state->fault_notifications,
          .acknowledgements = supervisor_state->acknowledgements,
      };
      return result;
    }
    case KernelServiceRequestKind::FaultSummary: {
      result.status = KernelServiceStatus::Ok;
      result.fault_summary = KernelFaultSummaryView{
          .recorded_faults = state.fault_count(),
          .pending_faults = state.pending_fault_count(),
          .audit_events = state.audit_count(),
          .last_delivered_fault = state.last_delivered_fault,
      };
      return result;
    }
    case KernelServiceRequestKind::DeviceSummary: {
      if (!state.device_arbitration.has_value()) {
        result.status = KernelServiceStatus::NoDeviceArbitration;
        return result;
      }
      result.status = KernelServiceStatus::Ok;
      result.device_summary = KernelDeviceSummaryView{
          .has_device_arbitration = true,
          .device_count = state.device_arbitration->devices.size(),
          .has_storage = state.device_arbitration->has_storage,
          .has_network = state.device_arbitration->has_network,
          .has_display = state.device_arbitration->has_display,
      };
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
  ++state.counters.supervisor_acknowledgements;
  record_audit_event(state,
                     KernelAuditEventKind::SupervisorGroupAcknowledged,
                     KernelRuntimeState::kKernelTid,
                     process_group_id);
  if (!axion_kernel_ack_process_group_fault(state, process_group_id)) {
    --supervisor_state->acknowledgements;
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
  return true;
}

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
