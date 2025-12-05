#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>

#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include <string>
#include <string_view>
#include <utility>

#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/enum_meta.hpp"
#include "t81/vm/vm.hpp"

namespace t81::vm {
namespace {
constexpr std::size_t kDefaultStackSize = 256;
constexpr std::size_t kDefaultHeapSize = 768;
constexpr std::size_t kDefaultTensorSpace = 256;
constexpr std::size_t kDefaultMetaSpace = 256;

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
    state_.tensors = program_.tensor_pool;
    state_.shapes = program_.shape_pool;
    state_.weights_model = program_.weights_model;
    state_.weights_tensor_refs.clear();
    state_.weights_tensor_handles.clear();
    state_.stack_frames.clear();
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
    state_.policy.reset();
    state_.gc_cycles = 0;
    instructions_since_gc_ = 0;
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
          reason << "loop hint file=" << loop.file
                 << " line=" << loop.line
                 << " column=" << loop.column
                 << " bound=";
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
    if (state_.pc >= program_.insns.size()) {
      auto verdict = eval_axion_call("step", state_.pc, t81::tisc::Opcode::Halt);
      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        return Trap::SecurityFault;
      }
      state_.halted = true;
      return {};
    }

    const std::size_t current_pc = state_.pc++;
    const auto& insn = program_.insns[current_pc];

