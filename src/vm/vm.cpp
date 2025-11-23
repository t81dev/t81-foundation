#include "t81/vm/vm.hpp"

#include <algorithm>
#include <memory>

namespace t81::vm {
namespace {
class Interpreter : public IVirtualMachine {
 public:
  void load_program(const t81::tisc::Program& program) override {
    program_ = program;
    state_ = State{};
    // Layout: code [0, code_limit), stack [code_limit, stack_limit), heap [stack_limit, heap_limit)
    state_.layout.code_limit = program_.insns.size();
    state_.layout.stack_limit = state_.layout.code_limit + 256;
    state_.layout.heap_limit = state_.layout.stack_limit + 768;
    state_.memory.resize(state_.layout.heap_limit, 0);
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
    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop:
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::LoadImm:
        if (!reg_ok(insn.a)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        state_.registers[insn.a] = insn.b;
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Add:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        state_.registers[insn.a] = state_.registers[insn.b] + state_.registers[insn.c];
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Sub:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        state_.registers[insn.a] = state_.registers[insn.b] - state_.registers[insn.c];
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Load:
        if (!reg_ok(insn.a) || !mem_ok(insn.b)) { log_trace(insn.opcode, Trap::InvalidMemory); return Trap::InvalidMemory; }
        state_.registers[insn.a] = state_.memory[static_cast<std::size_t>(insn.b)];
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Store:
        if (!reg_ok(insn.b) || !mem_ok(insn.a)) { log_trace(insn.opcode, Trap::InvalidMemory); return Trap::InvalidMemory; }
        state_.memory[static_cast<std::size_t>(insn.a)] = state_.registers[insn.b];
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Mul:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        state_.registers[insn.a] = state_.registers[insn.b] * state_.registers[insn.c];
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        auto divisor = state_.registers[insn.c];
        if (divisor == 0) { log_trace(insn.opcode, Trap::DivideByZero); return Trap::DivideByZero; }
        auto lhs = state_.registers[insn.b];
        if (insn.opcode == t81::tisc::Opcode::Div) {
          state_.registers[insn.a] = lhs / divisor;
        } else {
          state_.registers[insn.a] = lhs % divisor;
        }
        log_trace(insn.opcode);
        break;
      }
      case t81::tisc::Opcode::Jump:
        if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        state_.pc = static_cast<std::size_t>(insn.a);
        log_trace(insn.opcode);
        break;
      case t81::tisc::Opcode::JumpIfZero:
        if (!reg_ok(insn.b)) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
        if (state_.registers[insn.b] == 0) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) { log_trace(insn.opcode, Trap::IllegalInstruction); return Trap::IllegalInstruction; }
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        log_trace(insn.opcode);
        break;
      default:
        log_trace(insn.opcode, Trap::IllegalInstruction);
        return Trap::IllegalInstruction;
    }
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
