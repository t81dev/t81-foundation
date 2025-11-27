#include "t81/frontend/semantic_analyzer.hpp"
#include <iostream>

namespace t81 {
namespace frontend {

SemanticAnalyzer::SemanticAnalyzer(const std::vector<std::unique_ptr<Stmt>>& statements)
    : _statements(statements) {}

void SemanticAnalyzer::analyze() {
    for (const auto& stmt : _statements) {
        analyze(*stmt);
    }
}

void SemanticAnalyzer::analyze(const Stmt& stmt) {
    stmt.accept(*this);
}

std::any SemanticAnalyzer::analyze(const Expr& expr) {
    return expr.accept(*this);
}

void SemanticAnalyzer::error(const Token& token, const std::string& message) {
    _had_error = true;
    std::cerr << "Semantic Error: " << message << " at line " << token.line << std::endl;
}

// --- Visitor Method Implementations ---

std::any SemanticAnalyzer::visit(const ExpressionStmt& stmt) {
    analyze(*stmt.expression);
    return {};
}

std::any SemanticAnalyzer::visit(const VarStmt& stmt) {
    if (stmt.initializer) {
        analyze(*stmt.initializer);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const LetStmt& stmt) {
    if (stmt.initializer) {
        analyze(*stmt.initializer);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const BlockStmt& stmt) {
    for (const auto& statement : stmt.statements) {
        analyze(*statement);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const IfStmt& stmt) {
    analyze(*stmt.condition);
    analyze(*stmt.then_branch);
    if (stmt.else_branch) {
        analyze(*stmt.else_branch);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const WhileStmt& stmt) {
    analyze(*stmt.condition);
    analyze(*stmt.body);
    return {};
}

std::any SemanticAnalyzer::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        analyze(*stmt.value);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const FunctionStmt& stmt) {
    for (const auto& statement : stmt.body) {
        analyze(*statement);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const AssignExpr& expr) {
    analyze(*expr.value);
    return {};
}

std::any SemanticAnalyzer::visit(const BinaryExpr& expr) {
    analyze(*expr.left);
    analyze(*expr.right);
    return {};
}

std::any SemanticAnalyzer::visit(const CallExpr& expr) {
    analyze(*expr.callee);

    if (auto var_expr = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
        if (var_expr->name.lexeme == "Some") {
            if (expr.arguments.size() != 1) {
                error(var_expr->name, "The 'Some' constructor expects exactly one argument.");
            }
        }
    }

    for (const auto& arg : expr.arguments) {
        analyze(*arg);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const GroupingExpr& expr) {
    return analyze(*expr.expression);
}

std::any SemanticAnalyzer::visit(const LiteralExpr& expr) {
    return {};
}

std::any SemanticAnalyzer::visit(const UnaryExpr& expr) {
    return analyze(*expr.right);
}

std::any SemanticAnalyzer::visit(const VariableExpr& expr) {
    return {};
}

std::any SemanticAnalyzer::visit(const SimpleTypeExpr& expr) {
    return {};
}

std::any SemanticAnalyzer::visit(const GenericTypeExpr& expr) {
    const std::string& type_name = expr.name.lexeme;
    if (type_name == "Option") {
        if (expr.num_params != 1) {
            error(expr.name, "The 'Option' type expects exactly one type parameter.");
        }
    } else if (type_name == "Result") {
        if (expr.num_params != 2) {
            error(expr.name, "The 'Result' type expects exactly two type parameters.");
        }
    }
    return {};
}

} // namespace frontend
} // namespace t81
