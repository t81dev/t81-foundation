#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  // Program: load imm 7 into R0; store to mem[5]; load mem[5] into R1; halt.
  tisc::Program p;
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 7, 0});
  p.insns.push_back({tisc::Opcode::Store, 5, 0, 0});
  p.insns.push_back({tisc::Opcode::Load, 1, 5, 0});
  p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  assert(res.has_value());
  assert(vm->state().memory[5] == 7);
  assert(vm->state().registers[1] == 7);

  // Invalid memory should trap
  tisc::Program bad;
  bad.insns.push_back({tisc::Opcode::Load, 0, 9999, 0});
  bad.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});
  vm->load_program(bad);
  auto step = vm->step();
  assert(!step.has_value());
  assert(step.error() == vm::Trap::InvalidMemory);

  return 0;
}
