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

// BG-10 Phase 2: Dynamic Pool Management Functions
bool check_memory_pressure(State& state, MemorySegmentKind segment);
bool expand_memory_pool(State& state, MemorySegmentKind segment);
bool contract_memory_pool(State& state, MemorySegmentKind segment);
void enable_dynamic_sizing(State& state, bool enable = true);
void configure_pool_settings(State& state, const State::DynamicPoolConfig& config);
std::size_t calculate_optimal_pool_size(State& state, MemorySegmentKind segment);

// BG-10 Phase 3: Unified Memory System Functions
bool initialize_unified_memory(State& state, std::size_t total_size);
std::optional<std::size_t> allocate_unified_memory(State& state, std::size_t size, MemorySegmentKind type);
bool deallocate_unified_memory(State& state, std::size_t address);
bool compact_unified_memory(State& state);
std::size_t calculate_fragmentation(const State& state);
void enable_unified_memory(State& state, bool enable = true);
void print_unified_memory_stats(const State& state);

// BG-10 Phase 4: Performance Optimization Functions
void update_performance_metrics(State& state);
void configure_memory_settings(State& state, const State::MemoryConfig& config);
bool should_auto_compact(const State& state);
void print_performance_metrics(const State& state);
void reset_performance_metrics(State& state);
std::size_t get_memory_efficiency_score(const State& state);

// BG-10 Phase 5: Advanced Memory Management Functions
void enable_leak_detection(State& state, bool enable = true);
void track_allocation(State& state, std::size_t address, std::size_t size, MemorySegmentKind type);
void track_deallocation(State& state, std::size_t address);
void detect_memory_leaks(const State& state);
void print_leak_report(const State& state);
void enable_pool_hierarchy(State& state, bool enable = true);
std::size_t get_pool_category(std::size_t size, const State& state);
void update_pool_statistics(State& state, std::size_t size);
void run_garbage_collection(State& state, bool forced = false);
void print_gc_metrics(const State& state);

}  // namespace t81::vm::internal
