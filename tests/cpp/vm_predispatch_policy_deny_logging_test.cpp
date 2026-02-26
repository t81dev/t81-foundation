#include <algorithm>
#include <memory>
#include <string>

#include "test_runtime_check.hpp"

#include "t81/axion/engine.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

class DenyAllEngine final : public t81::axion::Engine {
public:
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext&) override {
    return {t81::axion::VerdictKind::Deny, "unit-test predispatch deny"};
  }
};

}  // namespace

int main() {
  t81::tisc::Program program;
  program.insns.push_back({t81::tisc::Opcode::Nop, 0, 0, 0});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm(std::make_unique<DenyAllEngine>());
  vm->load_program(program);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  const auto& log = vm->state().axion_log;
  T81_TEST_CHECK(!log.empty());
  const bool saw_predispatch_deny = std::any_of(log.begin(), log.end(), [](const auto& event) {
    return event.opcode == t81::tisc::Opcode::Nop &&
           event.verdict.kind == t81::axion::VerdictKind::Deny &&
           event.verdict.reason == "unit-test predispatch deny";
  });
  T81_TEST_CHECK(saw_predispatch_deny);

  return 0;
}
