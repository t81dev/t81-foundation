#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/vm/vm.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

t81::T729DynamicTensor attention_expected(const t81::T729DynamicTensor& q,
                                          const t81::T729DynamicTensor& k,
                                          const t81::T729DynamicTensor& v) {
  return t81::ops::attention(q, k, v);
}

std::uint64_t tensor_hash(const t81::T729DynamicTensor& tensor) {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t h = kOffset;
  auto mix = [&](std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
      h ^= (value >> (i * 8)) & 0xFFULL;
      h *= kPrime;
    }
  };
  for (int dim : tensor.shape()) {
    mix(static_cast<std::uint64_t>(static_cast<std::int64_t>(dim)));
  }
  for (float value : tensor.data()) {
    const auto q = static_cast<std::int64_t>(std::llround(static_cast<double>(value) * 1'000'000.0));
    mix(static_cast<std::uint64_t>(q));
  }
  return h;
}

std::int32_t pack_reg_pair(std::int32_t first, std::int32_t second) {
  return static_cast<std::int32_t>((first & 0xFF) | ((second & 0xFF) << 8));
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

  // Phase-1 canonical packed-operand encoding:
  // ATTN RD, RQ, PACK(RK,RV)
  p.insns.push_back({t81::tisc::Opcode::ATTN, 4, 1, pack_reg_pair(2, 3)});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm1 = t81::vm::make_interpreter_vm();
  vm1->load_program(p);
  auto r1 = vm1->run_to_halt();
  T81_TEST_CHECK(r1.has_value());

  const auto& state1 = vm1->state();
  auto out_handle1 = state1.contexts[0].registers[4];
  T81_TEST_CHECK(out_handle1 > 0);
  T81_TEST_CHECK(state1.contexts[0].register_tags[4] == t81::vm::ValueTag::TensorHandle);
  const auto& out_opt1 = state1.tensors[static_cast<std::size_t>(out_handle1 - 1)];
  T81_TEST_CHECK(out_opt1.has_value());
  const auto& out1 = out_opt1.value();
  T81_TEST_CHECK(out1.strict_core_eligible());

  auto expected = attention_expected(p.tensor_pool[0], p.tensor_pool[1], p.tensor_pool[2]);
  T81_TEST_CHECK(expected.canonical_fixed_authoritative());
  T81_TEST_CHECK(expected.numeric_class() == out1.numeric_class());
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
  auto out_handle2 = state2.contexts[0].registers[4];
  T81_TEST_CHECK(out_handle2 > 0);
  const auto& out_opt2 = state2.tensors[static_cast<std::size_t>(out_handle2 - 1)];
  T81_TEST_CHECK(out_opt2.has_value());
  const auto& out2 = out_opt2.value();
  T81_TEST_CHECK(out2.numeric_class() == out1.numeric_class());
  T81_TEST_CHECK(out2.shape() == out1.shape());
  T81_TEST_CHECK(out2.data().size() == out1.data().size());
  for (std::size_t i = 0; i < out1.data().size(); ++i) {
    T81_TEST_CHECK(out2.data()[i] == out1.data()[i]);
  }

  const auto hash = tensor_hash(out1);
  std::cout << "AI_PHASE1_HASH ATTN " << std::hex << std::setw(16) << std::setfill('0') << hash
            << std::dec << "\n";

  t81::tisc::Program float_prog;
  float_prog.tensor_pool.push_back(
      t81::T729DynamicTensor({2, 2}, {0.5F, 1.25F, -0.75F, 0.5F}));            // Q
  float_prog.tensor_pool.push_back(
      t81::T729DynamicTensor({2, 2}, {1.5F, -0.5F, 0.25F, 0.75F}));            // K
  float_prog.tensor_pool.push_back(
      t81::T729DynamicTensor({2, 2}, {0.1F, 2.5F, -1.0F, 0.25F}));             // V
  float_prog.insns.push_back(lq);
  float_prog.insns.push_back(lk);
  float_prog.insns.push_back(lv);
  float_prog.insns.push_back({t81::tisc::Opcode::ATTN, 4, 1, pack_reg_pair(2, 3)});
  float_prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm3 = t81::vm::make_interpreter_vm();
  vm3->load_program(float_prog);
  auto r3 = vm3->run_to_halt();
  T81_TEST_CHECK(r3.has_value());
  const auto& out3 =
      vm3->state().tensors[static_cast<std::size_t>(vm3->state().contexts[0].registers[4] - 1)].value();
  T81_TEST_CHECK(out3.numeric_class() == t81::TensorNumericClass::HostFloat);
  T81_TEST_CHECK(out3.has_canonical_fixed_data());

  auto vm4 = t81::vm::make_interpreter_vm();
  vm4->load_program(float_prog);
  auto r4 = vm4->run_to_halt();
  T81_TEST_CHECK(r4.has_value());
  const auto& out4 =
      vm4->state().tensors[static_cast<std::size_t>(vm4->state().contexts[0].registers[4] - 1)].value();
  T81_TEST_CHECK(out4.numeric_class() == t81::TensorNumericClass::HostFloat);
  T81_TEST_CHECK(out4.has_canonical_fixed_data());
  T81_TEST_CHECK(out4.shape() == out3.shape());
  T81_TEST_CHECK(out4.data().size() == out3.data().size());
  for (std::size_t i = 0; i < out3.data().size(); ++i) {
    T81_TEST_CHECK(out4.data()[i] == out3.data()[i]);
  }

  return 0;
}
