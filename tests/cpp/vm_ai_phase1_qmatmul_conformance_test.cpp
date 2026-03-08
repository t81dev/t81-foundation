#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/types/detail/dmath_types.hpp"
#include "t81/vm/vm.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace {

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
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 3}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3, 2}, {7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F}));

  t81::tisc::Insn la{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  la.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(la);
  t81::tisc::Insn lb{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  lb.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(lb);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 1, 0});  // scale=1
  p.insns.push_back({t81::tisc::Opcode::QMATMUL, 3, 1, pack_reg_pair(2, 4)});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm1 = t81::vm::make_interpreter_vm();
  vm1->load_program(p);
  auto res1 = vm1->run_to_halt();
  T81_TEST_CHECK(res1.has_value());
  T81_TEST_CHECK(vm1->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);
  const auto handle1 = vm1->state().contexts[0].registers[3];
  T81_TEST_CHECK(handle1 > 0);
  const auto& out1_opt = vm1->state().tensors[static_cast<std::size_t>(handle1 - 1)];
  T81_TEST_CHECK(out1_opt.has_value());
  const auto& out1 = out1_opt.value();

  auto expected =
      t81::ops::qmatmul(p.tensor_pool[0], p.tensor_pool[1], t81::core::detail::DFixed(1));
  T81_TEST_CHECK(out1.shape() == expected.shape());
  T81_TEST_CHECK(out1.data().size() == expected.data().size());
  T81_TEST_CHECK(out1.numeric_class() == expected.numeric_class());
  for (std::size_t i = 0; i < out1.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(out1.data()[i] - expected.data()[i]) < 1e-6F);
  }

  // Determinism check across independent runs.
  auto vm2 = t81::vm::make_interpreter_vm();
  vm2->load_program(p);
  auto res2 = vm2->run_to_halt();
  T81_TEST_CHECK(res2.has_value());
  const auto handle2 = vm2->state().contexts[0].registers[3];
  T81_TEST_CHECK(handle2 > 0);
  const auto& out2_opt = vm2->state().tensors[static_cast<std::size_t>(handle2 - 1)];
  T81_TEST_CHECK(out2_opt.has_value());
  const auto& out2 = out2_opt.value();
  T81_TEST_CHECK(out2.shape() == out1.shape());
  T81_TEST_CHECK(out2.data().size() == out1.data().size());
  for (std::size_t i = 0; i < out1.data().size(); ++i) {
    T81_TEST_CHECK(out2.data()[i] == out1.data()[i]);
  }
  const auto hash = tensor_hash(out1);
  std::cout << "AI_PHASE1_HASH QMATMUL " << std::hex << std::setw(16) << std::setfill('0') << hash
            << std::dec << "\n";

  // Shape fault check.
  t81::tisc::Program bad;
  bad.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0F, 2.0F, 3.0F, 4.0F}));
  bad.tensor_pool.push_back(t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
  t81::tisc::Insn bad_la{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  bad_la.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  bad.insns.push_back(bad_la);
  t81::tisc::Insn bad_lb{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  bad_lb.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  bad.insns.push_back(bad_lb);
  bad.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 1, 0});  // scale=1
  bad.insns.push_back({t81::tisc::Opcode::QMATMUL, 3, 1, pack_reg_pair(2, 4)});
  bad.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  auto vm_bad = t81::vm::make_interpreter_vm();
  vm_bad->load_program(bad);
  auto bad_res = vm_bad->run_to_halt();
  T81_TEST_CHECK(!bad_res.has_value());
  T81_TEST_CHECK(bad_res.error() == t81::vm::Trap::ShapeFault);

  return 0;
}
