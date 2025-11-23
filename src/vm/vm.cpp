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
    state_.memory.resize(256, 0);
  }

  std::expected<void, Trap> step() override {
    if (state_.halted || state_.pc >= program_.insns.size()) {
      state_.halted = true;
      return {};
    }
    const auto& insn = program_.insns[state_.pc++];
    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop:
        break;
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        break;
      case t81::tisc::Opcode::LoadImm:
        if (static_cast<std::size_t>(insn.a) >= state_.registers.size()) return Trap::IllegalInstruction;
        state_.registers[insn.a] = insn.b;
        break;
      case t81::tisc::Opcode::Add:
        if (static_cast<std::size_t>(insn.a) >= state_.registers.size() ||
            static_cast<std::size_t>(insn.b) >= state_.registers.size() ||
            static_cast<std::size_t>(insn.c) >= state_.registers.size()) {
          return Trap::IllegalInstruction;
        }
        state_.registers[insn.a] = state_.registers[insn.b] + state_.registers[insn.c];
        break;
      case t81::tisc::Opcode::Sub:
        if (static_cast<std::size_t>(insn.a) >= state_.registers.size() ||
            static_cast<std::size_t>(insn.b) >= state_.registers.size() ||
            static_cast<std::size_t>(insn.c) >= state_.registers.size()) {
          return Trap::IllegalInstruction;
        }
        state_.registers[insn.a] = state_.registers[insn.b] - state_.registers[insn.c];
        break;
      case t81::tisc::Opcode::Jump:
        if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) return Trap::IllegalInstruction;
        state_.pc = static_cast<std::size_t>(insn.a);
        break;
      case t81::tisc::Opcode::JumpIfZero:
        if (static_cast<std::size_t>(insn.b) >= state_.registers.size()) return Trap::IllegalInstruction;
        if (state_.registers[insn.b] == 0) {
          if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program_.insns.size()) return Trap::IllegalInstruction;
          state_.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      case t81::tisc::Opcode::Load:
      case t81::tisc::Opcode::Store:
        // Memory operations omitted for brevity; TODO align with spec/t81vm-spec.md.
        return Trap::InvalidMemory;
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

