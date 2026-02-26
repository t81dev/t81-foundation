#include <iostream>
#include <optional>
#include <string>

#include "t81/axion/verdict.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "axion_policy_invariants_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

struct RunSummary {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::optional<std::string> parse_error_reason;
  std::string trace_signature;
  std::size_t axion_event_count{0};
};

RunSummary run_with_policy(std::string policy_text) {
  t81::tisc::Program program;
  program.insns.push_back({t81::tisc::Opcode::Nop, 0, 0, 0});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  program.axion_policy_text = std::move(policy_text);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt(128);

  RunSummary out;
  out.ok = result.has_value();
  out.trap = result.has_value() ? t81::vm::Trap::None : result.error();
  out.axion_event_count = vm->state().axion_log.size();
  for (const auto& trace : vm->state().trace) {
    out.trace_signature += std::to_string(trace.pc);
    out.trace_signature += "/";
    out.trace_signature += std::to_string(static_cast<int>(trace.opcode));
    out.trace_signature += "/";
    out.trace_signature +=
        trace.trap.has_value() ? std::to_string(static_cast<int>(trace.trap.value())) : "none";
    out.trace_signature += ";";
  }
  for (const auto& event : vm->state().axion_log) {
    if (!out.parse_error_reason.has_value() &&
        event.verdict.reason.find("Axion policy parse failed:") != std::string::npos) {
      out.parse_error_reason = event.verdict.reason;
    }
  }
  return out;
}

}  // namespace

int main() {
  const std::string deny_policy = R"(
    (policy
      (tier 1)
      (max-instructions 0))
  )";

  RunSummary deny_a = run_with_policy(deny_policy);
  RunSummary deny_b = run_with_policy(deny_policy);
  if (!expect(!deny_a.ok && !deny_b.ok, "deny policy should fail closed")) return 1;
  if (!expect(deny_a.trap == t81::vm::Trap::SecurityFault,
              "deny policy should trap SecurityFault")) {
    return 1;
  }
  if (!expect(deny_b.trap == t81::vm::Trap::SecurityFault,
              "deny policy should trap SecurityFault")) {
    return 1;
  }
  if (!expect(deny_a.trace_signature == deny_b.trace_signature,
              "deny policy trace signature drift across identical runs")) {
    return 1;
  }
  if (!expect(deny_a.axion_event_count == deny_b.axion_event_count,
              "deny policy axion event count drift")) {
    return 1;
  }

  const std::string allow_policy = R"(
    (policy
      (tier 1)
      (max-instructions 8))
  )";
  RunSummary allow_a = run_with_policy(allow_policy);
  RunSummary allow_b = run_with_policy(allow_policy);
  if (!expect(allow_a.ok && allow_b.ok, "allow policy should pass")) return 1;
  if (!expect(allow_a.trace_signature == allow_b.trace_signature,
              "allow policy trace signature drift across identical runs")) {
    return 1;
  }
  if (!expect(allow_a.axion_event_count == allow_b.axion_event_count,
              "allow policy axion event count drift")) {
    return 1;
  }

  const std::string invalid_policy = "(policy (tier 1) (unknown-clause 1))";
  RunSummary invalid_a = run_with_policy(invalid_policy);
  RunSummary invalid_b = run_with_policy(invalid_policy);
  if (!expect(!invalid_a.ok && !invalid_b.ok, "invalid policy must fail closed")) return 1;
  if (!expect(invalid_a.trap == t81::vm::Trap::SecurityFault, "invalid policy trap mismatch A")) {
    return 1;
  }
  if (!expect(invalid_b.trap == t81::vm::Trap::SecurityFault, "invalid policy trap mismatch B")) {
    return 1;
  }
  if (!expect(invalid_a.parse_error_reason.has_value(), "parse error reason missing A")) return 1;
  if (!expect(invalid_b.parse_error_reason.has_value(), "parse error reason missing B")) return 1;
  if (!expect(invalid_a.parse_error_reason.value() == invalid_b.parse_error_reason.value(),
              "parse error reason drift across runs")) {
    return 1;
  }

  return 0;
}
