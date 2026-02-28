#include "internal/memory_segments.hpp"

namespace t81::vm::internal {

bool mem_ok(const State& state, int addr, bool code_segment) {
  if (addr < 0) return false;
  const std::size_t a = static_cast<std::size_t>(addr);
  const auto& layout = state.layout;
  if (code_segment) {
    return a >= layout.code.start && a <= layout.code.limit;
  }
  if (a >= state.memory.size()) return false;
  return layout.stack.contains(a) || layout.heap.contains(a) || layout.tensor.contains(a) ||
         layout.meta.contains(a);
}

std::optional<std::size_t> push_stack_word(State& state, ThreadContext& ctx, std::int64_t value,
                                           ValueTag tag) {
  if (ctx.sp <= ctx.stack_limit) return std::nullopt;
  const std::size_t new_sp = ctx.sp - 1;
  if (state.policy && state.policy->max_stack &&
      static_cast<std::int64_t>(ctx.stack_base - new_sp) > *state.policy->max_stack) {
    return std::nullopt;
  }

  ctx.sp = new_sp;
  state.memory[ctx.sp] = value;
  state.memory_tags[ctx.sp] = tag;
  return static_cast<std::size_t>(ctx.sp);
}

std::optional<std::size_t> pop_stack_word(const State& state, ThreadContext& ctx,
                                          std::int64_t& value, ValueTag& tag) {
  if (ctx.sp >= ctx.stack_base) return std::nullopt;
  const std::size_t addr = ctx.sp;
  value = state.memory[addr];
  tag = state.memory_tags[addr];
  ++ctx.sp;
  return addr;
}

std::size_t align_block81(std::size_t size) {
  if (size % 81 != 0) {
    size = ((size / 81) + 1) * 81;
  }
  return size;
}

MemorySegmentKind segment_for_address(const State& state, std::size_t addr) {
  const auto& layout = state.layout;
  if (layout.stack.contains(addr)) {
    return MemorySegmentKind::Stack;
  }
  if (layout.heap.contains(addr)) {
    return MemorySegmentKind::Heap;
  }
  if (layout.tensor.contains(addr)) {
    return MemorySegmentKind::Tensor;
  }
  if (layout.meta.contains(addr)) {
    return MemorySegmentKind::Meta;
  }
  return MemorySegmentKind::Unknown;
}

}  // namespace t81::vm::internal
