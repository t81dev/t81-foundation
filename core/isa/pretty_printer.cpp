#include "t81/isa/pretty_printer.hpp"
#include <sstream>

namespace t81 {
namespace tisc {

namespace {

std::string opcode_to_string(ir::Opcode opcode) {
  switch (opcode) {
    case ir::Opcode::ADD:
      return "ADD";
    case ir::Opcode::SUB:
      return "SUB";
    case ir::Opcode::MUL:
      return "MUL";
    case ir::Opcode::DIV:
      return "DIV";
    case ir::Opcode::LOADI:
      return "LOADI";
    case ir::Opcode::MOV:
      return "MOV";
    case ir::Opcode::NEG:
      return "NEG";
    case ir::Opcode::CMP:
      return "CMP";
    case ir::Opcode::MOD:
      return "MOD";
    case ir::Opcode::STRVECNEW:
      return "STRVECNEW";
    case ir::Opcode::STRVECPUSH:
      return "STRVECPUSH";
    case ir::Opcode::STRSPLIT:
      return "STRSPLIT";
    case ir::Opcode::STRJOIN:
      return "STRJOIN";
    case ir::Opcode::VECLEN:
      return "VECLEN";
    case ir::Opcode::VECEMPTY:
      return "VECEMPTY";
    case ir::Opcode::VECFIRST:
      return "VECFIRST";
    case ir::Opcode::VECLAST:
      return "VECLAST";
    case ir::Opcode::VECPUSH:
      return "VECPUSH";
    case ir::Opcode::VECPOP:
      return "VECPOP";
    case ir::Opcode::JMP:
      return "JMP";
    case ir::Opcode::JZ:
      return "JZ";
    case ir::Opcode::CALL:
      return "CALL";
    case ir::Opcode::RET:
      return "RET";
    case ir::Opcode::INT2BIGINT:
      return "INT2BIGINT";
    case ir::Opcode::BITAND:
      return "BitAnd";
    case ir::Opcode::BITOR:
      return "BitOr";
    case ir::Opcode::BITXOR:
      return "BitXor";
    case ir::Opcode::BITNOT:
      return "BitNot";
    case ir::Opcode::BITSHL:
      return "BitShl";
    case ir::Opcode::BITSHR:
      return "BitShr";
    case ir::Opcode::BITUSHR:
      return "BitUShr";
    case ir::Opcode::SYMLOAD:
      return "SYMLOAD";
    case ir::Opcode::SYMREWRITE:
      return "SYMREWRITE";
    case ir::Opcode::SYMCANON:
      return "SYMCANON";
    case ir::Opcode::SYMCONFLUENCE:
      return "SYMCONFLUENCE";
    case ir::Opcode::LABEL:
      return "L";
    default:
      return "???";
  }
}

std::string operand_to_string(const ir::Operand& op) {
  std::stringstream ss;
  std::visit(
      [&ss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ir::Register>) {
          ss << "r" << arg.index;
        } else if constexpr (std::is_same_v<T, ir::Immediate>) {
          ss << arg.value;
        } else if constexpr (std::is_same_v<T, ir::Label>) {
          ss << "L" << arg.id;
        }
      },
      op);
  return ss.str();
}

std::string relation_to_string(ir::ComparisonRelation relation) {
  using R = ir::ComparisonRelation;
  switch (relation) {
    case R::Less:
      return "<";
    case R::LessEqual:
      return "<=";
    case R::Greater:
      return ">";
    case R::GreaterEqual:
      return ">=";
    case R::Equal:
      return "==";
    case R::NotEqual:
      return "!=";
    default:
      return "";
  }
}

}  // namespace

std::string pretty_print(const ir::IntermediateProgram& program) {
  std::stringstream ss;
  for (const auto& instr : program.instructions()) {
    if (instr.opcode == ir::Opcode::LABEL) {
      ss << "L" << std::get<ir::Label>(instr.operands[0]).id << ":\n";
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

}  // namespace tisc
}  // namespace t81
