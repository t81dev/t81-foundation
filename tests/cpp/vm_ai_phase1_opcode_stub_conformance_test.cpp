#include "test_runtime_check.hpp"

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string_view>

namespace {

void run_fail_closed_case(t81::tisc::Opcode opcode, std::string_view opname) {
  t81::tisc::Program program;
  program.insns.push_back({opcode, 1, 2, 3});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();

  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw_reason = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.opcode != opcode) {
      continue;
    }
    if (event.verdict.reason.find("Blocked: unimplemented AI phase1 opcode") != std::string::npos) {
      saw_reason = true;
      break;
    }
  }
  if (!saw_reason) {
    std::cerr << "Missing AI phase1 fail-closed deny reason for " << opname << "\n";
  }
  T81_TEST_CHECK(saw_reason);
}

}  // namespace

int main() {
  run_fail_closed_case(t81::tisc::Opcode::QMATMUL, "QMATMUL");
  return 0;
}
