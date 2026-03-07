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

}  // namespace

int main() {
  // Success path: WLOAD materialises an independent copy of the weight tensor.
  {
    t81::tisc::Program p;
    // Tensor pool index 0 → handle 1.
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({2, 3}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));

    // R1 = TensorHandle(1)
    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    // R2 = 0  (policy register, ignored in phase-1)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
    // WLOAD R3, R1, R2
    p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());
    T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);
    auto out_handle = vm->state().contexts[0].registers[3];
    T81_TEST_CHECK(out_handle > 0);
    const auto& out_opt = vm->state().tensors[static_cast<std::size_t>(out_handle - 1)];
    T81_TEST_CHECK(out_opt.has_value());
    const auto& out = out_opt.value();

    T81_TEST_CHECK(out.rank() == 2);
    T81_TEST_CHECK(out.shape()[0] == 2);
    T81_TEST_CHECK(out.shape()[1] == 3);
    T81_TEST_CHECK(out.data().size() == 6);
    T81_TEST_CHECK(out.data()[0] == 1.0F);
    T81_TEST_CHECK(out.data()[5] == 6.0F);

    // Output handle must be distinct from input handle (independent copy).
    auto in_handle = vm->state().contexts[0].registers[1];
    T81_TEST_CHECK(out_handle != in_handle);

    const auto hash = tensor_hash(out);
    std::cout << "AI_PHASE1_HASH WLOAD " << std::hex << std::setw(16) << std::setfill('0') << hash
              << std::dec << "\n";
  }

  // Fault path: R_SRC is a plain integer, not a tensor — expect TypeFault.
  {
    t81::tisc::Program p;
    // R1 = plain integer 42 (not a tensor handle)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 42, 0});
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
    p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(!res.has_value());
  }

  return 0;
}
