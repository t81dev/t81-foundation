#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <vector>

int main() {
  // Success path.
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {1.0F, 2.0F, 10.0F, 20.0F, 100.0F, 200.0F}));

    t81::tisc::Insn ltab{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ltab.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ltab);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});  // index=1 (second row)
    p.insns.push_back({t81::tisc::Opcode::EMBED, 3, 1, 2});
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
    T81_TEST_CHECK(out.rank() == 1);
    T81_TEST_CHECK(out.shape().size() == 1);
    T81_TEST_CHECK(out.shape()[0] == 2);
    T81_TEST_CHECK(out.data()[0] == 10.0F);
    T81_TEST_CHECK(out.data()[1] == 20.0F);
  }

  // Bounds fault path.
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({2, 2}, {1.0F, 2.0F, 3.0F, 4.0F}));

    t81::tisc::Insn ltab{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ltab.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ltab);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 99, 0});  // OOB index
    p.insns.push_back({t81::tisc::Opcode::EMBED, 3, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(!res.has_value());
    T81_TEST_CHECK(res.error() == t81::vm::Trap::BoundsFault);
  }

  return 0;
}

