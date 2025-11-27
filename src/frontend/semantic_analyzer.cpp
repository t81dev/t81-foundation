#include "t81/frontend/semantic_analyzer.hpp"
#include <iostream>

namespace t81 {
namespace frontend {

SemanticAnalyzer::SemanticAnalyzer(const std::vector<std::unique_ptr<Stmt>>& statements)
    : _statements(statements) {
    // Start with global scope
    enter_scope();
}

void SemanticAnalyzer::analyze() {
    // First pass: declare all functions at global scope
    for (const auto& stmt : _statements) {
        if (auto* func = dynamic_cast<const FunctionStmt*>(stmt.get())) {
            if (is_defined_in_current_scope(std::string(func->name.lexeme))) {
                error(func->name, "Function '" + std::string(func->name.lexeme) + "' is already defined.");
            } else {
                define_symbol(func->name, SymbolKind::Function);
            }
        }
    }

    // Second pass: analyze all statements
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
    std::cerr << "semantic error: " << message << " at line " << token.line 
              << ", column " << token.column << std::endl;
}

void SemanticAnalyzer::error_at(const Token& token, const std::string& message) {
    error(token, message);
}

// --- Symbol Table Operations ---

void SemanticAnalyzer::enter_scope() {
    _scopes.emplace_back();
}

void SemanticAnalyzer::exit_scope() {
    if (!_scopes.empty()) {
        _scopes.pop_back();
    }
}

void SemanticAnalyzer::define_symbol(const Token& name, SymbolKind kind) {
    if (!_scopes.empty()) {
        std::string name_str = std::string(name.lexeme);
        _scopes.back()[name_str] = SemanticSymbol{kind, name};
    }
}

SemanticSymbol* SemanticAnalyzer::resolve_symbol(const Token& name) {
    std::string name_str = std::string(name.lexeme);
    // Search from innermost to outermost scope
    for (auto it = _scopes.rbegin(); it != _scopes.rend(); ++it) {
        auto found = it->find(name_str);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SemanticAnalyzer::is_defined_in_current_scope(const std::string& name) const {
    if (_scopes.empty()) return false;
    return _scopes.back().find(name) != _scopes.back().end();
}

// --- Visitor Method Implementations ---

std::any SemanticAnalyzer::visit(const ExpressionStmt& stmt) {
    analyze(*stmt.expression);
    return {};
}

std::any SemanticAnalyzer::visit(const VarStmt& stmt) {
    // Check if variable is already defined in current scope
    if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
    } else {
        // Analyze initializer before defining (allows self-reference in some cases)
        if (stmt.initializer) {
            analyze(*stmt.initializer);
        }
        define_symbol(stmt.name, SymbolKind::Variable);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const LetStmt& stmt) {
    // Analyze the type expression if it's generic
    if (stmt.type) {
        if (auto* generic = dynamic_cast<const GenericTypeExpr*>(stmt.type.get())) {
            analyze(*generic);
        }
    }
    
    // Check if variable is already defined in current scope
    if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
    } else {
        // Analyze initializer before defining
        if (stmt.initializer) {
            analyze(*stmt.initializer);
        }
        define_symbol(stmt.name, SymbolKind::Variable);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const BlockStmt& stmt) {
    enter_scope();
    for (const auto& statement : stmt.statements) {
        analyze(*statement);
    }
    exit_scope();
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
    // Function is already declared in global scope during first pass
    // Now we enter function scope and define parameters
    enter_scope();
    
    // Define function parameters
    for (const auto& param : stmt.params) {
        if (is_defined_in_current_scope(std::string(param.name.lexeme))) {
            error(param.name, "Parameter '" + std::string(param.name.lexeme) + "' is already defined.");
        } else {
            define_symbol(param.name, SymbolKind::Variable);
        }
    }
    
    // Analyze function body
    for (const auto& statement : stmt.body) {
        analyze(*statement);
    }
    
    exit_scope();
    return {};
}

std::any SemanticAnalyzer::visit(const AssignExpr& expr) {
    // Check if variable exists
    auto* symbol = resolve_symbol(expr.name);
    if (!symbol) {
        error(expr.name, "Undefined variable '" + std::string(expr.name.lexeme) + "'.");
    } else if (symbol->kind != SymbolKind::Variable) {
        error(expr.name, "Cannot assign to non-variable '" + std::string(expr.name.lexeme) + "'.");
    }
    
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
        std::string func_name = std::string(var_expr->name.lexeme);
        
        // Special handling for built-in constructors
        if (func_name == "Some") {
            if (expr.arguments.size() != 1) {
                error(var_expr->name, "The 'Some' constructor expects exactly one argument.");
            }
        } else {
            // Check if function is defined
            auto* symbol = resolve_symbol(var_expr->name);
            if (!symbol) {
                error(var_expr->name, "Undefined function '" + func_name + "'.");
            } else if (symbol->kind != SymbolKind::Function) {
                error(var_expr->name, "'" + func_name + "' is not a function.");
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
    // Resolve variable reference
    auto* symbol = resolve_symbol(expr.name);
    if (!symbol) {
        error(expr.name, "Undefined variable '" + std::string(expr.name.lexeme) + "'.");
    }
    return {};
}

std::any SemanticAnalyzer::visit(const SimpleTypeExpr& expr) {
    return {};
}

std::any SemanticAnalyzer::visit(const GenericTypeExpr& expr) {
    const std::string& type_name = std::string(expr.name.lexeme);
    if (type_name == "Option") {
        if (expr.param_count != 1) {
            error(expr.name, "The 'Option' type expects exactly one type parameter.");
        }
    } else if (type_name == "Result") {
        if (expr.param_count != 2) {
            error(expr.name, "The 'Result' type expects exactly two type parameters.");
        }
    }
    return {};
}

} // namespace frontend
} // namespace t81
