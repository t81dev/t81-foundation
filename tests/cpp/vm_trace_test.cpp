#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  tisc::Program p;
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
  p.insns.push_back({tisc::Opcode::Load, 1, 9999, 0}); // invalid -> trap

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto r1 = vm->step();
  assert(r1.has_value());
  auto r2 = vm->step();
  assert(!r2.has_value());
  assert(r2.error() == vm::Trap::InvalidMemory || r2.error() == vm::Trap::IllegalInstruction);
  assert(!vm->state().trace.empty());
  auto last = vm->state().trace.back();
  assert(last.trap.has_value());
  return 0;
}
