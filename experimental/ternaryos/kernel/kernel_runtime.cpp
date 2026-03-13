#include "kernel_main.hpp"

#include <algorithm>
#include <cstring>

namespace t81::ternaryos::kernel {

namespace {

bool is_pager_eligible_fault(mmu::MmuFault fault) {
  return fault == mmu::MmuFault::Unmapped;
}

void record_pager_fault_state_impl(KernelRuntimeState& state,
                                   ProcessGroupId process_group_id,
                                   const KernelFaultRecord& fault_record) {
  const auto address_space_id = state.find_process_group_address_space(process_group_id);
  if (!address_space_id.has_value() || !is_pager_eligible_fault(fault_record.fault)) {
    ++state.counters.policy_faults;
    return;
  }
  auto* address_space = state.find_address_space_mut(*address_space_id);
  if (!address_space) {
    ++state.counters.policy_faults;
    return;
  }
  address_space->pager_needed = true;
  if (address_space->pager_terminal) {
    address_space->pager_handoff_pending = false;
    address_space->pager_worker_owned = false;
  } else if (address_space->pager_handoff_pending || address_space->pager_worker_owned) {
    ++address_space->pager_faults_coalesced;
    ++state.counters.pager_faults_coalesced;
  } else {
    address_space->pager_handoff_pending = true;
    state.pending_pager_handoffs.push_back(*address_space_id);
    state.pending_pager_handoff_high_watermark =
        std::max(state.pending_pager_handoff_high_watermark,
                 state.pending_pager_handoffs.size());
    ++state.counters.pager_handoffs_queued;
  }
  ++address_space->pending_pager_fault_count;
  ++address_space->pager_faults;
  address_space->last_pager_fault = fault_record;
  address_space->last_pager_fault_sequence = state.counters.faults_delivered;
  ++state.counters.pager_eligible_faults;
}

std::vector<std::byte>* mutable_physical_page_storage(
    KernelRuntimeState& state,
    uint64_t phys_base) {
  auto [it, inserted] = state.physical_page_storage.try_emplace(phys_base);
  if (inserted) {
    it->second.resize(static_cast<std::size_t>(mmu::kPageSize));
  }
  return &it->second;
}

const std::vector<std::byte>* physical_page_storage(
    const KernelRuntimeState& state,
    uint64_t phys_base) {
  auto it = state.physical_page_storage.find(phys_base);
  return it == state.physical_page_storage.end() ? nullptr : &it->second;
}

}  // namespace

void record_pager_fault_state(KernelRuntimeState& state,
                              ProcessGroupId process_group_id,
                              const KernelFaultRecord& fault_record) {
  record_pager_fault_state_impl(state, process_group_id, fault_record);
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

bool axion_kernel_set_address_space_boot_critical(KernelRuntimeState& state,
                                                  AddressSpaceId address_space_id,
                                                  bool boot_critical) noexcept {
  auto* address_space = state.find_address_space_mut(address_space_id);
  if (!address_space) {
    return false;
  }
  address_space->boot_critical = boot_critical;
  return true;
}

bool axion_kernel_validate_address_space_span(const KernelRuntimeState& state,
                                              AddressSpaceId address_space_id,
                                              uint64_t tva,
                                              std::size_t size,
                                              mmu::MmuAccessMode mode) noexcept {
  if (!mmu::tva_valid(tva) ||
      (size > 0 && (!mmu::tva_valid(tva + size - 1) || tva + size - 1 < tva))) {
    return false;
  }
  for (std::size_t i = 0; i < size; ++i) {
    const auto translation =
        mmu::mmu_translate_checked(state.page_table, tva + i, mode);
    if (translation.fault != mmu::MmuFault::None || !translation.phys_addr.has_value()) {
      return false;
    }
    const auto it = state.page_table.entries().find(mmu::tva_vpn(tva + i));
    if (it == state.page_table.entries().end() || it->second.owner_pid != address_space_id) {
      return false;
    }
  }
  return true;
}

bool axion_kernel_write_address_space_bytes(KernelRuntimeState& state,
                                            AddressSpaceId address_space_id,
                                            uint64_t tva,
                                            const std::byte* data,
                                            std::size_t size) noexcept {
  if ((size > 0 && data == nullptr) || !mmu::tva_valid(tva) ||
      (size > 0 && (!mmu::tva_valid(tva + size - 1) || tva + size - 1 < tva))) {
    return false;
  }
  if (!axion_kernel_validate_address_space_span(
          state, address_space_id, tva, size, mmu::MmuAccessMode::Write)) {
    return false;
  }
  for (std::size_t i = 0; i < size; ++i) {
    const auto translation =
        mmu::mmu_translate_checked(state.page_table, tva + i, mmu::MmuAccessMode::Write);
    const auto phys_addr = *translation.phys_addr;
    const auto phys_base = phys_addr - (phys_addr % mmu::kPageSize);
    auto* page = mutable_physical_page_storage(state, phys_base);
    (*page)[static_cast<std::size_t>(phys_addr - phys_base)] = data[i];
  }
  return true;
}

bool axion_kernel_read_address_space_bytes(const KernelRuntimeState& state,
                                           AddressSpaceId address_space_id,
                                           uint64_t tva,
                                           std::byte* data,
                                           std::size_t size) noexcept {
  if ((size > 0 && data == nullptr) || !mmu::tva_valid(tva) ||
      (size > 0 && (!mmu::tva_valid(tva + size - 1) || tva + size - 1 < tva))) {
    return false;
  }
  if (!axion_kernel_validate_address_space_span(
          state, address_space_id, tva, size, mmu::MmuAccessMode::Read)) {
    return false;
  }
  for (std::size_t i = 0; i < size; ++i) {
    const auto translation =
        mmu::mmu_translate_checked(state.page_table, tva + i, mmu::MmuAccessMode::Read);
    const auto phys_addr = *translation.phys_addr;
    const auto phys_base = phys_addr - (phys_addr % mmu::kPageSize);
    const auto* page = physical_page_storage(state, phys_base);
    data[i] = page ? (*page)[static_cast<std::size_t>(phys_addr - phys_base)] : std::byte{0};
  }
  return true;
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
  state.last_delivered_interrupt.reset();
  if (!state.pending_faults.empty()) {
    handled_by_policy = axion_kernel_deliver_pending_fault(state);
  } else {
    state.last_delivered_fault.reset();
    if (!state.pending_interrupts.empty()) {
      handled_by_policy = axion_kernel_deliver_pending_interrupt(state);
    } else {
      handled_by_policy = axion_kernel_run_pager_policy(state);
    }
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

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
