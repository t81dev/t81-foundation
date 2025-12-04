#include "t81/tisc/binary_emitter.hpp"
#include <unordered_map>
#include <stdexcept>

namespace t81 {
namespace tisc {

// Helper to map IR opcodes to VM opcodes.
Opcode map_primitive_opcode(ir::Opcode ir_op, ir::PrimitiveKind kind) {
    switch (ir_op) {
        case ir::Opcode::ADD:
            switch (kind) {
                case ir::PrimitiveKind::Float: return Opcode::FAdd;
                case ir::PrimitiveKind::Fraction: return Opcode::FracAdd;
                default: return Opcode::Add;
            }
        case ir::Opcode::SUB:
            switch (kind) {
                case ir::PrimitiveKind::Float: return Opcode::FSub;
                case ir::PrimitiveKind::Fraction: return Opcode::FracSub;
                default: return Opcode::Sub;
            }
        case ir::Opcode::MUL:
            switch (kind) {
                case ir::PrimitiveKind::Float: return Opcode::FMul;
                case ir::PrimitiveKind::Fraction: return Opcode::FracMul;
                default: return Opcode::Mul;
            }
        case ir::Opcode::DIV:
            switch (kind) {
                case ir::PrimitiveKind::Float: return Opcode::FDiv;
                case ir::PrimitiveKind::Fraction: return Opcode::FracDiv;
                default: return Opcode::Div;
            }
        case ir::Opcode::MOD:
        case ir::Opcode::NEG:
        case ir::Opcode::CMP:
            // mod/neg/cmp always integer for now
            switch (ir_op) {
                case ir::Opcode::MOD: return Opcode::Mod;
                case ir::Opcode::NEG: return Opcode::Neg;
                case ir::Opcode::CMP: return Opcode::Cmp;
                default: break;
            }
        default:
            break;
    }
    throw std::runtime_error("Unsupported primitive opcode in binary emitter.");
}

Opcode map_relation(ir::ComparisonRelation relation) {
    switch (relation) {
        case ir::ComparisonRelation::Less: return Opcode::Less;
        case ir::ComparisonRelation::LessEqual: return Opcode::LessEqual;
        case ir::ComparisonRelation::Greater: return Opcode::Greater;
        case ir::ComparisonRelation::GreaterEqual: return Opcode::GreaterEqual;
        case ir::ComparisonRelation::Equal: return Opcode::Equal;
        case ir::ComparisonRelation::NotEqual: return Opcode::NotEqual;
        default: return Opcode::Cmp;
    }
}

Opcode map_opcode(const ir::Instruction& instr) {
    using O = ir::Opcode;
    if (instr.is_conversion) {
        switch (instr.opcode) {
            case O::I2F: return Opcode::I2F;
            case O::I2FRAC: return Opcode::I2Frac;
            default: break;
        }
    }
    switch (instr.opcode) {
        case O::ADD:
        case O::SUB:
        case O::MUL:
        case O::DIV:
        case O::MOD:
            return map_primitive_opcode(instr.opcode, instr.primitive);
        case O::FADD:
        case O::FSUB:
        case O::FMUL:
        case O::FDIV:
        case O::FRACADD:
        case O::FRACSUB:
        case O::FRACMUL:
        case O::FRACDIV:
            return map_primitive_opcode(instr.opcode, instr.primitive);
        case O::NEG: return Opcode::Neg;
        case O::CMP:
            if (instr.boolean_result && instr.relation != ir::ComparisonRelation::None) {
                return map_relation(instr.relation);
            }
            return Opcode::Cmp;
        case O::MOV: return Opcode::Mov;
        case O::LOADI: return Opcode::LoadImm;
        case O::LOAD: return Opcode::Load;
        case O::STORE: return Opcode::Store;
        case O::PUSH: return Opcode::Push;
        case O::POP: return Opcode::Pop;
        case O::JMP: return Opcode::Jump;
        case O::JZ: return Opcode::JumpIfZero;
        case O::JNZ: return Opcode::JumpIfNotZero;
        case O::JN: return Opcode::JumpIfNegative;
        case O::JP: return Opcode::JumpIfPositive;
        case O::CALL: return Opcode::Call;
        case O::RET: return Opcode::Ret;
        case O::I2F: return Opcode::I2F;
        case O::F2I: return Opcode::F2I;
        case O::I2FRAC: return Opcode::I2Frac;
        case O::FRAC2I: return Opcode::Frac2I;
        case O::MAKE_OPTION_SOME: return Opcode::MakeOptionSome;
        case O::MAKE_OPTION_NONE: return Opcode::MakeOptionNone;
        case O::MAKE_RESULT_OK: return Opcode::MakeResultOk;
        case O::MAKE_RESULT_ERR: return Opcode::MakeResultErr;
    case O::OPTION_IS_SOME: return Opcode::OptionIsSome;
    case O::OPTION_UNWRAP: return Opcode::OptionUnwrap;
    case O::RESULT_IS_OK: return Opcode::ResultIsOk;
    case O::RESULT_UNWRAP_OK: return Opcode::ResultUnwrapOk;
    case O::RESULT_UNWRAP_ERR: return Opcode::ResultUnwrapErr;
    case O::MAKE_ENUM_VARIANT: return Opcode::MakeEnumVariant;
    case O::MAKE_ENUM_VARIANT_PAYLOAD: return Opcode::MakeEnumVariantPayload;
    case O::ENUM_IS_VARIANT: return Opcode::EnumIsVariant;
    case O::ENUM_UNWRAP_PAYLOAD: return Opcode::EnumUnwrapPayload;
    case O::NOP: return Opcode::Nop;
    case O::HALT: return Opcode::Halt;
    case O::TRAP: return Opcode::Trap;
        case O::WEIGHTS_LOAD: return Opcode::WeightsLoad;
        default:
            throw std::runtime_error("Unsupported IR opcode in binary emitter.");
    }
}

Program BinaryEmitter::emit(const ir::IntermediateProgram& ir_program) {
    Program program;
    std::unordered_map<int, int> label_addresses;
    std::unordered_map<std::string, int> symbol_indices;
    auto ensure_symbol = [&](const std::string& text) -> int {
        auto it = symbol_indices.find(text);
        if (it != symbol_indices.end()) {
            return it->second;
        }
        program.symbol_pool.push_back(text);
        int index = static_cast<int>(program.symbol_pool.size());
        symbol_indices.emplace(program.symbol_pool.back(), index);
        return index;
    };
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
            vm_insn.opcode = map_opcode(instr);

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
                } else if (std::holds_alternative<ir::Immediate>(instr.operands[2])) {
                    vm_insn.c = std::get<ir::Immediate>(instr.operands[2]).value;
                }
            }

            if (instr.text_literal.has_value()) {
                int symbol_index = ensure_symbol(*instr.text_literal);
                vm_insn.b = symbol_index;
                vm_insn.literal_kind = instr.literal_kind;
            }

            program.insns.push_back(vm_insn);
        }
    }

    program.type_aliases = ir_program.type_aliases();
    program.tensor_pool = ir_program.tensor_pool();

    return program;
}

} // namespace tisc
} // namespace t81
