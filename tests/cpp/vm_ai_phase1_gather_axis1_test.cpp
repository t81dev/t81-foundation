#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
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
    const auto q =
        static_cast<std::int64_t>(std::llround(static_cast<double>(value) * 1'000'000.0));
    mix(static_cast<std::uint64_t>(q));
  }
  return h;
}

std::int32_t pack_reg_pair(std::int32_t first, std::int32_t second) {
  return static_cast<std::int32_t>((first & 0xFF) | ((second & 0xFF) << 8));
}

}  // namespace

int main() {
  // Success path: GATHER with axis=1 gathers column 0 from a 3×2 tensor.
  // src = [[1, 2], [10, 20], [100, 200]]
  // gather(src, index=0, axis=1) → [1, 10, 100]  (first column)
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 10.0F, 20.0F, 100.0F, 200.0F}));

    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    // R2 = 0 (column index)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
    // R3 = 1 (axis = 1)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});
    // GATHER R4, R1, PACK(R2=idx, R3=axis)
    p.insns.push_back({t81::tisc::Opcode::GATHER, 4, 1, pack_reg_pair(2, 3)});
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

    // Verify against the kernel reference.
    auto expected = t81::ops::gather(p.tensor_pool[0], 0, 1);
    T81_TEST_CHECK(out.rank() == 1);
    T81_TEST_CHECK(out.shape()[0] == 3);  // 3 rows → 3-element column vector
    T81_TEST_CHECK(out.shape() == expected.shape());
    T81_TEST_CHECK(out.data()[0] == 1.0F);    // row 0, col 0
    T81_TEST_CHECK(out.data()[1] == 10.0F);   // row 1, col 0
    T81_TEST_CHECK(out.data()[2] == 100.0F);  // row 2, col 0
    T81_TEST_CHECK(out.data()[0] == expected.data()[0]);
    T81_TEST_CHECK(out.data()[1] == expected.data()[1]);
    T81_TEST_CHECK(out.data()[2] == expected.data()[2]);

    const auto hash = tensor_hash(out);
    std::cout << "AI_PHASE1_HASH GATHER_AXIS1 " << std::hex << std::setw(16)
              << std::setfill('0') << hash << std::dec << "\n";
  }

  // Success path: GATHER with axis=1 gathers column 1.
  // gather(src, index=1, axis=1) → [2, 20, 200]  (second column)
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 10.0F, 20.0F, 100.0F, 200.0F}));

    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});  // index = 1
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});  // axis = 1
    p.insns.push_back({t81::tisc::Opcode::GATHER, 4, 1, pack_reg_pair(2, 3)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());
    const auto out_handle = vm->state().contexts[0].registers[4];
    const auto& out = vm->state().tensors[static_cast<std::size_t>(out_handle - 1)].value();

    T81_TEST_CHECK(out.data()[0] == 2.0F);
    T81_TEST_CHECK(out.data()[1] == 20.0F);
    T81_TEST_CHECK(out.data()[2] == 200.0F);

    const auto hash = tensor_hash(out);
    std::cout << "AI_PHASE1_HASH GATHER_AXIS1_COL1 " << std::hex << std::setw(16)
              << std::setfill('0') << hash << std::dec << "\n";
  }

  // Bounds fault path: axis=1, out-of-range column index.
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 10.0F, 20.0F, 100.0F, 200.0F}));

    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 99, 0});  // OOB index
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});   // axis = 1
    p.insns.push_back({t81::tisc::Opcode::GATHER, 4, 1, pack_reg_pair(2, 3)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(!res.has_value());
    T81_TEST_CHECK(res.error() == t81::vm::Trap::BoundsFault);
  }

  return 0;
}
