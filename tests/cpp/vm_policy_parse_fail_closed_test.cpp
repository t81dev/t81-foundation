#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <string_view>

#include "test_runtime_check.hpp"

namespace {

void run_policy_parse_fail_closed_test() {
  t81::tisc::Program program;

  t81::tisc::Insn noop{};
  noop.opcode = t81::tisc::Opcode::Nop;
  program.insns.push_back(noop);

  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns.push_back(halt);

  // Missing closing paren: parser should fail and VM must fail closed.
  program.axion_policy_text = "(policy (max-instructions 10)";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw_parse_error = false;
  bool saw_structured_parse_error = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.verdict.reason.find("Axion policy parse failed:") != std::string::npos) {
      saw_parse_error = true;
      if (event.structured.decision == "deny" &&
          event.structured.event_type == "policy_parse_failure" &&
          event.structured.reason_code == "AXION_POLICY_PARSE_FAILED" &&
          event.structured.reason == event.verdict.reason) {
        saw_structured_parse_error = true;
      }
      break;
    }
  }
  T81_TEST_CHECK(saw_parse_error);
  T81_TEST_CHECK(saw_structured_parse_error);
}

void run_policy_unknown_clause_fail_closed_test() {
  t81::tisc::Program program;

  t81::tisc::Insn noop{};
  noop.opcode = t81::tisc::Opcode::Nop;
  program.insns.push_back(noop);

  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns.push_back(halt);

  // Unknown policy clause should be rejected by parser and fail closed in VM.
  program.axion_policy_text = "(policy (tier 1) (unknown-clause 1))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw_parse_error = false;
  bool saw_structured_parse_error = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.verdict.reason.find("Axion policy parse failed:") != std::string::npos) {
      saw_parse_error = true;
      if (event.structured.decision == "deny" &&
          event.structured.event_type == "policy_parse_failure" &&
          event.structured.reason_code == "AXION_POLICY_PARSE_FAILED" &&
          event.structured.reason == event.verdict.reason) {
        saw_structured_parse_error = true;
      }
      break;
    }
  }
  T81_TEST_CHECK(saw_parse_error);
  T81_TEST_CHECK(saw_structured_parse_error);
}

}  // namespace

int main() {
  run_policy_parse_fail_closed_test();
  run_policy_unknown_clause_fail_closed_test();
  return 0;
}
