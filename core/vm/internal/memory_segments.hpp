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

// BG-10 Memory Pool Optimization Functions
void update_memory_stats(State& state, MemorySegmentKind segment, std::size_t usage);
std::size_t calculate_memory_efficiency(const State& state);
void print_memory_stats(const State& state);
void reset_memory_stats(State& state);

}  // namespace t81::vm::internal
