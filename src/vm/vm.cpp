#include "t81/vm/vm.hpp"

#include <memory>

namespace t81::vm {
namespace {
class Interpreter : public IVirtualMachine {
 public:
  void load_program(const t81::tisc::Program& program) override {
    program_ = program;
    state_ = State{};
    state_.layout.code_limit = program_.insns.size();
    state_.layout.stack_limit = state_.layout.code_limit + 256;
    state_.layout.heap_limit = state_.layout.stack_limit + 768;
    state_.memory.resize(state_.layout.heap_limit, 0);
    state_.sp = state_.layout.stack_limit;
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
    auto update_flags = [this](std::int64_t v) {
      state_.flags.zero = (v == 0);
      state_.flags.negative = (v < 0);
    };

    auto clamp_trit = [](std::int64_t v) -> int {
      if (v > 0) return 1;
      if (v < 0) return -1;
      return 0;
    };

    Trap trap = Trap::None;
    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop:
        break;
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        break;
      case t81::tisc::Opcode::LoadImm:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = insn.b;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Mov:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b];
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Inc:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] += 1;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Dec:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] -= 1;
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Add:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] + state_.registers[insn.c];
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Sub:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] - state_.registers[insn.c];
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Load:
        if (!reg_ok(insn.a) || !mem_ok(insn.b)) { trap = Trap::InvalidMemory; break; }
        state_.registers[insn.a] = state_.memory[static_cast<std::size_t>(insn.b)];
        update_flags(state_.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Store:
        if (!reg_ok(insn.b) || !mem_ok(insn.a)) { trap = Trap::InvalidMemory; break; }
        state_.memory[static_cast<std::size_t>(insn.a)] = state_.registers[insn.b];
        break;
      case t81::tisc::Opcode::Mul:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { trap = Trap::IllegalInstruction; break; }
        state_.registers[insn.a] = state_.registers[insn.b] * state_.registers[insn.c];
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
      case t81::tisc::Opcode::Cmp:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        {
          auto lhs = state_.registers[insn.a];
          auto rhs = state_.registers[insn.b];
          state_.flags.zero = (lhs == rhs);
          state_.flags.negative = (lhs < rhs);
        }
        break;
      case t81::tisc::Opcode::Push:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (state_.layout.stack_limit <= state_.layout.code_limit) { trap = Trap::BoundsFault; break; }
        if (state_.sp == state_.layout.code_limit) { trap = Trap::BoundsFault; break; }
        --state_.sp;
        state_.memory[state_.sp] = state_.registers[insn.a];
        break;
      case t81::tisc::Opcode::Pop:
        if (!reg_ok(insn.a)) { trap = Trap::IllegalInstruction; break; }
        if (state_.sp == state_.layout.stack_limit) { trap = Trap::BoundsFault; break; }
        state_.registers[insn.a] = state_.memory[state_.sp];
        update_flags(state_.registers[insn.a]);
        ++state_.sp;
        break;
      case t81::tisc::Opcode::TNot:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) { trap = Trap::IllegalInstruction; break; }
        {
          int t = clamp_trit(state_.registers[insn.b]);
          state_.registers[insn.a] = -t;
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
          update_flags(state_.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::AxRead:
      case t81::tisc::Opcode::AxSet:
      case t81::tisc::Opcode::AxVerify:
        trap = Trap::IllegalInstruction;
        break;
      default:
        trap = Trap::IllegalInstruction;
        break;
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
  State state_{};
  t81::tisc::Program program_{};
};
}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm() {
  return std::make_unique<Interpreter>();
}
}  // namespace t81::vm
