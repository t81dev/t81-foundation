#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include "t81/axion/policy.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/experimental/cog/tier.hpp"
#include "t81/experimental/cog/tier1/symbolic.hpp"
#include "t81/experimental/cog/tier2/reflective.hpp"
#include "t81/experimental/cog/tier3/recursive.hpp"
#include "t81/experimental/cog/tier5/infinite.hpp"
#include "t81/experimental/distributed/distributed.hpp"
#include "t81/fraction.hpp"
#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/traps.hpp"
#include "t81/weights.hpp"

namespace t81::vm {

constexpr std::size_t kMaxReflectionsPerEpoch = 81;
constexpr std::size_t kMaxMetaWritesPerEpoch = 243;

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

enum class ValueTag : std::uint8_t {
  Int = 0,
  Bool,
  BigIntHandle,
  FloatHandle,
  FractionHandle,
  SymbolHandle,
  WeightsTensorHandle,
  TensorHandle,
  ShapeHandle,
  OptionHandle,
  ResultHandle,
  EnumHandle,
  ComplexHandle,
  ReflectionHandle,
  ProofHandle,
  IoStreamHandle,
  IoNetHandle,
  AsyncThreadHandle,
  AsyncPromiseHandle,
  StringVectorHandle,
  SymbolicGraphHandle,
  Tier2FrameHandle,
  InfiniteHandle,
};

struct Flags {
  bool zero{false};
  bool negative{false};
  bool positive{false};
};

struct OptionValue {
  bool has_value{false};
  ValueTag payload_tag{ValueTag::Int};
  std::int64_t payload{0};
};

struct ResultValue {
  bool is_ok{false};
  ValueTag payload_tag{ValueTag::Int};
  std::int64_t payload{0};
};

struct EnumValue {
  int variant_id{0};
  bool has_payload{false};
  ValueTag payload_tag{ValueTag::Int};
  std::int64_t payload{0};
  int enum_id{-1};
};

struct ComplexValue {
  std::int64_t real{0};
  std::int64_t imag{0};
};

enum class MemorySegmentKind : std::int32_t {
  Unknown = 0,
  Code,
  Stack,
  Heap,
  Tensor,
  Meta,
  Registers = 100,
};

inline const char* to_string(MemorySegmentKind kind) {
  switch (kind) {
    case MemorySegmentKind::Code:
      return "code";
    case MemorySegmentKind::Stack:
      return "stack";
    case MemorySegmentKind::Heap:
      return "heap";
    case MemorySegmentKind::Tensor:
      return "tensor";
    case MemorySegmentKind::Meta:
      return "meta";
    case MemorySegmentKind::Registers:
      return "registers";
    default:
      return "unknown";
  }
}

struct MemorySegment {
  std::size_t start{0};
  std::size_t limit{0};  // exclusive

  [[nodiscard]] bool valid() const { return limit > start; }
  [[nodiscard]] std::size_t size() const { return valid() ? limit - start : 0; }
  [[nodiscard]] bool contains(std::size_t addr) const {
    return valid() && addr >= start && addr < limit;
  }
};

struct MemoryLayout {
  MemorySegment code;
  MemorySegment stack;
  MemorySegment heap;
  MemorySegment tensor;
  MemorySegment meta;

  [[nodiscard]] std::size_t total_size() const { return meta.limit; }
};

struct AxionEvent {
  t81::tisc::Opcode opcode;
  std::int32_t tag{0};
  std::int64_t value{0};
  t81::axion::Verdict verdict;
  t81::axion::StructuredEvent structured;
};

/**
 * @struct ReflectionSnapshot
 * @brief Captures a cognitive snapshot for Tier 4 reflection.
 */
struct ReflectionSnapshot {
  std::size_t pc;
  std::array<std::int64_t, 243> registers;
  std::array<ValueTag, 243> register_tags;
  Flags flags;
  std::vector<TraceEntry> recent_trace;
  uint64_t code_hash;
};

/**
 * @struct RefinementCommand
 * @brief Represents a single refinement command for MetaRefine.
 */
struct RefinementCommand {
  enum class Op : int64_t {
    Noop = 0,
    WriteCode = 1,
    WriteReg = 2,
    WriteMem = 3,
  };
  Op op;
  int64_t target;
  int64_t value;
  ValueTag tag;
};

/**
 * @struct FaultInjection
 * @brief Configuration for a deterministic fault injection point.
 */
struct FaultInjection {
  std::size_t instruction_count;
  Trap trap;
};

struct ResourceMetrics {
  std::size_t total_tensor_elements{0};
  std::size_t total_tensors{0};
  std::size_t total_symbolic_nodes{0};
  std::size_t total_symbolic_graphs{0};
  std::size_t total_infinite_forms{0};
};

// Saved register snapshot for one activation frame.
// Pushed on CALL, popped on RET.  Return values travel via the push/pop
// stack (not registers), so restoring the full register file is safe.
struct RegisterFrame {
  std::array<std::int64_t, 243> registers;
  std::array<ValueTag, 243> register_tags;
};

struct ThreadContext {
  std::array<std::int64_t, 243> registers{};  // R0..R242
  std::array<ValueTag, 243> register_tags{};
  Flags flags{};
  std::size_t pc{0};
  std::size_t sp{0};
  std::vector<std::pair<std::int64_t, std::int64_t>> stack_frames;
  std::size_t call_depth{0};
  // Shadow register stack — one frame per active function call.
  std::vector<RegisterFrame> register_frame_stack;
  bool halted{false};
  bool active{true};
  std::size_t stack_base{0};
  std::size_t stack_limit{0};

