#pragma once

#include "kernel_base.hpp"
#include "kernel_runtime_state.hpp"
#include "kernel_runtime_support.hpp"
#include "kernel_service_contract.hpp"

#include "../hal/hal.hpp"
#include "../hal/virtualbox_guest_devices.hpp"
#include "../ipc/canon_message.hpp"
#include "../mmu/ternary_page_alloc.hpp"
#include "../dev/block_device.hpp"
#include "t81/canonfs/canon_driver.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace t81::ternaryos::kernel {

struct KernelAccessReport {
  std::optional<uint64_t> phys_addr{};
  std::optional<KernelFaultRecord> fault{};
};

void record_audit_event(KernelRuntimeState& state,
                        KernelAuditEventKind kind,
                        sched::Tid subject_tid,
                        ProcessGroupId process_group_id,
                        mmu::MmuFault fault = mmu::MmuFault::None);

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept;

void record_fault(KernelRuntimeState& state,
                  uint64_t tva,
                  mmu::MmuAccessMode mode,
                  mmu::MmuFault fault);

void record_pager_fault_state(KernelRuntimeState& state,
                              ProcessGroupId process_group_id,
                              const KernelFaultRecord& fault_record);

KernelAccessReport axion_kernel_check_access(
    KernelRuntimeState& state,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread(
    KernelRuntimeState& state,
    sched::TiscContext ctx) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread_in_group(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    ProcessGroupId process_group_id) noexcept;

std::optional<sched::Tid> axion_kernel_spawn_thread_under_supervisor(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    SupervisorId supervisor_id) noexcept;
bool axion_kernel_set_address_space_boot_critical(KernelRuntimeState& state,
                                                  AddressSpaceId address_space_id,
                                                  bool boot_critical) noexcept;
bool axion_kernel_validate_address_space_span(const KernelRuntimeState& state,
                                              AddressSpaceId address_space_id,
                                              uint64_t tva,
                                              std::size_t size,
                                              mmu::MmuAccessMode mode) noexcept;
bool axion_kernel_write_address_space_bytes(KernelRuntimeState& state,
                                            AddressSpaceId address_space_id,
                                            uint64_t tva,
                                            const std::byte* data,
                                            std::size_t size) noexcept;
bool axion_kernel_read_address_space_bytes(const KernelRuntimeState& state,
                                           AddressSpaceId address_space_id,
                                           uint64_t tva,
                                           std::byte* data,
                                           std::size_t size) noexcept;

bool axion_kernel_record_interrupt(
    KernelRuntimeState& state,
    const hal::HardwareInterrupt& interrupt) noexcept;

/// RFC-00B5 §3.5 (Slice 28): called by the HAL unhandled-interrupt callback.
/// Emits UnhandledInterruptDropped audit event; does NOT enqueue the interrupt.
void axion_kernel_record_unhandled_interrupt(
    KernelRuntimeState& state,
    const hal::HardwareInterrupt& interrupt) noexcept;

bool axion_kernel_tick(KernelRuntimeState& state) noexcept;

bool axion_kernel_deliver_pending_fault(KernelRuntimeState& state) noexcept;

bool axion_kernel_deliver_pending_interrupt(KernelRuntimeState& state) noexcept;

bool axion_kernel_run_pager_policy(KernelRuntimeState& state) noexcept;

bool axion_kernel_step(KernelRuntimeState& state) noexcept;

bool axion_kernel_ipc_send(KernelRuntimeState& state,
                           sched::Tid dst,
                           ipc::CanonMessage msg) noexcept;

std::optional<ipc::CanonMessage> axion_kernel_ipc_recv(
    KernelRuntimeState& state,
    sched::Tid tid) noexcept;
bool axion_kernel_terminate_thread(KernelRuntimeState& state,
                                   sched::Tid tid) noexcept;

bool axion_kernel_claim_device(KernelRuntimeState& state,
                               std::string_view device_name,
                               sched::Tid owner) noexcept;

bool axion_kernel_release_device(KernelRuntimeState& state,
                                 std::string_view device_name,
                                 sched::Tid owner) noexcept;
bool axion_kernel_bind_published_executable_store(
    KernelRuntimeState& state,
    std::unique_ptr<t81::ternaryos::dev::IBlockDevice> device) noexcept;
bool axion_kernel_bind_published_executable_store_from_virtualbox_guest(
    KernelRuntimeState& state,
    t81::ternaryos::hal::VBoxGuestBootstrap& guest) noexcept;
bool axion_kernel_bind_published_executable_canonfs(
    KernelRuntimeState& state,
    std::unique_ptr<t81::canonfs::Driver> driver) noexcept;
std::optional<sched::Tid> axion_kernel_primary_tid_for_group(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) noexcept;
std::optional<KernelServiceStatus> axion_kernel_validate_requesting_group(
    const KernelRuntimeState& state,
    std::optional<ProcessGroupId> requesting_process_group_id) noexcept;
KernelServiceRequestRejection axion_kernel_requesting_group_request_rejection(
    KernelServiceStatus status) noexcept;
KernelServiceActionRejection axion_kernel_requesting_group_action_rejection(
    KernelServiceStatus status) noexcept;
bool axion_kernel_process_groups_share_supervisor(
    const KernelRuntimeState& state,
    ProcessGroupId lhs_process_group_id,
    ProcessGroupId rhs_process_group_id) noexcept;
bool axion_kernel_supervisor_matches_process_group(
    const KernelRuntimeState& state,
    SupervisorId supervisor_id,
    ProcessGroupId process_group_id) noexcept;
bool axion_kernel_grant_process_group_capability(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<ProcessGroupId> delegated_by_process_group_id,
    std::optional<SupervisorId> delegated_by_supervisor_id,
    const KernelCapabilityRecord& capability) noexcept;
bool axion_kernel_revoke_process_group_capability(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<CapabilityRecordId> capability_record_id,
    KernelCapabilityKind capability_kind,
    std::optional<ProcessGroupId> process_group_scope = std::nullopt) noexcept;
bool axion_kernel_revoke_delegated_process_group_capabilities(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    std::optional<ProcessGroupId> delegated_by_process_group_id,
    std::optional<SupervisorId> delegated_by_supervisor_id) noexcept;
std::vector<KernelCapabilityRecord> axion_kernel_list_process_group_capabilities(
    const KernelRuntimeState& state,
    ProcessGroupId process_group_id) noexcept;

bool axion_kernel_ack_thread_fault(KernelRuntimeState& state,
                                   sched::Tid tid) noexcept;

/// Resume a thread that was quarantined due to a pager fault after the PagerService
/// thread has supplied a mapping via RequestPageMapping.  Validates the fault TVA is
/// now mapped, drains the fault from the thread's inbox, and un-quarantines the thread.
/// Returns false if the thread is not quarantined, has no pager fault, or the TVA is
/// not yet mapped.  (RFC-00B7 §3.4)
bool axion_kernel_resume_pager_faulted_thread(KernelRuntimeState& state,
                                              sched::Tid tid) noexcept;

bool axion_kernel_ack_process_group_fault(KernelRuntimeState& state,
                                          ProcessGroupId process_group_id) noexcept;

bool axion_kernel_ack_supervisor_group_fault(KernelRuntimeState& state,
                                             SupervisorId supervisor_id,
                                             ProcessGroupId process_group_id) noexcept;

int axion_kernel_main(const hal::BootContext& ctx) noexcept;

}  // namespace t81::ternaryos::kernel
