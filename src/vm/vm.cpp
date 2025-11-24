#include "t81/vm/vm.hpp"

#include <functional>
#include <memory>

#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include <string>
#include <string_view>
#include <utility>

#include "t81/axion/engine.hpp"

namespace t81::vm {
namespace {
class Interpreter : public IVirtualMachine {
 public:
  explicit Interpreter(std::unique_ptr<t81::axion::Engine> engine)
      : axion_engine_(std::move(engine)) {
    if (!axion_engine_) {
      axion_engine_ = t81::axion::make_allow_all_engine();
    }
  }

  void load_program(const t81::tisc::Program& program) override {
    program_ = program;
    state_ = State{};
    state_.register_tags.fill(ValueTag::Int);
    state_.layout.code_limit = program_.insns.size();
    state_.layout.stack_limit = state_.layout.code_limit + 256;
    state_.layout.heap_limit = state_.layout.stack_limit + 768;
    state_.memory.resize(state_.layout.heap_limit, 0);
    state_.memory_tags.assign(state_.memory.size(), ValueTag::Int);
    state_.sp = state_.layout.stack_limit;
    state_.floats = program_.float_pool;
    state_.fractions = program_.fraction_pool;
    state_.symbols = program_.symbol_pool;
    state_.tensors = program_.tensor_pool;
    state_.shapes = program_.shape_pool;
    state_.options.clear();
    state_.results.clear();
    state_.policy.reset();
    state_.gc_cycles = 0;
    instructions_since_gc_ = 0;
    if (!program_.axion_policy_text.empty()) {
      auto policy = t81::axion::parse_policy(program_.axion_policy_text);
      if (policy.has_value()) {
        state_.policy = policy.value();
      }
    }
  }

  std::expected<void, Trap> step() override {
    if (state_.halted || state_.pc >= program_.insns.size()) {
      state_.halted = true;
      return {};
    }

    const std::size_t current_pc = state_.pc++;
    const auto& insn = program_.insns[current_pc];

    auto reg_ok = [this](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < state_.registers.size();
    };
    auto mem_ok = [this](int addr, bool code = false) {
      if (addr < 0) return false;
      std::size_t a = static_cast<std::size_t>(addr);
      if (code) return a < state_.layout.code_limit;
      return a < state_.memory.size();
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
    auto push_stack = [this](std::int64_t value, ValueTag tag) -> bool {
      if (state_.layout.stack_limit <= state_.layout.code_limit) return false;
      if (state_.sp == state_.layout.code_limit) return false;
      --state_.sp;
      state_.memory[state_.sp] = value;
      state_.memory_tags[state_.sp] = tag;
      return true;
    };
    auto pop_stack = [this](std::int64_t& value, ValueTag& tag) -> bool {
      if (state_.sp == state_.layout.stack_limit) return false;
      value = state_.memory[state_.sp];
      tag = state_.memory_tags[state_.sp];
      ++state_.sp;
      return true;
    };
    auto tensor_ptr = [this](std::int64_t handle) -> t81::T729Tensor* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.tensors.size()) return nullptr;
      return &state_.tensors[idx];
    };
    auto alloc_tensor = [this](t81::T729Tensor tensor) -> std::int64_t {
      state_.tensors.push_back(std::move(tensor));
      return static_cast<std::int64_t>(state_.tensors.size());
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
          }
          return std::nullopt;
        };

    Trap trap = Trap::None;
    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop:
        break;
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
      case t81::tisc::Opcode::Load:
        if (!reg_ok(insn.a) || !mem_ok(insn.b)) { trap = Trap::InvalidMemory; break; }
        {
          std::size_t addr = static_cast<std::size_t>(insn.b);
          state_.registers[insn.a] = state_.memory[addr];
          state_.register_tags[insn.a] = state_.memory_tags[addr];
        }
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Store:
        if (!reg_ok(insn.b) || !mem_ok(insn.a)) { trap = Trap::InvalidMemory; break; }
        {
          std::size_t addr = static_cast<std::size_t>(insn.a);
          state_.memory[addr] = state_.registers[insn.b];
          state_.memory_tags[addr] = state_.register_tags[insn.b];
        }
        break;
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
      case t81::tisc::Opcode::Push:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (!push_stack(state_.registers[insn.a], state_.register_tags[insn.a])) { trap = Trap::BoundsFault; break; }
        break;
      case t81::tisc::Opcode::Pop:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        {
          ValueTag tag = ValueTag::Int;
          if (!pop_stack(state_.registers[insn.a], tag)) { trap = Trap::BoundsFault; break; }
          state_.register_tags[insn.a] = tag;
        }
        update_flags(state_.registers[insn.a]);
        break;
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
        auto verdict = eval_axion_call("AXREAD");
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
        if (!reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        auto value = state_.registers[insn.b];
        auto verdict = eval_axion_call("AXSET");
        record_axion_event(insn.opcode, insn.a, value, verdict);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
        }
        break;
      }
      case t81::tisc::Opcode::AxVerify: {
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        auto verdict = eval_axion_call("AXVERIFY");
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
        if (!tensor || !expected) { trap = Trap::IllegalInstruction; break; }
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
      case t81::tisc::Opcode::TVecAdd: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        auto ta = tensor_ptr(state_.registers[insn.b]);
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!ta || !tb || ta->rank() != 1 || tb->rank() != 1 || ta->shape()[0] != tb->shape()[0]) {
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
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!ta || !tb || ta->rank() != 2 || tb->rank() != 2) {
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
        auto tb = tensor_ptr(state_.registers[insn.c]);
        if (!ta || !tb) { trap = Trap::IllegalInstruction; break; }
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
  t81::axion::Verdict eval_axion_call(std::string_view syscall) {
    t81::axion::SyscallContext ctx;
    ctx.caller = "t81vm";
    ctx.syscall.assign(syscall);
    return axion_engine_->evaluate(ctx);
  }

  void record_axion_event(t81::tisc::Opcode op, std::int32_t tag,
                          std::int64_t value, const t81::axion::Verdict& verdict) {
    state_.axion_log.push_back(AxionEvent{op, tag, value, verdict});
  }

  void run_gc_cycle_(const char* reason) {
    instructions_since_gc_ = 0;
    state_.gc_cycles++;
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    verdict.reason = reason;
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
