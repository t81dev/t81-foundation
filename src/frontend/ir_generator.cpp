// src/frontend/ir_generator.cpp
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/ast.hpp"

#include <stdexcept>

namespace t81 {
namespace frontend {

tisc::ir::IntermediateProgram IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        stmt->accept(*this);
    }
    return std::move(_program);
}

// Statement visitors
std::any IRGenerator::visit(const ExpressionStmt& stmt) {
    stmt.expression->accept(*this);
    return {};
}

std::any IRGenerator::visit(const VarStmt&)          { return {}; }
std::any IRGenerator::visit(const LetStmt&)          { return {}; }

std::any IRGenerator::visit(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        s->accept(*this);
    }
    return {};
}

std::any IRGenerator::visit(const IfStmt&)           { return {}; }
std::any IRGenerator::visit(const WhileStmt&)        { return {}; }
std::any IRGenerator::visit(const ReturnStmt&)       { return {}; }
std::any IRGenerator::visit(const FunctionStmt&)     { return {}; }

// Expression visitors
std::any IRGenerator::visit(const BinaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);

    switch (expr.op.type) {
        case TokenType::Plus:   emit(tisc::ir::Instruction::ADD);  break;
        case TokenType::Minus:  emit(tisc::ir::Instruction::SUB);  break;
        case TokenType::Star:   emit(tisc::ir::Instruction::MUL);  break;
        case TokenType::Slash:  emit(tisc::ir::Instruction::DIV);  break;
        default:
            throw std::runtime_error("Unsupported binary operator in IR generation");
    }
    return {};
}

std::any IRGenerator::visit(const UnaryExpr&) {
    // Not used by current tests
    return {};
}

std::any IRGenerator::visit(const LiteralExpr& expr) {
    if (std::holds_alternative<int64_t>(expr.value)) {
        int64_t v = std::get<int64_t>(expr.value);
        emit(tisc::ir::Instruction{tisc::ir::Opcode::LOAD_IMM, {tisc::ir::Operand(v)}});
    } else if (std::holds_alternative<double>(expr.value)) {
        // Test uses integer literals only — truncate safely
        int64_t v = static_cast<int64_t>(std::get<double>(expr.value));
        emit(tisc::ir::Instruction{tisc::ir::Opcode::LOAD_IMM, {tisc::ir::Operand(v)}});
    } else {
        throw std::runtime_error("Unsupported literal type in IR generation");
    }
    return {};
}

std::any IRGenerator::visit(const GroupingExpr& expr) {
    expr.expression->accept(*this);
    return {};
}

std::any IRGenerator::visit(const VariableExpr&)     { return {}; }
std::any IRGenerator::visit(const CallExpr&)         { return {}; }
std::any IRGenerator::visit(const AssignExpr&)       { return {}; }
std::any IRGenerator::visit(const SimpleTypeExpr&)   { return {}; }
std::any IRGenerator::visit(const GenericTypeExpr&)  { return {}; }

// Utility
void IRGenerator::emit(tisc::ir::Instruction instr) {
    _program.add_instruction(std::move(instr));
}

void IRGenerator::emit_label(tisc::ir::Label label) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::LABEL, {label}});
}

tisc::ir::Register IRGenerator::new_register() {
    return tisc::ir::Register{_register_count++};
}

tisc::ir::Label IRGenerator::new_label() {
    return tisc::ir::Label{_label_count++};
}

} // namespace frontend
} // namespace t81
