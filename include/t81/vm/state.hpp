#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "t81/tisc/program.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

struct MemoryLayout {
  std::size_t code_limit{0};   // exclusive
  std::size_t stack_limit{0};  // exclusive
  std::size_t heap_limit{0};   // exclusive
};
// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  std::array<std::int64_t, 27> registers{}; // R0..R26
  std::vector<std::int64_t> memory;
  MemoryLayout layout{};
  std::vector<TraceEntry> trace;
  std::size_t pc{0};
  bool halted{false};
};
}  // namespace t81::vm
