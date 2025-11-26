#include "t81/tisc/binary_emitter.hpp"
#include <unordered_map>
#include <stdexcept>

namespace t81 {
namespace tisc {

// Helper to map IR opcodes to VM opcodes.
Opcode map_opcode(ir::Opcode ir_op) {
    switch (ir_op) {
        case ir::Opcode::ADD: return Opcode::Add;
        case ir::Opcode::SUB: return Opcode::Sub;
        case ir::Opcode::MUL: return Opcode::Mul;
        case ir::Opcode::DIV: return Opcode::Div;
        case ir::Opcode::MOD: return Opcode::Mod;
        case ir::Opcode::NEG: return Opcode::Neg;
        case ir::Opcode::CMP: return Opcode::Cmp;
        case ir::Opcode::MOV: return Opcode::Mov;
        case ir::Opcode::LOADI: return Opcode::LoadImm;
        case ir::Opcode::LOAD: return Opcode::Load;
        case ir::Opcode::STORE: return Opcode::Store;
        case ir::Opcode::PUSH: return Opcode::Push;
        case ir::Opcode::POP: return Opcode::Pop;
        case ir::Opcode::JMP: return Opcode::Jump;
        case ir::Opcode::JZ: return Opcode::JumpIfZero;
        case ir::Opcode::JNZ: return Opcode::JumpIfNotZero;
        case ir::Opcode::JN: return Opcode::JumpIfNegative;
        case ir::Opcode::JP: return Opcode::JumpIfPositive;
        case ir::Opcode::CALL: return Opcode::Call;
        case ir::Opcode::RET: return Opcode::Ret;
        case ir::Opcode::I2F: return Opcode::I2F;
        case ir::Opcode::F2I: return Opcode::F2I;
        case ir::Opcode::I2FRAC: return Opcode::I2Frac;
        case ir::Opcode::FRAC2I: return Opcode::Frac2I;
        case ir::Opcode::NOP: return Opcode::Nop;
        case ir::Opcode::HALT: return Opcode::Halt;
        case ir::Opcode::TRAP: return Opcode::Trap;
        default:
            throw std::runtime_error("Unsupported IR opcode in binary emitter.");
    }
}

Program BinaryEmitter::emit(const ir::IntermediateProgram& ir_program) {
    Program program;
    std::unordered_map<int, int> label_addresses;
    int current_address = 0;

    // First pass: calculate label addresses
    for (const auto& instr : ir_program.instructions()) {
        if (instr.opcode == ir::Opcode::LABEL) {
            label_addresses[std::get<ir::Label>(instr.operands[0]).id] = current_address;
        } else {
            current_address++;
        }
    }

    // Second pass: generate bytecode
    for (const auto& instr : ir_program.instructions()) {
        if (instr.opcode != ir::Opcode::LABEL) {
            Insn vm_insn;
            vm_insn.opcode = map_opcode(instr.opcode);

            if (!instr.operands.empty()) {
                if (std::holds_alternative<ir::Register>(instr.operands[0])) {
                    vm_insn.a = std::get<ir::Register>(instr.operands[0]).index;
                } else if (std::holds_alternative<ir::Immediate>(instr.operands[0])) {
                     vm_insn.a = std::get<ir::Immediate>(instr.operands[0]).value;
                } else if (std::holds_alternative<ir::Label>(instr.operands[0])) {
                    vm_insn.a = label_addresses[std::get<ir::Label>(instr.operands[0]).id];
                }
            }

            if (instr.operands.size() > 1) {
                 if (std::holds_alternative<ir::Register>(instr.operands[1])) {
                    vm_insn.b = std::get<ir::Register>(instr.operands[1]).index;
                } else if (std::holds_alternative<ir::Immediate>(instr.operands[1])) {
                     vm_insn.b = std::get<ir::Immediate>(instr.operands[1]).value;
                }
            }

            if (instr.operands.size() > 2) {
                if (std::holds_alternative<ir::Register>(instr.operands[2])) {
                    vm_insn.c = std::get<ir::Register>(instr.operands[2]).index;
                }
            }

            program.insns.push_back(vm_insn);
        }
    }

    return program;
}

} // namespace tisc
} // namespace t81
