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

void test_comparison_relation() {
    IntermediateProgram ir_program;
    Instruction cmp{Opcode::CMP, {Register{0}, Register{1}, Register{2}}};
    cmp.boolean_result = true;
    cmp.relation = ComparisonRelation::LessEqual;
    ir_program.add_instruction(cmp);
    ir_program.add_instruction({Opcode::HALT, {}});

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    assert(program.insns.size() == 2);
    assert(program.insns[0].opcode == t81::tisc::Opcode::LessEqual);
    assert(program.insns[0].a == 0);
    assert(program.insns[0].b == 1);
    assert(program.insns[0].c == 2);

    std::cout << "BinaryEmitterTest test_comparison_relation passed!" << std::endl;
}

void test_all_comparison_relations() {
    std::vector<std::pair<ComparisonRelation, t81::tisc::Opcode>> cases = {
        {ComparisonRelation::Less, t81::tisc::Opcode::Less},
        {ComparisonRelation::LessEqual, t81::tisc::Opcode::LessEqual},
        {ComparisonRelation::Greater, t81::tisc::Opcode::Greater},
        {ComparisonRelation::GreaterEqual, t81::tisc::Opcode::GreaterEqual},
        {ComparisonRelation::Equal, t81::tisc::Opcode::Equal},
        {ComparisonRelation::NotEqual, t81::tisc::Opcode::NotEqual},
    };

    for (const auto& [relation, expected_opcode] : cases) {
        IntermediateProgram ir_program;
        Instruction cmp{Opcode::CMP, {Register{0}, Register{1}, Register{2}}};
        cmp.boolean_result = true;
        cmp.relation = relation;
        ir_program.add_instruction(cmp);
        ir_program.add_instruction({Opcode::HALT, {}});

        t81::tisc::BinaryEmitter emitter;
        auto program = emitter.emit(ir_program);

        assert(program.insns.size() == 2);
        assert(program.insns[0].opcode == expected_opcode);
        assert(program.insns[0].a == 0);
        assert(program.insns[0].b == 1);
        assert(program.insns[0].c == 2);
    }

    std::cout << "BinaryEmitterTest test_all_comparison_relations passed!" << std::endl;
}

int main() {
    test_simple_program();
    test_jump();
    test_comparison_relation();
    test_all_comparison_relations();
    return 0;
}
