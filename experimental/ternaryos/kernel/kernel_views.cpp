#include "kernel_main.hpp"

#include <algorithm>

namespace t81::ternaryos::kernel {

namespace {

void increment_interrupt_source_counter(KernelInterruptSourceCounters& counters,
                                        hal::InterruptSource source) {
  switch (source) {
    case hal::InterruptSource::Timer:
      ++counters.timer;
      break;
    case hal::InterruptSource::Storage:
      ++counters.storage;
      break;
    case hal::InterruptSource::Network:
      ++counters.network;
      break;
    case hal::InterruptSource::Keyboard:
      ++counters.keyboard;
      break;
    case hal::InterruptSource::Unknown:
      ++counters.unknown;
      break;
  }
}

KernelInterruptSourceCounters count_pending_interrupt_sources(
    const KernelRuntimeState& state) {
  KernelInterruptSourceCounters counters;
  for (const auto& interrupt : state.pending_interrupts) {
    increment_interrupt_source_counter(counters, interrupt.source);
  }
  return counters;
}

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

const KernelRuntimeState::AddressSpaceState* find_group_address_space_state(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) {
  const auto address_space_id = state.find_process_group_address_space(process_group_id);
  if (!address_space_id.has_value()) {
    return nullptr;
  }
  return state.find_address_space(*address_space_id);
}

std::size_t count_pager_needed_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) { return entry.second.pager_needed; });
}

std::size_t count_pager_terminal_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) { return entry.second.pager_terminal; });
}

std::size_t count_boot_critical_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) { return entry.second.boot_critical; });
}

std::size_t count_boot_critical_pager_needed_address_spaces(
    const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) {
                         return entry.second.boot_critical &&
                                entry.second.pager_needed &&
                                !entry.second.pager_terminal;
                       });
}

std::size_t count_boot_critical_terminal_address_spaces(
    const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) {
                         return entry.second.boot_critical &&
                                entry.second.pager_terminal;
                       });
}

std::size_t count_pending_pager_handoff_address_spaces(const KernelRuntimeState& state) {
  return std::count_if(state.address_spaces.begin(),
                       state.address_spaces.end(),
                       [](const auto& entry) {
                         return entry.second.pager_handoff_pending;
                       });
}

std::size_t count_ready_pager_backlog_address_spaces(
    const KernelRuntimeState& state) {
  if (!state.pager_worker.active_work.has_value()) {
    return 0;
  }
  return static_cast<std::size_t>(std::count_if(
      state.pager_worker.inbox.begin(),
      state.pager_worker.inbox.end(),
      [&](const auto& work_item) {
        const auto* address_space =
            state.find_address_space(work_item.handoff.address_space_id);
        if (!address_space || !address_space->last_pager_fault.has_value()) {
          return false;
        }
        const auto translation = mmu::mmu_translate_checked(
            state.page_table,
            address_space->last_pager_fault->tva,
            address_space->last_pager_fault->access_mode);
        return translation.fault == mmu::MmuFault::None;
      }));
}

