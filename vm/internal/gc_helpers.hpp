#pragma once

#include <cstddef>

#include "t81/vm/state.hpp"

namespace t81::vm::internal {

struct GcReclaimCounts {
  std::size_t tensors{0};
  std::size_t infinite_forms{0};
};

GcReclaimCounts mark_and_sweep(State& state);
void compact_heap(State& state, std::size_t new_ptr);

}  // namespace t81::vm::internal
