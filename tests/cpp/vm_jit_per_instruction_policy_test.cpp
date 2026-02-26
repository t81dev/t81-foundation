#include <iostream>
#include <string>

#include "t81/axion/engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {
class DenyStepAfterThresholdEngine final : public t81::axion::Engine {
public:
  explicit DenyStepAfterThresholdEngine(std::size_t deny_after) : deny_after_(deny_after) {}

  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    if (ctx.syscall == t81::axion::reasons::kStep && ctx.instruction_count >= deny_after_) {
      return {t81::axion::VerdictKind::Deny,
              "deny-step-threshold instruction_count=" + std::to_string(ctx.instruction_count)};
    }
    return {t81::axion::VerdictKind::Allow, "allow"};
  }

private:
  std::size_t deny_after_;
};
}  // namespace

int main() {
  using t81::tisc::Insn;
  using t81::tisc::LiteralKind;
  using t81::tisc::Opcode;

  auto engine = std::make_unique<DenyStepAfterThresholdEngine>(260);
  auto vm = t81::vm::make_interpreter_vm(std::move(engine));

  t81::tisc::Program program;
  // R0 counter, R1 step=1, R2 limit=500, R3 cond
  program.insns.push_back({Opcode::LoadImm, 0, 0, 0, LiteralKind::Int});        // pc=0
  program.insns.push_back({Opcode::LoadImm, 1, 1, 0, LiteralKind::Int});        // pc=1
  program.insns.push_back({Opcode::LoadImm, 2, 500, 0, LiteralKind::Int});      // pc=2
  program.insns.push_back({Opcode::Add, 0, 0, 1, LiteralKind::Int});            // pc=3 loop
  program.insns.push_back({Opcode::Less, 3, 0, 2, LiteralKind::Int});           // pc=4
  program.insns.push_back({Opcode::JumpIfNotZero, 3, 3, 0, LiteralKind::Int});  // pc=5 -> 3
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});           // pc=6

  vm->load_program(program);
  const auto run = vm->run_to_halt(5000);
  if (run.has_value()) {
    std::cerr << "vm_jit_per_instruction_policy_test failure: expected SecurityFault trap\n";
    return 1;
  }
  if (run.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "vm_jit_per_instruction_policy_test failure: expected SecurityFault, got "
              << static_cast<int>(run.error()) << "\n";
    return 1;
  }

  bool saw_trace_enter = false;
  for (const auto& e : vm->state().axion_log) {
    if (e.verdict.reason.find("jit trace enter") != std::string::npos) {
      saw_trace_enter = true;
      break;
    }
  }
  if (!saw_trace_enter) {
    std::cerr
        << "vm_jit_per_instruction_policy_test failure: trace path did not engage before deny\n";
    return 1;
  }

  std::cout << "vm jit per-instruction policy test passed\n";
  return 0;
}
