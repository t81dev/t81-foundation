#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

namespace {

void mark_service_blocked(KernelRuntimeState& state,
                          ProcessGroupId process_group_id,
                          bool blocked) {
  const auto service_id = state.find_process_group_service(process_group_id);
  if (!service_id.has_value()) {
    return;
  }
  auto* service_state = state.find_service_mut(*service_id);
  if (!service_state || service_state->blocked == blocked) {
    return;
  }
  service_state->blocked = blocked;
  ++service_state->state_transitions;
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
  mark_service_blocked(state, group_state->id, false);
  record_audit_event(state,
                     KernelAuditEventKind::ThreadRecovered,
                     thread_state.tid,
                     group_state->id);
  return true;
}

}  // namespace

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

bool axion_kernel_deliver_pending_fault(KernelRuntimeState& state) noexcept {
  state.last_delivered_fault = state.pending_faults.front();
  state.pending_faults.pop_front();
  ++state.counters.faults_delivered;

  bool handled_by_policy = false;
  const sched::Tid subject_tid = state.last_delivered_fault->subject_tid;
  auto* thread_state = [&]() -> KernelRuntimeState::ThreadRuntimeState* {
    auto it = state.thread_runtime.find(subject_tid);
    return it == state.thread_runtime.end() ? nullptr : &it->second;
  }();
  if (!thread_state) {
    return false;
  }

  thread_state->fault_inbox.push_back(*state.last_delivered_fault);
  ++state.counters.faults_routed_to_threads;
  record_audit_event(state,
                     KernelAuditEventKind::FaultDelivered,
                     subject_tid,
                     thread_state->process_group_id,
                     state.last_delivered_fault->fault);

  auto* group_state = state.find_process_group_mut(thread_state->process_group_id);
  if (group_state) {
    record_pager_fault_state(state, group_state->id, *state.last_delivered_fault);
    const bool entering_fault_state = !group_state->faulted;
    group_state->faulted = true;
    group_state->blocked = true;
    group_state->acknowledgement_pending = true;
    ++group_state->pending_fault_count;
    ++group_state->counters.fault_entries;
    ++state.counters.process_group_fault_entries;
    mark_service_blocked(state, group_state->id, true);
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

  return handled_by_policy;
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

bool axion_kernel_resume_pager_faulted_thread(KernelRuntimeState& state,
                                              sched::Tid tid) noexcept {
  // RFC-00B7 §3.4 — complete the fault→handoff→service→resume lifecycle.
  // The PagerService thread calls this after RequestPageMapping to assert the
  // fault TVA is now mapped and the victim thread can safely resume.
  auto* thread_state = state.find_thread_runtime_mut(tid);
  if (!thread_state || !thread_state->quarantined ||
      thread_state->fault_inbox.empty()) {
    return false;
  }

  // Validate front fault is a pager-resolvable (Unmapped) fault.
  const KernelFaultRecord& fault = thread_state->fault_inbox.front();
  if (fault.fault != mmu::MmuFault::Unmapped) {
    return false;
  }

  // Verify the faulting TVA is now mapped — i.e. RequestPageMapping was called.
  const auto phys = mmu::mmu_translate(state.page_table, fault.tva);
  if (!phys.has_value()) {
    return false;
  }

  // Drain the pager fault from the thread inbox.
  thread_state->fault_inbox.pop_front();
  ++state.counters.thread_fault_acknowledgements;
  ++state.counters.pager_service_resumptions;
  record_audit_event(state,
                     KernelAuditEventKind::ThreadFaultAcknowledged,
                     tid,
                     thread_state->process_group_id);

  if (thread_state->fault_inbox.empty()) {
    auto* group_state = state.find_process_group_mut(thread_state->process_group_id);
    if (group_state && group_state->pending_fault_count > 0) {
      --group_state->pending_fault_count;
    }
    // Pager fault resolution is self-contained: the PagerService thread acts as the
    // complete acknowledger.  Clear acknowledgement_pending when no other faults remain,
    // so the thread can be recovered without requiring a separate supervisor ACK.
    if (group_state && group_state->pending_fault_count == 0) {
      group_state->acknowledgement_pending = false;
      group_state->faulted = false;
      group_state->blocked = false;
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

  for (auto tid : group_state->member_tids) {
    auto* thread_state = state.find_thread_runtime_mut(tid);
    if (thread_state) {
      (void)maybe_recover_thread(state, *thread_state);
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
  supervisor_state->last_acknowledged_group = process_group_id;
  ++state.counters.supervisor_acknowledgements;
  record_audit_event(state,
                     KernelAuditEventKind::SupervisorGroupAcknowledged,
                     KernelRuntimeState::kKernelTid,
                     process_group_id);
  if (!axion_kernel_ack_process_group_fault(state, process_group_id)) {
    --supervisor_state->acknowledgements;
    supervisor_state->last_acknowledged_group.reset();
    --state.counters.supervisor_acknowledgements;
    state.audit_log.pop_back();
    state.last_audit_event = state.audit_log.empty()
                                 ? std::nullopt
                                 : std::optional<KernelAuditRecord>(state.audit_log.back());
    --state.counters.audit_events_recorded;
    return false;
  }
  supervisor_state = state.find_supervisor_mut(supervisor_id);
  if (!supervisor_state) {
    return false;
  }
  supervisor_state->pending_groups.erase(pending_it);
  ++supervisor_state->recovered_groups;
  supervisor_state->last_recovered_group = process_group_id;
  return true;
}

}  // namespace t81::ternaryos::kernel
