#include "t81/axion/engine.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void test_instruction_counter_axion_engine() {
    // Create an Axion engine that allows a maximum of 5 instructions.
    auto engine = t81::axion::make_instruction_counting_engine(5);

    // Create a VM with our custom engine.
    auto vm = t81::vm::make_interpreter_vm(std::move(engine));

    // Create a simple program that will run for more than 5 instructions.
    t81::tisc::Program program;
    program.insns = {
        {t81::tisc::Opcode::Nop, 0, 0, 0},
        {t81::tisc::Opcode::Nop, 0, 0, 0},
        {t81::tisc::Opcode::Nop, 0, 0, 0},
        {t81::tisc::Opcode::Nop, 0, 0, 0},
        {t81::tisc::Opcode::Nop, 0, 0, 0},
        {t81::tisc::Opcode::Nop, 0, 0, 0}, // The 6th instruction should be denied.
        {t81::tisc::Opcode::Halt, 0, 0, 0},
    };

    vm->load_program(program);

    // Run the VM and expect a security fault.
    [[maybe_unused]] auto result = vm->run_to_halt();

    assert(!result.has_value() && "VM should have halted with a trap");
    assert(result.error() == t81::vm::Trap::SecurityFault &&
           "VM did not halt with a SecurityFault");

    std::cout << "AxionTest test_instruction_counter_axion_engine passed!"
              << std::endl;
}

int main() {
    test_instruction_counter_axion_engine();
    return 0;
}
