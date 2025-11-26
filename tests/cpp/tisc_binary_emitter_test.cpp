#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/ir.hpp"
#include <cassert>
#include <iostream>

using namespace t81::tisc::ir;

void test_simple_program() {
    IntermediateProgram ir_program;
    ir_program.add_instruction({Opcode::LOADI, {Register{0}, Immediate{10}}});
    ir_program.add_instruction({Opcode::HALT, {}});

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    assert(program.insns.size() == 2);
    assert(program.insns[0].opcode == t81::tisc::Opcode::LoadImm);
    assert(program.insns[0].a == 0);
    assert(program.insns[0].b == 10);
    assert(program.insns[1].opcode == t81::tisc::Opcode::Halt);

    std::cout << "BinaryEmitterTest test_simple_program passed!" << std::endl;
}

void test_jump() {
    IntermediateProgram ir_program;
    ir_program.add_instruction({Opcode::JMP, {Label{0}}});
    ir_program.add_instruction({Opcode::LABEL, {Label{0}}});
    ir_program.add_instruction({Opcode::HALT, {}});

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    assert(program.insns.size() == 2);
    assert(program.insns[0].opcode == t81::tisc::Opcode::Jump);
    assert(program.insns[0].a == 1); // address of HALT
    assert(program.insns[1].opcode == t81::tisc::Opcode::Halt);

    std::cout << "BinaryEmitterTest test_jump passed!" << std::endl;
}

int main() {
    test_simple_program();
    test_jump();
    return 0;
}
