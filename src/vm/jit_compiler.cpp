#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/vm/jit.hpp"

namespace t81::vm {

class ThreadedJitTrace : public JitTrace {
public:
  explicit ThreadedJitTrace(std::vector<t81::tisc::Insn> insns) : insns_(std::move(insns)) {}

  std::size_t size() const override { return insns_.size(); }

  ExecResult execute(State& state) override {
    auto reg_ok = [&state](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < state.registers.size();
    };
    auto mem_ok = [&state](std::int64_t addr) {
      return addr >= 0 && static_cast<std::size_t>(addr) < state.memory.size();
    };
    auto code_ok = [&state](std::int64_t addr) {
      if (addr < 0) return false;
      const std::size_t uaddr = static_cast<std::size_t>(addr);
      if (uaddr >= state.memory.size()) return false;
      if (state.layout.code.valid()) {
        return state.layout.code.contains(uaddr);
      }
      return true;
    };
    auto clamp_trit = [](std::int64_t value) -> int {
      if (value < -1) return -1;
      if (value > 1) return 1;
      return static_cast<int>(value);
    };
    auto tensor_ptr = [&state](std::int64_t handle) -> t81::T729Tensor* {
      if (handle <= 0) return nullptr;
      const auto idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state.tensors.size()) return nullptr;
      return &state.tensors[idx];
    };
    auto alloc_tensor = [&state](t81::T729Tensor tensor) -> std::int64_t {
      state.tensors.push_back(std::move(tensor));
      return static_cast<std::int64_t>(state.tensors.size());
    };
    auto intern_option = [&state](bool has_value, ValueTag payload_tag,
                                  std::int64_t payload) -> std::int64_t {
      state.options.push_back(OptionValue{has_value, payload_tag, payload});
      return static_cast<std::int64_t>(state.options.size());
    };
    auto option_ptr = [&state](std::int64_t handle) -> OptionValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state.options.size()) return nullptr;
      return &state.options[idx];
    };
    auto intern_result = [&state](bool is_ok, ValueTag payload_tag,
                                  std::int64_t payload) -> std::int64_t {
      state.results.push_back(ResultValue{is_ok, payload_tag, payload});
      return static_cast<std::int64_t>(state.results.size());
    };
    auto result_ptr = [&state](std::int64_t handle) -> ResultValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state.results.size()) return nullptr;
      return &state.results[idx];
    };
    auto intern_enum = [&state](int variant_id, bool has_payload, ValueTag payload_tag,
                                std::int64_t payload) -> std::int64_t {
      state.enums.push_back(EnumValue{variant_id, has_payload, payload_tag, payload, -1});
      return static_cast<std::int64_t>(state.enums.size());
    };
    auto enum_ptr = [&state](std::int64_t handle) -> EnumValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state.enums.size()) return nullptr;
      return &state.enums[idx];
    };
    auto intern_complex = [&state](std::int64_t real, std::int64_t imag) -> std::int64_t {
      state.complexes.push_back(ComplexValue{real, imag});
      return static_cast<std::int64_t>(state.complexes.size());
    };

    ExecResult result{};
    for (const auto& insn : insns_) {
      result.instructions_executed++;
      bool stop_trace = false;
      bool guard_deopt = false;
      switch (insn.opcode) {
        case t81::tisc::Opcode::Add:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b] + state.registers[insn.c];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Sub:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b] - state.registers[insn.c];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mul:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b] * state.registers[insn.c];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Div:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.registers[insn.c] == 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b] / state.registers[insn.c];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mod:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.registers[insn.c] == 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b] % state.registers[insn.c];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Inc:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a]++;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Dec:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a]--;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mov:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.registers[insn.b];
          state.register_tags[insn.a] = state.register_tags[insn.b];
          break;
        case t81::tisc::Opcode::Neg:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = -state.registers[insn.b];
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::TNot: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          int t = clamp_trit(state.registers[insn.b]);
          state.registers[insn.a] = -t;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::TAnd:
        case t81::tisc::Opcode::TOr:
        case t81::tisc::Opcode::TXor: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          int lhs = clamp_trit(state.registers[insn.b]);
          int rhs = clamp_trit(state.registers[insn.c]);
          int out = 0;
          if (insn.opcode == t81::tisc::Opcode::TAnd) {
            out = (lhs < rhs) ? lhs : rhs;
          } else if (insn.opcode == t81::tisc::Opcode::TOr) {
            out = (lhs > rhs) ? lhs : rhs;
          } else {
            out = lhs - rhs;
            if (out > 1) out = -1;
            if (out < -1) out = 1;
          }
          state.registers[insn.a] = out;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::LoadImm:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = insn.b;
          switch (insn.literal_kind) {
            case t81::tisc::LiteralKind::Int:
              state.register_tags[insn.a] = ValueTag::Int;
              break;
            case t81::tisc::LiteralKind::Bool:
              state.register_tags[insn.a] = ValueTag::Bool;
              break;
            case t81::tisc::LiteralKind::FloatHandle:
              state.register_tags[insn.a] = ValueTag::FloatHandle;
              break;
            case t81::tisc::LiteralKind::FractionHandle:
              state.register_tags[insn.a] = ValueTag::FractionHandle;
              break;
            case t81::tisc::LiteralKind::SymbolHandle:
              state.register_tags[insn.a] = ValueTag::SymbolHandle;
              if (insn.b > 0 && static_cast<std::size_t>(insn.b) <= state.symbols.size()) {
                const auto& symbol = state.symbols[static_cast<std::size_t>(insn.b - 1)];
                if (symbol == "std.sys.proof") {
                  state.register_tags[insn.a] = ValueTag::ProofHandle;
                  state.registers[insn.a] = 1;
                } else if (symbol == "std.io.stream") {
                  state.register_tags[insn.a] = ValueTag::IoStreamHandle;
                  state.registers[insn.a] = 1;
                } else if (symbol == "std.io.net") {
                  state.register_tags[insn.a] = ValueTag::IoNetHandle;
                  state.registers[insn.a] = 1;
                } else if (symbol == "std.async.thread") {
                  state.register_tags[insn.a] = ValueTag::AsyncThreadHandle;
                  state.registers[insn.a] = 1;
                } else if (symbol == "std.async.promise") {
                  state.register_tags[insn.a] = ValueTag::AsyncPromiseHandle;
                  state.registers[insn.a] = 1;
                }
              }
              break;
            case t81::tisc::LiteralKind::TensorHandle:
              state.register_tags[insn.a] = ValueTag::TensorHandle;
              break;
            case t81::tisc::LiteralKind::ShapeHandle:
              state.register_tags[insn.a] = ValueTag::ShapeHandle;
              break;
          }
          break;
        case t81::tisc::Opcode::Load:
          if (!reg_ok(insn.a) || !mem_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.memory[static_cast<std::size_t>(insn.b)];
          state.register_tags[insn.a] = state.memory_tags[static_cast<std::size_t>(insn.b)];
          break;
        case t81::tisc::Opcode::Store:
          if (!reg_ok(insn.b) || !mem_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.memory[static_cast<std::size_t>(insn.a)] = state.registers[insn.b];
          state.memory_tags[static_cast<std::size_t>(insn.a)] = state.register_tags[insn.b];
          break;
        case t81::tisc::Opcode::Push: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto& stack = state.layout.stack;
          if (!stack.valid() || state.sp <= stack.start) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::size_t new_sp = state.sp - 1;
          if (!stack.contains(new_sp)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.sp = new_sp;
          state.memory[state.sp] = state.registers[insn.a];
          state.memory_tags[state.sp] = state.register_tags[insn.a];
          break;
        }
        case t81::tisc::Opcode::Pop: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto& stack = state.layout.stack;
          if (!stack.valid() || state.sp >= stack.limit) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = state.memory[state.sp];
          state.register_tags[insn.a] = state.memory_tags[state.sp];
          state.sp += 1;
          break;
        }
        case t81::tisc::Opcode::MakeOptionSome:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] =
              intern_option(true, state.register_tags[insn.b], state.registers[insn.b]);
          state.register_tags[insn.a] = ValueTag::OptionHandle;
          break;
        case t81::tisc::Opcode::MakeOptionNone:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = intern_option(false, ValueTag::Int, 0);
          state.register_tags[insn.a] = ValueTag::OptionHandle;
          break;
        case t81::tisc::Opcode::MakeResultOk:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] =
              intern_result(true, state.register_tags[insn.b], state.registers[insn.b]);
          state.register_tags[insn.a] = ValueTag::ResultHandle;
          break;
        case t81::tisc::Opcode::MakeResultErr:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] =
              intern_result(false, state.register_tags[insn.b], state.registers[insn.b]);
          state.register_tags[insn.a] = ValueTag::ResultHandle;
          break;
        case t81::tisc::Opcode::OptionIsSome: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::OptionHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* opt = option_ptr(state.registers[insn.b]);
          if (!opt) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = opt->has_value ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::OptionUnwrap: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::OptionHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* opt = option_ptr(state.registers[insn.b]);
          if (!opt || !opt->has_value) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = opt->payload;
          state.register_tags[insn.a] = opt->payload_tag;
          break;
        }
        case t81::tisc::Opcode::ResultIsOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(state.registers[insn.b]);
          if (!res) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = res->is_ok ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(state.registers[insn.b]);
          if (!res || !res->is_ok) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = res->payload;
          state.register_tags[insn.a] = res->payload_tag;
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapErr: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(state.registers[insn.b]);
          if (!res || res->is_ok) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = res->payload;
          state.register_tags[insn.a] = res->payload_tag;
          break;
        }
        case t81::tisc::Opcode::MakeEnumVariant:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
          state.register_tags[insn.a] = ValueTag::EnumHandle;
          break;
        case t81::tisc::Opcode::MakeEnumVariantPayload:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || insn.c < 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = intern_enum(
              static_cast<int>(insn.c), true, state.register_tags[insn.b], state.registers[insn.b]);
          state.register_tags[insn.a] = ValueTag::EnumHandle;
          break;
        case t81::tisc::Opcode::EnumIsVariant: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::EnumHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* val = enum_ptr(state.registers[insn.b]);
          if (!val) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (val->variant_id == insn.c) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::EnumUnwrapPayload: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              state.register_tags[insn.b] != ValueTag::EnumHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* val = enum_ptr(state.registers[insn.b]);
          if (!val || !val->has_payload) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = val->payload;
          state.register_tags[insn.a] = val->payload_tag;
          break;
        }
        case t81::tisc::Opcode::MakeComplex:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c) ||
              state.register_tags[insn.b] != ValueTag::Int ||
              state.register_tags[insn.c] != ValueTag::Int) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] =
              intern_complex(state.registers[insn.b], state.registers[insn.c]);
          state.register_tags[insn.a] = ValueTag::ComplexHandle;
          break;
        case t81::tisc::Opcode::TMatMul: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.register_tags[insn.b] != ValueTag::TensorHandle ||
              state.register_tags[insn.c] != ValueTag::TensorHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto ta = tensor_ptr(state.registers[insn.b]);
          auto tb = tensor_ptr(state.registers[insn.c]);
          if (!ta || !tb || ta->rank() != 2 || tb->rank() != 2 ||
              ta->shape()[1] != tb->shape()[0]) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = alloc_tensor(t81::ops::matmul(*ta, *tb));
          state.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TRMSNorm: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.register_tags[insn.b] != ValueTag::TensorHandle ||
              state.register_tags[insn.c] != ValueTag::TensorHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto t = tensor_ptr(state.registers[insn.b]);
          auto w = tensor_ptr(state.registers[insn.c]);
          if (!t || !w || t->rank() == 0 || w->rank() != 1 || w->shape()[0] != t->shape().back()) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = alloc_tensor(t81::ops::rmsnorm(*t, *w));
          state.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::Less:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] < state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::LessEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] <= state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Greater:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] > state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::GreaterEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] >= state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Equal:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] == state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::NotEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.registers[insn.a] = (state.registers[insn.b] != state.registers[insn.c]) ? 1 : 0;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Cmp: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.register_tags[insn.a] != ValueTag::Int ||
              state.register_tags[insn.b] != ValueTag::Int) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto lhs = state.registers[insn.a];
          const auto rhs = state.registers[insn.b];
          state.flags.zero = (lhs == rhs);
          state.flags.negative = (lhs < rhs);
          state.flags.positive = (lhs > rhs);
          break;
        }
        case t81::tisc::Opcode::SetF: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::int64_t flag_value = 0;
          if (state.flags.negative) {
            flag_value = -1;
          } else if (!state.flags.zero) {
            flag_value = 1;
          }
          state.registers[insn.a] = flag_value;
          state.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::Jump:
          state.pc = static_cast<size_t>(insn.a);
          stop_trace = true;
          result.exit_kind = ExitKind::Branch;
          break;
        case t81::tisc::Opcode::JumpIfZero:
          if (!reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.registers[insn.b] == 0) {
            state.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::JumpIfNotZero:
          if (!reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.registers[insn.b] != 0) {
            state.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::JumpIfNegative:
          if (state.flags.negative) {
            state.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::JumpIfPositive:
          if (state.flags.positive) {
            state.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::Call: {
          if (!reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto target = state.registers[insn.b];
          if (!code_ok(target)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto& stack = state.layout.stack;
          if (!stack.valid() || state.sp <= stack.start) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::size_t new_sp = state.sp - 1;
          if (!stack.contains(new_sp)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.sp = new_sp;
          // Interpreter pushes the post-call PC.
          state.memory[state.sp] =
              static_cast<std::int64_t>(state.pc + result.instructions_executed);
          state.memory_tags[state.sp] = ValueTag::Int;
          state.call_depth += 1;
          state.pc = static_cast<std::size_t>(target);
          stop_trace = true;
          result.exit_kind = ExitKind::Branch;
          break;
        }
        case t81::tisc::Opcode::Ret: {
          const auto& stack = state.layout.stack;
          if (!stack.valid() || state.sp >= stack.limit) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto addr = state.memory[state.sp];
          const auto tag = state.memory_tags[state.sp];
          state.sp += 1;
          if (tag != ValueTag::Int || !code_ok(addr)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (state.call_depth > 0) {
            state.call_depth -= 1;
          }
          state.pc = static_cast<std::size_t>(addr);
          stop_trace = true;
          result.exit_kind = ExitKind::Branch;
          break;
        }
        default:
          stop_trace = true;
          guard_deopt = true;
          break;
      }
      if (insn.opcode != t81::tisc::Opcode::Jump && insn.opcode != t81::tisc::Opcode::JumpIfZero &&
          insn.opcode != t81::tisc::Opcode::JumpIfNotZero &&
          insn.opcode != t81::tisc::Opcode::JumpIfNegative &&
          insn.opcode != t81::tisc::Opcode::JumpIfPositive &&
          insn.opcode != t81::tisc::Opcode::Call && insn.opcode != t81::tisc::Opcode::Ret &&
          insn.opcode != t81::tisc::Opcode::Store && insn.opcode != t81::tisc::Opcode::Push &&
          insn.opcode != t81::tisc::Opcode::Cmp && insn.opcode != t81::tisc::Opcode::TMatMul &&
          insn.opcode != t81::tisc::Opcode::TRMSNorm) {
        state.flags.zero = (state.registers[insn.a] == 0);
        state.flags.negative = (state.registers[insn.a] < 0);
        state.flags.positive = (state.registers[insn.a] > 0);
      }
      if (stop_trace) {
        if (guard_deopt) {
          if (result.instructions_executed > 0) {
            // Resume interpreter at the first non-executed instruction.
            state.pc += (result.instructions_executed - 1);
          }
          result.exit_kind = ExitKind::GuardDeopt;
        }
        return result;
      }
    }
    state.pc += result.instructions_executed;
    result.exit_kind = ExitKind::Completed;
    return result;
  }

private:
  std::vector<t81::tisc::Insn> insns_;
};

void JitCompiler::start_tracing(std::size_t pc) {
  tracing_ = true;
  start_pc_ = pc;
  trace_buffer_.clear();
}

void JitCompiler::record_instruction(const t81::tisc::Insn& insn) {
  if (!tracing_) return;

  // Only record supported opcodes.
  switch (insn.opcode) {
    case t81::tisc::Opcode::Add:
    case t81::tisc::Opcode::Sub:
    case t81::tisc::Opcode::Mul:
    case t81::tisc::Opcode::Div:
    case t81::tisc::Opcode::Mod:
    case t81::tisc::Opcode::Inc:
    case t81::tisc::Opcode::Dec:
    case t81::tisc::Opcode::Mov:
    case t81::tisc::Opcode::Neg:
    case t81::tisc::Opcode::TNot:
    case t81::tisc::Opcode::TAnd:
    case t81::tisc::Opcode::TOr:
    case t81::tisc::Opcode::TXor:
    case t81::tisc::Opcode::LoadImm:
    case t81::tisc::Opcode::Load:
    case t81::tisc::Opcode::Store:
    case t81::tisc::Opcode::Push:
    case t81::tisc::Opcode::Pop:
    case t81::tisc::Opcode::Less:
    case t81::tisc::Opcode::LessEqual:
    case t81::tisc::Opcode::Greater:
    case t81::tisc::Opcode::GreaterEqual:
    case t81::tisc::Opcode::Equal:
    case t81::tisc::Opcode::NotEqual:
    case t81::tisc::Opcode::Cmp:
    case t81::tisc::Opcode::SetF:
    case t81::tisc::Opcode::MakeOptionSome:
    case t81::tisc::Opcode::MakeOptionNone:
    case t81::tisc::Opcode::MakeResultOk:
    case t81::tisc::Opcode::MakeResultErr:
    case t81::tisc::Opcode::OptionIsSome:
    case t81::tisc::Opcode::OptionUnwrap:
    case t81::tisc::Opcode::ResultIsOk:
    case t81::tisc::Opcode::ResultUnwrapOk:
    case t81::tisc::Opcode::ResultUnwrapErr:
    case t81::tisc::Opcode::MakeEnumVariant:
    case t81::tisc::Opcode::MakeEnumVariantPayload:
    case t81::tisc::Opcode::EnumIsVariant:
    case t81::tisc::Opcode::EnumUnwrapPayload:
    case t81::tisc::Opcode::MakeComplex:
    case t81::tisc::Opcode::TMatMul:
    case t81::tisc::Opcode::TRMSNorm:
      trace_buffer_.push_back(insn);
      break;
    case t81::tisc::Opcode::Call:
    case t81::tisc::Opcode::Ret:
    case t81::tisc::Opcode::Jump:
    case t81::tisc::Opcode::JumpIfZero:
    case t81::tisc::Opcode::JumpIfNotZero:
    case t81::tisc::Opcode::JumpIfNegative:
    case t81::tisc::Opcode::JumpIfPositive:
      trace_buffer_.push_back(insn);
      tracing_ = false;  // Always stop at branch.
      break;
    default:
      // Stop tracing on unsupported opcodes.
      tracing_ = false;
      break;
  }
}

std::unique_ptr<JitTrace> JitCompiler::compile() {
  tracing_ = false;
  if (trace_buffer_.empty()) return nullptr;
  return std::make_unique<ThreadedJitTrace>(std::move(trace_buffer_));
}

}  // namespace t81::vm
