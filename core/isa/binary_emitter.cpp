#include "t81/isa/binary_emitter.hpp"
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace t81 {
namespace tisc {

// Helper to map IR opcodes to VM opcodes.
Opcode map_primitive_opcode(ir::Opcode ir_op, ir::PrimitiveKind kind) {
  switch (ir_op) {
    case ir::Opcode::ADD:
    case ir::Opcode::FADD:
    case ir::Opcode::FRACADD:
      switch (kind) {
        case ir::PrimitiveKind::Float:
          return Opcode::FAdd;
        case ir::PrimitiveKind::Fraction:
          return Opcode::FracAdd;
        default:
          return Opcode::Add;
      }
    case ir::Opcode::SUB:
    case ir::Opcode::FSUB:
    case ir::Opcode::FRACSUB:
      switch (kind) {
        case ir::PrimitiveKind::Float:
          return Opcode::FSub;
        case ir::PrimitiveKind::Fraction:
          return Opcode::FracSub;
        default:
          return Opcode::Sub;
      }
    case ir::Opcode::MUL:
    case ir::Opcode::FMUL:
    case ir::Opcode::FRACMUL:
      switch (kind) {
        case ir::PrimitiveKind::Float:
          return Opcode::FMul;
        case ir::PrimitiveKind::Fraction:
          return Opcode::FracMul;
        default:
          return Opcode::Mul;
      }
    case ir::Opcode::DIV:
    case ir::Opcode::FDIV:
    case ir::Opcode::FRACDIV:
      switch (kind) {
        case ir::PrimitiveKind::Float:
          return Opcode::FDiv;
        case ir::PrimitiveKind::Fraction:
          return Opcode::FracDiv;
        default:
          return Opcode::Div;
      }
    case ir::Opcode::MOD:
    case ir::Opcode::NEG:
    case ir::Opcode::CMP:
      // mod/neg/cmp always integer for now
      switch (ir_op) {
        case ir::Opcode::MOD:
          return Opcode::Mod;
        case ir::Opcode::NEG:
          return Opcode::Neg;
        case ir::Opcode::CMP:
          return Opcode::Cmp;
        default:
          break;
      }
    default:
      break;
  }
  throw std::runtime_error("Unsupported primitive opcode in binary emitter.");
}

Opcode map_relation(ir::ComparisonRelation relation) {
  switch (relation) {
    case ir::ComparisonRelation::Less:
      return Opcode::Less;
    case ir::ComparisonRelation::LessEqual:
      return Opcode::LessEqual;
    case ir::ComparisonRelation::Greater:
      return Opcode::Greater;
    case ir::ComparisonRelation::GreaterEqual:
      return Opcode::GreaterEqual;
    case ir::ComparisonRelation::Equal:
      return Opcode::Equal;
    case ir::ComparisonRelation::NotEqual:
      return Opcode::NotEqual;
    default:
      return Opcode::Cmp;
  }
}

T81BigInt parse_decimal_bigint_literal(std::string_view text) {
  if (text.empty()) {
    throw std::runtime_error("Empty BigInt literal.");
  }
  bool neg = false;
  std::size_t pos = 0;
  if (text[pos] == '+' || text[pos] == '-') {
    neg = (text[pos] == '-');
    ++pos;
    if (pos >= text.size()) {
      throw std::runtime_error("Invalid BigInt literal sign.");
    }
  }
  T81BigInt acc = T81BigInt::from_i64(0);
  const T81BigInt ten = T81BigInt::from_i64(10);
  bool have_digit = false;
  for (; pos < text.size(); ++pos) {
    const unsigned char ch = static_cast<unsigned char>(text[pos]);
    if (!std::isdigit(ch)) {
      throw std::runtime_error("Invalid character in BigInt literal.");
    }
    acc = acc * ten;
    acc = acc + T81BigInt::from_i64(static_cast<std::int64_t>(text[pos] - '0'));
    have_digit = true;
  }
  if (!have_digit) {
    throw std::runtime_error("BigInt literal missing digits.");
  }
  return neg ? -acc : acc;
}

Opcode map_opcode(const ir::Instruction& instr) {
  using O = ir::Opcode;
  if (instr.is_conversion) {
    switch (instr.opcode) {
      case O::I2F:
        return Opcode::I2F;
      case O::I2FRAC:
        return Opcode::I2Frac;
      default:
        break;
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
    case O::FSIN:
      return Opcode::FSin;
    case O::FCOS:
      return Opcode::FCos;
    case O::FTAN:
      return Opcode::FTan;
    case O::FASIN:
      return Opcode::FAsin;
    case O::FACOS:
      return Opcode::FAcos;
    case O::FATAN:
      return Opcode::FAtan;
    case O::FSINH:
      return Opcode::FSinh;
    case O::FCOSH:
      return Opcode::FCosh;
    case O::FTANH:
      return Opcode::FTanh;
    case O::FSQRT:
      return Opcode::FSqrt;
    case O::FEXP:
      return Opcode::FExp;
    case O::FLOG:
      return Opcode::FLog;
    case O::FPOW:
      return Opcode::FPow;
    case O::NEG:
      return Opcode::Neg;
    case O::CMP:
      if (instr.boolean_result && instr.relation != ir::ComparisonRelation::None) {
        return map_relation(instr.relation);
      }
      return Opcode::Cmp;
    case O::MOV:
      return Opcode::Mov;
    case O::LOADI:
      return Opcode::LoadImm;
    case O::LOAD:
      return Opcode::Load;
    case O::STORE:
      return Opcode::Store;
    case O::PUSH:
      return Opcode::Push;
    case O::POP:
      return Opcode::Pop;
    case O::JMP:
      return Opcode::Jump;
    case O::JZ:
      return Opcode::JumpIfZero;
    case O::JNZ:
      return Opcode::JumpIfNotZero;
    case O::JN:
      return Opcode::JumpIfNegative;
    case O::JP:
      return Opcode::JumpIfPositive;
    case O::CALL:
      return Opcode::Call;
    case O::RET:
      return Opcode::Ret;
    case O::I2F:
      return Opcode::I2F;
    case O::F2I:
      return Opcode::F2I;
    case O::I2FRAC:
      return Opcode::I2Frac;
    case O::FRAC2I:
      return Opcode::Frac2I;
    case O::F2FRAC:
      return Opcode::F2Frac;
    case O::FRAC2F:
      return Opcode::Frac2F;
    case O::MAKE_OPTION_SOME:
      return Opcode::MakeOptionSome;
    case O::MAKE_OPTION_NONE:
      return Opcode::MakeOptionNone;
    case O::MAKE_RESULT_OK:
      return Opcode::MakeResultOk;
    case O::MAKE_RESULT_ERR:
      return Opcode::MakeResultErr;
    case O::OPTION_IS_SOME:
      return Opcode::OptionIsSome;
    case O::OPTION_UNWRAP:
      return Opcode::OptionUnwrap;
    case O::RESULT_IS_OK:
      return Opcode::ResultIsOk;
    case O::RESULT_UNWRAP_OK:
      return Opcode::ResultUnwrapOk;
    case O::RESULT_UNWRAP_ERR:
      return Opcode::ResultUnwrapErr;
    case O::MAKE_ENUM_VARIANT:
      return Opcode::MakeEnumVariant;
    case O::MAKE_ENUM_VARIANT_PAYLOAD:
      return Opcode::MakeEnumVariantPayload;
    case O::ENUM_IS_VARIANT:
      return Opcode::EnumIsVariant;
    case O::ENUM_UNWRAP_PAYLOAD:
      return Opcode::EnumUnwrapPayload;
    case O::MAKE_COMPLEX:
      return Opcode::MakeComplex;
    case O::NOP:
      return Opcode::Nop;
    case O::HALT:
      return Opcode::Halt;
    case O::TRAP:
      return Opcode::Trap;
    case O::PRINT:
      return Opcode::Print;
    case O::STRLEN:
      return Opcode::StrLen;
    case O::STREMPTY:
      return Opcode::StrEmpty;
    case O::VECLEN:
      return Opcode::VecLen;
    case O::VECEMPTY:
      return Opcode::VecEmpty;
    case O::VECFIRST:
      return Opcode::VecFirst;
    case O::VECLAST:
      return Opcode::VecLast;
    case O::VECPUSH:
      return Opcode::VecPush;
    case O::VECPOP:
      return Opcode::VecPop;
    case O::STRCONCAT:
      return Opcode::StrConcat;
    case O::STRSTARTSWITH:
      return Opcode::StrStartsWith;
    case O::STRENDSWITH:
      return Opcode::StrEndsWith;
    case O::STRCONTAINS:
      return Opcode::StrContains;
    case O::STRINDEXOF:
      return Opcode::StrIndexOf;
    case O::STRREPLACE:
      return Opcode::StrReplace;
    case O::STRVECNEW:
      return Opcode::StrVecNew;
    case O::STRVECPUSH:
      return Opcode::StrVecPush;
    case O::STRSPLIT:
      return Opcode::StrSplit;
    case O::STRJOIN:
      return Opcode::StrJoin;
    case O::WEIGHTS_LOAD:
      return Opcode::WeightsLoad;
    case O::META_READ:
      return Opcode::MetaRead;
    case O::META_WRITE:
      return Opcode::MetaWrite;
    case O::META_REFLECT:
      return Opcode::MetaReflect;
    case O::META_REFINE:
      return Opcode::MetaRefine;
    case O::TMATMUL:
      return Opcode::TMatMul;
    case O::TVECADD:
      return Opcode::TVecAdd;
    case O::TTENDOT:
      return Opcode::TTenDot;
    case O::TGET:
      return Opcode::TGet;
    case O::TNEW:
      return Opcode::TNew;
    case O::TSET:
      return Opcode::TSet;
    case O::TSHAPE:
      return Opcode::TShape;
    case O::TNEURAL_FWD:
      return Opcode::TNeuralFwd;
    case O::TNEURAL_BWD:
      return Opcode::TNeuralBwd;
    case O::BITAND:
      return Opcode::BitAnd;
    case O::BITOR:
      return Opcode::BitOr;
    case O::BITXOR:
      return Opcode::BitXor;
    case O::BITNOT:
      return Opcode::BitNot;
    case O::BITSHL:
      return Opcode::BitShl;
    case O::BITSHR:
      return Opcode::BitShr;
    case O::BITUSHR:
      return Opcode::BitUShr;
    case O::MapNew:
      return Opcode::MapNew;
    case O::MapPut:
      return Opcode::MapPut;
    case O::MapGet:
      return Opcode::MapGet;
    case O::MapHas:
      return Opcode::MapHas;
    case O::MapRemove:
      return Opcode::MapRemove;
    case O::MapKeys:
      return Opcode::MapKeys;
    case O::MapSize:
      return Opcode::MapSize;
    case O::SetNew:
      return Opcode::SetNew;
    case O::SetAdd:
      return Opcode::SetAdd;
    case O::SetRemove:
      return Opcode::SetRemove;
    case O::SetHas:
      return Opcode::SetHas;
    case O::SetSize:
      return Opcode::SetSize;
    case O::AXVERIFY:
      return Opcode::AxVerify;
    case O::TAND:
      return Opcode::TAnd;
    case O::TOR:
      return Opcode::TOr;
    case O::TXOR:
      return Opcode::TXor;
    default:
      // Fallback for Tier 4 opcodes if they have direct mapping
      switch (instr.opcode) {
        case O::GOSSIP:
          return Opcode::Gossip;
        case O::MERGE:
          return Opcode::Merge;
        case O::TICKSYNC:
          return Opcode::TickSync;
        case O::COHERENCE:
          return Opcode::Coherence;
        case O::DISTSEAL:
          return Opcode::DistSeal;
        case O::SYMLOAD:
          return Opcode::SymLoad;
        case O::SYMREWRITE:
          return Opcode::SymRewrite;
        case O::SYMCANON:
          return Opcode::SymCanon;
        case O::SYMCONFLUENCE:
          return Opcode::SymConfluence;
        default:
          throw std::runtime_error("Unsupported IR opcode in binary emitter.");
      }
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
      vm_insn.literal_kind = instr.literal_kind;

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
        } else if (std::holds_alternative<ir::Label>(instr.operands[1])) {
          vm_insn.b = label_addresses[std::get<ir::Label>(instr.operands[1]).id];
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
        switch (instr.literal_kind) {
          case LiteralKind::FloatHandle: {
            double parsed = std::stod(*instr.text_literal);
            program.float_pool.push_back(parsed);
            vm_insn.b = static_cast<std::int64_t>(program.float_pool.size());
            break;
          }
          case LiteralKind::SymbolHandle: {
            int symbol_index = ensure_symbol(*instr.text_literal);
            vm_insn.b = symbol_index;
            break;
          }
          case LiteralKind::BigIntHandle: {
            program.bigint_pool.push_back(parse_decimal_bigint_literal(*instr.text_literal));
            vm_insn.b = static_cast<std::int64_t>(program.bigint_pool.size());
            break;
          }
          default:
            throw std::runtime_error("Unsupported text literal kind in binary emitter.");
        }
      }

      program.insns.push_back(vm_insn);
    }
  }

  program.type_aliases = ir_program.type_aliases();
  program.tensor_pool = ir_program.tensor_pool();

  return program;
}

}  // namespace tisc
}  // namespace t81
