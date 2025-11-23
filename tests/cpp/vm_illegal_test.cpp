#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  // Invalid register index
  tisc::Program bad_reg;
  bad_reg.insns.push_back({tisc::Opcode::LoadImm, 99, 1, 0});
  auto vm = vm::make_interpreter_vm();
  vm->load_program(bad_reg);
  auto r = vm->step();
  assert(!r.has_value());
  assert(r.error() == vm::Trap::IllegalInstruction);

  // Jump out of bounds
  tisc::Program bad_jump;
  bad_jump.insns.push_back({tisc::Opcode::Jump, 5, 0, 0});
  vm->load_program(bad_jump);
  r = vm->step();
  assert(!r.has_value());
  assert(r.error() == vm::Trap::IllegalInstruction);

  // Unknown opcode
  tisc::Insn bogus{static_cast<tisc::Opcode>(99), 0, 0, 0};
  tisc::Program bad_op;
  bad_op.insns.push_back(bogus);
  vm->load_program(bad_op);
  r = vm->step();
  assert(!r.has_value());
  assert(r.error() == vm::Trap::IllegalInstruction);

  return 0;
}
