#ifndef T81_TISC_IR_HPP
#define T81_TISC_IR_HPP

#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <variant>
#include "t81/tensor.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tisc/type_alias.hpp"

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
    ADD, SUB, MUL, DIV, MOD, NEG,
    FADD, FSUB, FMUL, FDIV,
    FRACADD, FRACSUB, FRACMUL, FRACDIV,

    // Comparison
    CMP,

    // Data Movement
    MOV, LOADI,

    // Memory
    LOAD, STORE, PUSH, POP,

    // Control Flow
    JMP, JZ, JNZ, JN, JP,
    CALL, RET,

    // Conversions
    I2F, F2I, I2FRAC, FRAC2I,

    // Option/Result Helpers
    MAKE_OPTION_SOME, MAKE_OPTION_NONE,
    MAKE_RESULT_OK, MAKE_RESULT_ERR,
    OPTION_IS_SOME, OPTION_UNWRAP,
    RESULT_IS_OK, RESULT_UNWRAP_OK, RESULT_UNWRAP_ERR,

    // System
    NOP, HALT, TRAP,
    WEIGHTS_LOAD,

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
    void add_instruction(Instruction instr) {
        _instructions.push_back(std::move(instr));
    }

    const std::vector<Instruction>& instructions() const {
        return _instructions;
    }

    void add_type_alias(TypeAliasMetadata meta) {
        _type_aliases.push_back(std::move(meta));
    }

    const std::vector<TypeAliasMetadata>& type_aliases() const {
        return _type_aliases;
    }

    int add_tensor(t81::T729Tensor tensor) {
        _tensor_pool.push_back(std::move(tensor));
        return static_cast<int>(_tensor_pool.size());
    }

    const std::vector<t81::T729Tensor>& tensor_pool() const {
        return _tensor_pool;
    }

private:
    std::vector<Instruction> _instructions;
    std::vector<TypeAliasMetadata> _type_aliases;
    std::vector<t81::T729Tensor> _tensor_pool;
};

using TypeAliasMetadata = t81::tisc::TypeAliasMetadata;

} // namespace ir
} // namespace tisc
} // namespace t81

#endif // T81_TISC_IR_HPP
