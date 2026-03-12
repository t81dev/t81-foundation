#pragma once

#include "kernel_base.hpp"
#include "../ipc/canon_message.hpp"

#include <optional>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState;

enum class KernelCapabilityKind : uint8_t {
  Yield = 0,
  IpcSend,
  IpcReceive,
  FaultObserve,
  FaultAcknowledge,
};

struct KernelCapabilityRecord {
  KernelCapabilityKind kind{KernelCapabilityKind::Yield};
  std::optional<ProcessGroupId> process_group_scope{};
};

enum class KernelCallKind : uint8_t {
  Yield = 0,
  SendMessage,
  ReceiveMessage,
  ReadFaultInbox,
  AcknowledgeThreadFault,
  QueryCapabilities,
  GrantCapability,
  RevokeCapability,
};

enum class KernelCallStatus : uint8_t {
  Ok = 0,
  InvalidRequest,
  CapabilityDenied,
  FaultedCaller,
  NotFound,
  Conflict,
  RetryLater,
  PolicyDenied,
};

enum class KernelCallRejection : uint8_t {
  None = 0,
  MissingCallerThread,
  MissingCallerProcessGroup,
  FaultedCaller,
  MissingDestinationThread,
  MissingMessage,
  IpcSendFailed,
  IpcReceiveEmpty,
  MissingTargetThread,
  CrossProcessGroupTarget,
  FaultInboxEmpty,
  MissingCapability,
  MissingTargetProcessGroup,
  SupervisorMismatch,
};

struct KernelCallRequest {
  KernelCallKind kind{KernelCallKind::Yield};
  std::optional<sched::Tid> target_tid{};
  std::optional<ProcessGroupId> process_group_id{};
  std::optional<sched::Tid> ipc_dst{};
  std::optional<ipc::CanonMessage> message{};
  std::optional<KernelCapabilityRecord> capability{};
};

struct KernelCallResult {
  KernelCallStatus status{KernelCallStatus::InvalidRequest};
  KernelCallRejection rejection{KernelCallRejection::None};
  bool action_performed{false};
  bool yielded{false};
  std::optional<sched::Tid> caller_tid{};
  std::optional<ProcessGroupId> caller_process_group_id{};
  std::optional<ipc::CanonMessage> message{};
  std::optional<KernelFaultRecord> fault{};
  std::vector<KernelCapabilityRecord> capabilities;
};

KernelCallResult axion_kernel_call(KernelRuntimeState& state,
                                   const KernelCallRequest& request) noexcept;

}  // namespace t81::ternaryos::kernel
