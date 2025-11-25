#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/tisc/ir.hpp"
#include <any>

namespace t81 {
namespace frontend {

class IRGenerator : public ExprVisitor, public StmtVisitor {
public:
    tisc::Program generate(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    std::any visit(const ExpressionStmt& stmt) override;
    std::any visit(const VarStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionStmt& stmt) override;

    std::any visit(const BinaryExpr& expr) override;
    std::any visit(const UnaryExpr& expr) override;
    std::any visit(const LiteralExpr& expr) override;
    std::any visit(const GroupingExpr& expr) override;
    std::any visit(const VariableExpr& expr) override;
    std::any visit(const CallExpr& expr) override;
    std::any visit(const AssignExpr& expr) override;
    std::any visit(const TypeExpr& expr) override;

    void emit(tisc::Instruction instr);
    void emit_label(tisc::Label label);
    tisc::Register new_register();
    tisc::Label new_label();

    tisc::Program _program;
    SymbolTable _symbols;
    int _register_count = 0;
    int _label_count = 0;
};

} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_IR_GENERATOR_HPP
