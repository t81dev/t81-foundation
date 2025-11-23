#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "t81/tisc/program.hpp"

namespace t81::vm {
// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  std::array<std::int64_t, 8> registers{};
  std::vector<std::int64_t> memory;
  std::size_t pc{0};
  bool halted{false};
};
}  // namespace t81::vm

