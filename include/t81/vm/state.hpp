#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "t81/axion/policy.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include "t81/tisc/program.hpp"
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

// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  std::array<std::int64_t, 243> registers{};  // R0..R242
  std::array<ValueTag, 243> register_tags{};
  std::vector<std::int64_t> memory;
  std::vector<ValueTag> memory_tags;
  MemoryLayout layout{};
  std::size_t sp{0};
  std::vector<t81::T729Tensor> tensors;
  std::vector<double> floats;
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
  Flags flags{};
  std::size_t pc{0};
  bool halted{false};
  std::size_t gc_cycles{0};
  std::optional<t81::axion::Policy> policy;
  std::shared_ptr<t81::weights::ModelFile> weights_model;
  std::vector<const t81::weights::NativeTensor*> weights_tensor_refs;
  std::unordered_map<std::string, std::int64_t> weights_tensor_handles;
  std::vector<std::pair<std::int64_t, std::int64_t>> stack_frames;
  std::size_t call_depth{0};
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
};
}  // namespace t81::vm