  // Tier 2 Reflective
  std::vector<t81::cog::v2::ReflectiveFrame> tier2_frames;

  // Tier 3 Recursive
  t81::cog::v3::Recursor tier3_recursor;

  // Cognitive Tier Status (per thread)
  t81::cog::TierStatus tier_status;

  // RFC-0026 AI-M5: SCATTER aliasing detection.
  // Tracks (dst_handle, axis, index) tuples used in this execution frame.
  // A second SCATTER to the same tuple raises SecurityFault via Axion deny.
  std::set<std::tuple<std::int64_t, int, std::int64_t>> scatter_used;

  // RFC-0034 §5.17.6: once TACT triggers a quarantine verdict, a repeated
  // activation-ceiling violation escalates to Deny/ActivationFault.
  bool activation_quarantined{false};
};

// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  ResourceMetrics metrics;

  // Memory Pool Statistics (BG-10 Optimization)
  struct MemoryStats {
    std::size_t stack_peak_usage{0};
    std::size_t heap_peak_usage{0};
    std::size_t tensor_peak_usage{0};
    std::size_t meta_peak_usage{0};
    std::size_t total_allocations{0};
    std::size_t total_deallocations{0};
    std::size_t fragmentation_count{0};
    double memory_efficiency{0.0};  // used / allocated ratio
  } memory_stats;

  // Dynamic Memory Pool Configuration (BG-10 Phase 2)
  struct DynamicPoolConfig {
    bool enable_dynamic_sizing{false};
    double expansion_threshold{0.8};     // Expand when 80% full
    double contraction_threshold{0.3};    // Contract when 30% used
    std::size_t min_stack_size{64};      // Minimum stack size
    std::size_t min_heap_size{128};      // Minimum heap size
    std::size_t min_tensor_size{64};     // Minimum tensor size
    std::size_t min_meta_size{64};      // Minimum meta size
    std::size_t max_expansion_factor{4}; // Max 4x initial size
    std::size_t expansion_increment{128}; // Expand by 128 words
  } pool_config;

  // Unified Memory System (BG-10 Phase 3)
  struct UnifiedMemory {
    bool enable_unified_memory{false};
    std::vector<std::int64_t> unified_pool;
    std::vector<ValueTag> unified_tags;
    std::vector<bool> allocation_map;  // true = allocated, false = free
    std::size_t total_size{0};
    std::size_t allocated_size{0};
    std::size_t free_size{0};
    std::size_t fragmentation_count{0};
    
    // Memory block tracking
    struct MemoryBlock {
      std::size_t start{0};
      std::size_t size{0};
      MemorySegmentKind type{MemorySegmentKind::Unknown};
      bool allocated{false};
      
      bool operator==(const MemoryBlock& other) const {
        return start == other.start && size == other.size && 
               type == other.type && allocated == other.allocated;
      }
    };
    std::vector<MemoryBlock> memory_blocks;
  } unified_memory;

  // Performance Monitoring (BG-10 Phase 4)
  struct PerformanceMetrics {
    std::size_t allocation_count{0};
    std::size_t deallocation_count{0};
    std::size_t compaction_count{0};
    std::size_t expansion_count{0};
    std::size_t contraction_count{0};
    double total_allocation_time{0.0};
    double total_deallocation_time{0.0};
    double total_compaction_time{0.0};
    std::size_t peak_memory_usage{0};
    double average_fragmentation{0.0};
    std::size_t fragmentation_samples{0};
  } performance_metrics;

