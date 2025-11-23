#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  tisc::Program p;
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
  p.insns.push_back({tisc::Opcode::Store, 2000, 0, 0}); // way past heap_limit

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto r = vm->step(); // load imm ok
  assert(r.has_value());
  r = vm->step();
  assert(!r.has_value());
  assert(r.error() == vm::Trap::InvalidMemory || r.error() == vm::Trap::BoundsFault);

  return 0;
}
