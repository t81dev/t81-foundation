#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/binary.hpp"
#include <unordered_map>
#include <stdexcept>

namespace t81 {
namespace tisc {

std::vector<uint8_t> BinaryEmitter::emit(const Program& program) {
    std::vector<uint8_t> bytecode;
    std::unordered_map<int, uint32_t> label_addresses;
    uint32_t current_address = 0;

    // First pass: calculate label addresses
    for (const auto& instr : program.instructions()) {
        if (instr.opcode == Opcode::LABEL) {
            label_addresses[std::get<Label>(instr.operands[0]).id] = current_address;
        } else {
            current_address++; // Assume each instruction is 1 byte for now
            for (const auto& operand : instr.operands) {
                if (std::holds_alternative<Immediate>(operand)) {
                    current_address += sizeof(long long);
                } else if (std::holds_alternative<Register>(operand)) {
                    current_address += sizeof(int);
                } else if (std::holds_alternative<Label>(operand)) {
                    current_address += sizeof(uint32_t);
                }
            }
        }
    }

    // Second pass: generate bytecode
    for (const auto& instr : program.instructions()) {
        if (instr.opcode != Opcode::LABEL) {
            bytecode.push_back(static_cast<uint8_t>(instr.opcode));

            for (const auto& operand : instr.operands) {
                if (std::holds_alternative<Immediate>(operand)) {
                    long long value = std::get<Immediate>(operand).value;
                    for (int i = 0; i < sizeof(long long); ++i) {
                        bytecode.push_back((value >> (i * 8)) & 0xFF);
                    }
                } else if (std::holds_alternative<Register>(operand)) {
                    int index = std::get<Register>(operand).index;
                    for (int i = 0; i < sizeof(int); ++i) {
                        bytecode.push_back((index >> (i * 8)) & 0xFF);
                    }
                } else if (std::holds_alternative<Label>(operand)) {
                    uint32_t address = label_addresses[std::get<Label>(operand).id];
                    for (int i = 0; i < sizeof(uint32_t); ++i) {
                        bytecode.push_back((address >> (i * 8)) & 0xFF);
                    }
                }
            }
        }
    }

    return bytecode;
}

} // namespace tisc
} // namespace t81
