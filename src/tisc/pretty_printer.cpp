#include "t81/tisc/pretty_printer.hpp"
#include <sstream>

namespace t81 {
namespace tisc {

using namespace ir;

namespace {

std::string opcode_to_string(Opcode opcode) {
    switch (opcode) {
        case Opcode::ADD:   return "ADD";
        case Opcode::SUB:   return "SUB";
        case Opcode::MUL:   return "MUL";
        case Opcode::DIV:   return "DIV";
        case Opcode::LOADI: return "LOADI";
        case Opcode::MOV:   return "MOV";
        case Opcode::NEG:   return "NEG";
        case Opcode::CMP:   return "CMP";
        case Opcode::MOD:   return "MOD";
        case Opcode::JMP:   return "JMP";
        case Opcode::JZ:    return "JZ";
        case Opcode::CALL:  return "CALL";
        case Opcode::RET:   return "RET";
        case Opcode::LABEL: return "L";
        default:            return "???";
    }
}

std::string operand_to_string(const Operand& op) {
    std::stringstream ss;
    std::visit([&ss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Register>) {
            ss << "r" << arg.index;
        } else if constexpr (std::is_same_v<T, Immediate>) {
            ss << arg.value;
        } else if constexpr (std::is_same_v<T, Label>) {
            ss << "L" << arg.id;
        }
    }, op);
    return ss.str();
}

std::string relation_to_string(ir::ComparisonRelation relation) {
    using R = ir::ComparisonRelation;
    switch (relation) {
        case R::Less: return "<";
        case R::LessEqual: return "<=";
        case R::Greater: return ">";
        case R::GreaterEqual: return ">=";
        case R::Equal: return "==";
        case R::NotEqual: return "!=";
        default: return "";
    }
}

} // namespace

std::string pretty_print(const ir::IntermediateProgram& program) {
    std::stringstream ss;
    for (const auto& instr : program.instructions()) {
        if (instr.opcode == Opcode::LABEL) {
            ss << "L" << std::get<Label>(instr.operands[0]).id << ":\n";
        } else {
            ss << "  " << opcode_to_string(instr.opcode);
            for (const auto& op : instr.operands) {
                ss << " " << operand_to_string(op);
            }
            if (instr.boolean_result && instr.relation != ir::ComparisonRelation::None) {
                ss << " ; relation=" << relation_to_string(instr.relation);
            }
            ss << "\n";
        }
    }
    return ss.str();
}

} // namespace tisc
} // namespace t81
