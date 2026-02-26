#include "t81/axion/engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <memory>
#include <string>

#include "test_runtime_check.hpp"

namespace {

class DenyAxReportEngine final : public t81::axion::Engine {
public:
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    if (ctx.syscall == t81::axion::reasons::kAxReport) {
      return {t81::axion::VerdictKind::Deny, "policy deny: axreport"};
    }
    return {t81::axion::VerdictKind::Allow, "allow"};
  }
};

}  // namespace

int main() {
  t81::tisc::Program program;
  program.symbol_pool = {"DeniedReport"};
  program.insns.push_back(
      {t81::tisc::Opcode::LoadImm, 1, 1, 0, t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::AxReport, 1, 0, 0});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm(std::make_unique<DenyAxReportEngine>());
  vm->load_program(program);

  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  bool saw_axreport_deny = false;
  for (const auto& event : vm->state().axion_log) {
    if (event.opcode == t81::tisc::Opcode::AxReport &&
        event.verdict.reason.find("policy deny: axreport") != std::string::npos) {
      saw_axreport_deny = true;
      break;
    }
  }
  T81_TEST_CHECK(saw_axreport_deny);
  return 0;
}