  // Memory Configuration Options (BG-10 Phase 4)
  struct MemoryConfig {
    bool enable_performance_monitoring{false};
    bool auto_compaction{false};
    double compaction_threshold{0.5};  // Compact when fragmentation > 50%
    std::size_t compaction_interval{1000};  // Check every 1000 operations
    bool enable_allocation_cache{false};
    std::size_t cache_size{64};  // Cache recent allocations
    bool prefer_contiguous_allocation{true};
    std::size_t max_fragmentation_before_compact{25};  // Max 25% fragmentation
  } memory_config;

  // Advanced Memory Management (BG-10 Phase 5)
  struct MemoryLeakDetector {
    bool enabled{false};
    std::vector<std::size_t> active_allocations;
    std::vector<std::size_t> allocation_sizes;
    std::vector<MemorySegmentKind> allocation_types;
    std::size_t total_leaked_bytes{0};
    std::size_t leak_count{0};
  } leak_detector;

  struct GCMetrics {
    std::size_t gc_cycles{0};
    std::size_t objects_collected{0};
    std::size_t bytes_collected{0};
    double total_gc_time{0.0};
    std::size_t forced_gc_count{0};
    std::size_t automatic_gc_count{0};
  } gc_metrics;

  struct MemoryPoolHierarchy {
    bool enabled{false};
    std::size_t small_pool_size{64};     // < 64 words
    std::size_t medium_pool_size{256};   // 64-256 words  
    std::size_t large_pool_size{1024};   // > 256 words
    std::size_t tiny_allocations{0};
    std::size_t small_allocations{0};
    std::size_t medium_allocations{0};
    std::size_t large_allocations{0};
  } pool_hierarchy;

  // Concurrency
  std::vector<ThreadContext> contexts;
  std::size_t current_context{0};

  std::vector<std::int64_t> memory;
  std::vector<ValueTag> memory_tags;
  MemoryLayout layout{};
  std::vector<std::optional<t81::T729DynamicTensor>> tensors;
  std::vector<std::size_t> free_tensor_indices;
  std::size_t total_tensor_elements{0};
  std::vector<double> floats;
  std::vector<t81::T81BigInt> bigints;
  std::vector<t81::T81Fraction> fractions;
  std::vector<std::string> symbols;
  std::vector<std::vector<std::string>> string_vectors;
  std::vector<std::string> printed_output;
  std::vector<std::vector<int>> shapes;
  std::vector<OptionValue> options;
  std::vector<ResultValue> results;
  std::vector<EnumValue> enums;
  std::vector<ComplexValue> complexes;
  std::vector<TraceEntry> trace;
  std::vector<AxionEvent> axion_log;
  bool halted{false};  // Global halt
  std::size_t gc_cycles{0};
  std::optional<t81::axion::Policy> policy;
  std::shared_ptr<t81::weights::ModelFile> weights_model;
  std::vector<const t81::weights::NativeTensor*> weights_tensor_refs;
  std::unordered_map<std::string, std::int64_t> weights_tensor_handles;
  std::vector<std::optional<std::vector<std::int8_t>>> weights_tensor_trits;
  std::size_t contradiction_events{0};
  std::vector<std::pair<std::int64_t, std::int64_t>> heap_frames;
  std::size_t heap_ptr{0};
  std::size_t meta_ptr{0};
  std::vector<t81::tisc::EnumMetadata> enum_metadata;
  std::unordered_map<int, std::size_t> enum_metadata_index;

  // Tier 4 Reflection
  std::vector<ReflectionSnapshot> reflection_snapshots;
  std::size_t reflection_count{0};
  std::size_t meta_write_count{0};

  // Tier 1 Symbolic
  std::vector<t81::cog::v1::SymbolicGraph> symbolic_graphs;
  std::size_t total_symbolic_nodes{0};

  // Tier 4 Distributed
  t81::cog::v4::NodeState tier4_state;

  // Tier 5 Infinite
  std::vector<std::optional<t81::cog::v5::InfiniteCanonicalForm>> infinite_forms;
  std::vector<std::size_t> free_infinite_indices;

  // Deterministic Fault Injection
  std::vector<FaultInjection> pending_faults;
};
}  // namespace t81::vm
