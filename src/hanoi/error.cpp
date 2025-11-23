#include "t81/hanoi/error.hpp"

namespace t81::hanoi {
std::string to_string(Error error) {
  switch (error) {
    case Error::AxionRejection:
      return "AxionRejection";
    case Error::CapabilityMissing:
      return "CapabilityMissing";
    case Error::CapabilityRevoked:
      return "CapabilityRevoked";
    case Error::CanonCorruption:
      return "CanonCorruption";
    case Error::CanonMismatch:
      return "CanonMismatch";
    case Error::InvalidExec:
      return "InvalidExec";
    case Error::OutOfMemory:
      return "OutOfMemory";
    case Error::RepairError:
      return "RepairError";
    case Error::SealError:
      return "SealError";
  }
  return "Unknown";
}
}  // namespace t81::hanoi