    // Evaluate Axion policy before every instruction.
    auto verdict = eval_axion_call("step", current_pc, insn.opcode);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
        return Trap::SecurityFault;
    }

    auto reg_ok = [this](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < state_.registers.size();
    };
    auto mem_ok = [this](int addr, bool code = false) {
      if (addr < 0) return false;
      std::size_t a = static_cast<std::size_t>(addr);
      const auto& layout = state_.layout;
      if (code) {
        return layout.code.contains(a);
      }
      if (a >= state_.memory.size()) return false;
      return layout.stack.contains(a) || layout.heap.contains(a) ||
             layout.tensor.contains(a) || layout.meta.contains(a);
    };
    auto log_trace = [this, current_pc](t81::tisc::Opcode op, Trap trap = Trap::None) {
      TraceEntry t{current_pc, op, std::nullopt};
      if (trap != Trap::None) t.trap = trap;
      state_.trace.push_back(t);
    };
    auto literal_kind_to_tag = [](t81::tisc::LiteralKind kind) -> ValueTag {
      switch (kind) {
        case t81::tisc::LiteralKind::FloatHandle: return ValueTag::FloatHandle;
        case t81::tisc::LiteralKind::FractionHandle: return ValueTag::FractionHandle;
        case t81::tisc::LiteralKind::SymbolHandle: return ValueTag::SymbolHandle;
        case t81::tisc::LiteralKind::TensorHandle: return ValueTag::TensorHandle;
        case t81::tisc::LiteralKind::ShapeHandle: return ValueTag::ShapeHandle;
        case t81::tisc::LiteralKind::Int:
        default: return ValueTag::Int;
      }
    };
    auto set_reg = [this](int reg, std::int64_t value, ValueTag tag) {
      state_.registers[reg] = value;
      state_.register_tags[reg] = tag;
    };
    auto copy_reg = [this](int dst, int src) {
      state_.registers[dst] = state_.registers[src];
      state_.register_tags[dst] = state_.register_tags[src];
    };
    auto update_flags = [this](std::int64_t v) {
      state_.flags.zero = (v == 0);
      state_.flags.negative = (v < 0);
      state_.flags.positive = (v > 0);
    };
    auto push_stack = [this](std::int64_t value, ValueTag tag) -> std::optional<std::size_t> {
      const auto& stack = state_.layout.stack;
      if (!stack.valid()) return false;
      if (state_.sp <= stack.start) return false;
      --state_.sp;
      if (!stack.contains(static_cast<std::size_t>(state_.sp))) {
        ++state_.sp;
        return false;
      }
      state_.memory[state_.sp] = value;
      state_.memory_tags[state_.sp] = tag;
      return static_cast<std::size_t>(state_.sp);
    };
    auto pop_stack = [this](std::int64_t& value, ValueTag& tag) -> std::optional<std::size_t> {
      const auto& stack = state_.layout.stack;
      if (!stack.valid()) return false;
      if (state_.sp >= stack.limit) return false;
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
    auto alloc_tensor = [this](t81::T729Tensor tensor) -> std::int64_t {
      state_.tensors.push_back(std::move(tensor));
      auto idx = state_.tensors.size();
      log_memory_segment_access(t81::tisc::Opcode::Nop, MemorySegmentKind::Tensor, idx, 1,
                                "tensor slot allocated");
      return static_cast<std::int64_t>(idx);
    };
    auto float_ptr = [this](std::int64_t handle) -> double* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.floats.size()) return nullptr;
      return &state_.floats[idx];
    };
    auto alloc_float = [this](double value) -> std::int64_t {
      state_.floats.push_back(value);
      return static_cast<std::int64_t>(state_.floats.size());
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
    auto alloc_fraction = [this](t81::T81Fraction frac) -> std::int64_t {
      state_.fractions.push_back(std::move(frac));
      return static_cast<std::int64_t>(state_.fractions.size());
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
    auto clamp_trit = [](std::int64_t v) -> int {
      if (v > 0) return 1;
      if (v < 0) return -1;
      return 0;
    };

    std::function<std::optional<int>(ValueTag, std::int64_t, std::int64_t)> compare_value =
        [&](ValueTag tag, std::int64_t lhs_val, std::int64_t rhs_val) -> std::optional<int> {
          switch (tag) {
            case ValueTag::Int:
              if (lhs_val == rhs_val) return 0;
              return (lhs_val < rhs_val) ? -1 : 1;
            case ValueTag::FloatHandle: {
              auto lhs = float_ptr(lhs_val);
              auto rhs = float_ptr(rhs_val);
              if (!lhs || !rhs) return std::nullopt;
              if (*lhs == *rhs) return 0;
              return (*lhs < *rhs) ? -1 : 1;
            }
            case ValueTag::FractionHandle: {
              auto lhs = fraction_ptr(lhs_val);
              auto rhs = fraction_ptr(rhs_val);
              if (!lhs || !rhs) return std::nullopt;
              return t81::T81Fraction::cmp(*lhs, *rhs);
            }
            case ValueTag::SymbolHandle: {
              auto lhs = symbol_ptr(lhs_val);
              auto rhs = symbol_ptr(rhs_val);
              if (!lhs || !rhs) return std::nullopt;
              if (*lhs == *rhs) return 0;
              return (*lhs < *rhs) ? -1 : 1;
            }
            case ValueTag::TensorHandle:
            case ValueTag::ShapeHandle:
            case ValueTag::WeightsTensorHandle:
              if (lhs_val == rhs_val) return 0;
              return (lhs_val < rhs_val) ? -1 : 1;
            case ValueTag::OptionHandle: {
              auto lhs = option_ptr(lhs_val);
              auto rhs = option_ptr(rhs_val);
              if (!lhs || !rhs) return std::nullopt;
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
              if (!lhs || !rhs) return std::nullopt;
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
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto tag = literal_kind_to_tag(insn.literal_kind);
        auto handle_ok = [&](std::size_t limit) {
          return insn.b > 0 && static_cast<std::size_t>(insn.b) <= limit;
        };
        switch (tag) {
          case ValueTag::FloatHandle:
            if (!handle_ok(state_.floats.size())) { trap = Trap::IllegalInstruction; break; }
            break;
          case ValueTag::FractionHandle:
            if (!handle_ok(state_.fractions.size())) { trap = Trap::IllegalInstruction; break; }
            break;
          case ValueTag::SymbolHandle:
            if (!handle_ok(state_.symbols.size())) { trap = Trap::IllegalInstruction; break; }
            break;
          case ValueTag::TensorHandle:
            if (!handle_ok(state_.tensors.size())) { trap = Trap::IllegalInstruction; break; }
            break;
          case ValueTag::ShapeHandle:
            if (!handle_ok(state_.shapes.size())) { trap = Trap::IllegalInstruction; break; }
            break;
          default:
            break;
        }
        if (trap != Trap::None) break;
        set_reg(insn.a, insn.b, tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Mov:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        copy_reg(insn.a, insn.b);
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Inc:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] += 1;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Dec:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] -= 1;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Add:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] + state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Sub:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] - state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Load: {
        if (!reg_ok(insn.a)) { trap = Trap::InvalidMemory; break; }
        if (!mem_ok(insn.b)) {
          log_bounds_fault(insn.opcode, insn.b, "memory load");
          trap = Trap::InvalidMemory;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.b);
        state_.registers[insn.a] = state_.memory[addr];
        state_.register_tags[insn.a] = state_.memory_tags[addr];
        log_memory_segment_access(insn.opcode, segment_for_address(addr), addr, 1, "memory load");
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::WeightsLoad: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (insn.b <= 0 || static_cast<std::size_t>(insn.b) > state_.symbols.size()) {
          trap = Trap::IllegalInstruction;
          break;
        }
        const std::string& name = state_.symbols[static_cast<std::size_t>(insn.b - 1)];
        auto handle = intern_weights_tensor(name);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::WeightsTensorHandle;
        break;
      }
      case t81::tisc::Opcode::Store: {
        if (!reg_ok(insn.b)) { trap = Trap::InvalidMemory; break; }
        if (!mem_ok(insn.a)) {
          log_bounds_fault(insn.opcode, insn.a, "memory store");
          trap = Trap::InvalidMemory;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.a);
        state_.memory[addr] = state_.registers[insn.b];
        state_.memory_tags[addr] = state_.register_tags[insn.b];
        log_memory_segment_access(insn.opcode, segment_for_address(addr), addr, 1, "memory store");
        break;
      }
      case t81::tisc::Opcode::Mul:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] * state_.registers[insn.c];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto divisor = state_.registers[insn.c];
        if (divisor == 0) { trap = Trap::DivideByZero; break; }
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
        if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
        state_.pc = static_cast<std::size_t>(insn.a);
        break;
      case t81::tisc::Opcode::JumpIfZero:
        if (!reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.registers[insn.b] == 0) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::JumpIfNotZero:
        if (!reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.registers[insn.b] != 0) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::Neg:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = -state_.registers[insn.b];
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::JumpIfNegative:
        if (state_.flags.negative) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::JumpIfPositive:
        if (state_.flags.positive) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::Less:
      case t81::tisc::Opcode::LessEqual:
      case t81::tisc::Opcode::Greater:
      case t81::tisc::Opcode::GreaterEqual:
      case t81::tisc::Opcode::Equal:
      case t81::tisc::Opcode::NotEqual: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto tag_b = state_.register_tags[insn.b];
        auto tag_c = state_.register_tags[insn.c];
        if (tag_b != tag_c) { trap = Trap::IllegalInstruction; break; }
        auto relation_opt = compare_value(tag_b, state_.registers[insn.b], state_.registers[insn.c]);
        if (!relation_opt.has_value()) { trap = Trap::IllegalInstruction; break; }
        int relation = relation_opt.value();
        bool result = false;
        switch (insn.opcode) {
          case t81::tisc::Opcode::Less: result = relation < 0; break;
          case t81::tisc::Opcode::LessEqual: result = relation <= 0; break;
          case t81::tisc::Opcode::Greater: result = relation > 0; break;
          case t81::tisc::Opcode::GreaterEqual: result = relation >= 0; break;
          case t81::tisc::Opcode::Equal: result = relation == 0; break;
          case t81::tisc::Opcode::NotEqual: result = relation != 0; break;
          default: break;
        }
        state_.registers[insn.a] = result ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Cmp: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto tag_a = state_.register_tags[insn.a];
        auto tag_b = state_.register_tags[insn.b];
        if (tag_a != tag_b) { trap = Trap::IllegalInstruction; break; }
        auto relation_opt =
            compare_value(tag_a, state_.registers[insn.a], state_.registers[insn.b]);
        if (!relation_opt.has_value()) { trap = Trap::IllegalInstruction; break; }
        int relation = relation_opt.value();
        state_.flags.zero = (relation == 0);
        state_.flags.negative = (relation < 0);
        state_.flags.positive = (relation > 0);
        break;
      }
      case t81::tisc::Opcode::SetF: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
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
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto addr_opt = push_stack(state_.registers[insn.a], state_.register_tags[insn.a]);
        if (!addr_opt.has_value()) { trap = Trap::BoundsFault; break; }
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1, "stack push");
        break;
      }
      case t81::tisc::Opcode::Pop: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        ValueTag tag = ValueTag::Int;
        auto addr_opt = pop_stack(state_.registers[insn.a], tag);
        if (!addr_opt.has_value()) { trap = Trap::BoundsFault; break; }
        state_.register_tags[insn.a] = tag;
        update_flags(state_.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1, "stack pop");
        break;
      }
      case t81::tisc::Opcode::StackAlloc: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (insn.b < 0) { trap = Trap::IllegalInstruction; break; }
        const auto& stack = state_.layout.stack;
        if (!stack.valid()) { trap = Trap::IllegalInstruction; break; }
        std::size_t size = static_cast<std::size_t>(insn.b);
        std::size_t available = state_.sp - stack.start;
        if (size > available) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack,
                           static_cast<int>(state_.sp), "stack frame allocate");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t new_sp = state_.sp - size;
        if (new_sp < stack.start) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack,
                           static_cast<int>(new_sp), "stack frame allocate");
          trap = Trap::BoundsFault;
          break;
        }
        std::int64_t addr = static_cast<std::int64_t>(new_sp);
        state_.stack_frames.emplace_back(addr, static_cast<std::int64_t>(size));
        state_.sp = new_sp;
        set_reg(insn.a, addr, ValueTag::Int);
        update_flags(addr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(addr), size,
                                  "stack frame allocated");
        break;
      }
      case t81::tisc::Opcode::StackFree: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (insn.b < 0) { trap = Trap::IllegalInstruction; break; }
        const auto& stack = state_.layout.stack;
        if (!stack.valid()) { trap = Trap::BoundsFault; break; }
        if (state_.stack_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack,
                           static_cast<int>(state_.sp), "stack frame free");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t size = static_cast<std::size_t>(insn.b);
        std::int64_t ptr = state_.registers[insn.a];
        if (!stack.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack,
                           static_cast<int>(ptr), "stack frame free");
          trap = Trap::IllegalInstruction;
          break;
        }
        auto [expected_addr, expected_size] = state_.stack_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack,
                           static_cast<int>(ptr), "stack frame free");
          trap = Trap::IllegalInstruction;
          break;
        }
        state_.stack_frames.pop_back();
        state_.sp = static_cast<std::size_t>(ptr + size);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(ptr), size,
                                  "stack frame freed");
        break;
      }
      case t81::tisc::Opcode::HeapAlloc: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) { trap = Trap::IllegalInstruction; break; }
        if (insn.b < 0) { trap = Trap::IllegalInstruction; break; }
        std::size_t size = static_cast<std::size_t>(insn.b);
        if (size > heap.size()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                           static_cast<int>(heap.limit), "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = state_.heap_ptr;
        if (addr < heap.start || addr + size > heap.limit) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                           static_cast<int>(addr), "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        state_.heap_frames.emplace_back(static_cast<std::int64_t>(addr), static_cast<std::int64_t>(size));
        state_.heap_ptr = addr + size;
        set_reg(insn.a, static_cast<std::int64_t>(addr), ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap, addr, size,
                                  "heap block allocated");
        break;
      }
      case t81::tisc::Opcode::HeapFree: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) { trap = Trap::BoundsFault; break; }
        if (insn.b < 0) { trap = Trap::IllegalInstruction; break; }
        if (state_.heap_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                           static_cast<int>(state_.heap_ptr), "heap block free");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t size = static_cast<std::size_t>(insn.b);
        std::int64_t ptr = state_.registers[insn.a];
        if (!heap.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                           static_cast<int>(ptr), "heap block free");
          trap = Trap::IllegalInstruction;
          break;
        }
        auto [expected_addr, expected_size] = state_.heap_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                           static_cast<int>(ptr), "heap block free");
          trap = Trap::IllegalInstruction;
          break;
        }
        state_.heap_frames.pop_back();
        state_.heap_ptr = static_cast<std::size_t>(ptr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap,
                                  static_cast<std::size_t>(ptr), size,
                                  "heap block freed");
        break;
      }
      case t81::tisc::Opcode::TNot:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
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
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
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
            if (result > 1) result = -1;
            if (result < -1) result = 1;
          }
          state_.registers[insn.a] = result;
          state_.register_tags[insn.a] = ValueTag::Int;
          update_flags(state_.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::AxRead: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto verdict = eval_axion_call("AXREAD", current_pc, insn.opcode);
        std::size_t guard_addr = static_cast<std::size_t>(insn.b);
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
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto value = state_.registers[insn.b];
        auto verdict = eval_axion_call("AXSET", current_pc, insn.opcode);
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
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto verdict = eval_axion_call("AXVERIFY", current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          record_axion_event(insn.opcode, insn.b, 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        state_.registers[insn.a] =
            (verdict.kind == t81::axion::VerdictKind::Defer) ? 1 : 0;
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        record_axion_event(insn.opcode, insn.b, state_.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::Call: {
        if (!reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto target = state_.registers[insn.b];
        if (target < 0 || static_cast<std::size_t>(target) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
        if (!push_stack(static_cast<std::int64_t>(state_.pc), ValueTag::Int)) { trap = Trap::BoundsFault; break; }
        state_.pc = static_cast<std::size_t>(target);
        break;
      }
      case t81::tisc::Opcode::Ret: {
        std::int64_t addr = 0;
        ValueTag tag = ValueTag::Int;
        if (!pop_stack(addr, tag)) { trap = Trap::BoundsFault; break; }
        if (tag != ValueTag::Int) { trap = Trap::IllegalInstruction; break; }
        if (addr < 0 || static_cast<std::size_t>(addr) >= program_.insns.size()) { trap = Trap::IllegalInstruction; break; }
        state_.pc = static_cast<std::size_t>(addr);
        break;
      }
      case t81::tisc::Opcode::Trap:
        trap = Trap::TrapInstruction;
        break;
      case t81::tisc::Opcode::I2F: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        double value = static_cast<double>(state_.registers[insn.b]);
        state_.registers[insn.a] = alloc_float(value);
        state_.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::F2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto fp = float_ptr(state_.registers[insn.b]);
        if (!fp) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = static_cast<std::int64_t>(*fp);
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::I2Frac: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto frac = t81::T81Fraction::from_int(state_.registers[insn.b]);
        state_.registers[insn.a] = alloc_fraction(std::move(frac));
        state_.register_tags[insn.a] = ValueTag::FractionHandle;
        break;
      }
      case t81::tisc::Opcode::Frac2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto fp = fraction_ptr(state_.registers[insn.b]);
        if (!fp || !t81::T81BigInt::is_one(fp->den)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = fp->num.to_int64();
        state_.register_tags[insn.a] = ValueTag::Int;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::FAdd:
      case t81::tisc::Opcode::FSub:
      case t81::tisc::Opcode::FMul:
      case t81::tisc::Opcode::FDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto lhs = float_ptr(state_.registers[insn.b]);
        auto rhs = float_ptr(state_.registers[insn.c]);
        if (!lhs || !rhs) { trap = Trap::IllegalInstruction; break; }
        double result = 0.0;
        switch (insn.opcode) {
          case t81::tisc::Opcode::FAdd: result = *lhs + *rhs; break;
          case t81::tisc::Opcode::FSub: result = *lhs - *rhs; break;
          case t81::tisc::Opcode::FMul: result = *lhs * *rhs; break;
          case t81::tisc::Opcode::FDiv:
            if (*rhs == 0.0) { trap = Trap::DivideByZero; break; }
            result = *lhs / *rhs;
            break;
          default: break;
        }
        if (trap != Trap::None) break;
        state_.registers[insn.a] = alloc_float(result);
        state_.register_tags[insn.a] = ValueTag::FloatHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::FracAdd:
      case t81::tisc::Opcode::FracSub:
      case t81::tisc::Opcode::FracMul:
      case t81::tisc::Opcode::FracDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto lhs = fraction_ptr(state_.registers[insn.b]);
        auto rhs = fraction_ptr(state_.registers[insn.c]);
        if (!lhs || !rhs) { trap = Trap::IllegalInstruction; break; }
        try {
          t81::T81Fraction result;
          switch (insn.opcode) {
            case t81::tisc::Opcode::FracAdd: result = t81::T81Fraction::add(*lhs, *rhs); break;
            case t81::tisc::Opcode::FracSub: result = t81::T81Fraction::sub(*lhs, *rhs); break;
            case t81::tisc::Opcode::FracMul: result = t81::T81Fraction::mul(*lhs, *rhs); break;
            case t81::tisc::Opcode::FracDiv:
              if (t81::T81BigInt::is_zero(rhs->num)) { trap = Trap::DivideByZero; break; }
              result = t81::T81Fraction::div(*lhs, *rhs);
              break;
            default: break;
          }
          if (trap != Trap::None) break;
          state_.registers[insn.a] = alloc_fraction(std::move(result));
          state_.register_tags[insn.a] = ValueTag::FractionHandle;
          update_flags(state_.registers[insn.a]);
        } catch (...) {
          trap = Trap::IllegalInstruction;
        }
        break;
      }
      case t81::tisc::Opcode::ChkShape: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::TensorHandle ||
            state_.register_tags[insn.c] != ValueTag::ShapeHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto tensor = tensor_ptr(state_.registers[insn.b]);
        auto expected = shape_ptr(state_.registers[insn.c]);
        if (!tensor) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        if (!expected) { trap = Trap::IllegalInstruction; break; }
        bool match = tensor->shape() == *expected;
        set_reg(insn.a, match ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeOptionSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto handle =
            intern_option(true, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeOptionNone: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto handle = intern_option(false, ValueTag::Int, 0);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto handle =
            intern_result(true, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto handle =
            intern_result(false, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariant: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto handle = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariantPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (insn.c < 0) { trap = Trap::IllegalInstruction; break; }
        auto handle =
            intern_enum(static_cast<int>(insn.c), true, state_.register_tags[insn.b], state_.registers[insn.b]);
        state_.registers[insn.a] = handle;
        state_.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionIsSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto opt = option_ptr(state_.registers[insn.b]);
        if (!opt) { trap = Trap::IllegalInstruction; break; }
        set_reg(insn.a, opt->has_value ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionUnwrap: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto opt = option_ptr(state_.registers[insn.b]);
        if (!opt || !opt->has_value) { trap = Trap::IllegalInstruction; break; }
        set_reg(insn.a, opt->payload, opt->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultIsOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto res = result_ptr(state_.registers[insn.b]);
        if (!res) { trap = Trap::IllegalInstruction; break; }
        set_reg(insn.a, res->is_ok ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto res = result_ptr(state_.registers[insn.b]);
        if (!res || !res->is_ok) { trap = Trap::IllegalInstruction; break; }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto res = result_ptr(state_.registers[insn.b]);
        if (!res || res->is_ok) { trap = Trap::IllegalInstruction; break; }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(state_.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::EnumIsVariant: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto val = enum_ptr(state_.registers[insn.b]);
        if (!val) { trap = Trap::IllegalInstruction; break; }
        bool matches = (val->variant_id == insn.c);
        set_reg(insn.a, matches ? 1 : 0, ValueTag::Int);
        update_flags(state_.registers[insn.a]);
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          const int guard_variant_id = insn.c;
          const int guard_enum_id = t81::enum_meta::decode_enum_id(guard_variant_id);
          const int guard_local_variant = t81::enum_meta::decode_variant_id(guard_variant_id);
          const auto* meta = enum_metadata_for(guard_enum_id);
          const auto* variant_meta = variant_metadata(meta, guard_local_variant);
          reason << "enum guard";
          if (meta) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          reason << " match=" << (matches ? "pass" : "fail");
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, insn.c, matches ? 1 : 0, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::EnumUnwrapPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        if (state_.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::IllegalInstruction;
          break;
        }
        auto val = enum_ptr(state_.registers[insn.b]);
        if (!val || !val->has_payload) { trap = Trap::IllegalInstruction; break; }
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
          if (meta) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(global_variant_id), val->payload, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::TVecAdd: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto ta = tensor_ptr(state_.registers[insn.b]);
        if (!ta) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!tb) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        if (ta->rank() != 1 || tb->rank() != 1 || ta->shape()[0] != tb->shape()[0]) {
          trap = Trap::IllegalInstruction;
          break;
        }
        std::vector<float> data(ta->data().size());
        for (std::size_t i = 0; i < data.size(); ++i) {
          data[i] = ta->data()[i] + tb->data()[i];
        }
        t81::T729Tensor result({ta->shape()[0]}, std::move(data));
        state_.registers[insn.a] = alloc_tensor(std::move(result));
        break;
      }
      case t81::tisc::Opcode::TMatMul: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto ta = tensor_ptr(state_.registers[insn.b]);
        if (!ta) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!tb) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        if (ta->rank() != 2 || tb->rank() != 2) {
          trap = Trap::IllegalInstruction;
          break;
        }
        int m = ta->shape()[0];
        int k = ta->shape()[1];
        if (tb->shape()[0] != k) { trap = Trap::IllegalInstruction; break; }
        int n = tb->shape()[1];
        std::vector<float> data(static_cast<std::size_t>(m * n), 0.0f);
        for (int i = 0; i < m; ++i) {
          for (int j = 0; j < n; ++j) {
            float sum = 0.0f;
            for (int z = 0; z < k; ++z) {
              sum += ta->data()[static_cast<std::size_t>(i) * k + z] *
                     tb->data()[static_cast<std::size_t>(z) * n + j];
            }
            data[static_cast<std::size_t>(i) * n + j] = sum;
          }
        }
        t81::T729Tensor result({m, n}, std::move(data));
        state_.registers[insn.a] = alloc_tensor(std::move(result));
        break;
      }
      case t81::tisc::Opcode::TTenDot: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto ta = tensor_ptr(state_.registers[insn.b]);
        if (!ta) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.b]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!tb) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(state_.registers[insn.c]),
                           "tensor handle access");
          trap = Trap::IllegalInstruction;
          break;
        }
        try {
          auto result = t81::T729Tensor::contract_dot(*ta, *tb);
          state_.registers[insn.a] = alloc_tensor(std::move(result));
        } catch (...) {
          trap = Trap::IllegalInstruction;
        }
        break;
      }
      default:
        trap = Trap::IllegalInstruction;
        break;
    }
    ++instructions_since_gc_;
    if (instructions_since_gc_ >= kGcInterval) {
      run_gc_cycle_("interval");
    }

    log_trace(insn.opcode, trap);
    if (trap != Trap::None) return trap;
    return {};
  }

  std::expected<void, Trap> run_to_halt(std::size_t max_steps) override {
    for (std::size_t i = 0; i < max_steps && !state_.halted; ++i) {
      auto result = step();
      if (!result.has_value()) return result;
    }
    return {};
  }

  const State& state() const override { return state_; }

 private:
  std::int64_t intern_weights_tensor(std::string_view name) {
    if (name.empty() || !state_.weights_model) return 0;
    auto key = std::string(name);
    auto it = state_.weights_tensor_handles.find(key);
    if (it != state_.weights_tensor_handles.end()) {
      return it->second;
    }
    auto native_it = state_.weights_model->native.find(key);
    if (native_it == state_.weights_model->native.end()) return 0;
    state_.weights_tensor_refs.push_back(&native_it->second);
    auto handle = static_cast<std::int64_t>(state_.weights_tensor_refs.size());
    state_.weights_tensor_handles.emplace(std::move(key), handle);
    return handle;
  }

  t81::axion::Verdict eval_axion_call(std::string_view syscall,
                                      std::size_t pc,
                                      t81::tisc::Opcode opcode) {
    t81::axion::SyscallContext ctx;
    ctx.caller = "t81vm";
    ctx.syscall.assign(syscall);
    ctx.pc = pc;
    ctx.next_opcode = opcode;
    ctx.policy = state_.policy ? &*state_.policy : nullptr;
    ctx.trace_reasons.reserve(state_.axion_log.size());
    for (const auto& entry : state_.axion_log) {
      ctx.trace_reasons.push_back(entry.verdict.reason);
    }
    return axion_engine_->evaluate(ctx);
  }

  const t81::tisc::EnumMetadata* enum_metadata_for(int enum_id) const {
    auto it = state_.enum_metadata_index.find(enum_id);
    if (it == state_.enum_metadata_index.end()) {
      return nullptr;
    }
    return &state_.enum_metadata[it->second];
  }

  const t81::tisc::EnumVariantMetadata* variant_metadata(const t81::tisc::EnumMetadata* meta,
                                                        int variant_id) const {
    if (!meta) return nullptr;
    for (const auto& variant : meta->variants) {
      if (variant.variant_id == variant_id) {
        return &variant;
      }
    }
    return nullptr;
  }

  MemorySegmentKind segment_for_address(std::size_t addr) const {
    const auto& layout = state_.layout;
    if (layout.stack.contains(addr)) return MemorySegmentKind::Stack;
    if (layout.heap.contains(addr)) return MemorySegmentKind::Heap;
    if (layout.tensor.contains(addr)) return MemorySegmentKind::Tensor;
    if (layout.meta.contains(addr)) return MemorySegmentKind::Meta;
    return MemorySegmentKind::Unknown;
  }

  void log_memory_segment_access(t81::tisc::Opcode opcode, MemorySegmentKind kind,
                                 std::size_t addr, std::size_t size,
                                 std::string_view action) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason;
    reason << action << " " << to_string(kind) << " addr=" << addr << " size=" << size;
    verdict.reason = reason.str();
    record_axion_event(opcode, static_cast<std::int32_t>(kind), static_cast<std::int64_t>(addr),
                       verdict);
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, MemorySegmentKind kind, int addr,
                        std::string_view action) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason;
    reason << "bounds fault segment=" << to_string(kind) << " addr=" << addr << " action=" << action;
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
    state_.axion_log.push_back(event);
  }

  void log_meta_slot(const char* label) {
    if (!state_.layout.meta.contains(state_.meta_ptr)) return;
    AxionEvent meta_event;
    meta_event.opcode = t81::tisc::Opcode::Nop;
    meta_event.tag = static_cast<std::int32_t>(MemorySegmentKind::Meta);
    meta_event.value = static_cast<std::int64_t>(state_.meta_ptr);
    meta_event.verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason;
    reason << "meta slot " << label << " addr=" << state_.meta_ptr;
    meta_event.verdict.reason = reason.str();
    push_axion_event(meta_event);
    ++state_.meta_ptr;
  }

  void apply_segment_reason(t81::axion::Verdict& verdict, const char* action,
                            MemorySegmentKind kind, std::size_t addr) {
    std::ostringstream reason;
    reason << action << " segment=" << to_string(kind) << " addr=" << addr;
    if (!verdict.reason.empty()) {
      reason << " " << verdict.reason;
    }
    verdict.reason = reason.str();
  }

  void record_axion_event(t81::tisc::Opcode op, std::int32_t tag,
                          std::int64_t value, const t81::axion::Verdict& verdict) {
    log_meta_slot("axion event");
    push_axion_event(AxionEvent{op, tag, value, verdict});
  }

  void run_gc_cycle_(const char* reason) {
    instructions_since_gc_ = 0;
    state_.gc_cycles++;
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream os;
    os << reason << " stack_frames=" << state_.stack_frames.size()
       << " heap_frames=" << state_.heap_frames.size()
       << " heap_ptr=" << state_.heap_ptr
       << " tensor_slots=" << state_.tensors.size()
       << " meta_space=" << state_.layout.meta.size();
    verdict.reason = os.str();
    record_axion_event(t81::tisc::Opcode::Trap,
                       static_cast<std::int32_t>(state_.gc_cycles),
                       static_cast<std::int64_t>(state_.gc_cycles), verdict);
  }

  State state_{};
  t81::tisc::Program program_{};
  std::unique_ptr<t81::axion::Engine> axion_engine_;
  static constexpr std::size_t kGcInterval = 64;
  std::size_t instructions_since_gc_{0};
};
}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm(std::unique_ptr<t81::axion::Engine> engine) {
  return std::make_unique<Interpreter>(std::move(engine));
}
}  // namespace t81::vm
