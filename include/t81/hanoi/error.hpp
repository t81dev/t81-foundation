#pragma once

#include <string>

namespace t81::hanoi {
enum class Error {
  AxionRejection,
  CapabilityMissing,
  CapabilityRevoked,
  CanonCorruption,
  CanonMismatch,
  InvalidExec,
  OutOfMemory,
  RepairError,
  SealError,
};

std::string to_string(Error error);
}  // namespace t81::hanoi

