#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/fraction.hpp"
#include "t81/tisc/program.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

enum class ValueTag : std::uint8_t {
  Int = 0,
  FloatHandle,
  FractionHandle,
  SymbolHandle,
};

struct Flags {
  bool zero{false};
  bool negative{false};
  bool positive{false};
};

struct MemoryLayout {
  std::size_t code_limit{0};   // exclusive
  std::size_t stack_limit{0};  // exclusive
  std::size_t heap_limit{0};   // exclusive
};

struct AxionEvent {
  t81::tisc::Opcode opcode;
  std::int32_t tag{0};
  std::int64_t value{0};
  t81::axion::Verdict verdict;
};

// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  std::array<std::int64_t, 27> registers{}; // R0..R26
  std::array<ValueTag, 27> register_tags{};
  std::vector<std::int64_t> memory;
  std::vector<ValueTag> memory_tags;
  MemoryLayout layout{};
  std::size_t sp{0};
  std::vector<t81::T729Tensor> tensors;
  std::vector<double> floats;
  std::vector<t81::T81Fraction> fractions;
  std::vector<std::string> symbols;
  std::vector<TraceEntry> trace;
  std::vector<AxionEvent> axion_log;
  Flags flags{};
  std::size_t pc{0};
  bool halted{false};
};
}  // namespace t81::vm
