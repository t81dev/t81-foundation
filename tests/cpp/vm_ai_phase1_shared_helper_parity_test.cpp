#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/types/detail/dmath_types.hpp"
#include "t81/vm/vm.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void assert_tensor_near(const t81::T729DynamicTensor& actual, const t81::T729DynamicTensor& expected,
                        float eps = 1e-5f) {
  T81_TEST_CHECK(actual.shape() == expected.shape());
  T81_TEST_CHECK(actual.numeric_class() == expected.numeric_class());
  T81_TEST_CHECK(actual.data().size() == expected.data().size());
  for (std::size_t i = 0; i < actual.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(actual.data()[i] - expected.data()[i]) <= eps);
  }
}

std::int32_t pack_reg_pair(std::int32_t first, std::int32_t second) {
  return static_cast<std::int32_t>((first & 0xFF) | ((second & 0xFF) << 8));
}

const t81::T729DynamicTensor& tensor_result(const t81::vm::State& state, int reg) {
  const auto handle = state.contexts[0].registers[reg];
  T81_TEST_CHECK(handle > 0);
  T81_TEST_CHECK(state.contexts[0].register_tags[reg] == t81::vm::ValueTag::TensorHandle);
  const auto& slot = state.tensors[static_cast<std::size_t>(handle - 1)];
  T81_TEST_CHECK(slot.has_value());
  return slot.value();
}

void test_attention_parity() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 0.0F, 0.0F, 1.0F}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 0.0F, 0.0F, 1.0F}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {10.0F, 1.0F, 2.0F, 20.0F}));

  t81::tisc::Insn lq{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  lq.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn lk{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  lk.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn lv{t81::tisc::Opcode::LoadImm, 3, 3, 0};
  lv.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lq);
  p.insns.push_back(lk);
  p.insns.push_back(lv);
  p.insns.push_back({t81::tisc::Opcode::ATTN, 4, 1, pack_reg_pair(2, 3)});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& actual = tensor_result(vm->state(), 4);
  const auto expected = t81::ops::attention(p.tensor_pool[0], p.tensor_pool[1], p.tensor_pool[2]);
  assert_tensor_near(actual, expected);
}

void test_embed_parity() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(
      t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 10.0F, 20.0F, 100.0F, 200.0F}));

  t81::tisc::Insn ltab{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  ltab.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(ltab);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::EMBED, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& actual = tensor_result(vm->state(), 3);
  const auto expected = t81::ops::embed(p.tensor_pool[0], 1);
  assert_tensor_near(actual, expected, 0.0f);
}

void test_qmatmul_parity() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 3}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3, 2}, {1.0F, 0.0F, 0.0F, 1.0F, 2.0F, 3.0F}));

  t81::tisc::Insn lact{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  lact.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn lwt{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  lwt.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lact);
  p.insns.push_back(lwt);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::QMATMUL, 4, 1, pack_reg_pair(2, 3)});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto expected =
      t81::ops::qmatmul(p.tensor_pool[0], p.tensor_pool[1], t81::core::detail::DFixed(2));
  const auto& actual = tensor_result(vm->state(), 4);
  assert_tensor_near(actual, expected);
}

void test_qmatmul_fractional_scale_parity() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 2.0F, 3.0F, 4.0F}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 0.0F, 0.0F, 1.0F}));

  t81::tisc::Insn lact{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  lact.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn lwt{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  lwt.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lact);
  p.insns.push_back(lwt);
  t81::tisc::Insn lscale{t81::tisc::Opcode::LoadImm, 3, 1, 0};
  lscale.literal_kind = t81::tisc::LiteralKind::FloatHandle;
  p.float_pool.push_back(0.5);
  p.insns.push_back(lscale);
  p.insns.push_back({t81::tisc::Opcode::QMATMUL, 4, 1, pack_reg_pair(2, 3)});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto expected = t81::ops::qmatmul(
      p.tensor_pool[0], p.tensor_pool[1], t81::core::detail::DFixed::from_decimal(0, 5, 1));
  const auto& actual = tensor_result(vm->state(), 4);
  T81_TEST_CHECK(actual.numeric_class() == expected.numeric_class());
  T81_TEST_CHECK(actual.strict_core_eligible());
  assert_tensor_near(actual, expected);
}

}  // namespace

int main() {
  test_attention_parity();
  test_embed_parity();
  test_qmatmul_parity();
  test_qmatmul_fractional_scale_parity();
  return 0;
}
