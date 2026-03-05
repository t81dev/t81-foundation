#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/vm/vm.hpp"

#include <cmath>
#include <vector>

namespace {

t81::T729DynamicTensor attention_expected(const t81::T729DynamicTensor& q,
                                          const t81::T729DynamicTensor& k,
                                          const t81::T729DynamicTensor& v) {
  auto k_t = k.transpose2d();
  auto scores = t81::ops::matmul(q, k_t);
  const float inv_scale = 1.0f / std::sqrt(static_cast<float>(q.shape()[1]));
  for (auto& x : scores.data()) {
    x *= inv_scale;
  }
  auto probs = t81::ops::softmax(scores);
  return t81::ops::matmul(probs, v);
}

}  // namespace

int main() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 0.0F, 0.0F, 1.0F}));      // Q
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 0.0F, 0.0F, 1.0F}));      // K
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {10.0F, 1.0F, 2.0F, 20.0F}));    // V

  t81::tisc::Insn lq{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  lq.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lq);
  t81::tisc::Insn lk{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  lk.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lk);
  t81::tisc::Insn lv{t81::tisc::Opcode::LoadImm, 3, 3, 0};
  lv.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lv);

  // Phase-1 provisional ATTN encoding: A is both V source and destination.
  p.insns.push_back({t81::tisc::Opcode::ATTN, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm1 = t81::vm::make_interpreter_vm();
  vm1->load_program(p);
  auto r1 = vm1->run_to_halt();
  T81_TEST_CHECK(r1.has_value());

  const auto& state1 = vm1->state();
  auto out_handle1 = state1.contexts[0].registers[3];
  T81_TEST_CHECK(out_handle1 > 0);
  T81_TEST_CHECK(state1.contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);
  const auto& out_opt1 = state1.tensors[static_cast<std::size_t>(out_handle1 - 1)];
  T81_TEST_CHECK(out_opt1.has_value());
  const auto& out1 = out_opt1.value();

  auto expected = attention_expected(p.tensor_pool[0], p.tensor_pool[1], p.tensor_pool[2]);
  T81_TEST_CHECK(out1.shape() == expected.shape());
  T81_TEST_CHECK(out1.data().size() == expected.data().size());
  for (std::size_t i = 0; i < out1.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(out1.data()[i] - expected.data()[i]) < 1e-5F);
  }

  // Determinism check: same program should produce identical outputs across runs.
  auto vm2 = t81::vm::make_interpreter_vm();
  vm2->load_program(p);
  auto r2 = vm2->run_to_halt();
  T81_TEST_CHECK(r2.has_value());
  const auto& state2 = vm2->state();
  auto out_handle2 = state2.contexts[0].registers[3];
  T81_TEST_CHECK(out_handle2 > 0);
  const auto& out_opt2 = state2.tensors[static_cast<std::size_t>(out_handle2 - 1)];
  T81_TEST_CHECK(out_opt2.has_value());
  const auto& out2 = out_opt2.value();
  T81_TEST_CHECK(out2.shape() == out1.shape());
  T81_TEST_CHECK(out2.data().size() == out1.data().size());
  for (std::size_t i = 0; i < out1.data().size(); ++i) {
    T81_TEST_CHECK(out2.data()[i] == out1.data()[i]);
  }

  return 0;
}

