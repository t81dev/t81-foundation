#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "t81/axion/policy.hpp"
#include "t81/experimental/hanoi/types.hpp"
#include "t81/isa/opcodes.hpp"

namespace t81::axion {
// Syscall context used by policy evaluation without colliding with legacy Context.
struct SyscallContext {
  t81::hanoi::SnapshotRef snapshot;
  std::string caller;
  std::string syscall;
  std::string payload;  // Extended payload (e.g. hash for TLOADHASH)
  const Policy* policy{nullptr};
  std::vector<std::string_view> trace_reasons;
  std::size_t pc{0};
  t81::tisc::Opcode next_opcode{t81::tisc::Opcode::Nop};
  std::size_t recursion_depth{0};
  std::size_t instruction_count{0};
  std::size_t stack_usage{0};
  std::size_t reflection_count{0};
  std::size_t meta_write_count{0};
  int current_tier{0};
  // Complete authorization at a host side-effect boundary. Requires an active
  // policy and checks all required evidence now, without inventing a VM opcode.
  // Normal VM evaluation still completes required-evidence checks at Halt.
  bool final_authorization{false};
};
}  // namespace t81::axion
