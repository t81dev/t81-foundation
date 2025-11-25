#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/ir.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace t81::tisc;

void test_simple_program() {
    Program program;
    program.add_instruction({Opcode::LOADI, {Register{0}, Immediate{10}}});
    program.add_instruction({Opcode::HALT, {}});

    BinaryEmitter emitter;
    auto bytecode = emitter.emit(program);

    assert(bytecode.size() == 14);
    assert(bytecode[0] == static_cast<uint8_t>(Opcode::LOADI));
    assert(bytecode[1] == 0); // r0
    assert(bytecode[2] == 0);
    assert(bytecode[3] == 0);
    assert(bytecode[4] == 0);
    assert(bytecode[5] == 10); // immediate value
    assert(bytecode[6] == 0);
    assert(bytecode[7] == 0);
    assert(bytecode[8] == 0);
    assert(bytecode[9] == 0);
    assert(bytecode[10] == 0);
    assert(bytecode[11] == 0);
    assert(bytecode[12] == 0);
    assert(bytecode[13] == static_cast<uint8_t>(Opcode::HALT));

    std::cout << "BinaryEmitterTest test_simple_program passed!" << std::endl;
}

void test_jump() {
    Program program;
    program.add_instruction({Opcode::JMP, {Label{0}}});
    program.add_instruction({Opcode::LABEL, {Label{0}}});
    program.add_instruction({Opcode::HALT, {}});

    BinaryEmitter emitter;
    auto bytecode = emitter.emit(program);

    assert(bytecode.size() == 6);
    assert(bytecode[0] == static_cast<uint8_t>(Opcode::JMP));
    assert(bytecode[1] == 5); // address of HALT
    assert(bytecode[2] == 0);
    assert(bytecode[3] == 0);
    assert(bytecode[4] == 0);
    assert(bytecode[5] == static_cast<uint8_t>(Opcode::HALT));

    std::cout << "BinaryEmitterTest test_jump passed!" << std::endl;
}

int main() {
    test_simple_program();
    test_jump();
    return 0;
}
