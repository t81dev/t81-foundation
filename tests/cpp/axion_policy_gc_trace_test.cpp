#include "t81/isa/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>

constexpr int kGcTrigger = 70;

std::vector<t81::tisc::Insn> make_gc_program() {
  [[maybe_unused]] std::vector<t81::tisc::Insn> insns;
  insns.reserve(kGcTrigger + 1);
  for (int i = 0; i < kGcTrigger; ++i) {
    t81::tisc::Insn insn{};
    insn.opcode = t81::tisc::Opcode::LoadImm;
    insn.a = 1;
    insn.b = i;
    insn.literal_kind = t81::tisc::LiteralKind::Int;
    insns.push_back(insn);
  }
  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  insns.push_back(halt);
  return insns;
}

int main() {
  auto build_program = []() {
    [[maybe_unused]] t81::tisc::Program program;
    program.insns = make_gc_program();
    return program;
  };

  [[maybe_unused]] auto program_ok = build_program();
  program_ok.axion_policy_text = R"(
(policy
  (tier 1)
  (require-axion-event
    (reason "GC cycle reason=interval"))
  (require-axion-event
    (reason "heap compaction heap_frames="))
  (require-axion-event
    (reason "heap relocation from=")))
)";
  [[maybe_unused]] auto vm_ok = t81::vm::make_interpreter_vm();
  vm_ok->load_program(program_ok);
  [[maybe_unused]] auto result = vm_ok->run_to_halt();
  if (!result) {
    std::cerr << "GC policy run trapped: " << t81::vm::to_string(result.error()) << '\n';
    for (const auto& event : vm_ok->state().axion_log) {
      std::cerr << "  axion: " << event.verdict.reason << '\n';
    }
    return 1;
  }

  [[maybe_unused]] auto program_fail = build_program();
  program_fail.axion_policy_text = R"(
(policy
  (tier 1)
  (require-axion-event
    (reason "force")))
)";
  [[maybe_unused]] auto vm_fail = t81::vm::make_interpreter_vm();
  vm_fail->load_program(program_fail);
  [[maybe_unused]] auto fail_result = vm_fail->run_to_halt();
  if (fail_result.has_value()) {
    std::cerr << "GC policy failure did not trap\n";
    return 1;
  }
  if (fail_result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "Expected security fault, got " << t81::vm::to_string(fail_result.error())
              << '\n';
    return 1;
  }

  return 0;
}
