#include <cassert>
#include <iostream>
#include <vector>
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81::tisc;
using namespace t81::vm;

int main() {
  std::cout << "--- T81 Tier 4 Reflection Killer Demo: Self-Patching Agent ---" << std::endl;

  std::vector<Insn> insns = {
      /*  0 */ {Opcode::LoadImm, 1, 10},  // R1 = 10
      /*  1 */ {Opcode::Add, 0, 1, 1},    // R0 = R1 + R1 = 20 (BUG! Should be MUL)
      /*  2 */ {Opcode::Jump, 10},        // Go to Verification
      /*  3 */ {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      /*  7 */ {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},

      /* 10: Verification */
      /* 10 */ {Opcode::LoadImm, 2, 100},       // R2 = 100 (Expected)
      /* 11 */ {Opcode::Equal, 4, 0, 2},        // R4 = (R0 == R2)
      /* 12 */ {Opcode::JumpIfNotZero, 20, 4},  // If R4 != 0 (i.e., R0 == 100), Success!
      /* 13 */ {Opcode::Jump, 30},              // Else, Go to Fixer
      /* 14 */ {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      /* 18 */ {Opcode::Nop},
      {Opcode::Nop},

      /* 20: Success Exit */
      /* 20 */ {Opcode::Halt},
      /* 21 */ {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      /* 25 */ {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      {Opcode::Nop},
      /* 29 */ {Opcode::Nop},

      /* 30: Fixer (Tier 4 Cognition) */
      /* 30 */ {Opcode::Nop},                 // Log: "Starting reflection..."
      /* 31 */ {Opcode::MetaReflect, 10, 0},  // R10 = State Snapshot handle
      /* 32 */ {Opcode::LoadImm, 11, 1},      // R11 = Address of PC 1 (index 1)
      /* 33 */ {Opcode::MetaRead, 12, static_cast<int64_t>(MemorySegmentKind::Code), 11},  // R12 =
                                                                                           // Read
                                                                                           // Opcode
      /* 34 */ {Opcode::LoadImm, 13, static_cast<int64_t>(Opcode::Mul)},  // R13 = Target Opcode
                                                                          // (Mul)
      /* 35 */ {Opcode::MetaRefine, 14, 12, 13},                          // R14 = Refined patch
                                                                          /* 36 */
      {Opcode::MetaWrite, 13, static_cast<int64_t>(MemorySegmentKind::Code), 11},  // COMMIT
                                                                                   // PATCH
      /* 37 */ {Opcode::Jump, 0}  // Retry from start
  };

  Program program;
  program.insns = insns;

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  std::cout << "[Step 1] Initial execution with bug..." << std::endl;
  int steps = 0;
  while (vm->state().contexts[0].pc != 30 && steps < 100) {
    auto res = vm->step();
    if (!res) break;
    steps++;
  }
  std::cout << "  Failure detected: PC=" << vm->state().contexts[0].pc
            << " R0=" << vm->state().contexts[0].registers[0] << " (Expected 100)" << std::endl;

  std::cout << "[Step 2] Tier 4 Fixer running (Reflect -> Diagnose -> Patch)..." << std::endl;
  while (vm->state().contexts[0].pc != 0 && steps < 200) {
    auto res = vm->step();
    if (!res) break;
    steps++;
  }
  std::cout << "  Patch applied successfully. Retrying task..." << std::endl;

  std::cout << "[Step 3] Verification after patch..." << std::endl;
  while (!vm->state().halted && steps < 300) {
    auto res = vm->step();
    if (!res) break;
    steps++;
  }

  std::cout << "  Final result: R0=" << vm->state().contexts[0].registers[0] << " (Success!)"
            << std::endl;
  assert(vm->state().contexts[0].registers[0] == 100);
  assert(vm->state().halted);

  std::cout << "\nAxion Trace (Reflection events):" << std::endl;
  for (const auto& event : vm->state().axion_log) {
    if (event.opcode == Opcode::MetaRead || event.opcode == Opcode::MetaWrite ||
        event.opcode == Opcode::MetaReflect || event.opcode == Opcode::MetaRefine) {
      std::cout << "  " << event.verdict.reason << std::endl;
    }
  }

  return 0;
}
