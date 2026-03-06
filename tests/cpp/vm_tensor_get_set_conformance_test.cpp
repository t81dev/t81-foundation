#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

namespace {

void run_get_set_success_int_value() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});  // idx
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 7, 0});  // value
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::TGet, 4, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::F2I, 5, 4, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& state = vm->state();
  T81_TEST_CHECK(state.contexts[0].registers[5] == 7);
  const auto handle = state.contexts[0].registers[1];
  T81_TEST_CHECK(handle > 0);
  const auto& tensor = state.tensors[static_cast<std::size_t>(handle - 1)];
  T81_TEST_CHECK(tensor.has_value());
  T81_TEST_CHECK(tensor->data()[1] == 7.0f);
  T81_TEST_CHECK(tensor->numeric_class() == t81::TensorNumericClass::ExactInt);
}

void run_get_set_success_float_handle_value() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 2, 0});  // idx
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::I2F, 4, 3, 0});  // float handle
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 4});
  p.insns.push_back({t81::tisc::Opcode::TGet, 5, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::F2I, 6, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].registers[6] == 9);
  const auto handle = vm->state().contexts[0].registers[1];
  T81_TEST_CHECK(handle > 0);
  const auto& tensor = vm->state().tensors[static_cast<std::size_t>(handle - 1)];
  T81_TEST_CHECK(tensor.has_value());
  T81_TEST_CHECK(tensor->numeric_class() == t81::TensorNumericClass::HostFloat);
  T81_TEST_CHECK(tensor->canonical_fixed_authoritative());
  T81_TEST_CHECK(tensor->data()[2] == 9.0f);
}

void run_get_bad_index_type_fault() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {4.0f, 5.0f, 6.0f}));

  t81::tisc::Insn load_tensor_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_tensor_b{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_tensor_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;  // not Int tag
  p.insns.push_back(load_tensor_a);
  p.insns.push_back(load_tensor_b);
  p.insns.push_back({t81::tisc::Opcode::TGet, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::TypeFault);
}

void run_identity_copy_preserves_tensor_authority() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {4.0f, 5.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::TID, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& state = vm->state();
  const auto src_handle = state.contexts[0].registers[1];
  const auto dst_handle = state.contexts[0].registers[2];
  T81_TEST_CHECK(src_handle > 0);
  T81_TEST_CHECK(dst_handle > 0);
  T81_TEST_CHECK(src_handle != dst_handle);

  const auto& src = state.tensors[static_cast<std::size_t>(src_handle - 1)];
  const auto& dst = state.tensors[static_cast<std::size_t>(dst_handle - 1)];
  T81_TEST_CHECK(src.has_value());
  T81_TEST_CHECK(dst.has_value());
  T81_TEST_CHECK(dst->canonical_fixed_authoritative());
  T81_TEST_CHECK(dst->numeric_class() == src->numeric_class());
  T81_TEST_CHECK(dst->data() == src->data());
}

}  // namespace

int main() {
  run_get_set_success_int_value();
  run_get_set_success_float_handle_value();
  run_get_bad_index_type_fault();
  run_identity_copy_preserves_tensor_authority();
  return 0;
}
