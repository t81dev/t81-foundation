#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <vector>
#include "test_runtime_check.hpp"

namespace {
t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
  [[maybe_unused]] t81::tisc::Program program;
  program.insns = insns;
  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  [[maybe_unused]] auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  return result.error();
}

void run_injected_fault_test() {
  // Simple program: LoadImm, LoadImm, Add, Halt
  std::vector<t81::tisc::Insn> insns;

  t81::tisc::Insn i1;
  i1.opcode = t81::tisc::Opcode::LoadImm;
  i1.a = 1;
  i1.b = 10;
  insns.push_back(i1);

  t81::tisc::Insn i2;
  i2.opcode = t81::tisc::Opcode::LoadImm;
  i2.a = 2;
  i2.b = 20;
  insns.push_back(i2);

  t81::tisc::Insn i3;
  i3.opcode = t81::tisc::Opcode::Add;
  i3.a = 3;
  i3.b = 1;
  i3.c = 2;
  insns.push_back(i3);

  t81::tisc::Insn i4;
  i4.opcode = t81::tisc::Opcode::Halt;
  insns.push_back(i4);

  t81::tisc::Program program;
  program.insns = insns;

  // Test 1: Fault at instruction count 2 (should be Add, effectively)
  // Instructions:
  // 0: LoadImm (count becomes 1)
  // 1: LoadImm (count becomes 2)
  // 2: Add     (count becomes 3)
  // 3: Halt

  // Set injection at count 2. This should trigger BEFORE executing the instruction at index 2
  // (Add). Execution flow: Step 1: instruction_count=0. Exec LoadImm (idx 0). instruction_count
  // becomes 1. Step 2: instruction_count=1. Exec LoadImm (idx 1). instruction_count becomes 2. Step
  // 3: instruction_count=2. Fault check matches. Trigger fault. Add (idx 2) is NOT executed.

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  std::vector<t81::vm::FaultInjection> faults;
  faults.push_back({2, t81::vm::Trap::SecurityFault});
  vm->set_fault_injections(faults);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  // Verify state
  // R1 should be 10 (executed)
  // R2 should be 20 (executed)
  // R3 should be 0 (Add NOT executed)
  const auto& state = vm->state();
  T81_TEST_CHECK(state.contexts[0].registers[1] == 10);
  T81_TEST_CHECK(state.contexts[0].registers[2] == 20);
  T81_TEST_CHECK(state.contexts[0].registers[3] == 0);
}

}  // namespace

int main() {
  [[maybe_unused]] t81::tisc::Insn load_ten;
  load_ten.opcode = t81::tisc::Opcode::LoadImm;
  load_ten.a = 2;  // R2
  load_ten.b = 10;
  [[maybe_unused]] t81::tisc::Insn load_zero;
  load_zero.opcode = t81::tisc::Opcode::LoadImm;
  load_zero.a = 1;
  load_zero.b = 0;
  [[maybe_unused]] t81::tisc::Insn div;
  div.opcode = t81::tisc::Opcode::Div;
  div.a = 2;
  div.b = 2;
  div.c = 1;
  [[maybe_unused]] t81::tisc::Insn halt;
  halt.opcode = t81::tisc::Opcode::Halt;
  [[maybe_unused]] auto trap_div_zero = run_expected_trap({load_ten, load_zero, div, halt});
  T81_TEST_CHECK(trap_div_zero == t81::vm::Trap::DivisionFault);

  [[maybe_unused]] t81::tisc::Insn load_bad;
  load_bad.opcode = t81::tisc::Opcode::Load;
  load_bad.a = 1;
  load_bad.b = 999999;
  load_bad.c = 0;
  [[maybe_unused]] auto trap_invalid_mem = run_expected_trap({load_bad, halt});
  T81_TEST_CHECK(trap_invalid_mem == t81::vm::Trap::BoundsFault);

  t81::tisc::Insn pop{t81::tisc::Opcode::Pop, 1, 0, 0};
  [[maybe_unused]] auto trap_bounds = run_expected_trap({pop, halt});
  T81_TEST_CHECK(trap_bounds == t81::vm::Trap::StackFault);

  [[maybe_unused]] t81::tisc::Insn store_bad;
  store_bad.opcode = t81::tisc::Opcode::Store;
  store_bad.a = 999999;
  store_bad.b = 0;
  store_bad.c = 0;
  [[maybe_unused]] auto trap_store_invalid_mem = run_expected_trap({store_bad, halt});
  T81_TEST_CHECK(trap_store_invalid_mem == t81::vm::Trap::BoundsFault);

  [[maybe_unused]] t81::tisc::Insn load_neg;
  load_neg.opcode = t81::tisc::Opcode::Load;
  load_neg.a = 1;
  load_neg.b = -1;
  load_neg.c = 0;
  [[maybe_unused]] auto trap_load_neg = run_expected_trap({load_neg, halt});
  T81_TEST_CHECK(trap_load_neg == t81::vm::Trap::BoundsFault);

  [[maybe_unused]] t81::tisc::Insn store_neg;
  store_neg.opcode = t81::tisc::Opcode::Store;
  store_neg.a = -1;
  store_neg.b = 0;
  store_neg.c = 0;
  [[maybe_unused]] auto trap_store_neg = run_expected_trap({store_neg, halt});
  T81_TEST_CHECK(trap_store_neg == t81::vm::Trap::BoundsFault);

  run_injected_fault_test();

  return 0;
}
