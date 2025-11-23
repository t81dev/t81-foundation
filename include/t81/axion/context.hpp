#pragma once

#include <string>
#include "t81/hanoi/types.hpp"

namespace t81::axion {
// Syscall context used by policy evaluation without colliding with legacy Context.
struct SyscallContext {
  t81::hanoi::SnapshotRef snapshot;
  std::string caller;
  std::string syscall;
};
}  // namespace t81::axion

