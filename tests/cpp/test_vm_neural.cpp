#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <vector>
#include "test_runtime_check.hpp"

namespace {
t81::vm::Trap run_until_trap(const std::vector<t81::tisc::Insn>& insns) {
  t81::tisc::Program program;
  program.insns = insns;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    return result.error();
  }
  return t81::vm::Trap::None;
}
}  // namespace

int main() {
  // Test TNeuralFwd
  {
    std::vector<t81::tisc::Insn> prog;

    // R1 = 10 (Size)
    t81::tisc::Insn load_size;
    load_size.opcode = t81::tisc::Opcode::LoadImm;
    load_size.a = 1;
    load_size.b = 10;
    prog.push_back(load_size);

    // R2 = TNew(R1)
    t81::tisc::Insn tnew;
    tnew.opcode = t81::tisc::Opcode::TNew;
    tnew.a = 2; // Dest handle
    tnew.b = 1; // Size
    prog.push_back(tnew);

    // R3 = TNeuralFwd(R2)
    t81::tisc::Insn fwd;
    fwd.opcode = t81::tisc::Opcode::TNeuralFwd;
    fwd.a = 3; // Dest
    fwd.b = 2; // Src
    prog.push_back(fwd);

    t81::tisc::Insn halt;
    halt.opcode = t81::tisc::Opcode::Halt;
    prog.push_back(halt);

    t81::vm::Trap t = run_until_trap(prog);

    if (t == t81::vm::Trap::None) {
        std::cout << "TNeuralFwd passed\n";
    } else {
        std::cerr << "TNeuralFwd failed: " << to_string(t) << "\n";
        return 1;
    }
  }

  // Test TNeuralBwd
  {
    std::vector<t81::tisc::Insn> prog;

    // R1 = 10 (Size)
    t81::tisc::Insn load_size;
    load_size.opcode = t81::tisc::Opcode::LoadImm;
    load_size.a = 1;
    load_size.b = 10;
    prog.push_back(load_size);

    // R2 = TNew(R1)
    t81::tisc::Insn tnew;
    tnew.opcode = t81::tisc::Opcode::TNew;
    tnew.a = 2; // Dest handle
    tnew.b = 1; // Size
    prog.push_back(tnew);

    // TNeuralBwd(R2)
    t81::tisc::Insn bwd;
    bwd.opcode = t81::tisc::Opcode::TNeuralBwd;
    bwd.a = 2; // Model/Weights
    prog.push_back(bwd);

    t81::tisc::Insn halt;
    halt.opcode = t81::tisc::Opcode::Halt;
    prog.push_back(halt);

    t81::vm::Trap t = run_until_trap(prog);

    if (t == t81::vm::Trap::None) {
        std::cout << "TNeuralBwd passed\n";
    } else {
        std::cerr << "TNeuralBwd failed: " << to_string(t) << "\n";
        return 1;
    }
  }

  return 0;
}
