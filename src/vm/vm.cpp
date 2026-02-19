#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>

#include <string>
#include <string_view>
#include <utility>
#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"

#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/enum_meta.hpp"
#include "t81/vm/jit.hpp"
#include "t81/vm/vm.hpp"

namespace t81::vm {
namespace {
constexpr std::size_t kDefaultStackSize = 256;
constexpr std::size_t kDefaultHeapSize = 768;
constexpr std::size_t kDefaultTensorSpace = 256;
constexpr std::size_t kDefaultMetaSpace = 256;
constexpr std::size_t kHardRecursionCeiling = 729;

class Interpreter : public IVirtualMachine {
public:
  explicit Interpreter(std::unique_ptr<t81::axion::Engine> engine)
      : axion_engine_(std::move(engine)) {
    if (!axion_engine_) {
      axion_engine_ = t81::axion::make_allow_all_engine();
    }
  }

  std::int64_t load_weights_tensor(std::string_view name) override {
    return intern_weights_tensor(name);
  }

  const t81::weights::NativeTensor* weights_tensor(std::int64_t handle) const override {
    if (handle <= 0) return nullptr;
    std::size_t idx = static_cast<std::size_t>(handle - 1);
    if (idx >= state_.weights_tensor_refs.size()) return nullptr;
    return state_.weights_tensor_refs[idx];
  }

  void load_program(const t81::tisc::Program& program) override {
    program_ = program;
    state_ = State{};
    state_.register_tags.fill(ValueTag::Int);
    auto& layout = state_.layout;
    layout.code.start = 0;
    layout.code.limit = program_.insns.size();
    layout.stack.start = layout.code.limit;
    layout.stack.limit = layout.stack.start + kDefaultStackSize;
    layout.heap.start = layout.stack.limit;
    layout.heap.limit = layout.heap.start + kDefaultHeapSize;
    layout.tensor.start = layout.heap.limit;
    layout.tensor.limit = layout.tensor.start + kDefaultTensorSpace;
    layout.meta.start = layout.tensor.limit;
    layout.meta.limit = layout.meta.start + kDefaultMetaSpace;
    state_.memory.resize(layout.total_size(), 0);
    state_.memory_tags.assign(state_.memory.size(), ValueTag::Int);
    state_.sp = layout.stack.limit;
    state_.floats = program_.float_pool;
    state_.fractions = program_.fraction_pool;
    state_.symbols = program_.symbol_pool;
    state_.string_vectors.clear();
    state_.tensors = program_.tensor_pool;
    state_.shapes = program_.shape_pool;
    state_.weights_model = program_.weights_model;
    state_.weights_tensor_refs.clear();
    state_.weights_tensor_handles.clear();
    state_.stack_frames.clear();
    state_.call_depth = 0;
    state_.contradiction_events = 0;
    state_.heap_frames.clear();
    state_.heap_ptr = layout.heap.start;
    state_.meta_ptr = layout.meta.start;
    state_.options.clear();
    state_.results.clear();
    state_.enums.clear();
    state_.enum_metadata = program_.enum_metadata;
    state_.enum_metadata_index.clear();
    for (std::size_t i = 0; i < state_.enum_metadata.size(); ++i) {
      state_.enum_metadata_index[state_.enum_metadata[i].enum_id] = i;
    }
    sync_system_registers();
    state_.policy.reset();
    state_.gc_cycles = 0;
    instructions_since_gc_ = 0;
    instruction_count_ = 0;
    if (!program_.axion_policy_text.empty()) {
      auto policy = t81::axion::parse_policy(program_.axion_policy_text);
      if (policy.has_value()) {
        state_.policy = policy.value();
        axion_engine_ = t81::axion::make_policy_engine(state_.policy);
        for (const auto& loop : state_.policy->loops) {
          AxionEvent event;
          event.opcode = t81::tisc::Opcode::Nop;
          event.tag = loop.id;
          event.value = loop.depth;
          event.verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << "loop hint file=" << loop.file << " line=" << loop.line
                 << " column=" << loop.column << " bound=";
          if (loop.bound_infinite) {
            reason << "infinite";
          } else if (loop.bound_value) {
            reason << *loop.bound_value;
          } else {
            reason << "unknown";
          }
          event.verdict.reason = reason.str();
          state_.axion_log.push_back(event);
        }
      }
    }
    if (!program_.match_metadata_text.empty()) {
      AxionEvent event;
      event.opcode = t81::tisc::Opcode::Nop;
      event.tag = 0;
      event.value = 0;
      event.verdict.kind = t81::axion::VerdictKind::Allow;
      event.verdict.reason = "match metadata: " + program_.match_metadata_text;
      state_.axion_log.push_back(event);
    }
  }

  std::expected<void, Trap> step() override {
    if (state_.halted) {
      return {};
    }

    // Check if we have a compiled trace for the current PC.
    auto trace_it = compiled_traces_.find(state_.pc);
    if (trace_it != compiled_traces_.end()) {
      const std::size_t trace_pc = state_.pc;
      const auto first_opcode = trace_pc < program_.insns.size() ? program_.insns[trace_pc].opcode
                                                                 : t81::tisc::Opcode::Halt;

      auto enter = eval_axion_call(t81::axion::reasons::kJitTraceEnter, trace_pc, first_opcode);
      if (enter.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }

      t81::axion::Verdict enter_event{t81::axion::VerdictKind::Allow, ""};
      {
        std::ostringstream reason;
        reason << t81::axion::reasons::kJitTraceEnter << " pc=" << trace_pc
               << " trace-len=" << trace_it->second->size();
        enter_event.reason = reason.str();
      }
      record_axion_event(t81::tisc::Opcode::Nop,
                         static_cast<std::int32_t>(trace_it->second->size()),
                         static_cast<std::int64_t>(trace_pc), enter_event);

      const auto exec_result = trace_it->second->execute(state_);
      instruction_count_ += exec_result.instructions_executed;

      t81::axion::Verdict exit_event{t81::axion::VerdictKind::Allow, ""};
      {
        std::ostringstream reason;
        const bool deopt = exec_result.exit_kind == JitTrace::ExitKind::GuardDeopt;
        reason << (deopt ? t81::axion::reasons::kJitTraceDeopt : t81::axion::reasons::kJitTraceExit)
               << " pc=" << state_.pc << " executed=" << exec_result.instructions_executed
               << " exit-kind=";
        switch (exec_result.exit_kind) {
          case JitTrace::ExitKind::Completed:
            reason << "completed";
            break;
          case JitTrace::ExitKind::Branch:
            reason << "branch";
            break;
          case JitTrace::ExitKind::GuardDeopt:
            reason << "guard-deopt";
            break;
        }
        exit_event.reason = reason.str();
      }
      record_axion_event(t81::tisc::Opcode::Nop,
                         static_cast<std::int32_t>(exec_result.instructions_executed),
                         static_cast<std::int64_t>(state_.pc), exit_event);

      const auto exit_reason = exec_result.exit_kind == JitTrace::ExitKind::GuardDeopt
                                   ? t81::axion::reasons::kJitTraceDeopt
                                   : t81::axion::reasons::kJitTraceExit;
      auto exit = eval_axion_call(exit_reason, state_.pc, first_opcode);
      if (exit.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }

      if (exec_result.exit_kind != JitTrace::ExitKind::GuardDeopt) {
        return {};
      }

      // Guard deopt: invalidate the trace at this entry and continue with
      // interpreter execution from the resumed PC in this same step.
      compiled_traces_.erase(trace_pc);
    }

    if (state_.pc >= program_.insns.size()) {
      auto verdict =
          eval_axion_call(t81::axion::reasons::kStep, state_.pc, t81::tisc::Opcode::Halt);
      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }
      state_.halted = true;
      return {};
    }

    const std::size_t current_pc = state_.pc;
    const auto& insn = program_.insns[current_pc];
    state_.pc += 1;
    instruction_count_++;

    // Hot-spot detection and JIT compilation.
    if (!jit_compiler_.is_tracing()) {
      hot_spots_[current_pc]++;
      if (hot_spots_[current_pc] >= kHotSpotThreshold) {
        jit_compiler_.start_tracing(current_pc);
      }
    }

    if (jit_compiler_.is_tracing()) {
      jit_compiler_.record_instruction(insn);
      if (!jit_compiler_.is_tracing()) {
        const auto trace_start_pc = jit_compiler_.trace_start_pc();
        auto trace = jit_compiler_.compile();
        if (trace) {
          compiled_traces_[trace_start_pc] = std::move(trace);
        }
      }
    }

    // Evaluate Axion policy before every instruction.
    auto verdict = eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
    }