std::size_t count_parked_ready_pager_backlog_address_spaces(
    const KernelRuntimeState& state) {
  if (state.pager_worker.active_work.has_value() ||
      !state.pager_worker.parked_blocked_address_space_id.has_value()) {
    return 0;
  }
  std::size_t ready_count = 0;
  for (const auto& work_item : state.pager_worker.inbox) {
    const auto* address_space =
        state.find_address_space(work_item.handoff.address_space_id);
    if (!address_space || !address_space->last_pager_fault.has_value()) {
      continue;
    }
    const auto translation = mmu::mmu_translate_checked(
        state.page_table,
        address_space->last_pager_fault->tva,
        address_space->last_pager_fault->access_mode);
    if (translation.fault == mmu::MmuFault::None) {
      ++ready_count;
    }
  }
  return ready_count;
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

uint64_t count_managed_pager_resolutions(
    const KernelRuntimeState& state,
    const KernelRuntimeState::SupervisorState& supervisor) {
  uint64_t count = 0;
  for (auto group_id : supervisor.managed_groups) {
    const auto* address_space = find_group_address_space_state(state, group_id);
    if (address_space) {
      count += address_space->pager_resolutions;
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

KernelSupervisorServiceInventoryView make_supervisor_services_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  return KernelSupervisorServiceInventoryView{
      .supervisor_id = supervisor_state ? supervisor_state->id : supervisor_id,
      .service_count = supervisor_state ? supervisor_state->managed_services.size() : 0,
      .service_lifecycle_transitions =
          supervisor_state ? supervisor_state->service_lifecycle_transitions : 0,
      .capability_transitions =
          supervisor_state ? supervisor_state->capability_transitions : 0,
      .last_service_transition_id =
          supervisor_state ? supervisor_state->last_service_transition_id : std::nullopt,
      .last_service_transition_kind =
          supervisor_state ? supervisor_state->last_service_transition_kind : std::nullopt,
      .last_service_transition_sequence = supervisor_state
                                              ? supervisor_state->last_service_transition_sequence
                                              : std::nullopt,
      .last_capability_transition_group_id =
          supervisor_state ? supervisor_state->last_capability_transition_group_id
                           : std::nullopt,
      .last_capability_transition_record_id =
          supervisor_state ? supervisor_state->last_capability_transition_record_id
                           : std::nullopt,
      .last_capability_transition_kind =
          supervisor_state ? supervisor_state->last_capability_transition_kind
                           : std::nullopt,
      .last_capability_transition_sequence =
          supervisor_state ? supervisor_state->last_capability_transition_sequence
                           : std::nullopt,
      .recent_capability_transitions =
          supervisor_state ? std::vector<KernelCapabilityTransitionRecord>(
                                 supervisor_state->recent_capability_transitions.begin(),
                                 supervisor_state->recent_capability_transitions.end())
                           : std::vector<KernelCapabilityTransitionRecord>{},
      .service_ids = {},
      .services = {},
  };
}

}  // namespace

KernelRuntimeStatusView make_runtime_view(const KernelRuntimeState& state) {
  const auto service_summary = runtime_service_summary(state);
  const auto latest_service_transition = latest_service_transition_view(state);
  const auto pending_interrupt_sources = count_pending_interrupt_sources(state);
  const auto ready_pager_backlog_count =
      count_ready_pager_backlog_address_spaces(state);
  const auto parked_ready_pager_backlog_count =
      count_parked_ready_pager_backlog_address_spaces(state);
  const auto boot_critical_pager_needed_count =
      count_boot_critical_pager_needed_address_spaces(state);
  const auto boot_critical_terminal_count =
      count_boot_critical_terminal_address_spaces(state);
  return KernelRuntimeStatusView{
      .platform_id = state.platform_id,
      .memory_region_count = state.memory_region_count,
      .total_ternary_pages = state.total_ternary_pages,
      .address_space_count = state.address_space_count(),
      .mapped_pages = state.page_table.size(),
      .boot_critical_address_space_count =
          count_boot_critical_address_spaces(state),
      .boot_critical_pager_needed_count = boot_critical_pager_needed_count,
      .boot_critical_terminal_count = boot_critical_terminal_count,
      .boot_progress_pending = boot_critical_pager_needed_count > 0,
      .boot_progress_blocked = boot_critical_terminal_count > 0,
      .pager_needed_address_space_count = count_pager_needed_address_spaces(state),
      .pager_terminal_address_space_count =
          count_pager_terminal_address_spaces(state),
      .pending_pager_handoff_count = count_pending_pager_handoff_address_spaces(state),
      .pending_pager_handoff_high_watermark =
          state.pending_pager_handoff_high_watermark,
      .pager_worker_inbox_count = state.pager_worker.inbox.size(),
      .pager_worker_inbox_high_watermark =
          state.pager_worker.inbox_high_watermark,
      .pager_worker_ready_backlog_count = ready_pager_backlog_count,
      .pager_worker_ready_backlog_high_watermark =
          state.pager_worker.ready_backlog_high_watermark,
      .pager_worker_parked_ready_count = parked_ready_pager_backlog_count,
      .pager_worker_parked_ready_high_watermark =
          state.pager_worker.parked_ready_high_watermark,
      .pager_worker_busy = state.pager_worker.active_work.has_value(),
      .pager_worker_active_address_space_id =
          state.pager_worker.active_work.has_value()
              ? std::optional<AddressSpaceId>{
                    state.pager_worker.active_work->handoff.address_space_id}
              : std::nullopt,
      .pager_worker_active_handoff_sequence =
          state.pager_worker.active_work.has_value()
              ? std::optional<uint64_t>{
                    state.pager_worker.active_work->handoff.sequence}
              : std::nullopt,
      .pager_worker_next_queued_address_space_id =
          !state.pager_worker.inbox.empty()
              ? std::optional<AddressSpaceId>{
                    state.pager_worker.inbox.front().handoff.address_space_id}
              : std::nullopt,
      .pager_worker_next_queued_handoff_sequence =
          !state.pager_worker.inbox.empty()
              ? std::optional<uint64_t>{
                    state.pager_worker.inbox.front().handoff.sequence}
              : std::nullopt,
      .loop_iterations = state.counters.loop_iterations,
      .scheduler_ticks = state.counters.scheduler_ticks,
      .ipc_messages_sent = state.counters.ipc_messages_sent,
      .ipc_messages_received = state.counters.ipc_messages_received,
      .pending_interrupt_count = state.pending_interrupt_count(),
      .pending_interrupt_high_watermark = state.pending_interrupt_high_watermark,
      .pending_interrupt_sources = pending_interrupt_sources,
      .interrupts_recorded = state.counters.interrupts_recorded,
      .interrupts_delivered = state.counters.interrupts_delivered,
      .interrupt_sources_recorded = state.counters.interrupt_sources_recorded,
      .interrupt_sources_delivered = state.counters.interrupt_sources_delivered,
      .next_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.front()}
              : std::nullopt,
      .last_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.back()}
              : std::nullopt,
      .last_recorded_interrupt_audit_sequence =
          state.last_recorded_interrupt_audit_sequence,
      .last_delivered_interrupt_audit_sequence =
          state.last_delivered_interrupt_audit_sequence,
      .last_interrupt_audit_kind = state.last_interrupt_audit_kind,
      .last_interrupt_audit_source = state.last_interrupt_audit_source,
      .last_interrupt_audit_interrupt_sequence =
          state.last_interrupt_audit_interrupt_sequence,
      .last_interrupt_audit_payload = state.last_interrupt_audit_payload,
      .last_interrupt_audit_timestamp_ns =
          state.last_interrupt_audit_timestamp_ns,
      .last_interrupt_audit_sequence = state.last_interrupt_audit_sequence,
      .last_recorded_interrupt = state.last_recorded_interrupt,
      .last_delivered_interrupt = state.last_delivered_interrupt,
      .timer_interrupts_handled = state.counters.timer_interrupts_handled,
      .timer_preempts = state.counters.timer_preempts,
      .device_interrupts_handled = state.counters.device_interrupts_handled,
      .last_timer_preempt_cycle = state.last_timer_preempt_cycle,
      .last_timer_preempt_sequence = state.last_timer_preempt_sequence,
      .ipc_blocks = state.counters.ipc_blocks,
      .ipc_wakes = state.counters.ipc_wakes,
      .ipc_blocked_thread_count = state.ipc_blocked_tids.size(),
      .device_wakes = state.counters.device_wakes,
      .device_waiting_thread_count = [&state]() -> std::size_t {
        std::size_t n = 0;
        for (const auto& [k, s] : state.device_waiting_tids) n += s.size();
        return n;
      }(),
      .syscall_trap_dispatches = state.counters.syscall_trap_dispatches,
      .kernel_space_rejections = state.counters.kernel_space_rejections,
      .canonfs_fetch_spawns = state.counters.canonfs_fetch_spawns,
      .pager_service_mappings = state.counters.pager_service_mappings,
      .pager_handoff_wakes = state.counters.pager_handoff_wakes,
      .pager_service_resumptions = state.counters.pager_service_resumptions,
      .pager_handoff_waiting_thread_count = state.pager_handoff_waiting_tids.size(),
      .pager_eligible_faults = state.counters.pager_eligible_faults,
      .policy_faults = state.counters.policy_faults,
      .pager_handoffs_dispatched = state.counters.pager_handoffs_dispatched,
      .pager_resolutions = state.counters.pager_resolutions,
      .pager_faults_coalesced = state.counters.pager_faults_coalesced,
      .pager_worker_handoffs_received = state.pager_worker.handoffs_received,
      .pager_worker_last_received_address_space_id =
          state.pager_worker.last_received_address_space_id,
      .pager_worker_last_received_handoff_sequence =
          state.pager_worker.last_received_handoff_sequence,
      .pager_worker_ready_bypass_activations =
          state.pager_worker.ready_bypass_activations,
      .pager_worker_last_ready_bypass_blocked_address_space_id =
          state.pager_worker.last_ready_bypass_blocked_address_space_id,
      .pager_worker_last_ready_bypass_promoted_address_space_id =
          state.pager_worker.last_ready_bypass_promoted_address_space_id,
      .pager_worker_last_ready_bypass_cycle =
          state.pager_worker.last_ready_bypass_cycle,
      .pager_worker_ready_bypass_deferrals =
          state.pager_worker.ready_bypass_deferrals,
      .pager_worker_last_ready_bypass_deferred_blocked_address_space_id =
          state.pager_worker.last_ready_bypass_deferred_blocked_address_space_id,
      .pager_worker_last_ready_bypass_deferred_ready_address_space_id =
          state.pager_worker.last_ready_bypass_deferred_ready_address_space_id,
      .pager_worker_last_ready_bypass_deferred_cycle =
          state.pager_worker.last_ready_bypass_deferred_cycle,
      .pager_worker_parked_cycles = state.pager_worker.parked_cycles,
      .pager_worker_parked_resumptions =
          state.pager_worker.parked_resumptions,
      .pager_worker_last_parked_blocked_address_space_id =
          state.pager_worker.last_parked_blocked_address_space_id,
      .pager_worker_last_parked_ready_address_space_id =
          state.pager_worker.last_parked_ready_address_space_id,
      .pager_worker_last_parked_cycle =
          state.pager_worker.last_parked_cycle,
      .pager_worker_last_parked_ready_count =
          state.pager_worker.last_parked_ready_count,
      .pager_worker_last_parked_resumed_address_space_id =
          state.pager_worker.last_parked_resumed_address_space_id,
      .pager_worker_last_parked_resumed_handoff_sequence =
          state.pager_worker.last_parked_resumed_handoff_sequence,
      .pager_worker_last_parked_resumption_cycle =
          state.pager_worker.last_parked_resumption_cycle,
      .pager_worker_last_parked_resumed_ready_count =
          state.pager_worker.last_parked_resumed_ready_count,
      .pager_worker_last_parked_resumed_ready_address_space_id =
          state.pager_worker.last_parked_resumed_ready_address_space_id,
      .pager_worker_last_parked_resumed_ready_handoff_sequence =
          state.pager_worker.last_parked_resumed_ready_handoff_sequence,
      .pager_worker_parked_resolved_heads =
          state.pager_worker.parked_resolved_heads,
      .pager_worker_last_parked_resolved_address_space_id =
          state.pager_worker.last_parked_resolved_address_space_id,
      .pager_worker_last_parked_resolved_handoff_sequence =
          state.pager_worker.last_parked_resolved_handoff_sequence,
      .pager_worker_last_parked_resolved_resolution_sequence =
          state.pager_worker.last_parked_resolved_resolution_sequence,
      .pager_worker_last_parked_resolved_remaining_inbox_count =
          state.pager_worker.last_parked_resolved_remaining_inbox_count,
      .pager_worker_last_parked_resolved_remaining_address_space_id =
          state.pager_worker.last_parked_resolved_remaining_address_space_id,
      .pager_worker_last_parked_resolved_remaining_handoff_sequence =
          state.pager_worker.last_parked_resolved_remaining_handoff_sequence,
      .pager_worker_parked_resolution_follow_on_activations =
          state.pager_worker.parked_resolution_follow_on_activations,
      .pager_worker_last_parked_resolution_follow_on_address_space_id =
          state.pager_worker.last_parked_resolution_follow_on_address_space_id,
      .pager_worker_last_parked_resolution_follow_on_handoff_sequence =
          state.pager_worker.last_parked_resolution_follow_on_handoff_sequence,
      .pager_worker_last_parked_resolution_follow_on_activation_cycle =
          state.pager_worker.last_parked_resolution_follow_on_activation_cycle,
      .pager_worker_parked_resolution_follow_on_resolutions =
          state.pager_worker.parked_resolution_follow_on_resolutions,
      .pager_worker_last_parked_resolution_follow_on_resolved_address_space_id =
          state.pager_worker
              .last_parked_resolution_follow_on_resolved_address_space_id,
      .pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence =
          state.pager_worker
              .last_parked_resolution_follow_on_resolved_handoff_sequence,
      .pager_worker_last_parked_resolution_follow_on_resolution_sequence =
          state.pager_worker
              .last_parked_resolution_follow_on_resolution_sequence,
      .pager_worker_activations = state.pager_worker.activations,
      .pager_worker_last_activated_address_space_id =
          state.pager_worker.last_activated_address_space_id,
      .pager_worker_last_activation_cycle =
          state.pager_worker.last_activation_cycle,
      .pager_worker_stall_cycles = state.pager_worker.stall_cycles,
      .pager_worker_backlog_blocked_cycles =
          state.pager_worker.backlog_blocked_cycles,
      .pager_worker_ready_backlog_cycles =
          state.pager_worker.ready_backlog_cycles,
      .pager_worker_resolutions_completed =
          state.pager_worker.resolutions_completed,
      .pager_worker_last_completed_address_space_id =
          state.pager_worker.last_completed_address_space_id,
      .pager_worker_last_completed_resolution_sequence =
          state.pager_worker.last_completed_resolution_sequence,
      .pager_worker_last_stalled_address_space_id =
          state.pager_worker.last_stalled_address_space_id,
      .pager_worker_last_stall_cycle = state.pager_worker.last_stall_cycle,
      .pager_worker_last_ready_backlog_address_space_id =
          state.pager_worker.last_ready_backlog_address_space_id,
      .pager_worker_last_ready_backlog_cycle =
          state.pager_worker.last_ready_backlog_cycle,
      .pager_worker_last_ready_backlog_count =
          state.pager_worker.last_ready_backlog_count,
      .pager_worker_terminal_failures = state.pager_worker.terminal_failures,
      .pager_worker_last_terminal_address_space_id =
          state.pager_worker.last_terminal_address_space_id,
      .pager_worker_last_terminal_handoff_sequence =
          state.pager_worker.last_terminal_handoff_sequence,
      .pager_worker_last_terminal_cycle =
          state.pager_worker.last_terminal_cycle,
      .pager_worker_boot_critical_resolutions =
          state.pager_worker.boot_critical_resolutions,
      .pager_worker_last_boot_critical_address_space_id =
          state.pager_worker.last_boot_critical_address_space_id,
      .pager_worker_last_boot_critical_handoff_sequence =
          state.pager_worker.last_boot_critical_handoff_sequence,
      .pager_worker_last_boot_critical_resolution_sequence =
          state.pager_worker.last_boot_critical_resolution_sequence,
      .managed_service_count = service_summary.managed_service_count,
      .blocked_service_count = service_summary.blocked_service_count,
      .suspended_service_count = service_summary.suspended_service_count,
      .unhealthy_service_count = service_summary.unhealthy_service_count,
      .service_lifecycle_transitions = service_summary.service_lifecycle_transitions,
      .last_service_transition_id = latest_service_transition.service_id,
      .last_service_transition_kind = latest_service_transition.kind,
      .last_service_transition_sequence = latest_service_transition.sequence,
      // DPE epoch summary (RFC-DPE-0003 §7)
      .epoch_submissions       = state.counters.epoch_submissions,
      .epoch_commits           = state.counters.epoch_commits,
      .epoch_aborts            = state.counters.epoch_aborts,
      .epoch_task_executions   = state.counters.epoch_task_executions,
      .last_committed_epoch_id   = state.epoch.last_committed_epoch_id,
      .last_committed_epoch_hash = state.epoch.last_committed_epoch_hash,
  };
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
      .pager_worker_owned = address_space ? address_space->pager_worker_owned : false,
      .pending_pager_fault_count =
          address_space ? address_space->pending_pager_fault_count : 0,
      .pager_faults = address_space ? address_space->pager_faults : 0,
      .pager_handoffs = address_space ? address_space->pager_handoffs : 0,
      .pager_resolutions = address_space ? address_space->pager_resolutions : 0,
      .pager_faults_coalesced =
          address_space ? address_space->pager_faults_coalesced : 0,
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
      .pager_resolutions =
          supervisor_state ? count_managed_pager_resolutions(state, *supervisor_state)
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
      .pager_resolutions =
          supervisor_state ? count_managed_pager_resolutions(state, *supervisor_state)
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
      .object_ref = service_state ? service_state->object_ref : std::nullopt,
      .has_entry_descriptor =
          service_state && service_state->entry_descriptor.has_value(),
      .entry_descriptor =
          service_state ? service_state->entry_descriptor : std::nullopt,
      .address_space_id = address_space_id,
      .owned_page_count = address_space_id.has_value()
                              ? count_mapped_pages_for_address_space(state, *address_space_id)
                              : 0,
      .pager_needed = address_space ? address_space->pager_needed : false,
      .pager_handoff_pending =
          address_space ? address_space->pager_handoff_pending : false,
      .pager_worker_owned = address_space ? address_space->pager_worker_owned : false,
      .pending_pager_fault_count =
          address_space ? address_space->pending_pager_fault_count : 0,
      .pager_faults = address_space ? address_space->pager_faults : 0,
      .pager_handoffs = address_space ? address_space->pager_handoffs : 0,
      .pager_resolutions = address_space ? address_space->pager_resolutions : 0,
      .pager_faults_coalesced =
          address_space ? address_space->pager_faults_coalesced : 0,
      .last_pager_fault =
          address_space ? address_space->last_pager_fault : std::nullopt,
        .primary_tid =
          service_state
              ? axion_kernel_primary_tid_for_group(state, service_state->process_group_id)
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
        .object_ref = service_state->object_ref,
        .has_entry_descriptor = service_state->entry_descriptor.has_value(),
        .entry_descriptor = service_state->entry_descriptor,
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
        .pager_worker_owned = [&]() -> bool {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_worker_owned : false;
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
        .pager_resolutions = [&]() -> uint64_t {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_resolutions : 0;
        }(),
        .pager_faults_coalesced = [&]() -> uint64_t {
          const auto* address_space =
              find_group_address_space_state(state, service_state->process_group_id);
          return address_space ? address_space->pager_faults_coalesced : 0;
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

KernelSupervisorCapabilityInventoryView build_supervisor_capabilities_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  KernelSupervisorCapabilityInventoryView view{
      .supervisor_id = supervisor_state ? supervisor_state->id : supervisor_id,
      .process_group_count = supervisor_state ? supervisor_state->managed_groups.size() : 0,
      .capability_transitions =
          supervisor_state ? supervisor_state->capability_transitions : 0,
      .last_capability_transition_group_id =
          supervisor_state ? supervisor_state->last_capability_transition_group_id
                           : std::nullopt,
      .last_capability_transition_record_id =
          supervisor_state ? supervisor_state->last_capability_transition_record_id
                           : std::nullopt,
      .last_capability_transition_kind =
          supervisor_state ? supervisor_state->last_capability_transition_kind
                           : std::nullopt,
      .last_capability_transition_sequence =
          supervisor_state ? supervisor_state->last_capability_transition_sequence
                           : std::nullopt,
      .recent_capability_transitions =
          supervisor_state ? std::vector<KernelCapabilityTransitionRecord>(
                                 supervisor_state->recent_capability_transitions.begin(),
                                 supervisor_state->recent_capability_transitions.end())
                           : std::vector<KernelCapabilityTransitionRecord>{},
      .process_groups = {},
  };
  if (!supervisor_state) {
    return view;
  }
  for (auto process_group_id : supervisor_state->managed_groups) {
    const auto capabilities =
        axion_kernel_list_process_group_capabilities(state, process_group_id);
    view.process_groups.push_back(KernelSupervisorCapabilityEntryView{
        .process_group_id = process_group_id,
        .capability_count = capabilities.size(),
        .capabilities = capabilities,
    });
  }
  return view;
}

KernelSupervisorDelegationSummaryView build_supervisor_delegation_summary_view(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id) {
  const auto* supervisor_state = state.find_supervisor(supervisor_id);
  KernelSupervisorDelegationSummaryView view{
      .supervisor_id = supervisor_state ? supervisor_state->id : supervisor_id,
      .process_group_count = supervisor_state ? supervisor_state->managed_groups.size() : 0,
      .delegation_entry_count = 0,
      .delegated_capability_count = 0,
      .entries = {},
  };
  if (!supervisor_state) {
    return view;
  }

  for (auto process_group_id : supervisor_state->managed_groups) {
    const auto capabilities =
        axion_kernel_list_process_group_capabilities(state, process_group_id);
    for (const auto& capability : capabilities) {
      if (capability.kernel_seeded ||
          !capability.delegated_by_process_group_id.has_value() ||
          !capability.delegated_by_supervisor_id.has_value()) {
        continue;
      }
      auto existing = std::find_if(
          view.entries.begin(),
          view.entries.end(),
          [&](const auto& entry) {
            return entry.target_process_group_id == process_group_id &&
                   entry.delegated_by_process_group_id ==
                       *capability.delegated_by_process_group_id &&
                   entry.delegated_by_supervisor_id ==
                       *capability.delegated_by_supervisor_id;
          });
      if (existing == view.entries.end()) {
        view.entries.push_back(KernelSupervisorDelegationSummaryEntryView{
            .target_process_group_id = process_group_id,
            .delegated_by_process_group_id = *capability.delegated_by_process_group_id,
            .delegated_by_supervisor_id = *capability.delegated_by_supervisor_id,
            .delegated_capability_count = 1,
        });
      } else {
        ++existing->delegated_capability_count;
      }
      ++view.delegated_capability_count;
    }
  }
  view.delegation_entry_count = view.entries.size();
  return view;
}

KernelFaultSummaryView make_fault_summary_view(const KernelRuntimeState& state) {
  const auto latest_service_transition = latest_service_transition_view(state);
  const auto latest_pager_fault = latest_pager_fault_view(state);
  const auto pending_interrupt_sources = count_pending_interrupt_sources(state);
  const auto ready_pager_backlog_count =
      count_ready_pager_backlog_address_spaces(state);
  const auto parked_ready_pager_backlog_count =
      count_parked_ready_pager_backlog_address_spaces(state);
  const auto boot_critical_pager_needed_count =
      count_boot_critical_pager_needed_address_spaces(state);
  const auto boot_critical_terminal_count =
      count_boot_critical_terminal_address_spaces(state);
  return KernelFaultSummaryView{
      .recorded_faults = state.fault_count(),
      .pending_faults = state.pending_fault_count(),
      .pending_interrupts = state.pending_interrupt_count(),
      .pending_interrupt_high_watermark = state.pending_interrupt_high_watermark,
      .pending_interrupt_sources = pending_interrupt_sources,
      .delivered_faults = static_cast<std::size_t>(state.counters.faults_delivered),
      .interrupts_recorded = state.counters.interrupts_recorded,
      .interrupts_delivered = state.counters.interrupts_delivered,
      .interrupt_sources_recorded = state.counters.interrupt_sources_recorded,
      .interrupt_sources_delivered = state.counters.interrupt_sources_delivered,
      .routed_thread_faults =
          static_cast<std::size_t>(state.counters.faults_routed_to_threads),
      .quarantined_threads =
          static_cast<std::size_t>(state.counters.thread_quarantines),
      .audit_events = state.audit_count(),
      .pager_eligible_faults = state.counters.pager_eligible_faults,
      .policy_faults = state.counters.policy_faults,
      .boot_critical_address_spaces = count_boot_critical_address_spaces(state),
      .boot_critical_pager_needed_address_spaces =
          boot_critical_pager_needed_count,
      .boot_critical_terminal_address_spaces =
          boot_critical_terminal_count,
      .boot_progress_pending = boot_critical_pager_needed_count > 0,
      .boot_progress_blocked = boot_critical_terminal_count > 0,
      .pager_needed_address_spaces = count_pager_needed_address_spaces(state),
      .pager_terminal_address_spaces = count_pager_terminal_address_spaces(state),
      .pending_pager_handoffs = count_pending_pager_handoff_address_spaces(state),
      .pending_pager_handoff_high_watermark =
          state.pending_pager_handoff_high_watermark,
      .pager_handoffs_dispatched = state.counters.pager_handoffs_dispatched,
      .pager_resolutions = state.counters.pager_resolutions,
      .pager_faults_coalesced = state.counters.pager_faults_coalesced,
      .service_lifecycle_transitions =
          [&state]() {
            uint64_t total = 0;
            for (const auto& [_, supervisor_state] : state.supervisors) {
              total += supervisor_state.service_lifecycle_transitions;
            }
            return total;
          }(),
      .last_delivered_fault = state.last_delivered_fault,
      .next_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.front()}
              : std::nullopt,
      .last_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.back()}
              : std::nullopt,
      .last_recorded_interrupt_audit_sequence =
          state.last_recorded_interrupt_audit_sequence,
      .last_delivered_interrupt_audit_sequence =
          state.last_delivered_interrupt_audit_sequence,
      .last_interrupt_audit_kind = state.last_interrupt_audit_kind,
      .last_interrupt_audit_source = state.last_interrupt_audit_source,
      .last_interrupt_audit_interrupt_sequence =
          state.last_interrupt_audit_interrupt_sequence,
      .last_interrupt_audit_payload = state.last_interrupt_audit_payload,
      .last_interrupt_audit_timestamp_ns =
          state.last_interrupt_audit_timestamp_ns,
      .last_interrupt_audit_sequence = state.last_interrupt_audit_sequence,
      .last_recorded_interrupt = state.last_recorded_interrupt,
      .last_delivered_interrupt = state.last_delivered_interrupt,
      .last_pager_address_space_id = latest_pager_fault.address_space_id,
      .last_pager_fault = latest_pager_fault.fault,
      .last_pager_handoff = state.last_pager_handoff,
      .last_pager_resolution = state.last_pager_resolution,
      .pager_worker_inbox_count = state.pager_worker.inbox.size(),
      .pager_worker_inbox_high_watermark = state.pager_worker.inbox_high_watermark,
      .pager_worker_ready_backlog_count = ready_pager_backlog_count,
      .pager_worker_ready_backlog_high_watermark =
          state.pager_worker.ready_backlog_high_watermark,
      .pager_worker_parked_ready_count = parked_ready_pager_backlog_count,
      .pager_worker_parked_ready_high_watermark =
          state.pager_worker.parked_ready_high_watermark,
      .pager_worker_busy = state.pager_worker.active_work.has_value(),
      .pager_worker_active_address_space_id =
          state.pager_worker.active_work.has_value()
              ? std::optional<AddressSpaceId>{
                    state.pager_worker.active_work->handoff.address_space_id}
              : std::nullopt,
      .pager_worker_active_handoff_sequence =
          state.pager_worker.active_work.has_value()
              ? std::optional<uint64_t>{
                    state.pager_worker.active_work->handoff.sequence}
              : std::nullopt,
      .pager_worker_next_queued_address_space_id =
          !state.pager_worker.inbox.empty()
              ? std::optional<AddressSpaceId>{
                    state.pager_worker.inbox.front().handoff.address_space_id}
              : std::nullopt,
      .pager_worker_next_queued_handoff_sequence =
          !state.pager_worker.inbox.empty()
              ? std::optional<uint64_t>{
                    state.pager_worker.inbox.front().handoff.sequence}
              : std::nullopt,
      .pager_worker_handoffs_received = state.pager_worker.handoffs_received,
      .pager_worker_last_received_address_space_id =
          state.pager_worker.last_received_address_space_id,
      .pager_worker_last_received_handoff_sequence =
          state.pager_worker.last_received_handoff_sequence,
      .pager_worker_ready_bypass_activations =
          state.pager_worker.ready_bypass_activations,
      .pager_worker_last_ready_bypass_blocked_address_space_id =
          state.pager_worker.last_ready_bypass_blocked_address_space_id,
      .pager_worker_last_ready_bypass_promoted_address_space_id =
          state.pager_worker.last_ready_bypass_promoted_address_space_id,
      .pager_worker_last_ready_bypass_cycle =
          state.pager_worker.last_ready_bypass_cycle,
      .pager_worker_ready_bypass_deferrals =
          state.pager_worker.ready_bypass_deferrals,
      .pager_worker_last_ready_bypass_deferred_blocked_address_space_id =
          state.pager_worker.last_ready_bypass_deferred_blocked_address_space_id,
      .pager_worker_last_ready_bypass_deferred_ready_address_space_id =
          state.pager_worker.last_ready_bypass_deferred_ready_address_space_id,
      .pager_worker_last_ready_bypass_deferred_cycle =
          state.pager_worker.last_ready_bypass_deferred_cycle,
      .pager_worker_parked_cycles = state.pager_worker.parked_cycles,
      .pager_worker_parked_resumptions = state.pager_worker.parked_resumptions,
      .pager_worker_last_parked_blocked_address_space_id =
          state.pager_worker.last_parked_blocked_address_space_id,
      .pager_worker_last_parked_ready_address_space_id =
          state.pager_worker.last_parked_ready_address_space_id,
      .pager_worker_last_parked_cycle = state.pager_worker.last_parked_cycle,
      .pager_worker_last_parked_ready_count =
          state.pager_worker.last_parked_ready_count,
      .pager_worker_last_parked_resumed_address_space_id =
          state.pager_worker.last_parked_resumed_address_space_id,
      .pager_worker_last_parked_resumed_handoff_sequence =
          state.pager_worker.last_parked_resumed_handoff_sequence,
      .pager_worker_last_parked_resumption_cycle =
          state.pager_worker.last_parked_resumption_cycle,
      .pager_worker_last_parked_resumed_ready_count =
          state.pager_worker.last_parked_resumed_ready_count,
      .pager_worker_last_parked_resumed_ready_address_space_id =
          state.pager_worker.last_parked_resumed_ready_address_space_id,
      .pager_worker_last_parked_resumed_ready_handoff_sequence =
          state.pager_worker.last_parked_resumed_ready_handoff_sequence,
      .pager_worker_parked_resolved_heads =
          state.pager_worker.parked_resolved_heads,
      .pager_worker_last_parked_resolved_address_space_id =
          state.pager_worker.last_parked_resolved_address_space_id,
      .pager_worker_last_parked_resolved_handoff_sequence =
          state.pager_worker.last_parked_resolved_handoff_sequence,
      .pager_worker_last_parked_resolved_resolution_sequence =
          state.pager_worker.last_parked_resolved_resolution_sequence,
      .pager_worker_last_parked_resolved_remaining_inbox_count =
          state.pager_worker.last_parked_resolved_remaining_inbox_count,
      .pager_worker_last_parked_resolved_remaining_address_space_id =
          state.pager_worker.last_parked_resolved_remaining_address_space_id,
      .pager_worker_last_parked_resolved_remaining_handoff_sequence =
          state.pager_worker.last_parked_resolved_remaining_handoff_sequence,
      .pager_worker_parked_resolution_follow_on_activations =
          state.pager_worker.parked_resolution_follow_on_activations,
      .pager_worker_last_parked_resolution_follow_on_address_space_id =
          state.pager_worker.last_parked_resolution_follow_on_address_space_id,
      .pager_worker_last_parked_resolution_follow_on_handoff_sequence =
          state.pager_worker.last_parked_resolution_follow_on_handoff_sequence,
      .pager_worker_last_parked_resolution_follow_on_activation_cycle =
          state.pager_worker.last_parked_resolution_follow_on_activation_cycle,
      .pager_worker_parked_resolution_follow_on_resolutions =
          state.pager_worker.parked_resolution_follow_on_resolutions,
      .pager_worker_last_parked_resolution_follow_on_resolved_address_space_id =
          state.pager_worker.last_parked_resolution_follow_on_resolved_address_space_id,
      .pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence =
          state.pager_worker.last_parked_resolution_follow_on_resolved_handoff_sequence,
      .pager_worker_last_parked_resolution_follow_on_resolution_sequence =
          state.pager_worker.last_parked_resolution_follow_on_resolution_sequence,
      .pager_worker_activations = state.pager_worker.activations,
      .pager_worker_last_activated_address_space_id =
          state.pager_worker.last_activated_address_space_id,
      .pager_worker_last_activation_cycle =
          state.pager_worker.last_activation_cycle,
      .pager_worker_stall_cycles = state.pager_worker.stall_cycles,
      .pager_worker_backlog_blocked_cycles =
          state.pager_worker.backlog_blocked_cycles,
      .pager_worker_ready_backlog_cycles =
          state.pager_worker.ready_backlog_cycles,
      .pager_worker_resolutions_completed =
          state.pager_worker.resolutions_completed,
      .pager_worker_last_completed_address_space_id =
          state.pager_worker.last_completed_address_space_id,
      .pager_worker_last_completed_resolution_sequence =
          state.pager_worker.last_completed_resolution_sequence,
      .pager_worker_last_stalled_address_space_id =
          state.pager_worker.last_stalled_address_space_id,
      .pager_worker_last_stall_cycle = state.pager_worker.last_stall_cycle,
      .pager_worker_last_ready_backlog_address_space_id =
          state.pager_worker.last_ready_backlog_address_space_id,
      .pager_worker_last_ready_backlog_cycle =
          state.pager_worker.last_ready_backlog_cycle,
      .pager_worker_last_ready_backlog_count =
          state.pager_worker.last_ready_backlog_count,
      .pager_worker_terminal_failures = state.pager_worker.terminal_failures,
      .pager_worker_last_terminal_address_space_id =
          state.pager_worker.last_terminal_address_space_id,
      .pager_worker_last_terminal_handoff_sequence =
          state.pager_worker.last_terminal_handoff_sequence,
      .pager_worker_last_terminal_cycle =
          state.pager_worker.last_terminal_cycle,
      .pager_worker_boot_critical_resolutions =
          state.pager_worker.boot_critical_resolutions,
      .pager_worker_last_boot_critical_address_space_id =
          state.pager_worker.last_boot_critical_address_space_id,
      .pager_worker_last_boot_critical_handoff_sequence =
          state.pager_worker.last_boot_critical_handoff_sequence,
      .pager_worker_last_boot_critical_resolution_sequence =
          state.pager_worker.last_boot_critical_resolution_sequence,
      .last_audit_event = state.last_audit_event,
      .last_service_transition_id = latest_service_transition.service_id,
      .last_service_transition_kind = latest_service_transition.kind,
      .last_service_transition_sequence = latest_service_transition.sequence,
  };
}

