#include "t81/vm/vm.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"

#include <iostream>
#include <vector>

std::vector<t81::tisc::Insn> make_heap_prog() {
  std::vector<t81::tisc::Insn> insns;
  constexpr int kCount = 70;
  for (int i = 0; i < kCount; ++i) {
    t81::tisc::Insn insn{};
    insn.opcode = t81::tisc::Opcode::LoadImm;
    insn.a = i % 27;
    insn.b = i + 1;
    insn.literal_kind = t81::tisc::LiteralKind::Int;
    insns.push_back(insn);
  }
  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  insns.push_back(halt);
  return insns;
}

int main() {
  auto make_program = []() {
    t81::tisc::Program program;
    program.insns = make_heap_prog();
    return program;
  };

  auto program_ok = make_program();
  program_ok.axion_policy_text = R"(
(policy
  (tier 1)
  (require-axion-event
    (reason "heap compaction heap_frames=")))
)";
  auto vm_ok = t81::vm::make_interpreter_vm();
  vm_ok->load_program(program_ok);
  auto result = vm_ok->run_to_halt();
  if (!result) {
    std::cerr << "Heap compaction success run trapped: " << static_cast<int>(result.error()) << '\n';
    return 1;
  }

  auto program_fail = make_program();
  program_fail.axion_policy_text = R"(
(policy
  (tier 1)
  (require-axion-event
    (reason "heap compaction missing")))
)";
  auto vm_fail = t81::vm::make_interpreter_vm();
  vm_fail->load_program(program_fail);
  auto fail_result = vm_fail->run_to_halt();
  if (fail_result.has_value()) {
    std::cerr << "Heap compaction failure not trapped\n";
    return 1;
  }
  if (fail_result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "Expected security fault, got " << static_cast<int>(fail_result.error()) << '\n';
    return 1;
  }

  return 0;
}
