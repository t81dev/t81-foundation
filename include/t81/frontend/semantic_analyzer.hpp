#pragma once

#include <vector>
#include <memory>
#include <any>
#include "stmt.hpp"
#include "expr.hpp"
#include "token.hpp"

namespace t81 {
namespace frontend {

class SemanticAnalyzer : public StmtVisitor, public ExprVisitor {
public:
    explicit SemanticAnalyzer(const std::vector<std::unique_ptr<Stmt>>& statements);
    void analyze();
    bool had_error() const { return _had_error; }

    // Visitor methods for statements
    std::any visit(const ExpressionStmt& stmt) override;
    std::any visit(const VarStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionStmt& stmt) override;

    // Visitor methods for expressions
    std::any visit(const AssignExpr& expr) override;
    std::any visit(const BinaryExpr& expr) override;
    std::any visit(const CallExpr& expr) override;
    std::any visit(const GroupingExpr& expr) override;
    std::any visit(const LiteralExpr& expr) override;
    std::any visit(const UnaryExpr& expr) override;
    std::any visit(const VariableExpr& expr) override;
    std::any visit(const SimpleTypeExpr& expr) override;
    std::any visit(const GenericTypeExpr& expr) override;

private:
    const std::vector<std::unique_ptr<Stmt>>& _statements;
    bool _had_error = false;

    void analyze(const Stmt& stmt);
    std::any analyze(const Expr& expr);

    void error(const Token& token, const std::string& message);
};

} // namespace frontend
} // namespace t81
