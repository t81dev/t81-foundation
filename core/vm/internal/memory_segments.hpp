#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "t81/vm/state.hpp"

namespace t81::vm::internal {

bool mem_ok(const State& state, int addr, bool code_segment);

std::optional<std::size_t> push_stack_word(State& state, ThreadContext& ctx, std::int64_t value,
                                           ValueTag tag);

std::optional<std::size_t> pop_stack_word(const State& state, ThreadContext& ctx,
                                          std::int64_t& value, ValueTag& tag);

std::size_t align_block81(std::size_t size);
MemorySegmentKind segment_for_address(const State& state, std::size_t addr);

}  // namespace t81::vm::internal
