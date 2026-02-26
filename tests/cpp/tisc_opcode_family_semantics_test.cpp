#include <iostream>
#include <string>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "tisc_opcode_family_semantics_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::expected<t81::vm::State, t81::vm::Trap> run_program(const t81::tisc::Program& p,
                                                         std::size_t steps) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(steps);
  if (!run.has_value()) {
    return std::unexpected(run.error());
  }
  return vm->state();
}

bool check_arithmetic_family() {
  // Spec alignment: spec/tisc/opcode-semantics.md arithmetic integer opcodes.
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 14, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 6, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Sub, 43, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Mul, 44, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Div, 45, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Mod, 46, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto out = run_program(p, 128);
  if (!expect(out.has_value(), "arithmetic program trapped")) return false;
  const auto& regs = out->contexts[0].registers;
  if (!expect(regs[42] == 20, "Add mismatch")) return false;
  if (!expect(regs[43] == 8, "Sub mismatch")) return false;
  if (!expect(regs[44] == 84, "Mul mismatch")) return false;
  if (!expect(regs[45] == 2, "Div mismatch")) return false;
  if (!expect(regs[46] == 2, "Mod mismatch")) return false;
  return true;
}

bool check_control_flow_family() {
  // Spec alignment: spec/tisc/opcode-semantics.md branch/jump opcodes.
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 0, 0});        // counter
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 1, 0});        // step
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 42, 5, 0});        // limit
  p.insns.push_back({t81::tisc::Opcode::Add, 40, 40, 41});          // pc=3
  p.insns.push_back({t81::tisc::Opcode::Less, 43, 40, 42});         // pc=4
  p.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 3, 43, 0});  // pc=5 -> loop
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto out = run_program(p, 256);
  if (!expect(out.has_value(), "control-flow program trapped")) return false;
  const auto& regs = out->contexts[0].registers;
  if (!expect(regs[40] == 5, "loop counter mismatch")) return false;
  return expect(out->trace.size() > 7, "expected trace expansion from loop");
}

bool check_memory_stack_family() {
  // Spec alignment: spec/tisc/opcode-semantics.md memory + stack opcodes.
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::Store, 120, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 41, 120, 0});
  p.insns.push_back({t81::tisc::Opcode::Push, 41, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 42, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto out = run_program(p, 128);
  if (!expect(out.has_value(), "memory/stack program trapped")) return false;
  const auto& regs = out->contexts[0].registers;
  if (!expect(regs[41] == 42, "Load mismatch")) return false;
  if (!expect(regs[42] == 42, "Pop mismatch")) return false;
  return true;
}

bool check_sum_type_family() {
  // Spec alignment: spec/tisc/opcode-semantics.md option/result/enum opcodes.
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 77, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeOptionSome, 41, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionIsSome, 42, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionUnwrap, 43, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeResultOk, 44, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultIsOk, 45, 44, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultUnwrapOk, 46, 44, 0});
  constexpr std::int32_t kVariantId = 200;
  p.insns.push_back({t81::tisc::Opcode::MakeEnumVariantPayload, 47, 40, kVariantId});
  p.insns.push_back({t81::tisc::Opcode::EnumIsVariant, 48, 47, kVariantId});
  p.insns.push_back({t81::tisc::Opcode::EnumUnwrapPayload, 49, 47, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto out = run_program(p, 256);
  if (!expect(out.has_value(), "sum-type program trapped")) return false;
  const auto& regs = out->contexts[0].registers;
  if (!expect(regs[42] == 1, "OptionIsSome mismatch")) return false;
  if (!expect(regs[45] == 1, "ResultIsOk mismatch")) return false;
  if (!expect(regs[46] == 77, "ResultUnwrapOk mismatch")) return false;
  if (!expect(regs[48] == 1, "EnumIsVariant mismatch")) return false;
  if (!expect(regs[49] == 77, "EnumUnwrapPayload mismatch")) return false;
  return true;
}

bool check_deterministic_fault_semantics() {
  // Spec alignment: deterministic trap semantics in spec/determinism-profile.md.
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 3, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Div, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto out = run_program(p, 32);
  if (!expect(!out.has_value(), "division-by-zero must trap")) return false;
  return expect(out.error() == t81::vm::Trap::DivisionFault, "expected DivisionFault");
}

}  // namespace

int main() {
  if (!check_arithmetic_family()) return 1;
  if (!check_control_flow_family()) return 1;
  if (!check_memory_stack_family()) return 1;
  if (!check_sum_type_family()) return 1;
  if (!check_deterministic_fault_semantics()) return 1;
  return 0;
}
