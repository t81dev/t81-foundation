#pragma once

#include "kernel_abi.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace t81::ternaryos::kernel {

inline constexpr uint32_t kKernelAbiWireRequestMagic = 0x4B415152;   // KAQR
inline constexpr uint32_t kKernelAbiWireResponseMagic = 0x4B415250;  // KARP
inline constexpr uint16_t kKernelAbiWireVersion = 1;
inline constexpr std::size_t kKernelAbiWireTagBytes = 32;
inline constexpr std::size_t kKernelAbiWireServiceNameBytes = 32;
inline constexpr std::size_t kKernelAbiWireThreadLabelBytes = 32;
inline constexpr std::size_t kKernelAbiWireCapabilitySlots = 4;
inline constexpr std::size_t kKernelAbiWireCanonRefBytes = 32;

enum KernelCallWireFlags : uint64_t {
  kWireHasTargetTid = 1ull << 0,
  kWireHasProcessGroupId = 1ull << 1,
  kWireHasSupervisorId = 1ull << 2,
  kWireHasAddressSpaceId = 1ull << 3,
  kWireHasCapabilityTransitionSequence = 1ull << 4,
  kWireHasBootCritical = 1ull << 5,
  kWireHasIpcDst = 1ull << 6,
  kWireHasMessage = 1ull << 7,
  kWireHasCapability = 1ull << 8,
  kWireHasServiceId = 1ull << 9,
  kWireHasServiceName = 1ull << 10,
  kWireHasSpawnDescriptor = 1ull << 11,
  kWireMessageHasTag = 1ull << 12,
  kWireResponseHasMessage = 1ull << 13,
  kWireResponseHasFault = 1ull << 14,
  kWireResponseHasServiceId = 1ull << 15,
  kWireResponseHasServiceName = 1ull << 16,
  kWireResponseHasSupervisorId = 1ull << 17,
  kWireResponseHasAddressSpaceId = 1ull << 18,
  kWireResponseHasTargetProcessGroupId = 1ull << 19,
  kWireCapabilityHasProcessGroupScope = 1ull << 20,
  kWireCapabilityKernelSeeded = 1ull << 21,
  kWireCapabilityHasDelegatedByProcessGroupId = 1ull << 22,
  kWireCapabilityHasDelegatedBySupervisorId = 1ull << 23,
  kWireResponseHasCallerTid = 1ull << 24,
  kWireResponseHasCallerProcessGroupId = 1ull << 25,
  kWireResponseHasSupervisorLastPendingGroup = 1ull << 26,
  kWireResponseHasSupervisorLastAcknowledgedGroup = 1ull << 27,
  kWireResponseHasSupervisorLastRecoveredGroup = 1ull << 28,
  kWireResponseThreadExited = 1ull << 29,
  kWireResponseHasSpawnedTid = 1ull << 30,
  kWireResponseHasQueriedTid = 1ull << 31,
};

struct KernelCallWireHeader {
  uint32_t magic{0};
  uint16_t version{0};
  uint16_t bytes{0};
};

struct KernelCallWireCapabilityRecord {
  uint64_t flags{0};
  uint64_t record_id{0};
  uint32_t kind{0};
  uint32_t process_group_scope{0};
  uint32_t delegated_by_process_group_id{0};
  uint32_t delegated_by_supervisor_id{0};
};

struct KernelCallWireMessage {
  uint32_t sender{0};
  uint32_t reserved{0};
  uint64_t payload{0};
  std::array<char, kKernelAbiWireTagBytes> tag{};
  std::array<uint8_t, kKernelAbiWireCanonRefBytes> ref{};
};

struct KernelCallWireFault {
  uint64_t tva{0};
  uint32_t access_mode{0};
  uint32_t fault{0};
  uint32_t subject_tid{0};
  std::array<char, kKernelAbiWireTagBytes> platform_id{};
};

struct KernelCallWireRequestBlock {
  KernelCallWireHeader header{
      kKernelAbiWireRequestMagic,
      kKernelAbiWireVersion,
      static_cast<uint16_t>(sizeof(KernelCallWireRequestBlock)),
  };
  uint32_t kind{0};
  uint32_t reserved0{0};
  uint64_t flags{0};
  uint32_t target_tid{0};
  uint32_t process_group_id{0};
  uint32_t supervisor_id{0};
  uint32_t address_space_id{0};
  uint64_t capability_transition_sequence{0};
  uint8_t boot_critical{0};
  std::array<uint8_t, 7> reserved1{};
  uint32_t ipc_dst{0};
  uint32_t service_id{0};
  uint64_t spawn_pc{0};
  uint64_t spawn_sp{0};
  int64_t spawn_register0{0};
  uint8_t spawn_halted{0};
  uint8_t spawn_active{1};
  std::array<uint8_t, 6> reserved2{};
  KernelCallWireMessage message{};
  KernelCallWireCapabilityRecord capability{};
  std::array<char, kKernelAbiWireServiceNameBytes> service_name{};
  std::array<char, kKernelAbiWireThreadLabelBytes> spawn_label{};
};