    auto reg_ok = [this](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < state_.registers.size();
    };
    auto mem_ok = [this](int addr, bool code = false) {
      if (addr < 0) return false;
      std::size_t a = static_cast<std::size_t>(addr);
      const auto& layout = state_.layout;
      if (code) {
        return a >= layout.code.start && a <= layout.code.limit;
      }
      if (a >= state_.memory.size()) return false;
      // Strict segment containment: an address must resolve to exactly one segment.
      return layout.stack.contains(a) || layout.heap.contains(a) || layout.tensor.contains(a) ||
             layout.meta.contains(a);
    };
    auto check_mem = [this, mem_ok](t81::tisc::Opcode opcode, int addr, std::string_view action,
                                    bool code = false) -> bool {
      if (mem_ok(addr, code)) return true;
      this->log_bounds_fault(opcode, addr, action);
      return false;
    };
    auto log_trace = [this, current_pc](t81::tisc::Opcode op, Trap trap = Trap::None) {
      TraceEntry t{current_pc, op, std::nullopt};
      if (trap != Trap::None) t.trap = trap;
      state_.trace.push_back(t);
    };
    auto literal_kind_to_tag = [](t81::tisc::LiteralKind kind) -> ValueTag {
      switch (kind) {
        case t81::tisc::LiteralKind::Bool:
          return ValueTag::Bool;
        case t81::tisc::LiteralKind::FloatHandle:
          return ValueTag::FloatHandle;
        case t81::tisc::LiteralKind::FractionHandle:
          return ValueTag::FractionHandle;
        case t81::tisc::LiteralKind::SymbolHandle:
          return ValueTag::SymbolHandle;
        case t81::tisc::LiteralKind::TensorHandle:
          return ValueTag::TensorHandle;
        case t81::tisc::LiteralKind::ShapeHandle:
          return ValueTag::ShapeHandle;
        case t81::tisc::LiteralKind::Int:
        default:
          return ValueTag::Int;
      }
    };
    auto set_reg = [this](int reg, std::int64_t val_data, ValueTag tag) {
      if (reg == 0 || (reg >= 75 && reg <= 80)) return;
      state_.registers[reg] = val_data;
      state_.register_tags[reg] = tag;
    };
    auto copy_reg = [this](int dst, int src) {
      if (dst == 0 || (dst >= 75 && dst <= 80)) return;
      state_.registers[dst] = state_.registers[src];
      state_.register_tags[dst] = state_.register_tags[src];
    };
    auto update_flags = [this](std::int64_t v) {
      state_.flags.zero = (v == 0);
      state_.flags.negative = (v < 0);
      state_.flags.positive = (v > 0);
    };
    auto push_stack = [this](std::int64_t val_data, ValueTag tag) -> std::optional<std::size_t> {
      const auto& stack = state_.layout.stack;
      if (!stack.valid()) return std::nullopt;
      if (state_.sp <= stack.start) return std::nullopt;

      std::size_t new_sp = state_.sp - 1;
      if (!stack.contains(new_sp)) {
        return std::nullopt;
      }
      if (state_.policy && state_.policy->max_stack &&
          static_cast<std::int64_t>(stack.limit - new_sp) > *state_.policy->max_stack) {
        return std::nullopt;
      }

      state_.sp = new_sp;
      state_.memory[state_.sp] = val_data;
      state_.memory_tags[state_.sp] = tag;
      return static_cast<std::size_t>(state_.sp);
    };
    auto pop_stack = [this](std::int64_t& value, ValueTag& tag) -> std::optional<std::size_t> {
      const auto& stack = state_.layout.stack;
      if (!stack.valid()) return std::nullopt;
      if (state_.sp >= stack.limit) return std::nullopt;
      std::size_t addr = state_.sp;
      value = state_.memory[addr];
      tag = state_.memory_tags[addr];
      ++state_.sp;
      return addr;
    };
    auto tensor_ptr = [this](std::int64_t handle) -> t81::T729Tensor* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.tensors.size()) return nullptr;
      return &state_.tensors[idx];
    };
    auto alloc_tensor = [this, current_pc](t81::T729Tensor tensor) -> std::int64_t {
      state_.tensors.push_back(std::move(tensor));
      auto idx = state_.tensors.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Tensor, idx,
                                1, t81::axion::reasons::kTensorAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto promote_to_tensor = [&](int reg) -> std::expected<void, Trap> {
      if (reg < 0 || static_cast<std::size_t>(reg) >= state_.registers.size()) {
        return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
      }
      if (state_.register_tags[reg] == ValueTag::WeightsTensorHandle) {
        auto handle = state_.registers[reg];
        const auto* native = weights_tensor(handle);
        if (!native) return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);

        std::vector<float> float_data;
        float_data.reserve(native->num_trits());

        if (native->format == t81::weights::NativeFormat::T3_K) {
          const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(native->data.data());
          uint64_t total_trits = native->num_trits();
          for (uint64_t offset = 0; offset < total_trits; offset += 128) {
            float scale;
            std::memcpy(&scale, byte_ptr, sizeof(float));
            byte_ptr += sizeof(float);
            uint64_t count = std::min<uint64_t>(128, total_trits - offset);
            uint64_t trit_index = 0;
            for (uint64_t packed_idx = 0; packed_idx < 26; ++packed_idx) {
              uint8_t packed = *byte_ptr++;
              if (packed > 242) {
                return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
              }
              uint8_t rem = packed;
              for (uint64_t local = 0; local < 5; ++local, ++trit_index) {
                uint8_t digit = static_cast<uint8_t>(rem % 3);
                rem = static_cast<uint8_t>(rem / 3);
                if (trit_index < count) {
                  float trit = static_cast<float>(static_cast<int>(digit) - 1);
                  float_data.push_back(trit * scale);
                } else if (digit != 1) {
                  // Canonical padding requires extra trits to be zero (mapped digit=1).
                  return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
                }
              }
            }
          }
        } else {
          uint64_t remaining = native->trits;
          if (remaining == 0 && !native->data.empty()) {
            remaining = native->data.size() * 48;
          }

          for (uint64_t limb : native->data) {
            uint64_t count = std::min<uint64_t>(48, remaining);
            std::vector<float> block(count);
            uint64_t val = limb;
            for (int i = 47; i >= 0; --i) {
              uint64_t digit = val % 3;
              val /= 3;
              if (static_cast<uint64_t>(i) < count) {
                block[i] = static_cast<float>(static_cast<int>(digit) - 1);
              }
            }
            float_data.insert(float_data.end(), block.begin(), block.end());
            remaining -= count;
            if (remaining == 0) break;
          }
        }

        std::vector<int> shape;
        shape.reserve(native->shape.size());
        for (auto d : native->shape) shape.push_back(static_cast<int>(d));

        t81::T729Tensor promoted(std::move(shape), std::move(float_data));
        state_.registers[reg] = alloc_tensor(std::move(promoted));
        state_.register_tags[reg] = ValueTag::TensorHandle;
      }
      return {};
    };
    auto float_ptr = [this](std::int64_t handle) -> double* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.floats.size()) return nullptr;
      return &state_.floats[idx];
    };
    auto alloc_float = [this, current_pc](double value) -> std::int64_t {
      state_.floats.push_back(value);
      auto idx = state_.floats.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                t81::axion::reasons::kHeapAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto fraction_ptr = [this](std::int64_t handle) -> t81::T81Fraction* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.fractions.size()) return nullptr;
      return &state_.fractions[idx];
    };
    auto symbol_ptr = [this](std::int64_t handle) -> const std::string* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.symbols.size()) return nullptr;
      return &state_.symbols[idx];
    };
    auto runtime_token_text = [](ValueTag tag) -> std::optional<std::string_view> {
      switch (tag) {
        case ValueTag::ProofHandle:
          return std::string_view{"std.sys.proof"};
        case ValueTag::IoStreamHandle:
          return std::string_view{"std.io.stream"};
        case ValueTag::IoNetHandle:
          return std::string_view{"std.io.net"};
        case ValueTag::AsyncThreadHandle:
          return std::string_view{"std.async.thread"};
        case ValueTag::AsyncPromiseHandle:
          return std::string_view{"std.async.promise"};
        default:
          break;
      }
      return std::nullopt;
    };
    auto symbol_like_text = [&](ValueTag tag,
                                std::int64_t value) -> std::optional<std::string_view> {
      if (tag == ValueTag::SymbolHandle) {
        auto* symbol = symbol_ptr(value);
        if (symbol == nullptr) return std::nullopt;
        return std::string_view{*symbol};
      }
      return runtime_token_text(tag);
    };
    auto runtime_token_tag_from_symbol_handle =
        [&](std::int64_t symbol_handle) -> std::optional<ValueTag> {
      auto symbol = symbol_ptr(symbol_handle);
      if (symbol == nullptr) return std::nullopt;
      if (*symbol == "std.sys.proof") return ValueTag::ProofHandle;
      if (*symbol == "std.io.stream") return ValueTag::IoStreamHandle;
      if (*symbol == "std.io.net") return ValueTag::IoNetHandle;
      if (*symbol == "std.async.thread") return ValueTag::AsyncThreadHandle;
      if (*symbol == "std.async.promise") return ValueTag::AsyncPromiseHandle;
      return std::nullopt;
    };
    auto string_vector_ptr = [this](std::int64_t handle) -> const std::vector<std::string>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.string_vectors.size()) return nullptr;
      return &state_.string_vectors[idx];
    };
    auto intern_symbol = [this](std::string text) -> std::int64_t {
      for (std::size_t i = 0; i < state_.symbols.size(); ++i) {
        if (state_.symbols[i] == text) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      state_.symbols.push_back(std::move(text));
      return static_cast<std::int64_t>(state_.symbols.size());
    };
    auto string_vector_mut = [this](std::int64_t handle) -> std::vector<std::string>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.string_vectors.size()) return nullptr;
      return &state_.string_vectors[idx];
    };
    auto alloc_string_vector = [this]() -> std::int64_t {
      state_.string_vectors.emplace_back();
      return static_cast<std::int64_t>(state_.string_vectors.size());
    };
    auto alloc_fraction = [this, current_pc](t81::T81Fraction frac) -> std::int64_t {
      state_.fractions.push_back(std::move(frac));
      auto idx = state_.fractions.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                t81::axion::reasons::kHeapAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto shape_ptr = [this](std::int64_t handle) -> const std::vector<int>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.shapes.size()) return nullptr;
      return &state_.shapes[idx];
    };
    auto option_ptr = [this](std::int64_t handle) -> OptionValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.options.size()) return nullptr;
      return &state_.options[idx];
    };
    auto result_ptr = [this](std::int64_t handle) -> ResultValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.results.size()) return nullptr;
      return &state_.results[idx];
    };
    auto enum_ptr = [this](std::int64_t handle) -> EnumValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.enums.size()) return nullptr;
      return &state_.enums[idx];
    };
    auto complex_ptr = [this](std::int64_t handle) -> ComplexValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.complexes.size()) return nullptr;
      return &state_.complexes[idx];
    };
    auto intern_option = [this](bool has_value, ValueTag payload_tag,
                                std::int64_t payload) -> std::int64_t {
      for (std::size_t i = 0; i < state_.options.size(); ++i) {
        const auto& existing = state_.options[i];
        if (existing.has_value != has_value) continue;
        if (!has_value) return static_cast<std::int64_t>(i + 1);
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      OptionValue val;
      val.has_value = has_value;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.options.push_back(val);
      return static_cast<std::int64_t>(state_.options.size());
    };
    auto intern_result = [this](bool is_ok, ValueTag payload_tag,
                                std::int64_t payload) -> std::int64_t {
      for (std::size_t i = 0; i < state_.results.size(); ++i) {
        const auto& existing = state_.results[i];
        if (existing.is_ok != is_ok) continue;
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      ResultValue val;
      val.is_ok = is_ok;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.results.push_back(val);
      return static_cast<std::int64_t>(state_.results.size());
    };
    auto intern_enum = [this](int global_variant_id, bool has_payload, ValueTag payload_tag,
                              std::int64_t payload) -> std::int64_t {
      if (global_variant_id < 0) return 0;
      int enum_id = t81::enum_meta::decode_enum_id(global_variant_id);
      for (std::size_t i = 0; i < state_.enums.size(); ++i) {
        const auto& existing = state_.enums[i];
        if (existing.variant_id != global_variant_id) continue;
        if (existing.enum_id != enum_id) continue;
        if (existing.has_payload != has_payload) continue;
        if (!has_payload) {
          return static_cast<std::int64_t>(i + 1);
        }
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      EnumValue val;
      val.variant_id = global_variant_id;
      val.enum_id = enum_id;
      val.has_payload = has_payload;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.enums.push_back(val);
      return static_cast<std::int64_t>(state_.enums.size());
    };
    auto intern_complex = [this](std::int64_t real, std::int64_t imag) -> std::int64_t {
      for (std::size_t i = 0; i < state_.complexes.size(); ++i) {
        const auto& existing = state_.complexes[i];
        if (existing.real == real && existing.imag == imag) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      state_.complexes.push_back(ComplexValue{real, imag});
      return static_cast<std::int64_t>(state_.complexes.size());
    };
    auto clamp_trit = [](std::int64_t v) -> int {
      if (v > 0) return 1;
      if (v < 0) return -1;
      return 0;
    };

    std::function<std::optional<int>(ValueTag, std::int64_t, std::int64_t)> compare_value =
        [&](ValueTag tag, std::int64_t lhs_val, std::int64_t rhs_val) -> std::optional<int> {
      if (auto lhs = symbol_like_text(tag, lhs_val); lhs.has_value()) {
        auto rhs = symbol_like_text(tag, rhs_val);
        if (!rhs.has_value()) return std::nullopt;
        if (*lhs == *rhs) return 0;
        return (*lhs < *rhs) ? -1 : 1;
      }
      switch (tag) {
        case ValueTag::Int:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::Bool:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::FloatHandle: {
          auto* lhs = float_ptr(lhs_val);
          auto* rhs = float_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (*lhs == *rhs) return 0;
          return (*lhs < *rhs) ? -1 : 1;
        }
        case ValueTag::FractionHandle: {
          auto* lhs = fraction_ptr(lhs_val);
          auto* rhs = fraction_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          return t81::T81Fraction::cmp(*lhs, *rhs);
        }
        case ValueTag::SymbolHandle:
          return std::nullopt;
        case ValueTag::StringVectorHandle:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::TensorHandle:
        case ValueTag::ShapeHandle:
        case ValueTag::WeightsTensorHandle:
        case ValueTag::ReflectionHandle:
        case ValueTag::ProofHandle:
        case ValueTag::IoStreamHandle:
        case ValueTag::IoNetHandle:
        case ValueTag::AsyncThreadHandle:
        case ValueTag::AsyncPromiseHandle:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::ComplexHandle: {
          auto lhs = complex_ptr(lhs_val);
          auto rhs = complex_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->real == rhs->real && lhs->imag == rhs->imag) return 0;
          if (lhs->real == rhs->real) {
            return (lhs->imag < rhs->imag) ? -1 : 1;
          }
          return (lhs->real < rhs->real) ? -1 : 1;
        }
        case ValueTag::OptionHandle: {
          auto lhs = option_ptr(lhs_val);
          auto rhs = option_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->has_value != rhs->has_value) {
            return lhs->has_value ? 1 : -1;
          }
          if (!lhs->has_value) return 0;
          if (lhs->payload_tag != rhs->payload_tag) return std::nullopt;
          return compare_value(lhs->payload_tag, lhs->payload, rhs->payload);
        }
        case ValueTag::ResultHandle: {
          auto lhs = result_ptr(lhs_val);
          auto rhs = result_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->is_ok != rhs->is_ok) {
            return lhs->is_ok ? 1 : -1;
          }
          if (lhs->payload_tag != rhs->payload_tag) return std::nullopt;
          return compare_value(lhs->payload_tag, lhs->payload, rhs->payload);
        }
        case ValueTag::EnumHandle:
          return std::nullopt;
      }
      return std::nullopt;
    };

    std::function<std::optional<std::string>(ValueTag, std::int64_t, int)> format_value =
        [&](ValueTag tag, std::int64_t val_data, int depth) -> std::optional<std::string> {
      if (depth > 8) return std::nullopt;
      if (auto symbol = symbol_like_text(tag, val_data); symbol.has_value()) {
        return std::string(*symbol);
      }
      switch (tag) {
        case ValueTag::Int:
          return std::to_string(val_data);
        case ValueTag::Bool:
          return val_data != 0 ? "true" : "false";
        case ValueTag::FloatHandle: {
          auto* ptr_val = float_ptr(val_data);
          if (!ptr_val) return std::nullopt;
          double canonical = (*ptr_val == 0.0) ? 0.0 : *ptr_val;
          std::ostringstream out;
          out.imbue(std::locale::classic());
          out.precision(std::numeric_limits<double>::max_digits10);
          out << canonical << "t81";
          return out.str();
        }
        case ValueTag::FractionHandle: {
          auto* frac = fraction_ptr(val_data);
          if (!frac) return std::nullopt;
          return frac->num.to_string() + "/" + frac->den.to_string() + "t81";
        }
        case ValueTag::SymbolHandle:
          return std::nullopt;
        case ValueTag::StringVectorHandle: {
          auto* ptr_val = string_vector_ptr(val_data);
          if (!ptr_val) return std::nullopt;
          return "<strvec#" + std::to_string(val_data) + ">";
        }
        case ValueTag::TensorHandle:
          return "<tensor#" + std::to_string(val_data) + ">";
        case ValueTag::ShapeHandle:
          return "<shape#" + std::to_string(val_data) + ">";
        case ValueTag::WeightsTensorHandle:
          return "<weights#" + std::to_string(val_data) + ">";
        case ValueTag::ReflectionHandle:
          return "<reflection#" + std::to_string(val_data) + ">";
        case ValueTag::ProofHandle:
        case ValueTag::IoStreamHandle:
        case ValueTag::IoNetHandle:
        case ValueTag::AsyncThreadHandle:
        case ValueTag::AsyncPromiseHandle:
          return std::nullopt;
        case ValueTag::ComplexHandle: {
          auto* complex = complex_ptr(val_data);
          if (!complex) return std::nullopt;
          return "<complex(" + std::to_string(complex->real) + "," + std::to_string(complex->imag) +
                 ")>";
        }
        case ValueTag::OptionHandle: {
          auto* opt = option_ptr(val_data);
          if (!opt) return std::nullopt;
          if (!opt->has_value) return std::string{"None"};
          auto payload = format_value(opt->payload_tag, opt->payload, depth + 1);
          if (!payload) return std::nullopt;
          return "Some(" + *payload + ")";
        }
        case ValueTag::ResultHandle: {
          auto* result = result_ptr(val_data);
          if (!result) return std::nullopt;
          auto payload = format_value(result->payload_tag, result->payload, depth + 1);
          if (!payload) return std::nullopt;
          return result->is_ok ? "Ok(" + *payload + ")" : "Err(" + *payload + ")";
        }
        case ValueTag::EnumHandle: {
          auto* enum_value = enum_ptr(val_data);
          if (!enum_value) return std::nullopt;
          if (!enum_value->has_payload) {
            return "<enum#" + std::to_string(enum_value->variant_id) + ">";
          }
          auto payload = format_value(enum_value->payload_tag, enum_value->payload, depth + 1);
          if (!payload) return std::nullopt;
          return "<enum#" + std::to_string(enum_value->variant_id) + "(" + *payload + ")>";
        }
      }
      return std::nullopt;
    };

    Trap trap = Trap::None;
    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop: {
        if (insn.literal_kind == t81::tisc::LiteralKind::SymbolHandle && insn.b > 0) {
          auto idx = static_cast<std::size_t>(insn.b);
          if (idx <= state_.symbols.size()) {
            AxionEvent event;
            event.opcode = insn.opcode;
            event.tag = static_cast<std::int32_t>(insn.b);
            event.value = 0;
            event.verdict.kind = t81::axion::VerdictKind::Allow;
            event.verdict.reason = state_.symbols[idx - 1];
            state_.axion_log.push_back(event);
          }
        }
        break;
      }
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        break;
      case t81::tisc::Opcode::LoadImm: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto tag = literal_kind_to_tag(insn.literal_kind);
        std::int64_t value = insn.b;
        if (tag == ValueTag::SymbolHandle) {
          if (auto runtime_tag = runtime_token_tag_from_symbol_handle(insn.b);
              runtime_tag.has_value()) {
            tag = *runtime_tag;
            value = 1;
          }
        }
        set_reg(insn.a, value, tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Mov:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        copy_reg(insn.a, insn.b);
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Inc:
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] += 1;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Dec:
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] -= 1;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Add:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = state_.registers[insn.b] + state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Sub:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = state_.registers[insn.b] - state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Load: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!check_mem(insn.opcode, insn.b, "memory load")) {
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.b);
        state_.registers[insn.a] = state_.memory[addr];
        state_.register_tags[insn.a] = state_.memory_tags[addr];
        log_memory_segment_access(insn.opcode, segment_for_address(addr), addr, 1,
                                  t81::axion::reasons::kMemLoad);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::WeightsLoad: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b <= 0 || static_cast<std::size_t>(insn.b) > state_.symbols.size()) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::string& name = state_.symbols[static_cast<std::size_t>(insn.b - 1)];
        auto handle = intern_weights_tensor(name);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::WeightsTensorHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          verdict.reason = "weights.load \"" + name + "\"";
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), handle, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::TExp: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        std::vector<float> data = tensor->data();
        for (auto& val : data) val = std::exp(val);
        state_.registers[insn.a] = alloc_tensor(T729Tensor(tensor->shape(), std::move(data)));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::MetaRead: {
        if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
        std::int64_t addr = state_.registers[insn.c];
        auto verdict = eval_axion_call(t81::axion::reasons::kMetaRead, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }
        if (segment == MemorySegmentKind::Registers) {
          if (!reg_ok(static_cast<int>(addr))) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.registers[insn.a] = state_.registers[addr];
          state_.register_tags[insn.a] = state_.register_tags[addr];
        } else if (segment == MemorySegmentKind::Code) {
          if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.registers[insn.a] = static_cast<std::int64_t>(program_.insns[addr].opcode);
          state_.register_tags[insn.a] = ValueTag::Int;
        } else {
          std::size_t physical_addr = 0;
          bool ok = false;
          const auto& layout = state_.layout;
          switch (segment) {
            case MemorySegmentKind::Stack:
              if (layout.stack.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Heap:
              if (layout.heap.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Tensor:
              if (layout.tensor.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Meta:
              if (layout.meta.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            default:
              break;
          }
          if (!ok) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.registers[insn.a] = state_.memory[physical_addr];
          state_.register_tags[insn.a] = state_.memory_tags[physical_addr];
        }
        update_flags(state_.registers[insn.a]);
        apply_segment_reason(verdict, "MetaRead reflection", segment, static_cast<size_t>(addr));
        record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaWrite: {
        if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
        std::int64_t addr = state_.registers[insn.c];
        std::int64_t val = state_.registers[insn.a];
        ValueTag tag = state_.register_tags[insn.a];
        auto verdict = eval_axion_call(t81::axion::reasons::kMetaWrite, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }
        if (segment == MemorySegmentKind::Registers) {
          if (!reg_ok(static_cast<int>(addr))) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.registers[addr] = val;
          state_.register_tags[addr] = tag;
        } else if (segment == MemorySegmentKind::Code) {
          if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
            trap = Trap::BoundsFault;
            break;
          }
          program_.insns[addr].opcode = static_cast<t81::tisc::Opcode>(val);
          compiled_traces_.clear();
        } else {
          std::size_t physical_addr = 0;
          bool ok = false;
          const auto& layout = state_.layout;
          switch (segment) {
            case MemorySegmentKind::Stack:
              if (layout.stack.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Heap:
              if (layout.heap.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Tensor:
              if (layout.tensor.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Meta:
              if (layout.meta.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            default:
              break;
          }
          if (!ok) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.memory[physical_addr] = val;
          state_.memory_tags[physical_addr] = tag;
        }
        apply_segment_reason(verdict, "MetaWrite reflection", segment, static_cast<size_t>(addr));
        record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaReflect: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.reflection_count >= kMaxReflectionsPerEpoch) {
          trap = Trap::SecurityFault;
          break;
        }

        auto verdict = eval_axion_call(t81::axion::reasons::kMetaReflect, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }

        ReflectionSnapshot snapshot;
        snapshot.pc = current_pc;
        snapshot.registers = state_.registers;
        snapshot.register_tags = state_.register_tags;
        snapshot.flags = state_.flags;

        // Capture recent trace (up to 81 entries)
        std::size_t trace_start = (state_.trace.size() > 81) ? (state_.trace.size() - 81) : 0;
        for (std::size_t i = trace_start; i < state_.trace.size(); ++i) {
          snapshot.recent_trace.push_back(state_.trace[i]);
        }

        // Improved hash of code segment including operands
        uint64_t h = 0;
        for (const auto& pi : program_.insns) {
          auto combine = [&](uint64_t v) { h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2); };
          combine(static_cast<uint64_t>(pi.opcode));
          combine(static_cast<uint64_t>(pi.a));
          combine(static_cast<uint64_t>(pi.b));
          combine(static_cast<uint64_t>(pi.c));
        }
        snapshot.code_hash = h;

        state_.reflection_snapshots.push_back(std::move(snapshot));
        state_.reflection_count++;

        std::int64_t handle = static_cast<std::int64_t>(state_.reflection_snapshots.size());
        set_reg(insn.a, handle, ValueTag::ReflectionHandle);
        update_flags(handle);
        record_axion_event(insn.opcode, insn.b, handle, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaRefine: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        // RS1 (insn.b) = memory address of commands
        // RS2 (insn.c) = number of commands

        auto verdict = eval_axion_call(t81::axion::reasons::kMetaRefine, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }

        std::int64_t cmd_addr = state_.registers[insn.b];
        std::int64_t cmd_count = state_.registers[insn.c];

        if (cmd_count < 0 || cmd_count > static_cast<int64_t>(kMaxMetaWritesPerEpoch)) {
          trap = Trap::BoundsFault;
          break;
        }

        // Read commands into a temporary list for all-or-nothing atomicity
        std::vector<RefinementCommand> commands;
        bool read_ok = true;
        for (int i = 0; i < cmd_count; ++i) {
          std::size_t base = static_cast<std::size_t>(cmd_addr + i * 4);
          if (!mem_ok(base) || !mem_ok(base + 3)) {
            read_ok = false;
            break;
          }
          RefinementCommand cmd;
          cmd.op = static_cast<RefinementCommand::Op>(state_.memory[base]);
          cmd.target = state_.memory[base + 1];
          cmd.value = state_.memory[base + 2];
          cmd.tag = static_cast<ValueTag>(state_.memory[base + 3]);
          commands.push_back(cmd);
        }

        if (!read_ok) {
          trap = Trap::BoundsFault;
          break;
        }

        // VALIDATION PASS (Atomicity check)
        std::size_t future_meta_write_count = state_.meta_write_count;
        for (const auto& cmd : commands) {
          switch (cmd.op) {
            case RefinementCommand::Op::WriteCode:
              if (future_meta_write_count >= kMaxMetaWritesPerEpoch) {
                trap = Trap::SecurityFault;
                break;
              }
              if (cmd.target < 0 || static_cast<std::size_t>(cmd.target) >= program_.insns.size()) {
                trap = Trap::BoundsFault;
                break;
              }
              future_meta_write_count++;
              break;
            case RefinementCommand::Op::WriteReg:
              if (!reg_ok(static_cast<int>(cmd.target))) {
                trap = Trap::BoundsFault;
                break;
              }
              break;
            case RefinementCommand::Op::WriteMem:
              if (!mem_ok(static_cast<std::size_t>(cmd.target))) {
                trap = Trap::BoundsFault;
                break;
              }
              break;
            case RefinementCommand::Op::Noop:
              break;
          }
          if (trap != Trap::None) {
            break;
          }
        }

        if (trap != Trap::None) {
          break;
        }

        // APPLICATION PASS
        for (const auto& cmd : commands) {
          switch (cmd.op) {
            case RefinementCommand::Op::WriteCode:
              program_.insns[cmd.target].opcode = static_cast<t81::tisc::Opcode>(cmd.value);
              state_.meta_write_count++;
              compiled_traces_.clear();  // Invalidate JIT cache
              break;
            case RefinementCommand::Op::WriteReg:
              state_.registers[cmd.target] = cmd.value;
              state_.register_tags[cmd.target] = cmd.tag;
              break;
            case RefinementCommand::Op::WriteMem:
              state_.memory[cmd.target] = cmd.value;
              state_.memory_tags[cmd.target] = cmd.tag;
              break;
            case RefinementCommand::Op::Noop:
              break;
          }
        }

        set_reg(insn.a, 1, ValueTag::Int);  // Success
        update_flags(1);
        record_axion_event(insn.opcode, insn.b, 1, verdict);
        break;
      }
      case t81::tisc::Opcode::TSqrt: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        std::vector<float> data = tensor->data();
        for (auto& val : data) val = std::sqrt(val);
        state_.registers[insn.a] = alloc_tensor(T729Tensor(tensor->shape(), std::move(data)));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TSiLU: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSiLU kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        state_.registers[insn.a] = alloc_tensor(t81::ops::silu(*tensor));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TSoftmax: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr || tensor->rank() == 0) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSoftmax kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        state_.registers[insn.a] = alloc_tensor(t81::ops::softmax(*tensor));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TRMSNorm: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        auto* w = tensor_ptr(state_.registers[insn.c]);
        if (tensor == nullptr || w == nullptr || tensor->rank() == 0 || w->rank() != 1 ||
            w->shape()[0] != tensor->shape().back()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRMSNorm shape mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRMSNorm kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        state_.registers[insn.a] = alloc_tensor(t81::ops::rmsnorm(*tensor, *w));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TRoPE: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr || tensor->rank() < 2) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRoPE shape mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRoPE kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        int pos = static_cast<int>(state_.registers[insn.c]);
        state_.registers[insn.a] = alloc_tensor(t81::ops::rope(*tensor, pos));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::Store: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!check_mem(insn.opcode, insn.a, "memory store")) {
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.a);
        state_.memory[addr] = state_.registers[insn.b];
        state_.memory_tags[addr] = state_.register_tags[insn.b];
        log_memory_segment_access(insn.opcode, segment_for_address(addr), addr, 1,
                                  t81::axion::reasons::kMemStore);
        break;
      }
      case t81::tisc::Opcode::Mul:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = state_.registers[insn.b] * state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto divisor = state_.registers[insn.c];
        if (divisor == 0) {
          trap = Trap::DivisionFault;
          break;
        }
        auto lhs = state_.registers[insn.b];
        if (insn.opcode == t81::tisc::Opcode::Div) {
          state_.registers[insn.a] = lhs / divisor;
        } else {
          state_.registers[insn.a] = lhs % divisor;
        }
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Jump:
        if (!check_mem(insn.opcode, insn.a, "jump", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.pc = static_cast<std::size_t>(insn.a);
        break;
      case t81::tisc::Opcode::JumpIfZero:
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.registers[insn.b] == 0) {
          if (!check_mem(insn.opcode, insn.a, "jump if zero", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::JumpIfNotZero:
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.registers[insn.b] != 0) {
          if (!check_mem(insn.opcode, insn.a, "jump if not zero", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::Neg:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = -state_.registers[insn.b];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::JumpIfNegative:
        if (state_.flags.negative) {
          if (!check_mem(insn.opcode, insn.a, "jump if negative", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::JumpIfPositive:
        if (state_.flags.positive) {
          if (!check_mem(insn.opcode, insn.a, "jump if positive", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::Less:
      case t81::tisc::Opcode::LessEqual:
      case t81::tisc::Opcode::Greater:
      case t81::tisc::Opcode::GreaterEqual:
      case t81::tisc::Opcode::Equal:
      case t81::tisc::Opcode::NotEqual: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto tag_b = state_.register_tags[insn.b];
        auto tag_c = state_.register_tags[insn.c];
        if (tag_b != tag_c) {
          trap = Trap::TypeFault;
          break;
        }
        auto relation_opt =
            compare_value(tag_b, state_.registers[insn.b], state_.registers[insn.c]);
        if (!relation_opt.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        int relation = relation_opt.value();
        bool result = false;
        switch (insn.opcode) {
          case t81::tisc::Opcode::Less:
            result = relation < 0;
            break;
          case t81::tisc::Opcode::LessEqual:
            result = relation <= 0;
            break;
          case t81::tisc::Opcode::Greater:
            result = relation > 0;
            break;
          case t81::tisc::Opcode::GreaterEqual:
            result = relation >= 0;
            break;
          case t81::tisc::Opcode::Equal:
            result = relation == 0;
            break;
          case t81::tisc::Opcode::NotEqual:
            result = relation != 0;
            break;
          default:
            break;
        }
        state_.registers[insn.a] = result ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Cmp: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto tag_a = state_.register_tags[insn.a];
        auto tag_b = state_.register_tags[insn.b];
        if (tag_a != tag_b) {
          trap = Trap::TypeFault;
          break;
        }
        auto relation_opt =
            compare_value(tag_a, state_.registers[insn.a], state_.registers[insn.b]);
        if (!relation_opt.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        int relation = relation_opt.value();
        state_.flags.zero = (relation == 0);
        state_.flags.negative = (relation < 0);
        state_.flags.positive = (relation > 0);
        break;
      }
      case t81::tisc::Opcode::SetF: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t flag_value = 0;
        if (state_.flags.negative) {
          flag_value = -1;
        } else if (!state_.flags.zero) {
          flag_value = 1;
        }
        set_reg(insn.a, flag_value, ValueTag::Int);
        update_flags(flag_value);
        break;
      }
      case t81::tisc::Opcode::Push: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto addr_opt = push_stack(state_.registers[insn.a], state_.register_tags[insn.a]);
        if (!addr_opt.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack push");
          trap = Trap::StackFault;
          break;
        }
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1,
                                  t81::axion::reasons::kMemStore);
        break;
      }
      case t81::tisc::Opcode::Pop: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        ValueTag tag = ValueTag::Int;
        auto addr_opt = pop_stack(state_.registers[insn.a], tag);
        if (!addr_opt.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack pop");
          trap = Trap::StackFault;
          break;
        }
        state_.register_tags[insn.a] = tag;
        update_flags(state_.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1,
                                  t81::axion::reasons::kMemLoad);
        break;
      }
      case t81::tisc::Opcode::StackAlloc: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& stack = state_.layout.stack;
        if (!stack.valid()) {
          trap = Trap::DecodeFault;
          break;
        }
        auto size = static_cast<std::size_t>(insn.b);
        // Enforce 81-byte block alignment
        if (size % 81 != 0) {
          size = ((size / 81) + 1) * 81;
        }
        std::size_t available = state_.sp - stack.start;
        if (size > available) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        std::size_t new_sp = state_.sp - size;
        if (new_sp < stack.start) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(new_sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        if (state_.policy && state_.policy->max_stack &&
            static_cast<std::int64_t>(stack.limit - new_sp) > *state_.policy->max_stack) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(new_sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        std::int64_t addr = static_cast<std::int64_t>(new_sp);
        state_.stack_frames.emplace_back(addr, static_cast<std::int64_t>(size));
        state_.sp = new_sp;
        set_reg(insn.a, addr, ValueTag::Int);
        update_flags(addr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(addr), size,
                                  t81::axion::reasons::kStackAlloc);
        break;
      }
      case t81::tisc::Opcode::StackFree: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& stack = state_.layout.stack;
        auto size = static_cast<std::size_t>(insn.b);
        // Enforce 81-byte block alignment
        if (size % 81 != 0) {
          size = ((size / 81) + 1) * 81;
        }
        if (!stack.valid()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, 0, "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        if (state_.stack_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        std::int64_t ptr = state_.registers[insn.a];
        if (!stack.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ptr),
                           "stack frame free");
          trap = Trap::DecodeFault;
          break;
        }
        auto [expected_addr, expected_size] = state_.stack_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ptr),
                           "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        state_.stack_frames.pop_back();
        state_.sp = static_cast<std::size_t>(ptr + size);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(ptr), size,
                                  t81::axion::reasons::kStackFree);
        break;
      }
      case t81::tisc::Opcode::HeapAlloc: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        auto size = static_cast<std::size_t>(insn.b);
        // Enforce 81-byte block alignment
        if (size % 81 != 0) {
          size = ((size / 81) + 1) * 81;
        }
        if (size > heap.size()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(heap.limit),
                           "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = state_.heap_ptr;
        if (addr < heap.start || addr + size > heap.limit) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(addr),
                           "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        if (state_.registers[insn.a] != 0) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.heap_frames.emplace_back(static_cast<std::int64_t>(addr),
                                        static_cast<std::int64_t>(size));
        state_.heap_ptr = addr + size;
        set_reg(insn.a, static_cast<std::int64_t>(addr), ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap, addr, size,
                                  t81::axion::reasons::kHeapAlloc);
        break;
      }
      case t81::tisc::Opcode::HeapFree: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, 0, "heap block free");
          trap = Trap::BoundsFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.heap_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(state_.heap_ptr),
                           "heap block free");
          trap = Trap::BoundsFault;
          break;
        }
        auto size = static_cast<std::size_t>(insn.b);
        // Enforce 81-byte block alignment
        if (size % 81 != 0) {
          size = ((size / 81) + 1) * 81;
        }
        std::int64_t ptr = state_.registers[insn.a];
        if (!heap.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(ptr),
                           "heap block free");
          trap = Trap::DecodeFault;
          break;
        }
        auto [expected_addr, expected_size] = state_.heap_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(ptr),
                           "heap block free");
          trap = Trap::DecodeFault;
          break;
        }
        state_.heap_frames.pop_back();
        state_.heap_ptr = static_cast<std::size_t>(ptr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap,
                                  static_cast<std::size_t>(ptr), size,
                                  t81::axion::reasons::kHeapFree);
        break;
      }
      case t81::tisc::Opcode::TNot:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          int t = clamp_trit(state_.registers[insn.b]);
          state_.registers[insn.a] = -t;
          state_.register_tags[insn.a] = ValueTag::Int;
          update_flags(state_.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::TAnd:
      case t81::tisc::Opcode::TOr:
      case t81::tisc::Opcode::TXor:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          int lhs = clamp_trit(state_.registers[insn.b]);
          int rhs = clamp_trit(state_.registers[insn.c]);
          int result = 0;
          if (insn.opcode == t81::tisc::Opcode::TAnd) {
            result = (lhs < rhs) ? lhs : rhs;
          } else if (insn.opcode == t81::tisc::Opcode::TOr) {
            result = (lhs > rhs) ? lhs : rhs;
          } else {
            result = lhs - rhs;
            if (result > 1) {
              result = -1;
            }
            if (result < -1) {
              result = 1;
            }
          }
          state_.registers[insn.a] = result;
          state_.register_tags[insn.a] = ValueTag::Int;
          update_flags(state_.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::AxRead: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto verdict = eval_axion_call(t81::axion::reasons::kAxRead, current_pc, insn.opcode);
        auto guard_addr = static_cast<std::size_t>(insn.b);
        auto guard_kind = segment_for_address(guard_addr);
        apply_segment_reason(verdict, "AxRead guard", guard_kind, guard_addr);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          record_axion_event(insn.opcode, insn.b, 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        state_.registers[insn.a] = insn.b;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        record_axion_event(insn.opcode, insn.b, state_.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::AxSet: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = state_.registers[insn.b];
        auto verdict = eval_axion_call(t81::axion::reasons::kAxSet, current_pc, insn.opcode);
        std::size_t guard_addr = 0;
        MemorySegmentKind guard_kind = MemorySegmentKind::Unknown;
        if (state_.registers[insn.a] >= 0) {
          guard_addr = static_cast<std::size_t>(state_.registers[insn.a]);
          guard_kind = segment_for_address(guard_addr);
        }
        apply_segment_reason(verdict, "AxSet guard", guard_kind, guard_addr);
        record_axion_event(insn.opcode, insn.a, value, verdict);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
        }
        break;
      }
      case t81::tisc::Opcode::AxVerify: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto verdict = eval_axion_call(t81::axion::reasons::kAxVerify, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          record_axion_event(insn.opcode, insn.b, 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        state_.registers[insn.a] = (verdict.kind == t81::axion::VerdictKind::Defer) ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        record_axion_event(insn.opcode, insn.b, state_.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::Call: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.call_depth >= kHardRecursionCeiling) {
          ++state_.contradiction_events;
          t81::axion::Verdict recursion_verdict;
          recursion_verdict.kind = t81::axion::VerdictKind::Deny;
          std::ostringstream reason;
          reason << t81::axion::reasons::kRecursionCeiling << " depth=" << state_.call_depth
                 << " limit=" << kHardRecursionCeiling;
          recursion_verdict.reason = reason.str();
          record_axion_event(insn.opcode, insn.b, static_cast<std::int64_t>(state_.call_depth),
                             recursion_verdict);
          trap = Trap::SecurityFault;
          break;
        }
        auto target = state_.registers[insn.b];
        if (!check_mem(insn.opcode, static_cast<int>(target), "call", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!push_stack(static_cast<std::int64_t>(state_.pc), ValueTag::Int)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack call");
          trap = Trap::StackFault;
          break;
        }
        ++state_.call_depth;
        state_.pc = static_cast<std::size_t>(target);
        break;
      }
      case t81::tisc::Opcode::Ret: {
        std::int64_t addr = 0;
        ValueTag tag = ValueTag::Int;
        if (!pop_stack(addr, tag)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(state_.sp),
                           "stack return");
          trap = Trap::StackFault;
          break;
        }
        if (tag != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        if (!check_mem(insn.opcode, static_cast<int>(addr), "return", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.call_depth > 0) {
          --state_.call_depth;
        } else {
          ++state_.contradiction_events;
          t81::axion::Verdict contradiction_verdict;
          contradiction_verdict.kind = t81::axion::VerdictKind::Allow;
          contradiction_verdict.reason =
              std::string(t81::axion::reasons::kContradictionDetected) + " return-without-call";
          record_axion_event(insn.opcode, insn.a, addr, contradiction_verdict);
        }
        state_.pc = static_cast<std::size_t>(addr);
        break;
      }
      case t81::tisc::Opcode::Trap:
        trap = Trap::TrapInstruction;
        break;
      case t81::tisc::Opcode::Print: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto rendered = format_value(state_.register_tags[insn.a], state_.registers[insn.a], 0);
        if (!rendered.has_value()) {
          trap = Trap::TypeFault;
          break;
        }
        state_.printed_output.push_back(*rendered);
        break;
      }
      case t81::tisc::Opcode::StrLen: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto symbol = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        if (!symbol.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = static_cast<std::int64_t>(symbol->size());
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrEmpty: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto symbol = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        if (!symbol.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = symbol->empty() ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Bool;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecLen: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t length = 0;
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          length = static_cast<std::int64_t>(values->size());
        } else if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          length = static_cast<std::int64_t>(tensor->shape().front());
        } else {
          trap = Trap::TypeFault;
          break;
        }
        state_.registers[insn.a] = length;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecEmpty: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        bool is_empty = false;
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          is_empty = values->empty();
        } else if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          is_empty = tensor->shape().front() == 0;
        } else {
          trap = Trap::TypeFault;
          break;
        }
        state_.registers[insn.a] = is_empty ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Bool;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecFirst: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          state_.registers[insn.a] = intern_symbol(values->front());
          state_.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          const auto& data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          state_.registers[insn.a] = static_cast<std::int64_t>(data.front());
          state_.register_tags[insn.a] = ValueTag::Int;
          update_flags(state_.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecLast: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          state_.registers[insn.a] = intern_symbol(values->back());
          state_.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          const auto& data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          state_.registers[insn.a] = static_cast<std::int64_t>(data.back());
          state_.register_tags[insn.a] = ValueTag::Int;
          update_flags(state_.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecPush: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          if (state_.register_tags[insn.c] != ValueTag::SymbolHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          auto* value = symbol_ptr(state_.registers[insn.c]);
          if (values == nullptr || value == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          std::vector<std::string> pushed = *values;
          pushed.push_back(*value);
          state_.string_vectors.push_back(std::move(pushed));
          state_.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          state_.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          if (state_.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          const auto old_len = tensor->shape().front();
          if (old_len < 0) {
            trap = Trap::TypeFault;
            break;
          }
          auto data = tensor->data();
          data.push_back(static_cast<float>(state_.registers[insn.c]));
          state_.registers[insn.a] = alloc_tensor(T729Tensor({old_len + 1}, std::move(data)));
          state_.register_tags[insn.a] = ValueTag::TensorHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecPop: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(state_.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          std::vector<std::string> popped = *values;
          popped.pop_back();
          state_.string_vectors.push_back(std::move(popped));
          state_.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          state_.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        if (state_.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(state_.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          auto data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          data.pop_back();
          state_.registers[insn.a] =
              alloc_tensor(T729Tensor({tensor->shape().front() - 1}, std::move(data)));
          state_.register_tags[insn.a] = ValueTag::TensorHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::StrConcat: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto lhs = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto rhs = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!lhs.has_value() || !rhs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string combined(*lhs);
        combined += std::string(*rhs);
        state_.registers[insn.a] = intern_symbol(std::move(combined));
        state_.register_tags[insn.a] = ValueTag::SymbolHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrStartsWith: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto prefix = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!value.has_value() || !prefix.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool match =
            value->size() >= prefix->size() && value->compare(0, prefix->size(), *prefix) == 0;
        state_.registers[insn.a] = match ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Bool;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrEndsWith: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto suffix = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!value.has_value() || !suffix.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool match =
            value->size() >= suffix->size() &&
            value->compare(value->size() - suffix->size(), suffix->size(), *suffix) == 0;
        state_.registers[insn.a] = match ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Bool;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrContains: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto needle = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!value.has_value() || !needle.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool contains = value->find(*needle) != std::string::npos;
        state_.registers[insn.a] = contains ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Bool;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrIndexOf: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto needle = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!value.has_value() || !needle.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::size_t pos = value->find(*needle);
        state_.registers[insn.a] = pos == std::string::npos ? -1 : static_cast<std::int64_t>(pos);
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrReplace: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto source = symbol_like_text(state_.register_tags[insn.a], state_.registers[insn.a]);
        auto needle = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto replacement = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!source.has_value() || !needle.has_value() || !replacement.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (needle->empty()) {
          state_.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(state_.registers[insn.a]);
          break;
        }

        std::string replaced;
        replaced.reserve(source->size());
        std::size_t search_from = 0;
        while (true) {
          std::size_t pos = source->find(*needle, search_from);
          if (pos == std::string::npos) {
            replaced.append(*source, search_from, std::string::npos);
            break;
          }
          replaced.append(*source, search_from, pos - search_from);
          replaced.append(*replacement);
          search_from = pos + needle->size();
        }

        state_.registers[insn.a] = intern_symbol(std::move(replaced));
        state_.register_tags[insn.a] = ValueTag::SymbolHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrVecNew: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = alloc_string_vector();
        state_.register_tags[insn.a] = ValueTag::StringVectorHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrVecPush: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.a] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* values = string_vector_mut(state_.registers[insn.a]);
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        if (values == nullptr || !value.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        values->push_back(std::string(*value));
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrSplit: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(state_.register_tags[insn.b], state_.registers[insn.b]);
        auto sep = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (!value.has_value() || !sep.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (sep->empty()) {
          trap = Trap::TypeFault;
          break;
        }
        std::vector<std::string> parts;
        std::size_t start = 0;
        while (true) {
          std::size_t pos = value->find(*sep, start);
          if (pos == std::string::npos) {
            parts.push_back(std::string(value->substr(start)));
            break;
          }
          parts.push_back(std::string(value->substr(start, pos - start)));
          start = pos + sep->size();
        }
        state_.string_vectors.push_back(std::move(parts));
        state_.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
        state_.register_tags[insn.a] = ValueTag::StringVectorHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << t81::axion::reasons::kStringSplit << " input_len=" << value->size()
                 << " sep_len=" << sep->size() << " parts=" << state_.string_vectors.back().size();
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(state_.string_vectors.size()),
                             static_cast<std::int64_t>(state_.string_vectors.back().size()),
                             verdict);
        }
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrJoin: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* parts = string_vector_ptr(state_.registers[insn.b]);
        auto sep = symbol_like_text(state_.register_tags[insn.c], state_.registers[insn.c]);
        if (parts == nullptr || !sep.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string joined;
        if (!parts->empty()) {
          joined = parts->front();
          for (std::size_t i = 1; i < parts->size(); ++i) {
            joined += *sep;
            joined += parts->at(i);
          }
        }
        state_.registers[insn.a] = intern_symbol(std::move(joined));
        state_.register_tags[insn.a] = ValueTag::SymbolHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << t81::axion::reasons::kStringJoin << " parts=" << parts->size()
                 << " sep_len=" << sep->size();
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(parts->size()),
                             state_.registers[insn.a], verdict);
        }
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::I2F: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = static_cast<double>(state_.registers[insn.b]);
        state_.registers[insn.a] = alloc_float(value);
        state_.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::F2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = float_ptr(state_.registers[insn.b]);
        if (!ptr_val) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = static_cast<std::int64_t>(*ptr_val);
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::I2Frac: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto frac = t81::T81Fraction::from_int(state_.registers[insn.b]);
        state_.registers[insn.a] = alloc_fraction(std::move(frac));
        state_.register_tags[insn.a] = ValueTag::FractionHandle;
        break;
      }
      case t81::tisc::Opcode::Frac2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::FractionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = fraction_ptr(state_.registers[insn.b]);
        if (!ptr_val || !t81::T81BigInt::is_one(ptr_val->den)) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.registers[insn.a] = ptr_val->num.to_int64();
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::FAdd:
      case t81::tisc::Opcode::FSub:
      case t81::tisc::Opcode::FMul:
      case t81::tisc::Opcode::FDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::FloatHandle ||
            state_.register_tags[insn.c] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* lhs = float_ptr(state_.registers[insn.b]);
        auto* rhs = float_ptr(state_.registers[insn.c]);
        if (lhs == nullptr || rhs == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        double result = 0.0;
        switch (insn.opcode) {
          case t81::tisc::Opcode::FAdd:
            result = *lhs + *rhs;
            break;
          case t81::tisc::Opcode::FSub:
            result = *lhs - *rhs;
            break;
          case t81::tisc::Opcode::FMul:
            result = *lhs * *rhs;
            break;
          case t81::tisc::Opcode::FDiv:
            if (*rhs == 0.0) {
              trap = Trap::DivisionFault;
              break;
            }
            result = *lhs / *rhs;
            break;
          default:
            break;
        }
        if (trap != Trap::None) {
          break;
        }
        state_.registers[insn.a] = alloc_float(result);
        state_.register_tags[insn.a] = ValueTag::FloatHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::FSin:
      case t81::tisc::Opcode::FCos:
      case t81::tisc::Opcode::FTan:
      case t81::tisc::Opcode::FAsin:
      case t81::tisc::Opcode::FAcos:
      case t81::tisc::Opcode::FAtan:
      case t81::tisc::Opcode::FSinh:
      case t81::tisc::Opcode::FCosh:
      case t81::tisc::Opcode::FTanh:
      case t81::tisc::Opcode::FSqrt:
      case t81::tisc::Opcode::FExp:
      case t81::tisc::Opcode::FLog:
      case t81::tisc::Opcode::FPow: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
            (insn.opcode == t81::tisc::Opcode::FPow && !reg_ok(insn.c))) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        if (insn.opcode == t81::tisc::Opcode::FPow &&
            state_.register_tags[insn.c] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = float_ptr(state_.registers[insn.b]);
        if (!ptr_val) {
          trap = Trap::DecodeFault;
          break;
        }
        double result = 0.0;
        if (insn.opcode == t81::tisc::Opcode::FSin) {
          result = std::sin(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FCos) {
          result = std::cos(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FTan) {
          result = std::tan(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FAsin) {
          result = std::asin(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FAcos) {
          result = std::acos(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FAtan) {
          result = std::atan(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FSinh) {
          result = std::sinh(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FCosh) {
          result = std::cosh(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FTanh) {
          result = std::tanh(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FSqrt) {
          result = std::sqrt(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FExp) {
          result = std::exp(*ptr_val);
        } else if (insn.opcode == t81::tisc::Opcode::FLog) {
          result = std::log(*ptr_val);
        } else {
          auto* exponent = float_ptr(state_.registers[insn.c]);
          if (!exponent) {
            trap = Trap::DecodeFault;
            break;
          }
          result = std::pow(*ptr_val, *exponent);
        }
        state_.registers[insn.a] = alloc_float(result);
        state_.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::FracAdd:
      case t81::tisc::Opcode::FracSub:
      case t81::tisc::Opcode::FracMul:
      case t81::tisc::Opcode::FracDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::FractionHandle ||
            state_.register_tags[insn.c] != ValueTag::FractionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* lhs = fraction_ptr(state_.registers[insn.b]);
        auto* rhs = fraction_ptr(state_.registers[insn.c]);
        if (lhs == nullptr || rhs == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        try {
          t81::T81Fraction result;
          switch (insn.opcode) {
            case t81::tisc::Opcode::FracAdd:
              result = t81::T81Fraction::add(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracSub:
              result = t81::T81Fraction::sub(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracMul:
              result = t81::T81Fraction::mul(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracDiv:
              if (t81::T81BigInt::is_zero(rhs->num)) {
                trap = Trap::DivisionFault;
                break;
              }
              result = t81::T81Fraction::div(*lhs, *rhs);
              break;
            default:
              break;
          }
          if (trap != Trap::None) {
            break;
          }
          state_.registers[insn.a] = alloc_fraction(std::move(result));
          state_.register_tags[insn.a] = ValueTag::FractionHandle;
          update_flags(state_.registers[insn.a]);
        } catch (...) {
          trap = Trap::DecodeFault;
        }
        break;
      }
      case t81::tisc::Opcode::ChkShape: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::TensorHandle ||
            state_.register_tags[insn.c] != ValueTag::ShapeHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        const auto* expected = shape_ptr(state_.registers[insn.c]);
        if (tensor == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        if (expected == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        bool match = tensor->shape() == *expected;
        set_reg(insn.a, match ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeOptionSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_option(true, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeOptionNone: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_option(false, ValueTag::Int, 0);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_result(true, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_result(false, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariant: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariantPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.c < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_enum(static_cast<int>(insn.c), true, state_.register_tags[insn.b],
                                  state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeComplex: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::Int ||
            state_.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        auto handle = intern_complex(state_.registers[insn.b], state_.registers[insn.c]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::ComplexHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionIsSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* opt = option_ptr(state_.registers[insn.b]);
        if (opt == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, opt->has_value ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionUnwrap: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* opt = option_ptr(state_.registers[insn.b]);
        if (opt == nullptr || !opt->has_value) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, opt->payload, opt->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultIsOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(state_.registers[insn.b]);
        if (res == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->is_ok ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(state_.registers[insn.b]);
        if (res == nullptr || !res->is_ok) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(state_.registers[insn.b]);
        if (res == nullptr || res->is_ok) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::EnumIsVariant: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* val = enum_ptr(state_.registers[insn.b]);
        if (val == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        bool matches = (val->variant_id == insn.c);
        set_reg(insn.a, matches ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          const int guard_variant_id = static_cast<int>(insn.c);  // explicit cast
          const int guard_enum_id = t81::enum_meta::decode_enum_id(guard_variant_id);
          const int guard_local_variant = t81::enum_meta::decode_variant_id(guard_variant_id);
          const auto* meta = enum_metadata_for(guard_enum_id);
          const auto* variant_meta = variant_metadata(meta, guard_local_variant);
          reason << "enum guard";
          if (meta != nullptr) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta != nullptr) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          reason << " match=" << (matches ? "pass" : "fail");
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.c), matches ? 1 : 0,
                             verdict);
        }
        break;
      }
      case t81::tisc::Opcode::EnumUnwrapPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* val = enum_ptr(state_.registers[insn.b]);
        if (val == nullptr || !val->has_payload) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, val->payload, val->payload_tag);
        update_flags(state_.registers[insn.a]);
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          const int global_variant_id = val->variant_id;
          const int enum_id = t81::enum_meta::decode_enum_id(global_variant_id);
          const int local_variant = t81::enum_meta::decode_variant_id(global_variant_id);
          const auto* meta = enum_metadata_for(enum_id);
          const auto* variant_meta = variant_metadata(meta, local_variant);
          reason << "enum payload";
          if (meta != nullptr) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta != nullptr) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(global_variant_id),
                             val->payload, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::TVecAdd:
      case t81::tisc::Opcode::TVecMul: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_a = tensor_ptr(state_.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(state_.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        if (tensor_a->data().size() != tensor_b->data().size()) {
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    insn.opcode == t81::tisc::Opcode::TVecAdd
                                        ? "TVecAdd kernel execution"
                                        : "TVecMul kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), state_.registers[insn.b],
                           verdict);

        std::vector<float> data(tensor_a->data().size());
        if (insn.opcode == t81::tisc::Opcode::TVecAdd) {
          for (std::size_t i = 0; i < data.size(); ++i) {
            data[i] = tensor_a->data()[i] + tensor_b->data()[i];
          }
        } else {
          for (std::size_t i = 0; i < data.size(); ++i) {
            data[i] = tensor_a->data()[i] * tensor_b->data()[i];
          }
        }
        state_.registers[insn.a] =
            alloc_tensor(t81::T729Tensor(tensor_a->shape(), std::move(data)));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TTranspose: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr || tensor->rank() != 2) {
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TTranspose kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        state_.registers[insn.a] = alloc_tensor(tensor->transpose2d());
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TMatMul: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_a = tensor_ptr(state_.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(state_.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        if (tensor_a->rank() != 2 || tensor_b->rank() != 2) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TMatMul rank mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        int k_dim = tensor_a->shape()[1];
        if (tensor_b->shape()[0] != k_dim) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0,
                           "TMatMul inner dimension mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TMatMul kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        t81::T729Tensor result = t81::ops::matmul(*tensor_a, *tensor_b);
        state_.registers[insn.a] = alloc_tensor(std::move(result));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TTenDot: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::TensorHandle ||
            state_.register_tags[insn.c] != ValueTag::TensorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor_a = tensor_ptr(state_.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(state_.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        try {
          auto result = t81::T729Tensor::contract_dot(*tensor_a, *tensor_b);
          state_.registers[insn.a] = alloc_tensor(std::move(result));
          state_.register_tags[insn.a] = ValueTag::TensorHandle;
        } catch (...) {
          trap = Trap::ShapeFault;
        }
        break;
      }
      case t81::tisc::Opcode::TGet: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }

        auto* tensor = tensor_ptr(state_.registers[insn.b]);
        if (tensor == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }

        if (state_.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t index = state_.registers[insn.c];
        if (index < 0 || static_cast<std::size_t>(index) >= tensor->data().size()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(index),
                           "tensor index out of bounds");
          trap = Trap::BoundsFault;
          break;
        }

        float val = tensor->data()[static_cast<std::size_t>(index)];

        state_.registers[insn.a] = alloc_float(static_cast<double>(val));
        state_.register_tags[insn.a] = ValueTag::FloatHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TGet kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), state_.registers[insn.b],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::TNew: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.b] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t size = state_.registers[insn.b];
        if (size <= 0) {
          trap = Trap::BoundsFault;
          break;
        }

        std::vector<int> shape = {static_cast<int>(size)};
        t81::T729Tensor t(shape);
        state_.registers[insn.a] = alloc_tensor(std::move(t));
        state_.register_tags[insn.a] = ValueTag::TensorHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TNew"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(size), state_.registers[insn.a],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::TSet: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.register_tags[insn.a] != ValueTag::TensorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor = tensor_ptr(state_.registers[insn.a]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }

        if (state_.register_tags[insn.b] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t idx = state_.registers[insn.b];
        if (idx < 0 || static_cast<size_t>(idx) >= tensor->data().size()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(idx),
                           "TSet OOB");
          trap = Trap::BoundsFault;
          break;
        }

        float val = 0.0F;
        auto val_tag = state_.register_tags[insn.c];
        if (val_tag == ValueTag::FloatHandle) {
          auto* ptr_val = float_ptr(state_.registers[insn.c]);
          if (ptr_val) val = static_cast<float>(*ptr_val);
        } else if (val_tag == ValueTag::Int) {
          val = static_cast<float>(state_.registers[insn.c]);
        } else {
          trap = Trap::TypeFault;
          break;
        }

        tensor->data()[static_cast<size_t>(idx)] = val;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSet"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
        break;
      }
      default:
        trap = Trap::DecodeFault;
        break;
    }
    ++instructions_since_gc_;
    if (instructions_since_gc_ >= kGcInterval) {
      run_gc_cycle_("interval");
    }

    sync_system_registers();
    log_trace(insn.opcode, trap);
    if (trap != Trap::None) {
      return t81::unexpected(trap);
    }
    return {};
  }

  std::expected<void, Trap> run_to_halt(std::size_t max_steps) override {
    for (std::size_t i = 0; i < max_steps && !state_.halted; ++i) {
      auto result = step();
      if (!result.has_value()) {
        return result;
      }
    }
    return {};
  }

  const State& state() const override { return state_; }

  void set_register(int idx, std::int64_t val_data, ValueTag tag) override {
    if (idx < 0 || static_cast<std::size_t>(idx) >= state_.registers.size()) {
      return;
    }
    if (idx == 0 || (idx >= 75 && idx <= 80)) {
      return;
    }
    state_.registers[idx] = val_data;
    state_.register_tags[idx] = tag;

    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << "register mutation R" << idx << " value=" << val_data;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Nop, idx, val_data, verdict);
  }

private:
  void sync_system_registers() {
    state_.registers[0] = 0;
    state_.register_tags[0] = ValueTag::Int;

    // R75: Global Tick
    state_.registers[75] = static_cast<std::int64_t>(instruction_count_);
    state_.register_tags[75] = ValueTag::Int;

    // R76: Lineage Root Hash (Stub: using a fixed value for now)
    state_.registers[76] = 0xDE7A81;
    state_.register_tags[76] = ValueTag::Int;

    // R77: Current Entropy Signature (Stub)
    state_.registers[77] = static_cast<std::int64_t>(state_.contradiction_events);
    state_.register_tags[77] = ValueTag::Int;

    // R78: Active Constitutional Mask (Stub: Θ₁-Θ₉ enabled)
    state_.registers[78] = 0x1FF;
    state_.register_tags[78] = ValueTag::Int;

    // R79: Recursion Depth Counter
    state_.registers[79] =
        static_cast<std::int64_t>(std::max(state_.stack_frames.size(), state_.call_depth));
    state_.register_tags[79] = ValueTag::Int;

    // R80: Axion Seal / Capability Word
    state_.registers[80] = state_.halted ? 0 : 1;
    state_.register_tags[80] = ValueTag::Int;
  }

  std::int64_t intern_weights_tensor(std::string_view name) {
    if (name.empty() || !state_.weights_model) {
      return 0;
    }
    auto key = std::string(name);
    auto iter = state_.weights_tensor_handles.find(key);
    if (iter != state_.weights_tensor_handles.end()) {
      return iter->second;
    }
    auto native_iter = state_.weights_model->native.find(key);
    if (native_iter == state_.weights_model->native.end()) {
      return 0;
    }
    state_.weights_tensor_refs.push_back(&native_iter->second);
    auto handle = static_cast<std::int64_t>(state_.weights_tensor_refs.size());
    state_.weights_tensor_handles.emplace(std::move(key), handle);
    return handle;
  }

  t81::axion::Verdict eval_axion_call(std::string_view syscall, std::size_t prog_counter,
                                      t81::tisc::Opcode opcode) {
    if (syscall == t81::axion::reasons::kMetaRead) {
      // Internal MetaRead check could go here
    }
    t81::axion::SyscallContext ctx;
    ctx.caller = "t81vm";
    ctx.syscall.assign(syscall);
    ctx.pc = prog_counter;
    ctx.next_opcode = opcode;
    ctx.instruction_count = instruction_count_;
    ctx.recursion_depth = std::max(state_.stack_frames.size(), state_.call_depth);
    ctx.stack_usage = state_.layout.stack.limit - state_.sp;
    ctx.reflection_count = state_.reflection_count;
    ctx.meta_write_count = state_.meta_write_count;
    ctx.policy = state_.policy ? &*state_.policy : nullptr;
    ctx.trace_reasons.reserve(state_.axion_log.size());
    for (const auto& entry : state_.axion_log) {
      ctx.trace_reasons.push_back(entry.verdict.reason);
    }
    return axion_engine_->evaluate(ctx);
  }

  const t81::tisc::EnumMetadata* enum_metadata_for(int enum_id) const {
    auto iter = state_.enum_metadata_index.find(enum_id);
    if (iter == state_.enum_metadata_index.end()) {
      return nullptr;
    }
    return &state_.enum_metadata[iter->second];
  }

  static const t81::tisc::EnumVariantMetadata* variant_metadata(const t81::tisc::EnumMetadata* meta,
                                                                int variant_id) {
    if (meta == nullptr) {
      return nullptr;
    }
    for (const auto& variant : meta->variants) {
      if (variant.variant_id == variant_id) {
        return &variant;
      }
    }
    return nullptr;
  }

  MemorySegmentKind segment_for_address(std::size_t addr) const {
    const auto& layout = state_.layout;
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

  void log_memory_segment_access(t81::tisc::Opcode opcode, MemorySegmentKind kind, std::size_t addr,
                                 std::size_t size, std::string_view action) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason;
    // Format: '[action] [segment] addr=[address] size=[size]'
    reason << action << " " << to_string(kind) << " addr=" << addr;
    if (kind == MemorySegmentKind::Stack || action.find("allocated") != std::string_view::npos ||
        action.find("freed") != std::string_view::npos) {
      reason << " size=" << size;
    } else if (size > 1) {
      reason << " size=" << size;
    }
    verdict.reason = reason.str();
    record_axion_event(opcode, static_cast<std::int32_t>(kind), static_cast<std::int64_t>(addr),
                       verdict);
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, MemorySegmentKind kind, int addr,
                        std::string_view action) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason;
    reason << t81::axion::reasons::kBoundsFault << " segment=" << to_string(kind)
           << " addr=" << addr << " action=" << action;
    verdict.reason = reason.str();
    record_axion_event(opcode, static_cast<std::int32_t>(kind), static_cast<std::int64_t>(addr),
                       verdict);
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, int addr, std::string_view action) {
    MemorySegmentKind kind = MemorySegmentKind::Unknown;
    if (addr >= 0) {
      kind = segment_for_address(static_cast<std::size_t>(addr));
    }
    log_bounds_fault(opcode, kind, addr, action);
  }

  void push_axion_event(const AxionEvent& event) {
    std::cerr << "[VM] push_axion_event: opcode=" << static_cast<int>(event.opcode) << " reason=\""
              << event.verdict.reason << "\"\n";
    state_.axion_log.push_back(event);
  }

  void log_meta_slot(const char* label) {
    if (!state_.layout.meta.contains(state_.meta_ptr)) {
      return;
    }
    AxionEvent meta_event;
    meta_event.opcode = t81::tisc::Opcode::Nop;
    meta_event.tag = static_cast<std::int32_t>(MemorySegmentKind::Meta);
    meta_event.value = static_cast<std::int64_t>(state_.meta_ptr);
    meta_event.verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << "meta slot " << label << " segment=" << to_string(MemorySegmentKind::Meta)
                  << " addr=" << state_.meta_ptr;
    meta_event.verdict.reason = reason_stream.str();
    push_axion_event(meta_event);
    ++state_.meta_ptr;
  }

  static void apply_segment_reason(t81::axion::Verdict& verdict, const char* action,
                                   MemorySegmentKind kind, std::size_t addr) {
    std::ostringstream reason_stream;
    reason_stream << action << " segment=" << to_string(kind) << " addr=" << addr;
    if (!verdict.reason.empty()) {
      reason_stream << " " << verdict.reason;
    }
    verdict.reason = reason_stream.str();
  }

  void record_axion_event(t81::tisc::Opcode opcode, std::int32_t tag_val, std::int64_t val_data,
                          const t81::axion::Verdict& verdict) {
    log_meta_slot(t81::axion::reasons::kMetaSlotAxionEvent.data());
    AxionEvent event;
    event.opcode = opcode;
    event.tag = tag_val;
    event.value = val_data;
    event.verdict = verdict;
    event.structured.reason = verdict.reason;
    event.structured.pc = state_.pc;
    event.structured.handle_id = val_data;  // often used for handles
    event.structured.decision = (verdict.kind == t81::axion::VerdictKind::Allow) ? "allow" : "deny";
    push_axion_event(event);
  }

  void run_gc_cycle_(const char* reason) {
    instructions_since_gc_ = 0;
    state_.gc_cycles++;
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    // Format: 'GC cycle reason=[reason]'
    reason_stream << t81::axion::reasons::kGcCycle << " reason=" << reason;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(state_.gc_cycles),
                       static_cast<std::int64_t>(state_.gc_cycles), verdict);

    log_heap_compaction(state_.heap_ptr, state_.heap_frames.size());
    log_heap_relocation(state_.heap_ptr, state_.layout.heap.start, state_.heap_frames.size());
    compact_heap(state_.layout.heap.start);
  }

  void log_heap_compaction(std::size_t heap_ptr, std::size_t heap_frames) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << t81::axion::reasons::kHeapCompaction << " heap_frames=" << heap_frames
                  << " heap_ptr=" << heap_ptr;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(MemorySegmentKind::Heap),
                       static_cast<std::int64_t>(heap_ptr), verdict);
  }

  void log_heap_relocation(std::size_t addr_from, std::size_t addr_to, std::size_t size) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << t81::axion::reasons::kHeapRelocation << " from=" << addr_from
                  << " to=" << addr_to << " size=" << size;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(MemorySegmentKind::Heap),
                       static_cast<std::int64_t>(addr_to), verdict);
  }

  void compact_heap(std::size_t new_ptr) {
    for (auto& frame : state_.heap_frames) {
      frame.first = static_cast<std::int64_t>(new_ptr);
    }
    state_.heap_frames.clear();
    state_.heap_ptr = new_ptr;
  }

  State state_{};
  t81::tisc::Program program_{};
  std::unique_ptr<t81::axion::Engine> axion_engine_;
  static constexpr std::size_t kGcInterval = 64;
  std::size_t instructions_since_gc_{0};
  std::size_t instruction_count_{0};

  // JIT components
  JitCompiler jit_compiler_;
  std::unordered_map<std::size_t, std::size_t> hot_spots_;
  std::unordered_map<std::size_t, std::unique_ptr<JitTrace>> compiled_traces_;
  static constexpr std::size_t kHotSpotThreshold = 50;
};
}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm(std::unique_ptr<t81::axion::Engine> engine) {
  return std::make_unique<Interpreter>(std::move(engine));
}
}  // namespace t81::vm
