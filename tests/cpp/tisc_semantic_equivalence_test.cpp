#include <iostream>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "tisc_semantic_equivalence_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

bool check_add_semantics() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 20, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 22, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(64);
  if (!expect(run.has_value(), "add program should complete")) return false;
  return expect(vm->state().contexts[0].registers[42] == 42, "Add semantic mismatch");
}

bool check_div_zero_trap_semantics() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Div, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(64);
  if (!expect(!run.has_value(), "div-zero program must trap")) return false;
  return expect(run.error() == t81::vm::Trap::DivisionFault, "div-zero trap semantic mismatch");
}

bool check_jump_if_zero_semantics() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 0, 0});     // condition
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 5, 0});     // default path value
  p.insns.push_back({t81::tisc::Opcode::JumpIfZero, 5, 40, 0});  // jump to pc=5
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 99, 0});    // skipped
  p.insns.push_back({t81::tisc::Opcode::Jump, 6, 0, 0});         // skipped
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 42, 0});    // taken
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(64);
  if (!expect(run.has_value(), "jump program should complete")) return false;
  return expect(vm->state().contexts[0].registers[41] == 42, "JumpIfZero semantic mismatch");
}

bool check_option_result_semantics() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeOptionSome, 41, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionIsSome, 42, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionUnwrap, 43, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeResultOk, 44, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultIsOk, 45, 44, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultUnwrapOk, 46, 44, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(64);
  if (!expect(run.has_value(), "option/result program should complete")) return false;
  const auto& regs = vm->state().contexts[0].registers;
  if (!expect(regs[42] == 1, "OptionIsSome semantic mismatch")) return false;
  if (!expect(regs[45] == 1, "ResultIsOk semantic mismatch")) return false;
  return expect(regs[46] == 42, "ResultUnwrapOk semantic mismatch");
}

}  // namespace

int main() {
  if (!check_add_semantics()) return 1;
  if (!check_div_zero_trap_semantics()) return 1;
  if (!check_jump_if_zero_semantics()) return 1;
  if (!check_option_result_semantics()) return 1;
  return 0;
}
