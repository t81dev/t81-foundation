#include <vector>

#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

namespace {

t81::vm::Trap run_expected_trap(t81::tisc::Program program) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  return result.error();
}

t81::tisc::Program make_softmax_rank0_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({}, {}));
  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::TSoftmax, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_transpose_rank1_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::TTranspose, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_rope_rank1_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 3, 0});
  p.insns.push_back({t81::tisc::Opcode::TRoPE, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_rmsnorm_shape_mismatch_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0f, 2.0f, 3.0f, 4.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 1.0f, 1.0f}));
  t81::tisc::Insn load_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_w{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_w.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_a);
  p.insns.push_back(load_w);
  p.insns.push_back({t81::tisc::Opcode::TRMSNorm, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_vecadd_shape_mismatch_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {1.0f, 2.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {3.0f, 4.0f, 5.0f}));
  t81::tisc::Insn load_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_b{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_a);
  p.insns.push_back(load_b);
  p.insns.push_back({t81::tisc::Opcode::TVecAdd, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_matmul_shape_mismatch_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 3}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0f, 0.0f, 0.0f, 1.0f}));
  t81::tisc::Insn load_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_b{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_a);
  p.insns.push_back(load_b);
  p.insns.push_back({t81::tisc::Opcode::TMatMul, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tendot_shape_mismatch_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 3}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}));
  p.tensor_pool.push_back(
      t81::T729DynamicTensor({4, 2}, {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f}));
  t81::tisc::Insn load_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_b{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_a);
  p.insns.push_back(load_b);
  p.insns.push_back({t81::tisc::Opcode::TTenDot, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tget_oob_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  t81::tisc::Insn load_t{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_t.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_t);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 99, 0});
  p.insns.push_back({t81::tisc::Opcode::TGet, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tset_oob_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  t81::tisc::Insn load_t{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_t.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_t);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 99, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 7, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tset_bad_value_type_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {3.0f, 4.0f, 5.0f}));
  t81::tisc::Insn load_t{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_t.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_bad_val{t81::tisc::Opcode::LoadImm, 3, 2, 0};
  load_bad_val.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_t);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
  p.insns.push_back(load_bad_val);
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

}  // namespace

int main() {
  T81_TEST_CHECK(run_expected_trap(make_softmax_rank0_program()) == t81::vm::Trap::DecodeFault);
  T81_TEST_CHECK(run_expected_trap(make_transpose_rank1_program()) == t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_rope_rank1_program()) == t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_rmsnorm_shape_mismatch_program()) ==
                 t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_vecadd_shape_mismatch_program()) ==
                 t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_matmul_shape_mismatch_program()) ==
                 t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_tendot_shape_mismatch_program()) ==
                 t81::vm::Trap::ShapeFault);
  T81_TEST_CHECK(run_expected_trap(make_tget_oob_program()) == t81::vm::Trap::BoundsFault);
  T81_TEST_CHECK(run_expected_trap(make_tset_oob_program()) == t81::vm::Trap::BoundsFault);
  T81_TEST_CHECK(run_expected_trap(make_tset_bad_value_type_program()) == t81::vm::Trap::TypeFault);
  return 0;
}
