#include <cassert>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>

using namespace t81;

int main() {
  tisc::Program p;
  for (int i = 0; i < 80; ++i) {
    p.insns.push_back({tisc::Opcode::Nop, 0, 0, 0});
  }
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
  p.insns.push_back({tisc::Opcode::Load, 1, 9999, 0}); // invalid -> trap
  p.axion_policy_text = "(policy (tier 2) (max-stack 1024))";

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
  assert(vm->state().policy.has_value());
  assert(vm->state().policy->tier == 2);
  assert(vm->state().gc_cycles > 0);
  return 0;
}
