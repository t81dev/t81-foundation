#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/ir_generator.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ir_inspector <source.t81>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "error: cannot open source file\n";
        return 1;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        std::cerr << "Parse errors detected\n";
        return 1;
    }

    t81::frontend::SemanticAnalyzer semantic_analyzer(stmts);
    semantic_analyzer.analyze();
    if (semantic_analyzer.had_error()) {
        std::cerr << "Semantic errors detected\n";
        return 1;
    }

    t81::frontend::IRGenerator generator;
    auto program = generator.generate(stmts);
    const auto& instructions = program.instructions();

    auto opcode_name = [](t81::tisc::ir::Opcode op) -> const char* {
        switch (op) {
            case t81::tisc::ir::Opcode::ADD: return "ADD";
            case t81::tisc::ir::Opcode::SUB: return "SUB";
            case t81::tisc::ir::Opcode::MUL: return "MUL";
            case t81::tisc::ir::Opcode::DIV: return "DIV";
            case t81::tisc::ir::Opcode::MOD: return "MOD";
            case t81::tisc::ir::Opcode::NEG: return "NEG";
            case t81::tisc::ir::Opcode::CMP: return "CMP";
            case t81::tisc::ir::Opcode::MOV: return "MOV";
            case t81::tisc::ir::Opcode::LOAD: return "LOAD";
            case t81::tisc::ir::Opcode::STORE: return "STORE";
            case t81::tisc::ir::Opcode::PUSH: return "PUSH";
            case t81::tisc::ir::Opcode::POP: return "POP";
            case t81::tisc::ir::Opcode::JMP: return "JMP";
            case t81::tisc::ir::Opcode::JZ: return "JZ";
            case t81::tisc::ir::Opcode::JNZ: return "JNZ";
            case t81::tisc::ir::Opcode::JN: return "JN";
            case t81::tisc::ir::Opcode::JP: return "JP";
            case t81::tisc::ir::Opcode::CALL: return "CALL";
            case t81::tisc::ir::Opcode::RET: return "RET";
            case t81::tisc::ir::Opcode::I2F: return "I2F";
            case t81::tisc::ir::Opcode::F2I: return "F2I";
            case t81::tisc::ir::Opcode::I2FRAC: return "I2FRAC";
            case t81::tisc::ir::Opcode::FRAC2I: return "FRAC2I";
            case t81::tisc::ir::Opcode::MAKE_OPTION_SOME: return "MAKE_OPTION_SOME";
            case t81::tisc::ir::Opcode::MAKE_OPTION_NONE: return "MAKE_OPTION_NONE";
            case t81::tisc::ir::Opcode::MAKE_RESULT_OK: return "MAKE_RESULT_OK";
            case t81::tisc::ir::Opcode::MAKE_RESULT_ERR: return "MAKE_RESULT_ERR";
            case t81::tisc::ir::Opcode::OPTION_IS_SOME: return "OPTION_IS_SOME";
            case t81::tisc::ir::Opcode::OPTION_UNWRAP: return "OPTION_UNWRAP";
            case t81::tisc::ir::Opcode::RESULT_IS_OK: return "RESULT_IS_OK";
            case t81::tisc::ir::Opcode::RESULT_UNWRAP_OK: return "RESULT_UNWRAP_OK";
            case t81::tisc::ir::Opcode::RESULT_UNWRAP_ERR: return "RESULT_UNWRAP_ERR";
            case t81::tisc::ir::Opcode::NOP: return "NOP";
            case t81::tisc::ir::Opcode::HALT: return "HALT";
            case t81::tisc::ir::Opcode::TRAP: return "TRAP";
            case t81::tisc::ir::Opcode::LABEL: return "LABEL";
            default: return "UNKNOWN";
        }
    };

    auto primitive_name = [](t81::tisc::ir::PrimitiveKind kind) -> const char* {
        switch (kind) {
            case t81::tisc::ir::PrimitiveKind::Integer: return "Int";
            case t81::tisc::ir::PrimitiveKind::Float: return "Float";
            case t81::tisc::ir::PrimitiveKind::Fraction: return "Frac";
            case t81::tisc::ir::PrimitiveKind::Boolean: return "Bool";
            default: return "Unknown";
        }
    };

    auto relation_name = [](t81::tisc::ir::ComparisonRelation relation) -> const char* {
        using R = t81::tisc::ir::ComparisonRelation;
        switch (relation) {
            case R::Less: return "Less";
            case R::LessEqual: return "LessEqual";
            case R::Greater: return "Greater";
            case R::GreaterEqual: return "GreaterEqual";
            case R::Equal: return "Equal";
            case R::NotEqual: return "NotEqual";
            default: return "None";
        }
    };

    std::cout << "IR Instructions (" << instructions.size() << " total):\n";
    for (const auto& inst : instructions) {
        std::cout << std::setw(15) << std::left << opcode_name(inst.opcode);
        std::cout << " [" << primitive_name(inst.primitive) << (inst.boolean_result ? " Bool" : "") << "]";
        if (inst.boolean_result && inst.relation != t81::tisc::ir::ComparisonRelation::None) {
            std::cout << " <" << relation_name(inst.relation) << ">";
        }
        if (!inst.operands.empty()) {
            std::cout << " | ";
            for (size_t i = 0; i < inst.operands.size(); ++i) {
                std::visit([](auto&& operand) {
                    using T = std::decay_t<decltype(operand)>;
                    if constexpr (std::is_same_v<T, t81::tisc::ir::Register>) {
                        std::cout << "R" << operand.index;
                    } else if constexpr (std::is_same_v<T, t81::tisc::ir::Immediate>) {
                        std::cout << operand.value;
                    } else if constexpr (std::is_same_v<T, t81::tisc::ir::Label>) {
                        std::cout << "Lbl" << operand.id;
                    }
                }, inst.operands[i]);
                if (i + 1 < inst.operands.size()) std::cout << ", ";
            }
        }
        std::cout << "\n";
    }

    return 0;
}
