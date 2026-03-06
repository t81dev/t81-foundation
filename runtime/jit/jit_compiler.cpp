#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/jit/jit.hpp"

namespace t81::vm {

class ThreadedJitTrace : public JitTrace {
public:
  explicit ThreadedJitTrace(std::vector<t81::tisc::Insn> insns) : insns_(std::move(insns)) {}

  std::size_t size() const override { return insns_.size(); }

  ExecResult execute(State& state, const PolicyHook& policy_hook = {}) override {
    if (state.contexts.empty()) return {};
    auto& ctx = state.contexts[state.current_context];

    auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
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
    auto tensor_ptr = [&state](std::int64_t handle) -> t81::T729DynamicTensor* {
      if (handle <= 0) return nullptr;
      const auto idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state.tensors.size()) return nullptr;
      if (!state.tensors[idx].has_value()) return nullptr;
      return &state.tensors[idx].value();
    };
    auto alloc_tensor = [&state](t81::T729DynamicTensor tensor) -> std::int64_t {
      std::size_t idx_handle;
      if (!state.free_tensor_indices.empty()) {
        auto raw_idx = state.free_tensor_indices.back();
        state.free_tensor_indices.pop_back();
        state.tensors[raw_idx] = std::move(tensor);
        idx_handle = raw_idx + 1;
      } else {
        state.tensors.push_back(std::move(tensor));
        idx_handle = state.tensors.size();
      }
      return static_cast<std::int64_t>(idx_handle);
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
      const std::size_t current_pc = ctx.pc + result.instructions_executed;
      if (policy_hook && !policy_hook(current_pc, insn, result.instructions_executed)) {
        result.exit_kind = ExitKind::PolicyDeny;
        return result;
      }
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
          ctx.registers[insn.a] = ctx.registers[insn.b] + ctx.registers[insn.c];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Sub:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[insn.b] - ctx.registers[insn.c];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mul:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[insn.b] * ctx.registers[insn.c];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Div:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.registers[insn.c] == 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[insn.b] / ctx.registers[insn.c];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mod:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.registers[insn.c] == 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[insn.b] % ctx.registers[insn.c];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Inc:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a]++;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Dec:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a]--;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Mov:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[insn.b];
          ctx.register_tags[insn.a] = ctx.register_tags[insn.b];
          break;
        case t81::tisc::Opcode::Neg:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = -ctx.registers[insn.b];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::TNot: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          int t = clamp_trit(ctx.registers[insn.b]);
          ctx.registers[insn.a] = -t;
          ctx.register_tags[insn.a] = ValueTag::Int;
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
          int lhs = clamp_trit(ctx.registers[insn.b]);
          int rhs = clamp_trit(ctx.registers[insn.c]);
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
          ctx.registers[insn.a] = out;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::LoadImm:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = insn.b;
          switch (insn.literal_kind) {
            case t81::tisc::LiteralKind::Int:
              ctx.register_tags[insn.a] = ValueTag::Int;
              break;
            case t81::tisc::LiteralKind::Bool:
              ctx.register_tags[insn.a] = ValueTag::Bool;
              break;
            case t81::tisc::LiteralKind::FloatHandle:
              ctx.register_tags[insn.a] = ValueTag::FloatHandle;
              break;
            case t81::tisc::LiteralKind::FractionHandle:
              ctx.register_tags[insn.a] = ValueTag::FractionHandle;
              break;
            case t81::tisc::LiteralKind::SymbolHandle:
              ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
              if (insn.b > 0 && static_cast<std::size_t>(insn.b) <= state.symbols.size()) {
                const auto& symbol = state.symbols[static_cast<std::size_t>(insn.b - 1)];
                if (symbol == "std.sys.proof") {
                  ctx.register_tags[insn.a] = ValueTag::ProofHandle;
                  ctx.registers[insn.a] = 1;
                } else if (symbol == "std.io.stream") {
                  ctx.register_tags[insn.a] = ValueTag::IoStreamHandle;
                  ctx.registers[insn.a] = 1;
                } else if (symbol == "std.io.net") {
                  ctx.register_tags[insn.a] = ValueTag::IoNetHandle;
                  ctx.registers[insn.a] = 1;
                } else if (symbol == "std.async.thread") {
                  ctx.register_tags[insn.a] = ValueTag::AsyncThreadHandle;
                  ctx.registers[insn.a] = 1;
                } else if (symbol == "std.async.promise") {
                  ctx.register_tags[insn.a] = ValueTag::AsyncPromiseHandle;
                  ctx.registers[insn.a] = 1;
                }
              }
              break;
            case t81::tisc::LiteralKind::TensorHandle:
              ctx.register_tags[insn.a] = ValueTag::TensorHandle;
              break;
            case t81::tisc::LiteralKind::ShapeHandle:
              ctx.register_tags[insn.a] = ValueTag::ShapeHandle;
              break;
            case t81::tisc::LiteralKind::BigIntHandle:
              // Keep >64-bit literal semantics in interpreter path for now.
              stop_trace = true;
              guard_deopt = true;
              break;
          }
          break;
        case t81::tisc::Opcode::Load:
          if (!reg_ok(insn.a) || !mem_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = state.memory[static_cast<std::size_t>(insn.b)];
          ctx.register_tags[insn.a] = state.memory_tags[static_cast<std::size_t>(insn.b)];
          break;
        case t81::tisc::Opcode::Int2BigInt:
          stop_trace = true;
          guard_deopt = true;
          break;
        case t81::tisc::Opcode::Store:
          if (!reg_ok(insn.b) || !mem_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          state.memory[static_cast<std::size_t>(insn.a)] = ctx.registers[insn.b];
          state.memory_tags[static_cast<std::size_t>(insn.a)] = ctx.register_tags[insn.b];
          break;
        case t81::tisc::Opcode::Push: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto& stack = state.layout.stack;
          // Note: using global stack layout limits in JIT for simplicity, or should access ctx
          // limits? For now, assuming stack structure fits. Wait, JIT needs to use ctx.sp and
          // stack_limit.
          if (ctx.sp <=
              ctx.stack_limit) {  // Check underflow of free space (overflow of stack usage)
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          // Assuming single segment logic in JIT for now, or updating to match VM.
          // ctx.sp grows down.

          // Original JIT check:
          // if (!stack.valid() || state.sp <= stack.start)
          // New check:
          if (ctx.sp <= ctx.stack_limit) {  // limit is the lower bound (start of segment)
            stop_trace = true;
            guard_deopt = true;
            break;
          }

          std::size_t new_sp = ctx.sp - 1;
          // Global bounds check against stack segment?
          // ctx.stack_limit should be safe.

          ctx.sp = new_sp;
          state.memory[ctx.sp] = ctx.registers[insn.a];
          state.memory_tags[ctx.sp] = ctx.register_tags[insn.a];
          break;
        }
        case t81::tisc::Opcode::Pop: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          // Check against stack_base (upper bound)
          if (ctx.sp >= ctx.stack_base) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = state.memory[ctx.sp];
          ctx.register_tags[insn.a] = state.memory_tags[ctx.sp];
          ctx.sp += 1;
          break;
        }
        case t81::tisc::Opcode::MakeOptionSome:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] =
              intern_option(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          break;
        case t81::tisc::Opcode::MakeOptionNone:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = intern_option(false, ValueTag::Int, 0);
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          break;
        case t81::tisc::Opcode::MakeResultOk:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] =
              intern_result(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.register_tags[insn.a] = ValueTag::ResultHandle;
          break;
        case t81::tisc::Opcode::MakeResultErr:
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] =
              intern_result(false, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.register_tags[insn.a] = ValueTag::ResultHandle;
          break;
        case t81::tisc::Opcode::OptionIsSome: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* opt = option_ptr(ctx.registers[insn.b]);
          if (!opt) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = opt->has_value ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::OptionUnwrap: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* opt = option_ptr(ctx.registers[insn.b]);
          if (!opt || !opt->has_value) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = opt->payload;
          ctx.register_tags[insn.a] = opt->payload_tag;
          break;
        }
        case t81::tisc::Opcode::ResultIsOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (!res) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = res->is_ok ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (!res || !res->is_ok) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = res->payload;
          ctx.register_tags[insn.a] = res->payload_tag;
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapErr: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (!res || res->is_ok) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = res->payload;
          ctx.register_tags[insn.a] = res->payload_tag;
          break;
        }
        case t81::tisc::Opcode::MakeEnumVariant:
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
          ctx.register_tags[insn.a] = ValueTag::EnumHandle;
          break;
        case t81::tisc::Opcode::MakeEnumVariantPayload:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || insn.c < 0) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = intern_enum(static_cast<int>(insn.c), true,
                                              ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.register_tags[insn.a] = ValueTag::EnumHandle;
          break;
        case t81::tisc::Opcode::EnumIsVariant: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* val = enum_ptr(ctx.registers[insn.b]);
          if (!val) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (val->variant_id == insn.c) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::EnumUnwrapPayload: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto* val = enum_ptr(ctx.registers[insn.b]);
          if (!val || !val->has_payload) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = val->payload;
          ctx.register_tags[insn.a] = val->payload_tag;
          break;
        }
        case t81::tisc::Opcode::MakeComplex:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c) ||
              ctx.register_tags[insn.b] != ValueTag::Int ||
              ctx.register_tags[insn.c] != ValueTag::Int) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = intern_complex(ctx.registers[insn.b], ctx.registers[insn.c]);
          ctx.register_tags[insn.a] = ValueTag::ComplexHandle;
          break;
        case t81::tisc::Opcode::TMatMul: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
              ctx.register_tags[insn.c] != ValueTag::TensorHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto ta = tensor_ptr(ctx.registers[insn.b]);
          auto tb = tensor_ptr(ctx.registers[insn.c]);
          if (!ta || !tb || ta->rank() != 2 || tb->rank() != 2 ||
              ta->shape()[1] != tb->shape()[0]) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = alloc_tensor(t81::ops::matmul(*ta, *tb));
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TRMSNorm: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
              ctx.register_tags[insn.c] != ValueTag::TensorHandle) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          auto t = tensor_ptr(ctx.registers[insn.b]);
          auto w = tensor_ptr(ctx.registers[insn.c]);
          if (!t || !w || t->rank() == 0 || w->rank() != 1 || w->shape()[0] != t->shape().back()) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = alloc_tensor(t81::ops::rmsnorm(*t, *w));
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::Less:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] < ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::LessEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] <= ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Greater:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] > ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::GreaterEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] >= ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Equal:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] == ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::NotEqual:
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = (ctx.registers[insn.b] != ctx.registers[insn.c]) ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        case t81::tisc::Opcode::Cmp: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::Int ||
              ctx.register_tags[insn.b] != ValueTag::Int) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto lhs = ctx.registers[insn.a];
          const auto rhs = ctx.registers[insn.b];
          ctx.flags.zero = (lhs == rhs);
          ctx.flags.negative = (lhs < rhs);
          ctx.flags.positive = (lhs > rhs);
          break;
        }
        case t81::tisc::Opcode::SetF: {
          if (!reg_ok(insn.a)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::int64_t flag_value = 0;
          if (ctx.flags.negative) {
            flag_value = -1;
          } else if (!ctx.flags.zero) {
            flag_value = 1;
          }
          ctx.registers[insn.a] = flag_value;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::BitAnd:
        case t81::tisc::Opcode::BitOr:
        case t81::tisc::Opcode::BitXor: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::int64_t v = 0;
          if (insn.opcode == t81::tisc::Opcode::BitAnd) {
            v = ctx.registers[insn.b] & ctx.registers[insn.c];
          } else if (insn.opcode == t81::tisc::Opcode::BitOr) {
            v = ctx.registers[insn.b] | ctx.registers[insn.c];
          } else {
            v = ctx.registers[insn.b] ^ ctx.registers[insn.c];
          }
          ctx.registers[insn.a] = v;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::BitNot: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          ctx.registers[insn.a] = ~ctx.registers[insn.b];
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::BitShl:
        case t81::tisc::Opcode::BitShr:
        case t81::tisc::Opcode::BitUShr: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const std::int64_t value = ctx.registers[insn.b];
          const std::int64_t amount = ctx.registers[insn.c] & 0x3F;
          std::int64_t out = 0;
          if (insn.opcode == t81::tisc::Opcode::BitShl) {
            out = value << amount;
          } else if (insn.opcode == t81::tisc::Opcode::BitShr) {
            out = value >> amount;
          } else {
            out = static_cast<std::int64_t>(static_cast<std::uint64_t>(value) >> amount);
          }
          ctx.registers[insn.a] = out;
          ctx.register_tags[insn.a] = ValueTag::Int;
          break;
        }
        case t81::tisc::Opcode::Jump:
          ctx.pc = static_cast<size_t>(insn.a);
          stop_trace = true;
          result.exit_kind = ExitKind::Branch;
          break;
        case t81::tisc::Opcode::JumpIfZero:
          if (!reg_ok(insn.b)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.registers[insn.b] == 0) {
            ctx.pc = static_cast<size_t>(insn.a);
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
          if (ctx.registers[insn.b] != 0) {
            ctx.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::JumpIfNegative:
          if (ctx.flags.negative) {
            ctx.pc = static_cast<size_t>(insn.a);
            stop_trace = true;
            result.exit_kind = ExitKind::Branch;
          }
          break;
        case t81::tisc::Opcode::JumpIfPositive:
          if (ctx.flags.positive) {
            ctx.pc = static_cast<size_t>(insn.a);
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
          const auto target = ctx.registers[insn.b];
          if (!code_ok(target)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }

          if (ctx.sp <= ctx.stack_limit) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          std::size_t new_sp = ctx.sp - 1;
          // Global check? For now simplified.

          ctx.sp = new_sp;
          // Interpreter pushes the post-call PC.
          state.memory[ctx.sp] = static_cast<std::int64_t>(ctx.pc + result.instructions_executed);
          state.memory_tags[ctx.sp] = ValueTag::Int;
          ctx.call_depth += 1;
          ctx.pc = static_cast<std::size_t>(target);
          stop_trace = true;
          result.exit_kind = ExitKind::Branch;
          break;
        }
        case t81::tisc::Opcode::Ret: {
          if (ctx.sp >= ctx.stack_base) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          const auto addr = state.memory[ctx.sp];
          const auto tag = state.memory_tags[ctx.sp];
          ctx.sp += 1;
          if (tag != ValueTag::Int || !code_ok(addr)) {
            stop_trace = true;
            guard_deopt = true;
            break;
          }
          if (ctx.call_depth > 0) {
            ctx.call_depth -= 1;
          }
          ctx.pc = static_cast<std::size_t>(addr);
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
        ctx.flags.zero = (ctx.registers[insn.a] == 0);
        ctx.flags.negative = (ctx.registers[insn.a] < 0);
        ctx.flags.positive = (ctx.registers[insn.a] > 0);
      }
      if (stop_trace) {
        if (guard_deopt) {
          if (result.instructions_executed > 0) {
            // Resume interpreter at the first non-executed instruction.
            ctx.pc += (result.instructions_executed - 1);
          }
          result.exit_kind = ExitKind::GuardDeopt;
        }
        return result;
      }
    }
    ctx.pc += result.instructions_executed;
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
    case t81::tisc::Opcode::BitAnd:
    case t81::tisc::Opcode::BitOr:
    case t81::tisc::Opcode::BitXor:
    case t81::tisc::Opcode::BitNot:
    case t81::tisc::Opcode::BitShl:
    case t81::tisc::Opcode::BitShr:
    case t81::tisc::Opcode::BitUShr:
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
    case t81::tisc::Opcode::Jump:
    case t81::tisc::Opcode::JumpIfZero:
    case t81::tisc::Opcode::JumpIfNotZero:
    case t81::tisc::Opcode::JumpIfNegative:
    case t81::tisc::Opcode::JumpIfPositive:
      trace_buffer_.push_back(insn);
      tracing_ = false;  // Always stop at branch.
      break;
    case t81::tisc::Opcode::Call:
    case t81::tisc::Opcode::Ret:
      // Exclude Call/Ret from JIT to force interpreter fallback
      // for recursion checks and promotion logic.
      tracing_ = false;
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
