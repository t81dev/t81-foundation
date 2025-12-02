#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main() {
    t81::tisc::Program program;
    program.symbol_pool = {"tensorA"};
    auto model = std::make_shared<t81::weights::ModelFile>();
    t81::weights::NativeTensor tensor;
    tensor.shape = {2, 2};
    tensor.data = {0, 1, 2, 3};
    model->native["tensorA"] = tensor;
    program.weights_model = model;

    t81::tisc::Insn load_a;
    load_a.opcode = t81::tisc::Opcode::WeightsLoad;
    load_a.a = 0;
    load_a.b = 1;

    t81::tisc::Insn load_b = load_a;
    load_b.a = 1;

    t81::tisc::Insn halt;
    halt.opcode = t81::tisc::Opcode::Halt;

    program.insns = {load_a, load_b, halt};

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(result.has_value());

    auto handle0 = vm->state().registers[0];
    auto handle1 = vm->state().registers[1];
    assert(handle0 > 0);
    assert(handle0 == handle1);
    assert(vm->state().weights_tensor_refs.size() == 1);
    const auto* native = vm->weights_tensor(handle0);
    assert(native != nullptr);
    std::vector<uint64_t> expected_shape{2, 2};
    assert(native->shape == expected_shape);
    return 0;
}
