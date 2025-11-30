// include/t81/frontend/ir_generator.hpp
#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/tisc/ir.hpp"
#include <any>
#include <stdexcept>
#include <string>
#include <string_view>

namespace t81::frontend {

class IRGenerator : public ExprVisitor, public StmtVisitor {
public:
    tisc::ir::IntermediateProgram generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            stmt->accept(*this);
        }
        return std::move(_program);
    }

    // Statements
    std::any visit(const ExpressionStmt& stmt) override {
        stmt.expression->accept(*this);
        return {};
    }

    std::any visit(const BlockStmt& stmt) override {
        for (const auto& s : stmt.statements) s->accept(*this);
        return {};
    }

    std::any visit(const VarStmt&) override          { return {}; }
    std::any visit(const LetStmt&) override          { return {}; }
    std::any visit(const IfStmt&) override           { return {}; }
    std::any visit(const WhileStmt&) override        { return {}; }
    std::any visit(const ReturnStmt&) override       { return {}; }
    std::any visit(const FunctionStmt&) override     { return {}; }

    // Expressions
    std::any visit(const BinaryExpr& expr) override {
        expr.left->accept(*this);
        expr.right->accept(*this);

        using O = tisc::ir::Opcode;
        switch (expr.op.type) {
            case TokenType::Plus:   emit(tisc::ir::Instruction{O::ADD}); break;
            case TokenType::Minus:  emit(tisc::ir::Instruction{O::SUB}); break;
            case TokenType::Star:   emit(tisc::ir::Instruction{O::MUL}); break;
            case TokenType::Slash:  emit(tisc::ir::Instruction{O::DIV}); break;
            default:
                throw std::runtime_error("Unsupported binary operator");
        }
        return {};
    }

    std::any visit(const LiteralExpr& expr) override {
        std::string_view lexeme = expr.value.lexeme;
        int64_t value = std::stoll(std::string{lexeme});

        emit(tisc::ir::Instruction{
            tisc::ir::Opcode::LOADI,
            {tisc::ir::Immediate{value}}
        });
        return {};
    }

    std::any visit(const GroupingExpr& expr) override {
        expr.expression->accept(*this);
        return {};
    }

    std::any visit(const UnaryExpr&) override        { return {}; }
    std::any visit(const VariableExpr&) override     { return {}; }
    std::any visit(const CallExpr&) override         { return {}; }
    std::any visit(const AssignExpr&) override       { return {}; }
    std::any visit(const SimpleTypeExpr&) override   { return {}; }
    std::any visit(const GenericTypeExpr&) override  { return {}; }

private:
    void emit(tisc::ir::Instruction instr) {
        _program.add_instruction(std::move(instr));
    }

    void emit_label(tisc::ir::Label label) {
        emit(tisc::ir::Instruction{tisc::ir::Opcode::LABEL, {label}});
    }

    tisc::ir::Register new_register() {
        return tisc::ir::Register{_register_count++};
    }

    tisc::ir::Label new_label() {
        return tisc::ir::Label{_label_count++};
    }

    tisc::ir::IntermediateProgram _program;
    SymbolTable _symbols;
    int _register_count = 0;
    int _label_count = 0;
};

} // namespace t81::frontend

#endif // T81_FRONTEND_IR_GENERATOR_HPP
