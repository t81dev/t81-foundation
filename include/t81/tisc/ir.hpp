#ifndef T81_TISC_IR_HPP
#define T81_TISC_IR_HPP

#include <optional>
#include <string>
#include <vector>
#include <variant>
#include "t81/tisc/program.hpp"

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
};

class IntermediateProgram {
public:
    void add_instruction(Instruction instr) {
        _instructions.push_back(std::move(instr));
    }

    const std::vector<Instruction>& instructions() const {
        return _instructions;
    }

private:
    std::vector<Instruction> _instructions;
};

} // namespace ir
} // namespace tisc
} // namespace t81

#endif // T81_TISC_IR_HPP
