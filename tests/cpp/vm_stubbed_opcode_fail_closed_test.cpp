#include "test_runtime_check.hpp"

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string_view>

// Consolidated fail-closed test for all stubbed opcodes.
// Merges vm_stubbed_async_network_opcode_fail_closed_test and
// vm_stubbed_privileged_opcode_fail_closed_test into a single parameterized harness.
// Each case must produce SecurityFault and log the expected denial reason.

namespace {

// --- Async / network opcodes ---

void run_async_fail_closed(t81::tisc::Insn insn, std::string_view opname) {
  t81::tisc::Program program;
  program.insns.push_back(insn);
  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns.push_back(halt);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();

  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.opcode == insn.opcode &&
        event.verdict.reason.find("Blocked: unimplemented async/network opcode") !=
            std::string::npos) {
      saw = true;
      break;
    }
  }
  if (!saw) std::cerr << "Missing deny-log reason for " << opname << "\n";
  T81_TEST_CHECK(saw);
}

// --- Privileged Axion opcodes ---

void run_privileged_fail_closed(t81::tisc::Opcode opcode, std::string_view opname) {
  t81::tisc::Program program;
  t81::tisc::Insn insn{};
  insn.opcode = opcode;
  program.insns.push_back(insn);
  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns.push_back(halt);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();

  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.opcode == opcode &&
        event.verdict.reason.find("Blocked: unimplemented privileged Axion opcode") !=
            std::string::npos) {
      saw = true;
      break;
    }
  }
  if (!saw) std::cerr << "Missing deny-log reason for " << opname << "\n";
  T81_TEST_CHECK(saw);
}

}  // namespace

int main() {
  // Async / network opcodes
  { t81::tisc::Insn i{}; i.opcode = t81::tisc::Opcode::NSend;  i.b = 1;      run_async_fail_closed(i, "NSEND");  }
  { t81::tisc::Insn i{}; i.opcode = t81::tisc::Opcode::NRecv;  i.a = 1;      run_async_fail_closed(i, "NRECV");  }
  { t81::tisc::Insn i{}; i.opcode = t81::tisc::Opcode::VWait;  i.a = 1; i.b = 2; run_async_fail_closed(i, "VWAIT");  }
  { t81::tisc::Insn i{}; i.opcode = t81::tisc::Opcode::VYield; i.b = 1;      run_async_fail_closed(i, "VYIELD"); }

  // Privileged Axion opcodes
  run_privileged_fail_closed(t81::tisc::Opcode::AxSign,    "AXSIGN");
  run_privileged_fail_closed(t81::tisc::Opcode::AxLineage, "AXLINEAGE");
  run_privileged_fail_closed(t81::tisc::Opcode::AxCanon,   "AXCANON");

  return 0;
}
