#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/vm/vm.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

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
  // Success path: SCATTER scatter-adds a rank-1 source into row 1 of a 3×2 dst.
  {
    t81::tisc::Program p;
    // Tensor pool index 0 → handle 1: dst (3×2), all zeros.
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}));
    // Tensor pool index 1 → handle 2: src (rank-1, length 2).
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {7.0F, 8.0F}));

    // R1 = TensorHandle(1)  (dst)
    t81::tisc::Insn ldst{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ldst.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ldst);
    // R2 = TensorHandle(2)  (src)
    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 2, 2, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    // R3 = 1  (index — scatter into row 1)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});
    // SCATTER R4, R1, PACK(R3=idx, R2=src)
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 4, 1, pack_reg_pair(3, 2)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());
    T81_TEST_CHECK(vm->state().contexts[0].register_tags[4] == t81::vm::ValueTag::TensorHandle);
    auto out_handle = vm->state().contexts[0].registers[4];
    T81_TEST_CHECK(out_handle > 0);
    const auto& out_opt = vm->state().tensors[static_cast<std::size_t>(out_handle - 1)];
    T81_TEST_CHECK(out_opt.has_value());
    const auto& out = out_opt.value();

    auto expected = t81::ops::scatter_add(p.tensor_pool[0], 1, p.tensor_pool[1]);
    T81_TEST_CHECK(out.rank() == 2);
    T81_TEST_CHECK(out.shape()[0] == 3);
    T81_TEST_CHECK(out.shape()[1] == 2);
    T81_TEST_CHECK(out.shape() == expected.shape());
    // Rows 0 and 2 should be unchanged (all zero).
    T81_TEST_CHECK(out.data()[0] == 0.0F);
    T81_TEST_CHECK(out.data()[1] == 0.0F);
    T81_TEST_CHECK(out.data()[4] == 0.0F);
    T81_TEST_CHECK(out.data()[5] == 0.0F);
    // Row 1 should have src values scatter-added.
    T81_TEST_CHECK(out.data()[2] == 7.0F);
    T81_TEST_CHECK(out.data()[3] == 8.0F);
    T81_TEST_CHECK(out.data()[2] == expected.data()[2]);
    T81_TEST_CHECK(out.data()[3] == expected.data()[3]);

    const auto hash = tensor_hash(out);
    std::cout << "AI_PHASE1_HASH SCATTER " << std::hex << std::setw(16) << std::setfill('0') << hash
              << std::dec << "\n";
  }

  // Bounds fault path: out-of-range scatter index.
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({2, 2}, {0.0F, 0.0F, 0.0F, 0.0F}));
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {1.0F, 2.0F}));

    t81::tisc::Insn ldst{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ldst.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ldst);
    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 2, 2, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 99, 0});  // OOB index
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 4, 1, pack_reg_pair(3, 2)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(!res.has_value());
    T81_TEST_CHECK(res.error() == t81::vm::Trap::BoundsFault);
  }

  return 0;
}