struct KernelCallWireResponseBlock {
  KernelCallWireHeader header{
      kKernelAbiWireResponseMagic,
      kKernelAbiWireVersion,
      static_cast<uint16_t>(sizeof(KernelCallWireResponseBlock)),
  };
  uint32_t status{0};
  uint32_t rejection{0};
  uint64_t flags{0};
  uint8_t action_performed{0};
  uint8_t yielded{0};
  uint8_t service_registered{0};
  uint8_t service_suspended{0};
  uint8_t service_unhealthy{0};
  uint8_t service_blocked{0};
  std::array<uint8_t, 2> reserved0{};
  uint32_t spawned_tid{0};
  uint32_t queried_tid{0};
  uint32_t caller_tid{0};
  uint32_t caller_process_group_id{0};
  uint32_t supervisor_id{0};
  uint32_t address_space_id{0};
  uint32_t target_process_group_id{0};
  uint32_t service_id{0};
  uint32_t capability_count{0};
  uint32_t delegation_entry_count{0};
  uint32_t thread_state_bits{0};
  uint64_t thread_pc{0};
  uint64_t thread_sp{0};
  int64_t thread_register0{0};
  uint64_t process_group_owned_page_count{0};
  uint64_t process_group_pending_fault_count{0};
  uint64_t runtime_mapped_pages{0};
  uint64_t runtime_boot_critical_address_space_count{0};
  uint64_t fault_summary_recorded_faults{0};
  uint64_t fault_summary_pending_faults{0};
  uint64_t fault_summary_delivered_faults{0};
  uint64_t fault_summary_routed_thread_faults{0};
  uint64_t fault_summary_quarantined_threads{0};
  uint64_t supervisor_fault_notifications{0};
  uint64_t supervisor_acknowledgements{0};
  uint64_t supervisor_recovered_groups{0};
  uint64_t supervisor_capability_transitions{0};
  uint64_t supervisor_delegated_capability_count{0};
  uint32_t supervisor_managed_group_count{0};
  uint32_t supervisor_managed_faulted_group_count{0};
  uint32_t supervisor_pending_group_count{0};
  uint32_t supervisor_capability_process_group_count{0};
  uint32_t supervisor_delegation_process_group_count{0};
  uint32_t supervisor_delegation_entry_count{0};
  uint32_t supervisor_last_pending_group{0};
  uint32_t supervisor_last_acknowledged_group{0};
  uint32_t supervisor_last_recovered_group{0};
  uint32_t supervisor_last_capability_transition_group_id{0};
  uint64_t supervisor_last_capability_transition_record_id{0};
  KernelCallWireMessage message{};
  KernelCallWireFault fault{};
  std::array<KernelCallWireCapabilityRecord, kKernelAbiWireCapabilitySlots> capabilities{};
  std::array<char, kKernelAbiWireServiceNameBytes> service_name{};
  std::array<char, kKernelAbiWireThreadLabelBytes> thread_label{};
};

bool axion_kernel_validate_wire_request_block(
    const KernelCallWireRequestBlock& block) noexcept;
bool axion_kernel_validate_wire_response_block(
    const KernelCallWireResponseBlock& block) noexcept;

KernelCallWireRequestBlock axion_kernel_encode_wire_request(
    const KernelCallRequest& request) noexcept;
std::optional<KernelCallRequest> axion_kernel_decode_wire_request(
    const KernelCallWireRequestBlock& block) noexcept;

KernelCallWireResponseBlock axion_kernel_encode_wire_response(
    const KernelCallResult& result) noexcept;
std::optional<KernelCallResult> axion_kernel_decode_wire_response(
    const KernelCallWireResponseBlock& block) noexcept;

bool axion_kernel_call_wire(KernelRuntimeState& state,
                            const KernelCallWireRequestBlock* request_block,
                            KernelCallWireResponseBlock* response_block) noexcept;
bool axion_kernel_call_wire_bytes(KernelRuntimeState& state,
                                  const void* request_bytes,
                                  std::size_t request_size,
                                  void* response_bytes,
                                  std::size_t response_size) noexcept;
bool axion_kernel_call_wire_tva(KernelRuntimeState& state,
                                uint64_t request_tva,
                                uint64_t response_tva) noexcept;

}  // namespace t81::ternaryos::kernel
