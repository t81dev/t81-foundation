#include <algorithm>
#include <memory>

#include "test_runtime_check.hpp"

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

int main() {
  t81::tisc::Program program;
  program.insns.push_back({t81::tisc::Opcode::ATTN, 0, 1, static_cast<std::int32_t>(2 | (3 << 8))});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto engine = std::make_unique<t81::axion::AIHookEngine>(
      std::make_unique<t81::axion::PolicyEngine>(t81::axion::Policy{}));
  auto vm = t81::vm::make_interpreter_vm(std::move(engine));
  vm->load_program(program);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  const auto& log = vm->state().axion_log;
  T81_TEST_CHECK(!log.empty());

  const bool saw_ai_deny = std::any_of(log.begin(), log.end(), [](const auto& event) {
    return event.opcode == t81::tisc::Opcode::ATTN &&
           event.verdict.kind == t81::axion::VerdictKind::Deny &&
           event.structured.decision == "deny" &&
           event.structured.reason == event.verdict.reason &&
           event.structured.reason.find("attn_guard") != std::string::npos;
  });
  T81_TEST_CHECK(saw_ai_deny);

  return 0;
}
