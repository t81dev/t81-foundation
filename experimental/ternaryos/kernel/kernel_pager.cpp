#include "kernel_main.hpp"

#include <algorithm>

namespace t81::ternaryos::kernel {

namespace {

mmu::PagePermissions boot_critical_permissions_for_fault(
    mmu::MmuAccessMode access_mode) {
  switch (access_mode) {
    case mmu::MmuAccessMode::Read:
      return {.readable = true, .writable = false, .executable = false};
    case mmu::MmuAccessMode::Write:
      return {.readable = true, .writable = true, .executable = false};
    case mmu::MmuAccessMode::Execute:
      return {.readable = true, .writable = false, .executable = true};
  }
  return {};
}

bool try_boot_critical_pager_map(KernelRuntimeState& state,
                                 AddressSpaceId address_space_id) {
  auto* address_space = state.find_address_space_mut(address_space_id);
  if (!address_space || !address_space->boot_critical ||
      !address_space->last_pager_fault.has_value()) {
    return false;
  }
  const auto translation = mmu::mmu_translate_checked(
      state.page_table,
      address_space->last_pager_fault->tva,
      address_space->last_pager_fault->access_mode);
  if (translation.fault != mmu::MmuFault::Unmapped) {
    return false;
  }
  return mmu::mmu_map(state.page_table,
                      state.allocator,
                      address_space->last_pager_fault->tva,
                      address_space_id,
                      boot_critical_permissions_for_fault(
                          address_space->last_pager_fault->access_mode));
}

bool is_pager_work_item_ready(const KernelRuntimeState& state,
                              const KernelPagerWorkItem& work_item) {
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
}

std::optional<std::size_t> find_first_ready_pager_work_index(
    const KernelRuntimeState& state,
    const std::deque<KernelPagerWorkItem>& inbox) {
  for (std::size_t index = 0; index < inbox.size(); ++index) {
    if (is_pager_work_item_ready(state, inbox[index])) {
      return index;
    }
  }
  return std::nullopt;
}

bool dispatch_pending_pager_handoff(KernelRuntimeState& state) {
  const auto address_space_id = state.pending_pager_handoffs.front();
  state.pending_pager_handoffs.pop_front();
  auto* address_space = state.find_address_space_mut(address_space_id);
  if (!address_space || !address_space->last_pager_fault.has_value()) {
    return false;
  }
  address_space->pager_handoff_pending = false;
  ++address_space->pager_handoffs;
  address_space->last_pager_handoff_sequence =
      state.next_pager_handoff_sequence++;
  state.last_pager_handoff = KernelPagerHandoffRecord{
      .address_space_id = address_space_id,
      .process_group_id = address_space->process_group_id,
      .fault = *address_space->last_pager_fault,
      .sequence = *address_space->last_pager_handoff_sequence,
  };
  state.pager_worker.inbox.push_back(
      KernelPagerWorkItem{.handoff = *state.last_pager_handoff});
  state.pager_worker.inbox_high_watermark =
      std::max(state.pager_worker.inbox_high_watermark,
               state.pager_worker.inbox.size());
  address_space->pager_worker_owned = true;
  ++state.pager_worker.handoffs_received;
  state.pager_worker.last_received_address_space_id = address_space_id;
  state.pager_worker.last_received_handoff_sequence =
      address_space->last_pager_handoff_sequence;
  ++state.counters.pager_handoffs_dispatched;
  return true;
}

void activate_pager_work(KernelRuntimeState& state) {
  if (state.pager_worker.active_work.has_value() || state.pager_worker.inbox.empty()) {
    return;
  }

  std::size_t selected_index = 0;
  bool activate_selected_work = true;
  if (!is_pager_work_item_ready(state, state.pager_worker.inbox.front())) {
    (void)try_boot_critical_pager_map(
        state, state.pager_worker.inbox.front().handoff.address_space_id);
  }
  if (!is_pager_work_item_ready(state, state.pager_worker.inbox.front())) {
    if (const auto ready_index =
            find_first_ready_pager_work_index(state, state.pager_worker.inbox);
        ready_index.has_value() && *ready_index > 0) {
      if (state.pager_worker.inbox.front().ready_bypass_count == 0) {
        ++state.pager_worker.ready_bypass_activations;
        ++state.counters.pager_worker_ready_bypass_activations;
        ++state.pager_worker.inbox.front().ready_bypass_count;
        state.pager_worker.last_ready_bypass_blocked_address_space_id =
            state.pager_worker.inbox.front().handoff.address_space_id;
        state.pager_worker.last_ready_bypass_promoted_address_space_id =
            state.pager_worker.inbox[*ready_index].handoff.address_space_id;
        state.pager_worker.last_ready_bypass_cycle =
            state.pager_worker.ready_bypass_activations;
        selected_index = *ready_index;
      } else {
        const auto blocked_address_space_id =
            state.pager_worker.inbox.front().handoff.address_space_id;
        const auto ready_address_space_id =
            state.pager_worker.inbox[*ready_index].handoff.address_space_id;
        if (!state.pager_worker.parked_blocked_address_space_id.has_value() ||
            *state.pager_worker.parked_blocked_address_space_id !=
                blocked_address_space_id) {
          ++state.pager_worker.ready_bypass_deferrals;
          ++state.counters.pager_worker_ready_bypass_deferrals;
          state.pager_worker.parked_blocked_address_space_id =
              blocked_address_space_id;
          state.pager_worker
              .last_ready_bypass_deferred_blocked_address_space_id =
              blocked_address_space_id;
          state.pager_worker.last_ready_bypass_deferred_ready_address_space_id =
              ready_address_space_id;
          state.pager_worker.last_ready_bypass_deferred_cycle =
              state.pager_worker.ready_bypass_deferrals;
        }
        ++state.pager_worker.parked_cycles;
        ++state.counters.pager_worker_parked_cycles;
        ++state.pager_worker.inbox.front().parked_cycle_count;
        state.pager_worker.last_parked_blocked_address_space_id =
            blocked_address_space_id;
        state.pager_worker.last_parked_ready_address_space_id =
            ready_address_space_id;
        state.pager_worker.last_parked_cycle = state.pager_worker.parked_cycles;
        std::size_t ready_count = 0;
        for (const auto& work_item : state.pager_worker.inbox) {
          if (is_pager_work_item_ready(state, work_item)) {
            ++ready_count;
          }
        }
        state.pager_worker.parked_ready_high_watermark =
            std::max(state.pager_worker.parked_ready_high_watermark, ready_count);
        state.pager_worker.last_parked_ready_count = ready_count;
        if (state.pager_worker.inbox.front().parked_cycle_count >=
            KernelRuntimeState::kPagerWorkerParkedCycleLimit) {
          auto terminal_work_item = state.pager_worker.inbox.front();
          state.pager_worker.parked_blocked_address_space_id.reset();
          state.pager_worker.inbox.pop_front();
          ++state.pager_worker.terminal_failures;
          ++state.counters.pager_worker_terminal_failures;
          state.pager_worker.last_terminal_address_space_id =
              terminal_work_item.handoff.address_space_id;
          state.pager_worker.last_terminal_handoff_sequence =
              terminal_work_item.handoff.sequence;
          state.pager_worker.last_terminal_cycle =
              state.pager_worker.terminal_failures;
          if (auto* terminal_address_space = state.find_address_space_mut(
                  terminal_work_item.handoff.address_space_id)) {
            terminal_address_space->pager_handoff_pending = false;
            terminal_address_space->pager_worker_owned = false;
            terminal_address_space->pager_terminal = true;
            ++terminal_address_space->pager_terminal_failures;
            terminal_address_space->last_pager_terminal_sequence =
                state.pager_worker.last_terminal_cycle;
          }
        }
        activate_selected_work = false;
      }
    }
  }

  if (!activate_selected_work) {
    return;
  }

  const bool is_parked_resolution_follow_on =
      selected_index == 0 &&
      state.pager_worker.last_parked_resolved_remaining_address_space_id.has_value() &&
      state.pager_worker.last_parked_resolved_remaining_handoff_sequence.has_value() &&
      state.pager_worker.inbox[selected_index].handoff.address_space_id ==
          *state.pager_worker.last_parked_resolved_remaining_address_space_id &&
      state.pager_worker.inbox[selected_index].handoff.sequence ==
          *state.pager_worker.last_parked_resolved_remaining_handoff_sequence;
  if (selected_index == 0 &&
      state.pager_worker.parked_blocked_address_space_id.has_value() &&
      *state.pager_worker.parked_blocked_address_space_id ==
          state.pager_worker.inbox.front().handoff.address_space_id &&
      is_pager_work_item_ready(state, state.pager_worker.inbox.front())) {
    std::size_t resumed_ready_count = 0;
    std::optional<AddressSpaceId> resumed_ready_address_space_id{};
    std::optional<uint64_t> resumed_ready_handoff_sequence{};
    for (std::size_t i = 1; i < state.pager_worker.inbox.size(); ++i) {
      if (is_pager_work_item_ready(state, state.pager_worker.inbox[i])) {
        ++resumed_ready_count;
        resumed_ready_address_space_id =
            state.pager_worker.inbox[i].handoff.address_space_id;
        resumed_ready_handoff_sequence =
            state.pager_worker.inbox[i].handoff.sequence;
      }
    }
    ++state.pager_worker.parked_resumptions;
    ++state.counters.pager_worker_parked_resumptions;
    state.pager_worker.last_parked_resumed_address_space_id =
        state.pager_worker.inbox.front().handoff.address_space_id;
    state.pager_worker.last_parked_resumed_handoff_sequence =
        state.pager_worker.inbox.front().handoff.sequence;
    state.pager_worker.last_parked_resumption_cycle =
        state.pager_worker.parked_resumptions;
    state.pager_worker.last_parked_resumed_ready_count = resumed_ready_count;
    state.pager_worker.last_parked_resumed_ready_address_space_id =
        resumed_ready_address_space_id;
    state.pager_worker.last_parked_resumed_ready_handoff_sequence =
        resumed_ready_handoff_sequence;
  }
  state.pager_worker.parked_blocked_address_space_id.reset();
  state.pager_worker.active_work = state.pager_worker.inbox[selected_index];
  state.pager_worker.active_work->resumed_from_parked =
      selected_index == 0 &&
      state.pager_worker.last_parked_resumed_address_space_id.has_value() &&
      state.pager_worker.last_parked_resumed_handoff_sequence.has_value() &&
      state.pager_worker.active_work->handoff.address_space_id ==
          *state.pager_worker.last_parked_resumed_address_space_id &&
      state.pager_worker.active_work->handoff.sequence ==
          *state.pager_worker.last_parked_resumed_handoff_sequence;
  state.pager_worker.active_work->follow_on_from_parked_resolution =
      is_parked_resolution_follow_on;
  state.pager_worker.inbox.erase(state.pager_worker.inbox.begin() +
                                 static_cast<std::ptrdiff_t>(selected_index));
  ++state.pager_worker.activations;
  ++state.counters.pager_worker_activations;
  state.pager_worker.last_activated_address_space_id =
      state.pager_worker.active_work->handoff.address_space_id;
  state.pager_worker.last_activation_cycle = state.pager_worker.activations;
  if (is_parked_resolution_follow_on) {
    ++state.pager_worker.parked_resolution_follow_on_activations;
    ++state.counters.pager_worker_parked_resolution_follow_on_activations;
    state.pager_worker.last_parked_resolution_follow_on_address_space_id =
        state.pager_worker.active_work->handoff.address_space_id;
    state.pager_worker.last_parked_resolution_follow_on_handoff_sequence =
        state.pager_worker.active_work->handoff.sequence;
    state.pager_worker.last_parked_resolution_follow_on_activation_cycle =
        state.pager_worker.activations;
  }
}

void track_pager_stall(KernelRuntimeState& state) {
  if (!state.pager_worker.active_work.has_value()) {
    return;
  }
  const auto active_address_space_id =
      state.pager_worker.active_work->handoff.address_space_id;
  auto* active_address_space = state.find_address_space_mut(active_address_space_id);
  if (!active_address_space || !active_address_space->pager_needed ||
      active_address_space->pager_handoff_pending ||
      !active_address_space->last_pager_fault.has_value()) {
    return;
  }
  (void)try_boot_critical_pager_map(state, active_address_space_id);
  const auto active_translation = mmu::mmu_translate_checked(
      state.page_table,
      active_address_space->last_pager_fault->tva,
      active_address_space->last_pager_fault->access_mode);
  if (active_translation.fault == mmu::MmuFault::None) {
    return;
  }
  ++state.pager_worker.stall_cycles;
  ++state.counters.pager_worker_stall_cycles;
  state.pager_worker.last_stalled_address_space_id = active_address_space_id;
  state.pager_worker.last_stall_cycle = state.pager_worker.stall_cycles;
  if (state.pager_worker.inbox.empty()) {
    return;
  }
  ++state.pager_worker.backlog_blocked_cycles;
  ++state.counters.pager_worker_backlog_blocked_cycles;
  std::size_t ready_backlog_count = 0;
  for (const auto& work_item : state.pager_worker.inbox) {
    auto* queued_address_space =
        state.find_address_space_mut(work_item.handoff.address_space_id);
    if (!queued_address_space || !queued_address_space->last_pager_fault.has_value()) {
      continue;
    }
    const auto queued_translation = mmu::mmu_translate_checked(
        state.page_table,
        queued_address_space->last_pager_fault->tva,
        queued_address_space->last_pager_fault->access_mode);
    if (queued_translation.fault == mmu::MmuFault::None) {
      ++ready_backlog_count;
      ++state.pager_worker.ready_backlog_cycles;
      ++state.counters.pager_worker_ready_backlog_cycles;
      state.pager_worker.last_ready_backlog_address_space_id =
          work_item.handoff.address_space_id;
      state.pager_worker.last_ready_backlog_cycle = state.pager_worker.stall_cycles;
    }
  }
  state.pager_worker.ready_backlog_high_watermark =
      std::max(state.pager_worker.ready_backlog_high_watermark,
               ready_backlog_count);
  if (ready_backlog_count > 0) {
    state.pager_worker.last_ready_backlog_count = ready_backlog_count;
  }
}

void resolve_completed_pager_work(KernelRuntimeState& state) {
  for (AddressSpaceId address_space_id = 0;
       address_space_id < state.next_address_space_id;
       ++address_space_id) {
    auto* address_space = state.find_address_space_mut(address_space_id);
    if (!address_space || !address_space->pager_needed ||
        address_space->pager_handoff_pending ||
        !address_space->last_pager_fault.has_value()) {
      continue;
    }
    const auto translation = mmu::mmu_translate_checked(
        state.page_table,
        address_space->last_pager_fault->tva,
        address_space->last_pager_fault->access_mode);
    if (translation.fault != mmu::MmuFault::None) {
      continue;
    }
    if (!state.pager_worker.active_work.has_value() ||
        state.pager_worker.active_work->handoff.address_space_id != address_space_id) {
      continue;
    }
    address_space->pager_needed = false;
    address_space->pager_worker_owned = false;
    address_space->pager_terminal = false;
    address_space->pending_pager_fault_count = 0;
    ++address_space->pager_resolutions;
    address_space->last_pager_resolution_sequence =
        state.next_pager_resolution_sequence++;
    state.last_pager_resolution = KernelPagerResolutionRecord{
        .address_space_id = address_space_id,
        .process_group_id = address_space->process_group_id,
        .fault = *address_space->last_pager_fault,
        .sequence = *address_space->last_pager_resolution_sequence,
    };
    if (state.pager_worker.active_work->resumed_from_parked) {
      const std::size_t remaining_inbox_count = state.pager_worker.inbox.size();
      ++state.pager_worker.parked_resolved_heads;
      ++state.counters.pager_worker_parked_resolved_heads;
      state.pager_worker.last_parked_resolved_address_space_id = address_space_id;
      state.pager_worker.last_parked_resolved_handoff_sequence =
          state.pager_worker.active_work->handoff.sequence;
      state.pager_worker.last_parked_resolved_resolution_sequence =
          address_space->last_pager_resolution_sequence;
      state.pager_worker.last_parked_resolved_remaining_inbox_count =
          remaining_inbox_count;
      state.pager_worker.last_parked_resolved_remaining_address_space_id =
          !state.pager_worker.inbox.empty()
              ? std::optional<AddressSpaceId>{
                    state.pager_worker.inbox.front().handoff.address_space_id}
              : std::nullopt;
      state.pager_worker.last_parked_resolved_remaining_handoff_sequence =
          !state.pager_worker.inbox.empty()
              ? std::optional<uint64_t>{
                    state.pager_worker.inbox.front().handoff.sequence}
              : std::nullopt;
    }
    if (state.pager_worker.active_work->follow_on_from_parked_resolution) {
      ++state.pager_worker.parked_resolution_follow_on_resolutions;
      ++state.counters.pager_worker_parked_resolution_follow_on_resolutions;
      state.pager_worker.last_parked_resolution_follow_on_resolved_address_space_id =
          address_space_id;
      state.pager_worker.last_parked_resolution_follow_on_resolved_handoff_sequence =
          state.pager_worker.active_work->handoff.sequence;
      state.pager_worker.last_parked_resolution_follow_on_resolution_sequence =
          address_space->last_pager_resolution_sequence;
    }
    if (address_space->boot_critical) {
      ++address_space->pager_boot_critical_resolutions;
      ++state.pager_worker.boot_critical_resolutions;
      ++state.counters.pager_worker_boot_critical_resolutions;
      state.pager_worker.last_boot_critical_address_space_id = address_space_id;
      state.pager_worker.last_boot_critical_handoff_sequence =
          state.pager_worker.active_work->handoff.sequence;
      state.pager_worker.last_boot_critical_resolution_sequence =
          address_space->last_pager_resolution_sequence;
    }
    state.pager_worker.active_work.reset();
    ++state.pager_worker.resolutions_completed;
    state.pager_worker.last_completed_address_space_id = address_space_id;
    state.pager_worker.last_completed_resolution_sequence =
        address_space->last_pager_resolution_sequence;
    ++state.counters.pager_resolutions;
    break;
  }
}

}  // namespace

bool axion_kernel_run_pager_policy(KernelRuntimeState& state) noexcept {
  if (!state.pending_pager_handoffs.empty()) {
    (void)dispatch_pending_pager_handoff(state);
    return false;
  }

  activate_pager_work(state);
  track_pager_stall(state);
  resolve_completed_pager_work(state);
  return false;
}

}  // namespace t81::ternaryos::kernel
