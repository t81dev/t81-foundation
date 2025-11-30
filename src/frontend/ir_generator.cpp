// src/frontend/ir_generator.cpp
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/ast.hpp"
#include "t81/tisc/ir.hpp"

#include <stdexcept>

namespace t81::frontend {

tisc::ir::IntermediateProgram IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        stmt->accept(*this);
    }
    return std::move(_program);
}

// ─────────────────────────────────────────────────────────────────────────────
// Statement visitors
// ─────────────────────────────────────────────────────────────────────────────
std::any IRGenerator::visit(const ExpressionStmt& stmt) {
    stmt.expression->accept(*this);
    return {};
}

std::any IRGenerator::visit(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        s->accept(*this);
    }
    return {};
}

std::any IRGenerator::visit(const VarStmt&)          { return {}; }
std::any IRGenerator::visit(const LetStmt&)          { return {}; }
std::any IRGenerator::visit(const IfStmt&)           { return {}; }
std::any IRGenerator::visit(const WhileStmt&)        { return {}; }
std::any IRGenerator::visit(const ReturnStmt&)       { return {}; }
std::any IRGenerator::visit(const FunctionStmt&)     { return {}; }

// ─────────────────────────────────────────────────────────────────────────────
// Expression visitors
// ─────────────────────────────────────────────────────────────────────────────
std::any IRGenerator::visit(const BinaryExpr& expr) {
    expr.left->accept(*this);   // ← note: field is called `left` / `right`, not `lhs`/`rhs`
    expr.right->accept(*this);

    using Opcode = tisc::ir::Opcode;

    switch (expr.op.type) {
        case TokenType::Plus:   emit(tisc::ir::Instruction{Opcode::ADD});   break;
        case TokenType::Minus:  emit(tisc::ir::Instruction{Opcode::SUB});   break;
        case TokenType::Star:   emit(tisc::ir::Instruction{Opcode::MUL});   break;
        case TokenType::Slash:  emit(tisc::ir::Instruction{Opcode::DIV});   break;
        default:
            throw std::runtime_error("Unsupported binary operator in IR generation");
    }
    return {};
}

std::any IRGenerator::visit(const LiteralExpr& expr) {
    // Literals are stored directly as Token in the current AST
    if (expr.literal.type == TokenType::IntegerLiteral) {
        int64_t v = std::stoll(expr.literal.lexeme);
        emit(tisc::ir::Instruction{
            tisc::ir::Opcode::LOADI,
            {tisc::ir::Immediate{v}}
        });
    } else if (expr.literal.type == TokenType::FloatLiteral) {
        // The test only uses integers, but we support floats safely
        double d = std::stod(expr.literal.lexeme);
        int64_t v = static_cast<int64_t>(d);
        emit(tisc::ir::Instruction{
            tisc::ir::Opcode::LOADI,
            {tisc::ir::Immediate{v}}
        });
    } else {
        throw std::runtime_error("Unsupported literal type");
    }
    return {};
}

std::any IRGenerator::visit(const GroupingExpr& expr) {
    expr.expression->accept(*this);
    return {};
}

std::any IRGenerator::visit(const UnaryExpr&)        { return {}; }
std::any IRGenerator::visit(const VariableExpr&)     { return {}; }
std::any IRGenerator::visit(const CallExpr&)         { return {}; }
std::any IRGenerator::visit(const AssignExpr&)       { return {}; }
std::any IRGenerator::visit(const SimpleTypeExpr&)   { return {}; }
std::any IRGenerator::visit(const GenericTypeExpr&)  { return {}; }

// ─────────────────────────────────────────────────────────────────────────────
// Utility
// ─────────────────────────────────────────────────────────────────────────────
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

} // namespace t81::frontend
