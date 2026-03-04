#include "internal/memory_segments.hpp"
#include <iostream>
#include <iomanip>

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
  
  // BG-10: Update stack usage statistics
  std::size_t current_usage = ctx.stack_base - ctx.sp;
  if (current_usage > state.memory_stats.stack_peak_usage) {
    state.memory_stats.stack_peak_usage = current_usage;
  }
  state.memory_stats.total_allocations++;
  
  return static_cast<std::size_t>(ctx.sp);
}

std::optional<std::size_t> pop_stack_word(const State& state, ThreadContext& ctx,
                                          std::int64_t& value, ValueTag& tag) {
  if (ctx.sp >= ctx.stack_base) return std::nullopt;
  const std::size_t addr = ctx.sp;
  value = state.memory[addr];
  tag = state.memory_tags[addr];
  ++ctx.sp;
  
  // BG-10: Update deallocation statistics
  const_cast<State&>(state).memory_stats.total_deallocations++;
  
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

// BG-10 Memory Pool Optimization Functions Implementation
void update_memory_stats(State& state, MemorySegmentKind segment, std::size_t usage) {
  switch (segment) {
    case MemorySegmentKind::Stack:
      if (usage > state.memory_stats.stack_peak_usage) {
        state.memory_stats.stack_peak_usage = usage;
      }
      break;
    case MemorySegmentKind::Heap:
      if (usage > state.memory_stats.heap_peak_usage) {
        state.memory_stats.heap_peak_usage = usage;
      }
      break;
    case MemorySegmentKind::Tensor:
      if (usage > state.memory_stats.tensor_peak_usage) {
        state.memory_stats.tensor_peak_usage = usage;
      }
      break;
    case MemorySegmentKind::Meta:
      if (usage > state.memory_stats.meta_peak_usage) {
        state.memory_stats.meta_peak_usage = usage;
      }
      break;
    default:
      break;
  }
}

std::size_t calculate_memory_efficiency(const State& state) {
  std::size_t total_used = 0;
  std::size_t total_allocated = 0;
  
  // Calculate used memory
  total_used += state.memory_stats.stack_peak_usage;
  total_used += state.memory_stats.heap_peak_usage;
  total_used += state.memory_stats.tensor_peak_usage;
  total_used += state.memory_stats.meta_peak_usage;
  
  // Calculate allocated memory
  total_allocated += state.layout.stack.size();
  total_allocated += state.layout.heap.size();
  total_allocated += state.layout.tensor.size();
  total_allocated += state.layout.meta.size();
  
  if (total_allocated == 0) return 0;
  
  return (total_used * 100) / total_allocated;  // Return percentage
}

void print_memory_stats(const State& state) {
  std::cout << "\n=== BG-10 Memory Pool Statistics ===" << std::endl;
  std::cout << "Stack Usage: " << state.memory_stats.stack_peak_usage 
            << " / " << state.layout.stack.size() << " ("
            << (state.layout.stack.size() > 0 ? 
                (state.memory_stats.stack_peak_usage * 100) / state.layout.stack.size() : 0)
            << "%)" << std::endl;
  
  std::cout << "Heap Usage: " << state.memory_stats.heap_peak_usage 
            << " / " << state.layout.heap.size() << " ("
            << (state.layout.heap.size() > 0 ? 
                (state.memory_stats.heap_peak_usage * 100) / state.layout.heap.size() : 0)
            << "%)" << std::endl;
  
  std::cout << "Tensor Usage: " << state.memory_stats.tensor_peak_usage 
            << " / " << state.layout.tensor.size() << " ("
            << (state.layout.tensor.size() > 0 ? 
                (state.memory_stats.tensor_peak_usage * 100) / state.layout.tensor.size() : 0)
            << "%)" << std::endl;
  
  std::cout << "Meta Usage: " << state.memory_stats.meta_peak_usage 
            << " / " << state.layout.meta.size() << " ("
            << (state.layout.meta.size() > 0 ? 
                (state.memory_stats.meta_peak_usage * 100) / state.layout.meta.size() : 0)
            << "%)" << std::endl;
  
  std::cout << "Total Allocations: " << state.memory_stats.total_allocations << std::endl;
  std::cout << "Total Deallocations: " << state.memory_stats.total_deallocations << std::endl;
  std::cout << "Fragmentation Events: " << state.memory_stats.fragmentation_count << std::endl;
  std::cout << "Overall Efficiency: " << calculate_memory_efficiency(state) << "%" << std::endl;
  std::cout << "======================================" << std::endl;
}

void reset_memory_stats(State& state) {
  state.memory_stats.stack_peak_usage = 0;
  state.memory_stats.heap_peak_usage = 0;
  state.memory_stats.tensor_peak_usage = 0;
  state.memory_stats.meta_peak_usage = 0;
  state.memory_stats.total_allocations = 0;
  state.memory_stats.total_deallocations = 0;
  state.memory_stats.fragmentation_count = 0;
  state.memory_stats.memory_efficiency = 0.0;
}

}  // namespace t81::vm::internal