KernelAuditSummaryView make_audit_summary_view(const KernelRuntimeState& state) {
  const auto latest_service_transition = latest_service_transition_view(state);
  const auto pending_interrupt_sources = count_pending_interrupt_sources(state);
  KernelAuditSummaryView view{
      .audit_events = state.audit_count(),
      .fault_deliveries = state.counters.faults_delivered,
      .pending_interrupt_count = state.pending_interrupt_count(),
      .pending_interrupt_high_watermark = state.pending_interrupt_high_watermark,
      .pending_interrupt_sources = pending_interrupt_sources,
      .interrupts_recorded = state.counters.interrupts_recorded,
      .interrupt_deliveries = state.counters.interrupts_delivered,
      .interrupt_sources_recorded = state.counters.interrupt_sources_recorded,
      .interrupt_sources_delivered = state.counters.interrupt_sources_delivered,
      .next_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.front()}
              : std::nullopt,
      .last_pending_interrupt =
          !state.pending_interrupts.empty()
              ? std::optional<KernelInterruptRecord>{state.pending_interrupts.back()}
              : std::nullopt,
      .last_recorded_interrupt_audit_sequence =
          state.last_recorded_interrupt_audit_sequence,
      .last_delivered_interrupt_audit_sequence =
          state.last_delivered_interrupt_audit_sequence,
      .last_interrupt_audit_kind = state.last_interrupt_audit_kind,
      .last_interrupt_audit_source = state.last_interrupt_audit_source,
      .last_interrupt_audit_interrupt_sequence =
          state.last_interrupt_audit_interrupt_sequence,
      .last_interrupt_audit_payload = state.last_interrupt_audit_payload,
      .last_interrupt_audit_timestamp_ns =
          state.last_interrupt_audit_timestamp_ns,
      .last_interrupt_audit_sequence = state.last_interrupt_audit_sequence,
      .thread_quarantines = state.counters.thread_quarantines,
      .process_group_fault_entries = state.counters.process_group_fault_entries,
      .supervisor_notifications = state.counters.supervisor_fault_notifications,
      .thread_acknowledgements = state.counters.thread_fault_acknowledgements,
      .process_group_acknowledgements = state.counters.process_group_acknowledgements,
      .supervisor_acknowledgements = state.counters.supervisor_acknowledgements,
      .thread_recoveries = state.counters.thread_fault_recoveries,
      .service_lifecycle_transitions =
          runtime_service_summary(state).service_lifecycle_transitions,
      .last_recorded_interrupt = state.last_recorded_interrupt,
      .last_delivered_interrupt = state.last_delivered_interrupt,
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
      .service_lifecycle_transitions =
          runtime_service_summary(state).service_lifecycle_transitions,
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

}  // namespace t81::ternaryos::kernel
