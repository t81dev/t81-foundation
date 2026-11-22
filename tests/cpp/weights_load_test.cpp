#include "t81/isa/program.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

t81::tisc::Program make_program() {
  t81::tisc::Program program;
  program.symbol_pool = {"tensorA"};
  auto model = std::make_shared<t81::weights::ModelFile>();
  t81::weights::NativeTensor tensor;
  tensor.shape = {2, 2};
  tensor.data = {0, 1, 2, 3};
  model->native["tensorA"] = tensor;
  program.weights_model = model;

  t81::tisc::Insn load_a;
  load_a.opcode = t81::tisc::Opcode::WeightsLoad;
  load_a.a = 2;
  load_a.b = 1;
  t81::tisc::Insn load_b = load_a;
  load_b.a = 1;
  t81::tisc::Insn halt;
  halt.opcode = t81::tisc::Opcode::Halt;
  program.insns = {load_a, load_b, halt};
  return program;
}

bool expect_security_fault(const std::string& policy_text, const char* label) {
  auto program = make_program();
  program.axion_policy_text = policy_text;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (result.has_value() || result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "weights_load_test: expected SecurityFault for " << label << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  auto program = make_program();
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "weights_load_test: default VM failed to halt\n";
    return 1;
  }

  auto handle0 = vm->state().contexts[0].registers[2];
  auto handle1 = vm->state().contexts[0].registers[1];
  if (handle0 <= 0 || handle0 != handle1 || vm->state().weights_tensor_refs.size() != 1) {
    std::cerr << "weights_load_test: default VM did not reuse the weights handle\n";
    return 1;
  }
  const auto* native = vm->weights_tensor(handle0);
  std::vector<uint64_t> expected_shape{2, 2};
  if (native == nullptr || native->shape != expected_shape) {
    std::cerr << "weights_load_test: loaded tensor shape mismatch\n";
    return 1;
  }

  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes []))", "empty allowlist")) {
    return 1;
  }
  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes [\"other\"]))",
                             "name not admitted")) {
    return 1;
  }

  auto admitted = make_program();
  admitted.axion_policy_text = "(policy (tier 1) (allowed-tensor-hashes [\"tensorA\"]))";
  auto admitted_vm = t81::vm::make_interpreter_vm();
  admitted_vm->load_program(admitted);
  auto admitted_result = admitted_vm->run_to_halt();
  if (!admitted_result.has_value() || admitted_vm->state().contexts[0].registers[2] <= 0) {
    std::cerr << "weights_load_test: admitted tensor name was denied\n";
    return 1;
  }
  return 0;
}
