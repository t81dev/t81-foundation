#pragma once

#include <array>
#include <memory>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/fraction.hpp"
#include "t81/tisc/program.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/axion/policy.hpp"
#include "t81/vm/traps.hpp"
#include "t81/weights.hpp"

namespace t81::vm {

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

enum class ValueTag : std::uint8_t {
  Int = 0,
  FloatHandle,
  FractionHandle,
  SymbolHandle,
  WeightsTensorHandle,
  TensorHandle,
  ShapeHandle,
  OptionHandle,
  ResultHandle,
  EnumHandle,
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

enum class MemorySegmentKind : std::int32_t {
  Unknown = 0,
  Code,
  Stack,
  Heap,
  Tensor,
  Meta,
};

inline const char* to_string(MemorySegmentKind kind) {
  switch (kind) {
    case MemorySegmentKind::Code: return "code";
    case MemorySegmentKind::Stack: return "stack";
    case MemorySegmentKind::Heap: return "heap";
    case MemorySegmentKind::Tensor: return "tensor";
    case MemorySegmentKind::Meta: return "meta";
    default: return "unknown";
  }
}

struct MemorySegment {
  std::size_t start{0};
  std::size_t limit{0};  // exclusive

  [[nodiscard]] bool valid() const { return limit > start; }
  [[nodiscard]] std::size_t size() const { return valid() ? limit - start : 0; }
  [[nodiscard]] bool contains(std::size_t addr) const { return valid() && addr >= start && addr < limit; }
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
};

// Virtual machine register file per spec/t81vm-spec.md.
struct State {
  std::array<std::int64_t, 27> registers{}; // R0..R26
  std::array<ValueTag, 27> register_tags{};
  std::vector<std::int64_t> memory;
  std::vector<ValueTag> memory_tags;
  MemoryLayout layout{};
  std::size_t sp{0};
  std::vector<t81::T729Tensor> tensors;
  std::vector<double> floats;
  std::vector<t81::T81Fraction> fractions;
  std::vector<std::string> symbols;
  std::vector<std::vector<int>> shapes;
  std::vector<OptionValue> options;
  std::vector<ResultValue> results;
  std::vector<EnumValue> enums;
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
  std::vector<std::pair<std::int64_t, std::int64_t>> heap_frames;
  std::size_t heap_ptr{0};
  std::size_t meta_ptr{0};
  std::vector<t81::tisc::EnumMetadata> enum_metadata;
  std::unordered_map<int, std::size_t> enum_metadata_index;
};
}  // namespace t81::vm
