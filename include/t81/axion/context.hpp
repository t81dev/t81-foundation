#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "t81/axion/policy.hpp"
#include "t81/hanoi/types.hpp"
#include "t81/tisc/opcodes.hpp"

namespace t81::axion {
// Syscall context used by policy evaluation without colliding with legacy Context.
struct SyscallContext {
  t81::hanoi::SnapshotRef snapshot;
  std::string caller;
  std::string syscall;
  const Policy* policy{nullptr};
  std::vector<std::string_view> trace_reasons;
  std::size_t pc{0};
  t81::tisc::Opcode next_opcode{t81::tisc::Opcode::Nop};
};
}  // namespace t81::axion
