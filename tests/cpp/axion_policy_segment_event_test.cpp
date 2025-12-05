#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>

int main() {
  t81::tisc::Program program;
  t81::tisc::Insn stack_alloc{};
  stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
  stack_alloc.a = 0;
  stack_alloc.b = 16;
  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns = {stack_alloc, halt};
  program.axion_policy_text = R"(
(policy
  (tier 1)
  (require-segment-event
    (segment stack)
    (action "stack frame allocated")))
)";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result) {
    std::cerr << "Segment policy run trapped: " << static_cast<int>(result.error()) << '\n';
    return 1;
  }

  auto program_fail = program;
  program_fail.axion_policy_text = R"(
(policy
  (tier 1)
  (require-segment-event
    (segment stack)
    (action "stack frame allocated")
    (addr 9999)))
)";

  auto vm_fail = t81::vm::make_interpreter_vm();
  vm_fail->load_program(program_fail);
  auto fail_result = vm_fail->run_to_halt();
  if (fail_result.has_value()) {
    std::cerr << "Segment policy failure did not trap\n";
    return 1;
  }
  if (fail_result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "Expected security fault, got " << static_cast<int>(fail_result.error()) << '\n';
    return 1;
  }

  return 0;
}
