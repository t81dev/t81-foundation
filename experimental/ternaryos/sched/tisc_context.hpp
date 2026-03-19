#pragma once

// experimental/ternaryos/sched/tisc_context.hpp

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "t81/vm/state.hpp"

namespace t81::ternaryos::sched {

using Tid = uint32_t;

enum class ThreadState : int8_t {
  Sleeping = -1,
  Ready    =  0,
  Running  = +1,
};

struct TiscContext {
  Tid         tid{0};
  ThreadState state{ThreadState::Ready};

  std::array<std::int64_t, 243>      registers{};
  std::array<t81::vm::ValueTag, 243> register_tags{};

  std::size_t    pc{0};
  std::size_t    sp{0};
  t81::vm::Flags flags{};

  std::size_t call_depth{0};
  std::vector<std::pair<std::int64_t, std::int64_t>> stack_frames;

  bool halted{false};
  bool active{true};

  std::string label;

  std::string state_string() const {
    switch (state) {
      case ThreadState::Sleeping: return "Sleeping";
      case ThreadState::Ready:    return "Ready";
      case ThreadState::Running:  return "Running";
    }
    return "Unknown";
  }
};

}  // namespace t81::ternaryos::sched
