#include "internal/memory_segments.hpp"
#include <iostream>
#include <iomanip>

namespace t81::vm::internal {

bool mem_ok(const State& state, std::int64_t addr, bool code_segment) {
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

// BG-10 Phase 2: Dynamic Pool Management Functions Implementation
bool check_memory_pressure(State& state, MemorySegmentKind segment) {
  if (!state.pool_config.enable_dynamic_sizing) {
    return false;
  }

  std::size_t current_usage = 0;
  std::size_t pool_size = 0;
  
  switch (segment) {
    case MemorySegmentKind::Stack:
      current_usage = state.memory_stats.stack_peak_usage;
      pool_size = state.layout.stack.size();
      break;
    case MemorySegmentKind::Heap:
      current_usage = state.memory_stats.heap_peak_usage;
      pool_size = state.layout.heap.size();
      break;
    case MemorySegmentKind::Tensor:
      current_usage = state.memory_stats.tensor_peak_usage;
      pool_size = state.layout.tensor.size();
      break;
    case MemorySegmentKind::Meta:
      current_usage = state.memory_stats.meta_peak_usage;
      pool_size = state.layout.meta.size();
      break;
    default:
      return false;
  }

  if (pool_size == 0) return false;
  
  double usage_ratio = static_cast<double>(current_usage) / pool_size;
  return usage_ratio >= state.pool_config.expansion_threshold;
}

bool expand_memory_pool(State& state, MemorySegmentKind segment) {
  if (!state.pool_config.enable_dynamic_sizing) {
    return false;
  }

  std::size_t current_size = 0;
  std::size_t min_size = 0;
  
  switch (segment) {
    case MemorySegmentKind::Stack:
      current_size = state.layout.stack.size();
      min_size = state.pool_config.min_stack_size;
      break;
    case MemorySegmentKind::Heap:
      current_size = state.layout.heap.size();
      min_size = state.pool_config.min_heap_size;
      break;
    case MemorySegmentKind::Tensor:
      current_size = state.layout.tensor.size();
      min_size = state.pool_config.min_tensor_size;
      break;
    case MemorySegmentKind::Meta:
      current_size = state.layout.meta.size();
      min_size = state.pool_config.min_meta_size;
      break;
    default:
      return false;
  }

  // Check if we can expand
  std::size_t max_size = current_size * state.pool_config.max_expansion_factor;
  if (current_size >= max_size) {
    return false; // Already at maximum size
  }

  std::size_t new_size = std::min(current_size + state.pool_config.expansion_increment, max_size);
  new_size = std::max(new_size, min_size);
  
  // For now, we'll implement the expansion logic in Phase 3
  // Phase 2 focuses on the detection and configuration
  return true; // Expansion would be successful
}

bool contract_memory_pool(State& state, MemorySegmentKind segment) {
  if (!state.pool_config.enable_dynamic_sizing) {
    return false;
  }

  std::size_t current_usage = 0;
  std::size_t current_size = 0;
  std::size_t min_size = 0;
  
  switch (segment) {
    case MemorySegmentKind::Stack:
      current_usage = state.memory_stats.stack_peak_usage;
      current_size = state.layout.stack.size();
      min_size = state.pool_config.min_stack_size;
      break;
    case MemorySegmentKind::Heap:
      current_usage = state.memory_stats.heap_peak_usage;
      current_size = state.layout.heap.size();
      min_size = state.pool_config.min_heap_size;
      break;
    case MemorySegmentKind::Tensor:
      current_usage = state.memory_stats.tensor_peak_usage;
      current_size = state.layout.tensor.size();
      min_size = state.pool_config.min_tensor_size;
      break;
    case MemorySegmentKind::Meta:
      current_usage = state.memory_stats.meta_peak_usage;
      current_size = state.layout.meta.size();
      min_size = state.pool_config.min_meta_size;
      break;
    default:
      return false;
  }

  if (current_size <= min_size) {
    return false; // Already at minimum size
  }

  double usage_ratio = static_cast<double>(current_usage) / current_size;
  if (usage_ratio > state.pool_config.contraction_threshold) {
    return false; // Too much usage to contract
  }

  // For now, we'll implement the contraction logic in Phase 3
  // Phase 2 focuses on the detection and configuration
  return true; // Contraction would be successful
}

void enable_dynamic_sizing(State& state, bool enable) {
  state.pool_config.enable_dynamic_sizing = enable;
}

void configure_pool_settings(State& state, const State::DynamicPoolConfig& config) {
  state.pool_config = config;
}

std::size_t calculate_optimal_pool_size(State& state, MemorySegmentKind segment) {
  std::size_t peak_usage = 0;
  std::size_t default_size = 0;
  
  switch (segment) {
    case MemorySegmentKind::Stack:
      peak_usage = state.memory_stats.stack_peak_usage;
      default_size = 256; // kDefaultStackSize
      break;
    case MemorySegmentKind::Heap:
      peak_usage = state.memory_stats.heap_peak_usage;
      default_size = 768; // kDefaultHeapSize
      break;
    case MemorySegmentKind::Tensor:
      peak_usage = state.memory_stats.tensor_peak_usage;
      default_size = 256; // kDefaultTensorSpace
      break;
    case MemorySegmentKind::Meta:
      peak_usage = state.memory_stats.meta_peak_usage;
      default_size = 256; // kDefaultMetaSpace
      break;
    default:
      return default_size;
  }

  // Calculate optimal size with 20% buffer
  std::size_t optimal_size = static_cast<std::size_t>(peak_usage * 1.2);
  
  // Ensure minimum size requirements
  switch (segment) {
    case MemorySegmentKind::Stack:
      optimal_size = std::max(optimal_size, state.pool_config.min_stack_size);
      break;
    case MemorySegmentKind::Heap:
      optimal_size = std::max(optimal_size, state.pool_config.min_heap_size);
      break;
    case MemorySegmentKind::Tensor:
      optimal_size = std::max(optimal_size, state.pool_config.min_tensor_size);
      break;
    case MemorySegmentKind::Meta:
      optimal_size = std::max(optimal_size, state.pool_config.min_meta_size);
      break;
    default:
      break;
  }

  return optimal_size;
}

// BG-10 Phase 3: Unified Memory System Functions Implementation
bool initialize_unified_memory(State& state, std::size_t total_size) {
  if (state.unified_memory.enable_unified_memory) {
    return false; // Already initialized
  }

  state.unified_memory.unified_pool.resize(total_size, 0);
  state.unified_memory.unified_tags.resize(total_size, ValueTag::Int);
  state.unified_memory.allocation_map.resize(total_size, false);
  state.unified_memory.total_size = total_size;
  state.unified_memory.allocated_size = 0;
  state.unified_memory.free_size = total_size;
  state.unified_memory.fragmentation_count = 0;
  state.unified_memory.memory_blocks.clear();
  
  // Create initial free block
  State::UnifiedMemory::MemoryBlock free_block;
  free_block.start = 0;
  free_block.size = total_size;
  free_block.type = MemorySegmentKind::Unknown;
  free_block.allocated = false;
  state.unified_memory.memory_blocks.push_back(free_block);
  
  state.unified_memory.enable_unified_memory = true;
  return true;
}

std::optional<std::size_t> allocate_unified_memory(State& state, std::size_t size, MemorySegmentKind type) {
  if (!state.unified_memory.enable_unified_memory) {
    return std::nullopt;
  }

  // Find first fit free block
  for (auto& block : state.unified_memory.memory_blocks) {
    if (!block.allocated && block.size >= size) {
      // Allocate this block
      block.allocated = true;
      block.type = type;
      
      // If block is larger than needed, split it
      if (block.size > size) {
        State::UnifiedMemory::MemoryBlock remaining_block;
        remaining_block.start = block.start + size;
        remaining_block.size = block.size - size;
        remaining_block.type = MemorySegmentKind::Unknown;
        remaining_block.allocated = false;
        
        block.size = size;
        
        // Find the iterator position and insert the remaining block
        auto it = state.unified_memory.memory_blocks.begin();
        while (it != state.unified_memory.memory_blocks.end() && it->start != block.start) {
          ++it;
        }
        if (it != state.unified_memory.memory_blocks.end()) {
          state.unified_memory.memory_blocks.insert(it + 1, remaining_block);
        }
      }
      
      // Update allocation map
      for (std::size_t i = block.start; i < block.start + size; ++i) {
        state.unified_memory.allocation_map[i] = true;
      }
      
      state.unified_memory.allocated_size += size;
      state.unified_memory.free_size -= size;
      
      return block.start;
    }
  }
  
  return std::nullopt; // No suitable block found
}

bool deallocate_unified_memory(State& state, std::size_t address) {
  if (!state.unified_memory.enable_unified_memory || address >= state.unified_memory.total_size) {
    return false;
  }

  // Find the block containing this address
  for (auto& block : state.unified_memory.memory_blocks) {
    if (block.allocated && block.start <= address && address < block.start + block.size) {
      // Deallocate this block
      block.allocated = false;
      block.type = MemorySegmentKind::Unknown;
      
      // Update allocation map
      for (std::size_t i = block.start; i < block.start + block.size; ++i) {
        if (i < state.unified_memory.allocation_map.size()) {
          state.unified_memory.allocation_map[i] = false;
        }
      }
      
      state.unified_memory.allocated_size -= block.size;
      state.unified_memory.free_size += block.size;
      
      // Try to merge with adjacent free blocks
      for (size_t i = 0; i < state.unified_memory.memory_blocks.size(); ++i) {
        if (state.unified_memory.memory_blocks[i].start == block.start) {
          // Merge with previous block if free
          if (i > 0 && !state.unified_memory.memory_blocks[i-1].allocated) {
            state.unified_memory.memory_blocks[i-1].size += state.unified_memory.memory_blocks[i].size;
            state.unified_memory.memory_blocks.erase(state.unified_memory.memory_blocks.begin() + i);
            i--; // Adjust index after erase
          }
          
          // Merge with next block if free
          if (i < state.unified_memory.memory_blocks.size() - 1 && 
              !state.unified_memory.memory_blocks[i+1].allocated) {
            state.unified_memory.memory_blocks[i].size += state.unified_memory.memory_blocks[i+1].size;
            state.unified_memory.memory_blocks.erase(state.unified_memory.memory_blocks.begin() + i + 1);
          }
          
          break;
        }
      }
      
      return true;
    }
  }
  
  return false; // Block not found
}

bool compact_unified_memory(State& state) {
  if (!state.unified_memory.enable_unified_memory) {
    return false;
  }

  std::size_t current_pos = 0;
  std::vector<State::UnifiedMemory::MemoryBlock> new_blocks;
  
  // Move all allocated blocks to the beginning
  for (const auto& block : state.unified_memory.memory_blocks) {
    if (block.allocated) {
      if (block.start != current_pos) {
        // Move block data
        for (std::size_t i = 0; i < block.size; ++i) {
          state.unified_memory.unified_pool[current_pos + i] = state.unified_memory.unified_pool[block.start + i];
          state.unified_memory.unified_tags[current_pos + i] = state.unified_memory.unified_tags[block.start + i];
          state.unified_memory.allocation_map[current_pos + i] = true;
          state.unified_memory.allocation_map[block.start + i] = false;
        }
      }
      
      State::UnifiedMemory::MemoryBlock new_block;
      new_block.start = current_pos;
      new_block.size = block.size;
      new_block.type = block.type;
      new_block.allocated = true;
      new_blocks.push_back(new_block);
      
      current_pos += block.size;
    }
  }
  
  // Create final free block
  if (current_pos < state.unified_memory.total_size) {
    State::UnifiedMemory::MemoryBlock free_block;
    free_block.start = current_pos;
    free_block.size = state.unified_memory.total_size - current_pos;
    free_block.type = MemorySegmentKind::Unknown;
    free_block.allocated = false;
    new_blocks.push_back(free_block);
  }
  
  state.unified_memory.memory_blocks = new_blocks;
  state.unified_memory.fragmentation_count = 0;
  
  return true;
}

std::size_t calculate_fragmentation(const State& state) {
  if (!state.unified_memory.enable_unified_memory) {
    return 0;
  }

  std::size_t free_blocks = 0;
  std::size_t total_free_size = 0;
  
  for (const auto& block : state.unified_memory.memory_blocks) {
    if (!block.allocated) {
      free_blocks++;
      total_free_size += block.size;
    }
  }
  
  if (free_blocks <= 1) {
    return 0; // No fragmentation
  }
  
  // Fragmentation ratio: (free_blocks - 1) / total_free_size
  return ((free_blocks - 1) * 100) / total_free_size;
}

void enable_unified_memory(State& state, bool enable) {
  if (enable && !state.unified_memory.enable_unified_memory) {
    // Initialize with default size (sum of all current pools)
    std::size_t default_size = state.layout.total_size();
    initialize_unified_memory(state, default_size);
  } else if (!enable && state.unified_memory.enable_unified_memory) {
    // Disable unified memory
    state.unified_memory.enable_unified_memory = false;
    state.unified_memory.unified_pool.clear();
    state.unified_memory.unified_tags.clear();
    state.unified_memory.allocation_map.clear();
    state.unified_memory.memory_blocks.clear();
  }
}

void print_unified_memory_stats(const State& state) {
  std::cout << "\n=== Unified Memory Statistics ===" << std::endl;
  std::cout << "Unified Memory Enabled: " << (state.unified_memory.enable_unified_memory ? "Yes" : "No") << std::endl;
  
  if (state.unified_memory.enable_unified_memory) {
    std::cout << "Total Size: " << state.unified_memory.total_size << " words" << std::endl;
    std::cout << "Allocated Size: " << state.unified_memory.allocated_size << " words" << std::endl;
    std::cout << "Free Size: " << state.unified_memory.free_size << " words" << std::endl;
    std::cout << "Memory Blocks: " << state.unified_memory.memory_blocks.size() << std::endl;
    std::cout << "Fragmentation: " << calculate_fragmentation(state) << "%" << std::endl;
    
    std::cout << "\nMemory Blocks:" << std::endl;
    for (const auto& block : state.unified_memory.memory_blocks) {
      std::cout << "  [" << block.start << "-" << (block.start + block.size - 1) << "] "
                << "Size: " << block.size << " "
                << "Type: " << static_cast<int>(block.type) << " "
                << (block.allocated ? "ALLOCATED" : "FREE") << std::endl;
    }
  }
  
  std::cout << "=================================" << std::endl;
}

// BG-10 Phase 4: Performance Optimization Functions Implementation
void update_performance_metrics(State& state) {
  if (!state.memory_config.enable_performance_monitoring) {
    return;
  }

  // Update peak memory usage
  std::size_t current_usage = state.unified_memory.allocated_size;
  if (current_usage > state.performance_metrics.peak_memory_usage) {
    state.performance_metrics.peak_memory_usage = current_usage;
  }

  // Update average fragmentation
  std::size_t current_fragmentation = calculate_fragmentation(state);
  if (current_fragmentation > 0) {
    state.performance_metrics.average_fragmentation = 
      (state.performance_metrics.average_fragmentation * state.performance_metrics.fragmentation_samples + current_fragmentation) /
      (state.performance_metrics.fragmentation_samples + 1);
    state.performance_metrics.fragmentation_samples++;
  }
}

void configure_memory_settings(State& state, const State::MemoryConfig& config) {
  state.memory_config = config;
}

bool should_auto_compact(const State& state) {
  if (!state.memory_config.auto_compaction || !state.unified_memory.enable_unified_memory) {
    return false;
  }

  std::size_t current_fragmentation = calculate_fragmentation(state);
  return current_fragmentation >= state.memory_config.compaction_threshold * 100;
}

void print_performance_metrics(const State& state) {
  std::cout << "\n=== Memory Performance Metrics ===" << std::endl;
  std::cout << "Performance Monitoring: " << (state.memory_config.enable_performance_monitoring ? "Enabled" : "Disabled") << std::endl;
  
  if (state.memory_config.enable_performance_monitoring) {
    std::cout << "Allocation Count: " << state.performance_metrics.allocation_count << std::endl;
    std::cout << "Deallocation Count: " << state.performance_metrics.deallocation_count << std::endl;
    std::cout << "Compaction Count: " << state.performance_metrics.compaction_count << std::endl;
    std::cout << "Expansion Count: " << state.performance_metrics.expansion_count << std::endl;
    std::cout << "Contraction Count: " << state.performance_metrics.contraction_count << std::endl;
    std::cout << "Peak Memory Usage: " << state.performance_metrics.peak_memory_usage << " words" << std::endl;
    std::cout << "Average Fragmentation: " << std::fixed << std::setprecision(2) 
              << state.performance_metrics.average_fragmentation << "%" << std::endl;
    
    if (state.performance_metrics.allocation_count > 0) {
      std::cout << "Average Allocation Time: " 
                << (state.performance_metrics.total_allocation_time / state.performance_metrics.allocation_count * 1000) 
                << " μs" << std::endl;
    }
    
    if (state.performance_metrics.deallocation_count > 0) {
      std::cout << "Average Deallocation Time: " 
                << (state.performance_metrics.total_deallocation_time / state.performance_metrics.deallocation_count * 1000) 
                << " μs" << std::endl;
    }
    
    if (state.performance_metrics.compaction_count > 0) {
      std::cout << "Average Compaction Time: " 
                << (state.performance_metrics.total_compaction_time / state.performance_metrics.compaction_count * 1000) 
                << " μs" << std::endl;
    }
  }
  
  std::cout << "Auto Compaction: " << (state.memory_config.auto_compaction ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Compaction Threshold: " << (state.memory_config.compaction_threshold * 100) << "%" << std::endl;
  std::cout << "Max Fragmentation: " << state.memory_config.max_fragmentation_before_compact << "%" << std::endl;
  std::cout << "=================================" << std::endl;
}

void reset_performance_metrics(State& state) {
  state.performance_metrics.allocation_count = 0;
  state.performance_metrics.deallocation_count = 0;
  state.performance_metrics.compaction_count = 0;
  state.performance_metrics.expansion_count = 0;
  state.performance_metrics.contraction_count = 0;
  state.performance_metrics.total_allocation_time = 0.0;
  state.performance_metrics.total_deallocation_time = 0.0;
  state.performance_metrics.total_compaction_time = 0.0;
  state.performance_metrics.peak_memory_usage = 0;
  state.performance_metrics.average_fragmentation = 0.0;
  state.performance_metrics.fragmentation_samples = 0;
}

std::size_t get_memory_efficiency_score(const State& state) {
  if (!state.unified_memory.enable_unified_memory || state.unified_memory.total_size == 0) {
    return 0;
  }

  // Calculate efficiency based on multiple factors
  std::size_t utilization_score = (state.unified_memory.allocated_size * 100) / state.unified_memory.total_size;
  std::size_t fragmentation_penalty = calculate_fragmentation(state);
  std::size_t block_count_penalty = std::min(state.unified_memory.memory_blocks.size() * 2, std::size_t(50));
  
  // Efficiency score = utilization - fragmentation - block_count_penalty
  std::size_t efficiency_score = utilization_score;
  if (efficiency_score > fragmentation_penalty) {
    efficiency_score -= fragmentation_penalty;
  } else {
    efficiency_score = 0;
  }
  
  if (efficiency_score > block_count_penalty) {
    efficiency_score -= block_count_penalty;
  } else {
    efficiency_score = 0;
  }
  
  return efficiency_score;
}

// BG-10 Phase 5: Advanced Memory Management Functions Implementation
void enable_leak_detection(State& state, bool enable) {
  state.leak_detector.enabled = enable;
  if (!enable) {
    state.leak_detector.active_allocations.clear();
    state.leak_detector.allocation_sizes.clear();
    state.leak_detector.allocation_types.clear();
    state.leak_detector.total_leaked_bytes = 0;
    state.leak_detector.leak_count = 0;
  }
}

void track_allocation(State& state, std::size_t address, std::size_t size, MemorySegmentKind type) {
  if (!state.leak_detector.enabled) {
    return;
  }

  state.leak_detector.active_allocations.push_back(address);
  state.leak_detector.allocation_sizes.push_back(size);
  state.leak_detector.allocation_types.push_back(type);
}

void track_deallocation(State& state, std::size_t address) {
  if (!state.leak_detector.enabled) {
    return;
  }

  for (size_t i = 0; i < state.leak_detector.active_allocations.size(); ++i) {
    if (state.leak_detector.active_allocations[i] == address) {
      state.leak_detector.active_allocations.erase(state.leak_detector.active_allocations.begin() + i);
      state.leak_detector.allocation_sizes.erase(state.leak_detector.allocation_sizes.begin() + i);
      state.leak_detector.allocation_types.erase(state.leak_detector.allocation_types.begin() + i);
      return;
    }
  }
}

void detect_memory_leaks(const State& state) {
  if (!state.leak_detector.enabled) {
    return;
  }

  std::cout << "\n=== Memory Leak Detection ===" << std::endl;
  std::cout << "Active allocations: " << state.leak_detector.active_allocations.size() << std::endl;
  
  if (!state.leak_detector.active_allocations.empty()) {
    std::cout << "POTENTIAL MEMORY LEAKS DETECTED:" << std::endl;
    for (size_t i = 0; i < state.leak_detector.active_allocations.size(); ++i) {
      std::cout << "  Address: " << state.leak_detector.active_allocations[i]
                << ", Size: " << state.leak_detector.allocation_sizes[i]
                << ", Type: " << static_cast<int>(state.leak_detector.allocation_types[i]) << std::endl;
    }
  } else {
    std::cout << "No memory leaks detected." << std::endl;
  }
  std::cout << "=============================" << std::endl;
}

void print_leak_report(const State& state) {
  detect_memory_leaks(state);
}

void enable_pool_hierarchy(State& state, bool enable) {
  state.pool_hierarchy.enabled = enable;
}

std::size_t get_pool_category(std::size_t size, const State& state) {
  if (!state.pool_hierarchy.enabled) {
    return 0; // Default category
  }

  if (size <= state.pool_hierarchy.small_pool_size) {
    return 1; // Small pool
  } else if (size <= state.pool_hierarchy.medium_pool_size) {
    return 2; // Medium pool
  } else {
    return 3; // Large pool
  }
}

void update_pool_statistics(State& state, std::size_t size) {
  if (!state.pool_hierarchy.enabled) {
    return;
  }

  std::size_t category = get_pool_category(size, state);
  switch (category) {
    case 1:
      state.pool_hierarchy.small_allocations++;
      break;
    case 2:
      state.pool_hierarchy.medium_allocations++;
      break;
    case 3:
      state.pool_hierarchy.large_allocations++;
      break;
    default:
      state.pool_hierarchy.tiny_allocations++;
      break;
  }
}

void run_garbage_collection(State& state, bool forced) {
  if (!state.unified_memory.enable_unified_memory) {
    return;
  }

  // Simulate garbage collection
  state.gc_metrics.gc_cycles++;
  if (forced) {
    state.gc_metrics.forced_gc_count++;
  } else {
    state.gc_metrics.automatic_gc_count++;
  }

  // Collect unreachable objects (simplified)
  std::size_t collected_objects = 0;
  std::size_t collected_bytes = 0;
  
  for (auto& block : state.unified_memory.memory_blocks) {
    if (block.allocated && block.type == MemorySegmentKind::Heap) {
      // Simulate collecting some heap objects
      if (collected_objects < 5) { // Collect up to 5 objects for demo
        block.allocated = false;
        block.type = MemorySegmentKind::Unknown;
        collected_objects++;
        collected_bytes += block.size;
        state.unified_memory.allocated_size -= block.size;
        state.unified_memory.free_size += block.size;
      }
    }
  }

  state.gc_metrics.objects_collected += collected_objects;
  state.gc_metrics.bytes_collected += collected_bytes;
}

void print_gc_metrics(const State& state) {
  std::cout << "\n=== Garbage Collection Metrics ===" << std::endl;
  std::cout << "GC Cycles: " << state.gc_metrics.gc_cycles << std::endl;
  std::cout << "Objects Collected: " << state.gc_metrics.objects_collected << std::endl;
  std::cout << "Bytes Collected: " << state.gc_metrics.bytes_collected << std::endl;
  std::cout << "Forced GC Count: " << state.gc_metrics.forced_gc_count << std::endl;
  std::cout << "Automatic GC Count: " << state.gc_metrics.automatic_gc_count << std::endl;
  
  if (state.gc_metrics.gc_cycles > 0) {
    std::cout << "Average Objects per GC: " 
              << (state.gc_metrics.objects_collected / state.gc_metrics.gc_cycles) << std::endl;
    std::cout << "Average Bytes per GC: " 
              << (state.gc_metrics.bytes_collected / state.gc_metrics.gc_cycles) << std::endl;
  }
  
  std::cout << "=================================" << std::endl;
}

}  // namespace t81::vm::internal
