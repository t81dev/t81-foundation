#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  // Program: R0=10, R1=3, R2=DIV R0/R1, R3=MOD R0/R1
  tisc::Program p;
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 10, 0});
  p.insns.push_back({tisc::Opcode::LoadImm, 1, 3, 0});
  p.insns.push_back({tisc::Opcode::Div, 2, 0, 1});
  p.insns.push_back({tisc::Opcode::Mod, 3, 0, 1});
  p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  assert(res.has_value());
  assert(vm->state().registers[2] == 10 / 3);
  assert(vm->state().registers[3] == 10 % 3);

  // Divide by zero should trap
  tisc::Program bad;
  bad.insns.push_back({tisc::Opcode::LoadImm, 0, 5, 0});
  bad.insns.push_back({tisc::Opcode::LoadImm, 1, 0, 0});
  bad.insns.push_back({tisc::Opcode::Div, 2, 0, 1});
  auto vm2 = vm::make_interpreter_vm();
  vm2->load_program(bad);
  auto step = vm2->step();
  assert(step.has_value()); // first load
  step = vm2->step();
  assert(step.has_value()); // second load
  step = vm2->step();
  assert(!step.has_value());
  assert(step.error() == vm::Trap::DivideByZero);

  return 0;
}
