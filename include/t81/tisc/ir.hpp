#ifndef T81_TISC_IR_HPP
#define T81_TISC_IR_HPP

#include <string>
#include <vector>
#include <variant>

namespace t81 {
namespace tisc {

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

    // System
    NOP, HALT, TRAP,

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
};

class Program {
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

} // namespace tisc
} // namespace t81

#endif // T81_TISC_IR_HPP
