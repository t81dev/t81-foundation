#ifndef T81_TISC_IR_HPP
#define T81_TISC_IR_HPP

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "t81/isa/program.hpp"
#include "t81/isa/type_alias.hpp"
#include "t81/tensor.hpp"

namespace t81 {
namespace tisc {
namespace ir {

enum class PrimitiveKind {
  Unknown,
  Integer,
  Float,
  Fraction,
  Boolean,
};

enum class ComparisonRelation {
  None,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Equal,
  NotEqual,
};

enum class Opcode {
  // Arithmetic
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  NEG,
  FADD,
  FSUB,
  FMUL,
  FDIV,
  FSIN,
  FCOS,
  FTAN,
  FASIN,
  FACOS,
  FATAN,
  FSINH,
  FCOSH,
  FTANH,
  FSQRT,
  FEXP,
  FLOG,
  FPOW,
  FRACADD,
  FRACSUB,
  FRACMUL,
  FRACDIV,

  // Comparison
  CMP,

  // Data Movement
  MOV,
  LOADI,

  // Memory
  LOAD,
  STORE,
  PUSH,
  POP,

  // Control Flow
  JMP,
  JZ,
  JNZ,
  JN,
  JP,
  CALL,
  RET,

  // Conversions
  I2F,
  F2I,
  I2FRAC,
  FRAC2I,
  F2FRAC,
  FRAC2F,
  INT2BIGINT,

  // Option/Result Helpers
  MAKE_OPTION_SOME,
  MAKE_OPTION_NONE,
  MAKE_RESULT_OK,
  MAKE_RESULT_ERR,
  OPTION_IS_SOME,
  OPTION_UNWRAP,
  RESULT_IS_OK,
  RESULT_UNWRAP_OK,
  RESULT_UNWRAP_ERR,
  MAKE_ENUM_VARIANT,
  MAKE_ENUM_VARIANT_PAYLOAD,
  ENUM_IS_VARIANT,
  ENUM_UNWRAP_PAYLOAD,
  MAKE_COMPLEX,

  // System
  NOP,
  HALT,
  TRAP,
  PRINT,
  STRLEN,
  STREMPTY,
  VECLEN,
  VECEMPTY,
  VECFIRST,
  VECLAST,
  VECPUSH,
  VECPOP,
  STRCONCAT,
  STRSTARTSWITH,
  STRENDSWITH,
  STRCONTAINS,
  STRINDEXOF,
  STRREPLACE,
  STRVECNEW,
  STRVECPUSH,
  STRSPLIT,
  STRJOIN,
  WEIGHTS_LOAD,
  META_READ,
  META_WRITE,
  META_REFLECT,
  META_REFINE,
  TMATMUL,
  TVECADD,
  TTENDOT,
  TGET,
  TNEW,
  TSET,
  TSHAPE,

  // Neural
  TNEURAL_FWD,
  TNEURAL_BWD,

  // Bitwise
  BITAND,
  BITOR,
  BITXOR,
  BITNOT,
  BITSHL,
  BITSHR,
  BITUSHR,

  // Tier 4
  GOSSIP,
  MERGE,
  TICKSYNC,
  COHERENCE,
  DISTSEAL,
  SYMLOAD,
  SYMREWRITE,
  SYMCANON,
  SYMCONFLUENCE,

  // RFC-0026 AI-Native Inference (AI-M6)
  ATTN,     // Scaled dot-product attention: A=dest, B=q_reg, C=PACK(k_reg,v_reg)
  QMATMUL,  // Quantized matmul:             A=dest, B=act_reg, C=PACK(wt_reg,scale_reg)
  WLOAD,    // Weight load:                  A=dest, B=src_handle, C=policy
  GATHER,   // Sparse gather:                A=dest, B=src, C=PACK(idx_reg,axis_reg)
  SCATTER,  // Sparse scatter-add:           A=dest, B=dst, C=PACK(idx_reg,src_reg)

  // Map/Set Scaffolding
  MapNew,
  MapPut,
  MapGet,
  MapHas,
  MapRemove,
  MapKeys,
  MapSize,
  SetNew,
  SetAdd,
  SetRemove,
  SetHas,
  SetSize,

  // Ternary / Qutrit ops
  TAND,
  TOR,
  TXOR,

  // Axion Verification
  AXVERIFY,

  // Pseudo-instructions
  LABEL
};

struct Register {
  int index;
};

struct Immediate {
  long long value;
};

struct Label {
  int id;
};

using Operand = std::variant<Register, Immediate, Label>;

struct Instruction {
  Opcode opcode;
  std::vector<Operand> operands;
  PrimitiveKind primitive = PrimitiveKind::Unknown;
  bool boolean_result = false;
  bool is_conversion = false;
  ComparisonRelation relation = ComparisonRelation::None;
  tisc::LiteralKind literal_kind = tisc::LiteralKind::Int;
  std::optional<std::string> text_literal;

  Instruction(Opcode opcode_ = Opcode::NOP, std::vector<Operand> operands_ = {})
      : opcode(opcode_), operands(std::move(operands_)) {}
};

class IntermediateProgram {
public:
  void add_instruction(Instruction instr) { _instructions.push_back(std::move(instr)); }

  const std::vector<Instruction>& instructions() const { return _instructions; }

  void add_type_alias(TypeAliasMetadata meta) { _type_aliases.push_back(std::move(meta)); }

  const std::vector<TypeAliasMetadata>& type_aliases() const { return _type_aliases; }

  int add_tensor(t81::T729DynamicTensor tensor) {
    _tensor_pool.push_back(std::move(tensor));
    return static_cast<int>(_tensor_pool.size());
  }

  const std::vector<t81::T729DynamicTensor>& tensor_pool() const { return _tensor_pool; }

private:
  std::vector<Instruction> _instructions;
  std::vector<TypeAliasMetadata> _type_aliases;
  std::vector<t81::T729DynamicTensor> _tensor_pool;
};

using TypeAliasMetadata = t81::tisc::TypeAliasMetadata;

}  // namespace ir
}  // namespace tisc
}  // namespace t81

#endif  // T81_TISC_IR_HPP
