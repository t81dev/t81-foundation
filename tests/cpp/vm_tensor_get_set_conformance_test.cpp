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

void run_get_set_immediate_visibility_and_repeatability() {
  auto build_program = []() {
    t81::tisc::Program p;
    p.tensor_pool.push_back(t81::T729DynamicTensor({4}, {1.0f, 2.0f, 3.0f, 4.0f}));

    t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(load_tensor);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});   // idx 1
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 11, 0});  // value 11
    p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
    p.insns.push_back({t81::tisc::Opcode::TGet, 4, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::F2I, 5, 4, 0});
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 6, 3, 0});   // idx 3
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 7, -5, 0});  // value -5
    p.insns.push_back({t81::tisc::Opcode::TSet, 1, 6, 7});
    p.insns.push_back({t81::tisc::Opcode::TGet, 8, 1, 6});
    p.insns.push_back({t81::tisc::Opcode::F2I, 9, 8, 0});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    return p;
  };

  auto run_once = [&build_program]() {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(build_program());
    const auto result = vm->run_to_halt();
    T81_TEST_CHECK(result.has_value());
    return vm->state();
  };

  const auto state_a = run_once();
  const auto state_b = run_once();

  T81_TEST_CHECK(state_a.contexts[0].registers[5] == 11);
  T81_TEST_CHECK(state_a.contexts[0].registers[9] == -5);
  T81_TEST_CHECK(state_b.contexts[0].registers[5] == 11);
  T81_TEST_CHECK(state_b.contexts[0].registers[9] == -5);

  const auto handle_a = state_a.contexts[0].registers[1];
  const auto handle_b = state_b.contexts[0].registers[1];
  T81_TEST_CHECK(handle_a > 0);
  T81_TEST_CHECK(handle_b > 0);

  const auto& tensor_a = state_a.tensors[static_cast<std::size_t>(handle_a - 1)];
  const auto& tensor_b = state_b.tensors[static_cast<std::size_t>(handle_b - 1)];
  T81_TEST_CHECK(tensor_a.has_value());
  T81_TEST_CHECK(tensor_b.has_value());
  T81_TEST_CHECK(tensor_a->data()[1] == 11.0f);
  T81_TEST_CHECK(tensor_a->data()[3] == -5.0f);
  T81_TEST_CHECK(tensor_b->data() == tensor_a->data());
  T81_TEST_CHECK(tensor_a->numeric_class() == t81::TensorNumericClass::ExactInt);
  T81_TEST_CHECK(tensor_b->numeric_class() == tensor_a->numeric_class());
}

void run_identity_copy_mutation_does_not_alias_source() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {4.0f, 5.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::TID, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});   // idx
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 12, 0});  // value
  p.insns.push_back({t81::tisc::Opcode::TSet, 2, 3, 4});
  p.insns.push_back({t81::tisc::Opcode::TGet, 5, 1, 3});
  p.insns.push_back({t81::tisc::Opcode::F2I, 6, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::TGet, 7, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::F2I, 8, 7, 0});
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
  T81_TEST_CHECK(state.contexts[0].registers[6] == 5);
  T81_TEST_CHECK(state.contexts[0].registers[8] == 12);

  const auto& src = state.tensors[static_cast<std::size_t>(src_handle - 1)];
  const auto& dst = state.tensors[static_cast<std::size_t>(dst_handle - 1)];
  T81_TEST_CHECK(src.has_value());
  T81_TEST_CHECK(dst.has_value());
  T81_TEST_CHECK(src->data()[1] == 5.0f);
  T81_TEST_CHECK(dst->data()[1] == 12.0f);
  T81_TEST_CHECK(src->data() != dst->data());
}

void run_set_out_of_bounds_fault_is_deterministic() {
  auto build_program = []() {
    t81::tisc::Program p;
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {8.0f, 9.0f}));

    t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(load_tensor);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 5, 0});   // out of bounds
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 99, 0});  // value
    p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    return p;
  };

  for (int i = 0; i < 2; ++i) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(build_program());
    auto result = vm->run_to_halt();
    T81_TEST_CHECK(!result.has_value());
    T81_TEST_CHECK(result.error() == t81::vm::Trap::BoundsFault);
  }
}

}  // namespace

int main() {
  run_get_set_success_int_value();
  run_get_set_success_float_handle_value();
  run_get_bad_index_type_fault();
  run_identity_copy_preserves_tensor_authority();
  run_get_set_immediate_visibility_and_repeatability();
  run_identity_copy_mutation_does_not_alias_source();
  run_set_out_of_bounds_fault_is_deterministic();
  return 0;
}
