#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  // Branch taken when zero; ensures flags and PC update match spec intent.
  {
    tisc::Program p;
    // pc0: r0 = 0 (sets zero flag)
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 0, 0});
    // pc1: if r0 == 0 jump to pc3
    p.insns.push_back({tisc::Opcode::JumpIfZero, 3, 0, 0});
    // pc2: would set r1 = 1 (should be skipped)
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    // pc3: halt
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    [[maybe_unused]] auto r = vm->run_to_halt();
    assert(r.has_value());
    // r1 should remain zero because branch skipped pc2.
    assert(vm->state().registers[1] == 0);
    // zero flag should reflect last result (from LoadImm r0 = 0).
    assert(vm->state().flags.zero);
    assert(!vm->state().flags.negative);
  }

  // Invalid jump target should trap and be logged.
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::Jump, 5, 0, 0}); // target past program

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    [[maybe_unused]] auto r = vm->step();
    assert(!r.has_value());
    assert(r.error() == vm::Trap::IllegalInstruction);
    assert(!vm->state().trace.empty());
    assert(vm->state().trace.back().trap.has_value());
  }

  // Jump-if-nonzero should branch.
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
    p.insns.push_back({tisc::Opcode::JumpIfNotZero, 3, 0, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 99, 0}); // should skip
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    [[maybe_unused]] auto r = vm->run_to_halt();
    assert(r.has_value());
    assert(vm->state().registers[1] == 0);
  }

  // Call/Ret stack handling and Trap instruction.
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 4, 0});  // target index
    p.insns.push_back({tisc::Opcode::Call, 0, 0, 0});     // call function at r0
    p.insns.push_back({tisc::Opcode::LoadImm, 2, 7, 0});  // should run after return
    p.insns.push_back({tisc::Opcode::Trap, 1, 0, 0});     // trigger trap
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 42, 0}); // function body
    p.insns.push_back({tisc::Opcode::Ret, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    [[maybe_unused]] auto r = vm->run_to_halt();
    assert(!r.has_value());
    assert(r.error() == vm::Trap::TrapInstruction);
    assert(vm->state().registers[1] == 42);
    assert(vm->state().registers[2] == 7);
  }

  return 0;
}
