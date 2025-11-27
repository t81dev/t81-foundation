#pragma once

#include <vector>
#include <memory>
#include <any>
#include <string>
#include <unordered_map>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"

namespace t81 {
namespace frontend {

// Simple symbol information for semantic analysis
enum class SymbolKind {
    Variable,
    Function
};

struct SemanticSymbol {
    SymbolKind kind;
    Token declaration;  // Token where the symbol was declared
};

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

    // Scoped symbol table
    using Scope = std::unordered_map<std::string, SemanticSymbol>;
    std::vector<Scope> _scopes;

    void analyze(const Stmt& stmt);
    std::any analyze(const Expr& expr);

    void error(const Token& token, const std::string& message);
    void error_at(const Token& token, const std::string& message);

    // Symbol table operations
    void enter_scope();
    void exit_scope();
    void define_symbol(const Token& name, SymbolKind kind);
    SemanticSymbol* resolve_symbol(const Token& name);
    bool is_defined_in_current_scope(const std::string& name) const;
};

} // namespace frontend
} // namespace t81
