#include "test_runtime_check.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

namespace {

using t81::T729DynamicTensor;
using t81::TensorNumericClass;
using t81::tisc::Insn;
using t81::tisc::LiteralKind;
using t81::tisc::Opcode;
using t81::tisc::Program;
using t81::vm::Trap;
using t81::vm::ValueTag;

Insn load_tensor_handle(int reg, int pool_idx) {
  Insn insn{Opcode::LoadImm, reg, pool_idx, 0};
  insn.literal_kind = LiteralKind::TensorHandle;
  return insn;
}

T729DynamicTensor exact_trit_tensor(std::vector<int> shape, std::vector<float> values) {
  T729DynamicTensor tensor(std::move(shape), std::move(values));
  tensor.set_numeric_class(TensorNumericClass::ExactTrit);
  return tensor;
}

const T729DynamicTensor& result_tensor(const t81::vm::IVirtualMachine& vm, int reg) {
  const auto& ctx = vm.state().contexts[0];
  T81_TEST_CHECK(ctx.register_tags[reg] == ValueTag::TensorHandle);
  const auto handle = ctx.registers[reg];
  T81_TEST_CHECK(handle >= 1);
  const auto& slot = vm.state().tensors[static_cast<std::size_t>(handle - 1)];
  T81_TEST_CHECK(slot.has_value());
  return *slot;
}

int test_opcode_metadata() {
  T81_TEST_CHECK(t81::tisc::opcode_name(Opcode::TNOT_SWAR) == "TNOT_SWAR");
  T81_TEST_CHECK(t81::tisc::opcode_name(Opcode::TAND_SWAR) == "TAND_SWAR");
  T81_TEST_CHECK(t81::tisc::opcode_name(Opcode::TOR_SWAR) == "TOR_SWAR");
  T81_TEST_CHECK(t81::tisc::is_valid_opcode(static_cast<std::uint8_t>(Opcode::TOR_SWAR)));
  return 0;
}

int test_tnot_swar_tensor() {
  Program p;
  p.tensor_pool.push_back(exact_trit_tensor({4}, {-1.0f, 0.0f, 1.0f, -1.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back({Opcode::TNOT_SWAR, 2, 1, 0});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt();
  T81_TEST_CHECK(run.has_value());

  const auto& out = result_tensor(*vm, 2);
  T81_TEST_CHECK(out.shape() == std::vector<int>({4}));
  T81_TEST_CHECK(out.numeric_class() == TensorNumericClass::ExactTrit);
  const auto values = out.snapshot_values();
  T81_TEST_CHECK(values == std::vector<float>({1.0f, 0.0f, -1.0f, 1.0f}));
  return 0;
}

int test_tand_tor_swar_tensor() {
  Program p;
  p.tensor_pool.push_back(exact_trit_tensor({4}, {-1.0f, 0.0f, 1.0f, 1.0f}));
  p.tensor_pool.push_back(exact_trit_tensor({4}, {1.0f, -1.0f, 0.0f, 1.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::TAND_SWAR, 3, 1, 2});
  p.insns.push_back({Opcode::TOR_SWAR, 4, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt();
  T81_TEST_CHECK(run.has_value());

  const auto& and_out = result_tensor(*vm, 3);
  const auto& or_out = result_tensor(*vm, 4);
  T81_TEST_CHECK(and_out.snapshot_values() == std::vector<float>({-1.0f, -1.0f, 0.0f, 1.0f}));
  T81_TEST_CHECK(or_out.snapshot_values() == std::vector<float>({1.0f, 0.0f, 1.0f, 1.0f}));
  T81_TEST_CHECK(and_out.numeric_class() == TensorNumericClass::ExactTrit);
  T81_TEST_CHECK(or_out.numeric_class() == TensorNumericClass::ExactTrit);
  return 0;
}

int test_swar_shape_fault() {
  Program p;
  p.tensor_pool.push_back(exact_trit_tensor({3}, {-1.0f, 0.0f, 1.0f}));
  p.tensor_pool.push_back(exact_trit_tensor({2}, {-1.0f, 1.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::TAND_SWAR, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt();
  T81_TEST_CHECK(!run.has_value());
  T81_TEST_CHECK(run.error() == Trap::ShapeFault);
  return 0;
}

int test_swar_type_fault_on_non_exact_trit_tensor() {
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({3}, {0.5f, 0.0f, 1.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back({Opcode::TNOT_SWAR, 2, 1, 0});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt();
  T81_TEST_CHECK(!run.has_value());
  T81_TEST_CHECK(run.error() == Trap::TypeFault);
  return 0;
}

int test_swar_axion_log_reasons() {
  Program p;
  p.tensor_pool.push_back(exact_trit_tensor({4}, {-1.0f, 0.0f, 1.0f, -1.0f}));
  p.tensor_pool.push_back(exact_trit_tensor({4}, {1.0f, -1.0f, 0.0f, 1.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::TNOT_SWAR, 3, 1, 0});
  p.insns.push_back({Opcode::TAND_SWAR, 4, 1, 2});
  p.insns.push_back({Opcode::TOR_SWAR, 5, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt();
  T81_TEST_CHECK(run.has_value());

  const auto& log = vm->state().axion_log;
  const auto has_reason = [&](Opcode opcode, const char* needle) {
    return std::any_of(log.begin(), log.end(), [&](const auto& event) {
      return event.opcode == opcode &&
             event.verdict.reason.find(needle) != std::string::npos;
    });
  };

  T81_TEST_CHECK(has_reason(Opcode::TNOT_SWAR, "TNOT_SWAR kernel execution"));
  T81_TEST_CHECK(has_reason(Opcode::TAND_SWAR, "TAND_SWAR kernel execution"));
  T81_TEST_CHECK(has_reason(Opcode::TOR_SWAR, "TOR_SWAR kernel execution"));
  return 0;
}

}  // namespace

int main() {
  int failures = 0;
  failures += test_opcode_metadata();
  failures += test_tnot_swar_tensor();
  failures += test_tand_tor_swar_tensor();
  failures += test_swar_shape_fault();
  failures += test_swar_type_fault_on_non_exact_trit_tensor();
  failures += test_swar_axion_log_reasons();
  return failures;
}
