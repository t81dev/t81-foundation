#include <iostream>
#include <string>
#include <vector>
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

using namespace t81;
using namespace t81::vm;

int main() {
  auto vm = make_interpreter_vm();

  // Create a program with code for two threads
  // Thread 0: R1 = 1, Loop increment R1
  // Thread 1: R1 = 2, Loop increment R1
  t81::tisc::Program program;

  // Code for Thread 0 (starts at 0)
  // 0: LoadImm R1, 100
  // 1: Inc R1
  // 2: Jump 1
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 100, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Inc, 1, 0, 0});
  program.insns.push_back({t81::tisc::Opcode::Jump, 1, 0, 0});

  // Code for Thread 1 (starts at 3)
  // 3: LoadImm R1, 200
  // 4: Inc R1
  // 5: Jump 4
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 200, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Inc, 1, 0, 0});
  program.insns.push_back({t81::tisc::Opcode::Jump, 4, 0, 0});

  vm->load_program(program);

  // Manually inject a second context
  auto& state = const_cast<State&>(vm->state());
  state.contexts.emplace_back();
  auto& ctx1 = state.contexts.back();
  ctx1.pc = 3;  // Start at instruction 3
  ctx1.register_tags.fill(ValueTag::Int);

  // Allocate stack for ctx1 (simple partition)
  // Default stack is 256 size. ctx0 has it all.
  // We'll split it.
  auto& ctx0 = state.contexts[0];
  ctx0.stack_base = state.layout.stack.limit;
  ctx0.stack_limit = state.layout.stack.limit - 128;  // Top half
  ctx0.sp = ctx0.stack_base;

  ctx1.stack_base = state.layout.stack.limit - 128;
  ctx1.stack_limit = state.layout.stack.start;  // Bottom half
  ctx1.sp = ctx1.stack_base;

  // Run for a few steps
  // Scheduler should alternate: Ctx0 (PC=0), Ctx1 (PC=3), Ctx0 (PC=1), Ctx1 (PC=4) ...

  // Step 1: Ctx0 executes PC=0 (LoadImm R1, 100) -> Next PC=1
  // Post-step: switch to Ctx1
  (void)vm->step();
  T81_TEST_CHECK(state.contexts[0].registers[1] == 100);
  T81_TEST_CHECK(state.contexts[0].pc == 1);
  T81_TEST_CHECK(state.current_context == 1);  // Switched to 1

  // Step 2: Ctx1 executes PC=3 (LoadImm R1, 200) -> Next PC=4
  // Post-step: switch to Ctx0
  (void)vm->step();
  T81_TEST_CHECK(state.contexts[1].registers[1] == 200);
  T81_TEST_CHECK(state.contexts[1].pc == 4);
  T81_TEST_CHECK(state.current_context == 0);  // Switched to 0

  // Step 3: Ctx0 executes PC=1 (Inc R1) -> R1=101, Next PC=2
  (void)vm->step();
  T81_TEST_CHECK(state.contexts[0].registers[1] == 101);
  T81_TEST_CHECK(state.contexts[0].pc == 2);
  T81_TEST_CHECK(state.current_context == 1);

  // Step 4: Ctx1 executes PC=4 (Inc R1) -> R1=201, Next PC=5
  (void)vm->step();
  T81_TEST_CHECK(state.contexts[1].registers[1] == 201);
  T81_TEST_CHECK(state.contexts[1].pc == 5);
  T81_TEST_CHECK(state.current_context == 0);

  std::cout << "Concurrency test passed!\n";
  return 0;
}
