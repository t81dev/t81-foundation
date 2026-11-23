#include "t81/isa/program.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

t81::tisc::Program make_program(const std::string& symbol = "tensorA") {
  t81::tisc::Program program;
  program.symbol_pool = {symbol};
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

bool denied_reason_has(const t81::vm::IVirtualMachine& vm, std::string_view needle) {
  for (const auto& event : vm.state().axion_log) {
    if (event.opcode == t81::tisc::Opcode::WeightsLoad &&
        event.verdict.kind == t81::axion::VerdictKind::Deny &&
        event.verdict.reason.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool expect_security_fault(const std::string& policy_text, const char* label,
                           std::string_view reason_needle, const std::string& symbol = "tensorA") {
  auto program = make_program(symbol);
  program.axion_policy_text = policy_text;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (result.has_value() || result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "weights_load_test: expected SecurityFault for " << label << "\n";
    return false;
  }
  if (!denied_reason_has(*vm, reason_needle)) {
    std::cerr << "weights_load_test: missing deny reason '" << reason_needle << "' for " << label
              << "\n";
    return false;
  }
  return true;
}

bool expect_admitted(const std::string& policy_text, const char* label) {
  auto program = make_program();
  program.axion_policy_text = policy_text;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value() || vm->state().contexts[0].registers[2] <= 0) {
    std::cerr << "weights_load_test: admitted tensor name was denied for " << label << "\n";
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

  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes []))", "empty allowlist",
                             "allowed-tensor-hashes empty")) {
    return 1;
  }
  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes [\"other\"]))",
                             "name not admitted", "policy_violation name=tensorA")) {
    return 1;
  }
  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes [\"TensorA\"]))",
                             "case mismatch", "policy_violation name=tensorA")) {
    return 1;
  }
  if (!expect_security_fault("(policy (tier 1) (allowed-tensor-hashes [\"tensorA\"]))",
                             "empty tensor name", "policy_violation name=", "")) {
    return 1;
  }
  if (!expect_security_fault(
          "(policy (tier 1) (allowed-ternary-model-hashes [\"sha3-512:deadbeef\"]))",
          "model checksum is not the tensor name", "policy_violation name=tensorA")) {
    return 1;
  }
  if (!expect_security_fault(
          "(policy (tier 1) (allowed-tensor-hashes []) (allowed-ternary-model-hashes []))",
          "both allowlists empty", "allowed-tensor-hashes empty")) {
    return 1;
  }

  if (!expect_admitted("(policy (tier 1) (allowed-tensor-hashes [\"tensorA\"]))",
                       "allowed-tensor-hashes")) {
    return 1;
  }
  if (!expect_admitted("(policy (tier 1) (allowed-ternary-model-hashes [\"tensorA\"]))",
                       "allowed-ternary-model-hashes name entry")) {
    return 1;
  }
  return 0;
}
