#include "t81/frontend/semantic_analyzer.hpp"
#include <iostream>
#include <sstream>

namespace t81 {
namespace frontend {

bool Type::operator==(const Type& other) const {
    if (kind != other.kind) return false;
    if (kind == Kind::Custom) {
        return custom_name == other.custom_name;
    }
    return params == other.params;
}

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

    // Second pass: record all function signatures so calls can be checked even
    // when definitions appear later in the source.
    register_function_signatures();

    // Third pass: analyze all statements and bodies
    for (const auto& stmt : _statements) {
        if (stmt) {  // Skip null statements from parse errors
            analyze(*stmt);
        }
    }
}

void SemanticAnalyzer::analyze(const Stmt& stmt) {
    stmt.accept(*this);
}

std::any SemanticAnalyzer::analyze(const Expr& expr) {
    return expr.accept(*this);
}

void SemanticAnalyzer::error(const Token& token, const std::string& message) {
    if (!_had_error) {  // Only set once to avoid multiple error messages
        _had_error = true;
    }
    std::cerr << "semantic error: " << message << " at line " << token.line 
              << ", column " << token.column << std::endl;
    std::cerr.flush();  // Ensure error is written immediately
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
        _scopes.back()[name_str] = SemanticSymbol{kind, name, Type{}, {}, false};
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

Type SemanticAnalyzer::make_error_type() {
    return Type{Type::Kind::Error};
}

int SemanticAnalyzer::numeric_rank(const Type& type) const {
    switch (type.kind) {
        case Type::Kind::I2: return 1;
        case Type::Kind::I8: return 2;
        case Type::Kind::I16: return 3;
        case Type::Kind::I32: return 4;
        case Type::Kind::BigInt: return 5;
        case Type::Kind::Fraction: return 6;
        case Type::Kind::Float: return 7;
        default: return 0;
    }
}

bool SemanticAnalyzer::is_numeric(const Type& type) const {
    return numeric_rank(type) > 0;
}

Type SemanticAnalyzer::type_from_token(const Token& name) {
    switch (name.type) {
        case TokenType::Void: return Type{Type::Kind::Void};
        case TokenType::Bool: return Type{Type::Kind::Bool};
        case TokenType::I2: return Type{Type::Kind::I2};
        case TokenType::I8: return Type{Type::Kind::I8};
        case TokenType::I16: return Type{Type::Kind::I16};
        case TokenType::I32: return Type{Type::Kind::I32};
        case TokenType::T81BigInt: return Type{Type::Kind::BigInt};
        case TokenType::T81Float: return Type{Type::Kind::Float};
        case TokenType::T81Fraction: return Type{Type::Kind::Fraction};
        case TokenType::Vector: return Type{Type::Kind::Vector};
        case TokenType::Matrix: return Type{Type::Kind::Matrix};
        case TokenType::Tensor: return Type{Type::Kind::Tensor};
        case TokenType::Graph: return Type{Type::Kind::Graph};
        default: break;
    }

    std::string name_str{name.lexeme};
    if (name_str == "Option") return Type{Type::Kind::Option};
    if (name_str == "Result") return Type{Type::Kind::Result};
    if (name_str == "T81String") return Type{Type::Kind::String};
    return Type{Type::Kind::Custom, {}, name_str};
}

Type SemanticAnalyzer::analyze_type_expr(const TypeExpr& expr) {
    auto result = expr.accept(*this);
    if (result.has_value()) {
        try {
            return std::any_cast<Type>(result);
        } catch (const std::bad_any_cast&) {
            return make_error_type();
        }
    }
    return make_error_type();
}

std::string SemanticAnalyzer::type_to_string(const Type& type) const {
    switch (type.kind) {
        case Type::Kind::Void: return "void";
        case Type::Kind::Bool: return "bool";
        case Type::Kind::I2: return "i2";
        case Type::Kind::I8: return "i8";
        case Type::Kind::I16: return "i16";
        case Type::Kind::I32: return "i32";
        case Type::Kind::BigInt: return "T81BigInt";
        case Type::Kind::Float: return "T81Float";
        case Type::Kind::Fraction: return "T81Fraction";
        case Type::Kind::Vector: return "Vector";
        case Type::Kind::Matrix: return "Matrix";
        case Type::Kind::Tensor: return "Tensor";
        case Type::Kind::Graph: return "Graph";
        case Type::Kind::String: return "T81String";
        case Type::Kind::Option:
        case Type::Kind::Result: {
            std::ostringstream oss;
            oss << (type.kind == Type::Kind::Option ? "Option" : "Result");
            if (!type.params.empty()) {
                oss << '[';
                for (size_t i = 0; i < type.params.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << type_to_string(type.params[i]);
                }
                oss << ']';
            }
            return oss.str();
        }
        case Type::Kind::Custom: return type.custom_name;
        case Type::Kind::Unknown: return "<unknown>";
        case Type::Kind::Error: return "<error>";
    }
    return "<unknown>";
}

bool SemanticAnalyzer::is_assignable(const Type& target, const Type& value) {
    if (target.kind == Type::Kind::Error || value.kind == Type::Kind::Error) return true;
    if (target.kind == Type::Kind::Unknown || value.kind == Type::Kind::Unknown) return true;
    if (target == value) return true;

    if (target.kind == Type::Kind::Option && value.kind == Type::Kind::Option &&
        !target.params.empty() && !value.params.empty()) {
        return is_assignable(target.params[0], value.params[0]);
    }

    if (target.kind == Type::Kind::Result && value.kind == Type::Kind::Result &&
        target.params.size() == 2 && value.params.size() == 2) {
        return is_assignable(target.params[0], value.params[0]) &&
               is_assignable(target.params[1], value.params[1]);
    }

    if (is_numeric(target) && is_numeric(value)) {
        return numeric_rank(value) <= numeric_rank(target);
    }

    if (target.kind == Type::Kind::Custom && value.kind == Type::Kind::Custom) {
        return target.custom_name == value.custom_name;
    }

    return false;
}

Type SemanticAnalyzer::widen_numeric(const Type& left, const Type& right, const Token& op) {
    if (left.kind == Type::Kind::Error || right.kind == Type::Kind::Error) {
        return make_error_type();
    }
    if (left.kind == Type::Kind::Unknown || right.kind == Type::Kind::Unknown) {
        return Type{Type::Kind::Unknown};
    }
    if (!is_numeric(left) || !is_numeric(right)) {
        error(op, "Operands must be numeric, got '" + type_to_string(left) + "' and '" + type_to_string(right) + "'.");
        return make_error_type();
    }
    return numeric_rank(left) >= numeric_rank(right) ? left : right;
}

Type SemanticAnalyzer::evaluate_expression(const Expr& expr) {
    auto result = analyze(expr);
    if (!result.has_value()) {
        return Type{Type::Kind::Unknown};
    }
    try {
        return std::any_cast<Type>(result);
    } catch (const std::bad_any_cast&) {
        return make_error_type();
    }
}

Type SemanticAnalyzer::expect_condition_bool(const Expr& expr, const Token& location) {
    Type cond_type = evaluate_expression(expr);
    if (!is_assignable(Type{Type::Kind::Bool}, cond_type)) {
        error(location, "Condition must be bool, found '" + type_to_string(cond_type) + "'.");
        return make_error_type();
    }
    return Type{Type::Kind::Bool};
}

void SemanticAnalyzer::register_function_signatures() {
    for (const auto& stmt : _statements) {
        const auto* func = dynamic_cast<const FunctionStmt*>(stmt.get());
        if (!func) continue;

        SemanticSymbol* symbol = resolve_symbol(func->name);
        if (!symbol) continue;

        std::vector<Type> param_types;
        bool param_error = false;
        for (const auto& param : func->params) {
            if (!param.type) {
                param_error = true;
                error(param.name, "Parameter '" + std::string(param.name.lexeme) + "' is missing a type annotation.");
                param_types.push_back(make_error_type());
                continue;
            }
            param_types.push_back(analyze_type_expr(*param.type));
        }

        Type return_type = func->return_type ? analyze_type_expr(*func->return_type) : Type{Type::Kind::Void};
        symbol->param_types = param_types;
        symbol->type = return_type;
        symbol->is_defined = !param_error;
    }
}

Token SemanticAnalyzer::extract_token(const Expr& expr) const {
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) return binary->op;
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) return unary->op;
    if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) return literal->value;
    if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) return variable->name;
    if (auto* assign = dynamic_cast<const AssignExpr*>(&expr)) return assign->name;
    if (auto* call = dynamic_cast<const CallExpr*>(&expr)) return extract_token(*call->callee);
    if (auto* grouping = dynamic_cast<const GroupingExpr*>(&expr)) return extract_token(*grouping->expression);

    return Token{TokenType::Illegal, "", 0, 0};
}

// --- Visitor Method Implementations ---

std::any SemanticAnalyzer::visit(const ExpressionStmt& stmt) {
    evaluate_expression(*stmt.expression);
    return {};
}

std::any SemanticAnalyzer::visit(const VarStmt& stmt) {
    if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
        return {};
    }

    Type declared_type = stmt.type ? analyze_type_expr(*stmt.type) : Type{Type::Kind::Unknown};
    Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer) : Type{Type::Kind::Unknown};

    if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' requires a type annotation or initializer.");
    }

    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown &&
        !is_assignable(declared_type, init_type)) {
        error(stmt.name, "Cannot assign initializer of type '" + type_to_string(init_type) +
                              "' to variable of type '" + type_to_string(declared_type) + "'.");
    }

    Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : declared_type;
    define_symbol(stmt.name, SymbolKind::Variable);
    if (auto* symbol = resolve_symbol(stmt.name)) {
        symbol->type = final_type;
    }
    return final_type;
}

std::any SemanticAnalyzer::visit(const LetStmt& stmt) {
    if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
        return {};
    }

    Type declared_type = stmt.type ? analyze_type_expr(*stmt.type) : Type{Type::Kind::Unknown};
    Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer) : Type{Type::Kind::Unknown};

    if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
        error(stmt.name, "Constant '" + std::string(stmt.name.lexeme) + "' requires a type annotation or initializer.");
    }

    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown &&
        !is_assignable(declared_type, init_type)) {
        error(stmt.name, "Cannot assign initializer of type '" + type_to_string(init_type) +
                              "' to constant of type '" + type_to_string(declared_type) + "'.");
    }

    Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : declared_type;
    define_symbol(stmt.name, SymbolKind::Variable);
    if (auto* symbol = resolve_symbol(stmt.name)) {
        symbol->type = final_type;
    }
    return final_type;
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
    Token cond_token = extract_token(*stmt.condition);
    expect_condition_bool(*stmt.condition, cond_token);
    analyze(*stmt.then_branch);
    if (stmt.else_branch) {
        analyze(*stmt.else_branch);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const WhileStmt& stmt) {
    Token cond_token = extract_token(*stmt.condition);
    expect_condition_bool(*stmt.condition, cond_token);
    analyze(*stmt.body);
    return {};
}

std::any SemanticAnalyzer::visit(const ReturnStmt& stmt) {
    if (_function_return_stack.empty()) {
        error(stmt.keyword, "Return statement outside of a function.");
        return make_error_type();
    }

    const Type expected = _function_return_stack.back();
    if (!stmt.value) {
        if (expected.kind != Type::Kind::Void) {
            error(stmt.keyword, "Return type mismatch: expected '" + type_to_string(expected) + "' but got 'void'.");
        }
        return expected;
    }

    Type value_type = evaluate_expression(*stmt.value);
    if (!is_assignable(expected, value_type)) {
        error(stmt.keyword, "Return type mismatch: expected '" + type_to_string(expected) + "' but got '" +
                                 type_to_string(value_type) + "'.");
    }
    return expected;
}

std::any SemanticAnalyzer::visit(const FunctionStmt& stmt) {
    SemanticSymbol* symbol = resolve_symbol(stmt.name);
    if (!symbol) {
        define_symbol(stmt.name, SymbolKind::Function);
        symbol = resolve_symbol(stmt.name);
    }

    enter_scope();
    _function_return_stack.push_back(symbol ? symbol->type : Type{Type::Kind::Unknown});

    if (symbol && symbol->param_types.size() != stmt.params.size()) {
        error(stmt.name, "Function parameter count mismatch between declaration and definition.");
    }

    for (size_t i = 0; i < stmt.params.size(); ++i) {
        const auto& param = stmt.params[i];
        Type param_type = (symbol && i < symbol->param_types.size()) ? symbol->param_types[i]
                                                                     : Type{Type::Kind::Unknown};

        if (param.type && param_type.kind == Type::Kind::Unknown) {
            param_type = analyze_type_expr(*param.type);
        }

        if (is_defined_in_current_scope(std::string(param.name.lexeme))) {
            error(param.name, "Parameter '" + std::string(param.name.lexeme) + "' is already defined.");
        } else {
            define_symbol(param.name, SymbolKind::Variable);
            if (auto* param_symbol = resolve_symbol(param.name)) {
                param_symbol->type = param_type;
            }
        }
    }

    for (const auto& statement : stmt.body) {
        analyze(*statement);
    }

    _function_return_stack.pop_back();
    exit_scope();
    return symbol ? symbol->type : Type{Type::Kind::Unknown};
}

std::any SemanticAnalyzer::visit(const AssignExpr& expr) {
    auto* symbol = resolve_symbol(expr.name);
    if (!symbol) {
        error(expr.name, "Undefined variable '" + std::string(expr.name.lexeme) + "'.");
        evaluate_expression(*expr.value);
        return make_error_type();
    }
    if (symbol->kind != SymbolKind::Variable) {
        error(expr.name, "Cannot assign to non-variable '" + std::string(expr.name.lexeme) + "'.");
    }

    Type value_type = evaluate_expression(*expr.value);
    if (!is_assignable(symbol->type, value_type)) {
        error(expr.name, "Cannot assign value of type '" + type_to_string(value_type) +
                             "' to variable of type '" + type_to_string(symbol->type) + "'.");
    }
    return symbol->type;
}

std::any SemanticAnalyzer::visit(const BinaryExpr& expr) {
    Type left_type = evaluate_expression(*expr.left);
    Type right_type = evaluate_expression(*expr.right);

    switch (expr.op.type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            return widen_numeric(left_type, right_type, expr.op);
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
            if (!is_numeric(left_type) || !is_numeric(right_type)) {
                error(expr.op, "Comparison operands must be numeric.");
                return make_error_type();
            }
            return Type{Type::Kind::Bool};
        case TokenType::EqualEqual:
        case TokenType::BangEqual: {
            if (left_type == right_type) return Type{Type::Kind::Bool};
            if (is_numeric(left_type) && is_numeric(right_type)) return Type{Type::Kind::Bool};
            error(expr.op, "Equality operands must be of the same type.");
            return make_error_type();
        }
        case TokenType::AmpAmp:
        case TokenType::PipePipe:
            if (!is_assignable(Type{Type::Kind::Bool}, left_type) ||
                !is_assignable(Type{Type::Kind::Bool}, right_type)) {
                error(expr.op, "Logical operators require boolean operands.");
                return make_error_type();
            }
            return Type{Type::Kind::Bool};
        default:
            return make_error_type();
    }
}

std::any SemanticAnalyzer::visit(const CallExpr& expr) {
    std::vector<Type> arg_types;
    arg_types.reserve(expr.arguments.size());
    for (const auto& arg : expr.arguments) {
        arg_types.push_back(evaluate_expression(*arg));
    }

    if (auto var_expr = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
        std::string func_name = std::string(var_expr->name.lexeme);

        if (func_name == "Some") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Some' constructor expects exactly one argument.");
            }
            return Type{Type::Kind::Option, {arg_types.empty() ? Type{Type::Kind::Unknown} : arg_types[0]}};
        }
        if (func_name == "None") {
            if (!arg_types.empty()) {
                error(var_expr->name, "The 'None' constructor does not take arguments.");
            }
            return Type{Type::Kind::Option, {Type{Type::Kind::Unknown}}};
        }
        if (func_name == "Ok") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Ok' constructor expects exactly one argument.");
            }
            Type success = arg_types.empty() ? Type{Type::Kind::Unknown} : arg_types[0];
            return Type{Type::Kind::Result, {success, Type{Type::Kind::Unknown}}};
        }
        if (func_name == "Err") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Err' constructor expects exactly one argument.");
            }
            Type error_type = arg_types.empty() ? Type{Type::Kind::Unknown} : arg_types[0];
            return Type{Type::Kind::Result, {Type{Type::Kind::Unknown}, error_type}};
        }

        auto* symbol = resolve_symbol(var_expr->name);
        if (!symbol) {
            error(var_expr->name, "Undefined function '" + func_name + "'.");
            return make_error_type();
        }
        if (symbol->kind != SymbolKind::Function) {
            error(var_expr->name, "'" + func_name + "' is not a function.");
            return make_error_type();
        }

        if (symbol->param_types.size() != arg_types.size()) {
            error(var_expr->name, "Function '" + func_name + "' expects " + std::to_string(symbol->param_types.size()) +
                                       " arguments but got " + std::to_string(arg_types.size()) + ".");
            return symbol->type;
        }

        for (size_t i = 0; i < arg_types.size(); ++i) {
            if (!is_assignable(symbol->param_types[i], arg_types[i])) {
                error(var_expr->name, "Argument " + std::to_string(i) + " for function '" + func_name + "' expects '" +
                                           type_to_string(symbol->param_types[i]) + "' but got '" +
                                           type_to_string(arg_types[i]) + "'.");
            }
        }

        return symbol->type;
    }

    evaluate_expression(*expr.callee);
    return make_error_type();
}

std::any SemanticAnalyzer::visit(const GroupingExpr& expr) {
    return evaluate_expression(*expr.expression);
}

std::any SemanticAnalyzer::visit(const LiteralExpr& expr) {
    switch (expr.value.type) {
        case TokenType::True:
        case TokenType::False:
            return Type{Type::Kind::Bool};
        case TokenType::Integer:
        case TokenType::Base81Integer:
            return Type{Type::Kind::I32};
        case TokenType::Float:
        case TokenType::Base81Float:
            return Type{Type::Kind::Float};
        case TokenType::String:
            return Type{Type::Kind::String};
        default:
            return Type{Type::Kind::Unknown};
    }
}

std::any SemanticAnalyzer::visit(const UnaryExpr& expr) {
    Type right = evaluate_expression(*expr.right);
    if (expr.op.type == TokenType::Bang) {
        if (!is_assignable(Type{Type::Kind::Bool}, right)) {
            error(expr.op, "Logical not requires a boolean operand.");
            return make_error_type();
        }
        return Type{Type::Kind::Bool};
    }

    if (expr.op.type == TokenType::Minus) {
        if (!is_numeric(right)) {
            error(expr.op, "Unary minus requires a numeric operand.");
            return make_error_type();
        }
        return right;
    }

    return make_error_type();
}

std::any SemanticAnalyzer::visit(const VariableExpr& expr) {
    std::string name_str = std::string(expr.name.lexeme);

    if (name_str == "Some" || name_str == "None" || name_str == "Ok" || name_str == "Err") {
        return Type{Type::Kind::Unknown};
    }

    auto* symbol = resolve_symbol(expr.name);
    if (!symbol) {
        error(expr.name, "Undefined variable '" + name_str + "'.");
        return make_error_type();
    }
    return symbol->type;
}

std::any SemanticAnalyzer::visit(const SimpleTypeExpr& expr) {
    return type_from_token(expr.name);
}

std::any SemanticAnalyzer::visit(const GenericTypeExpr& expr) {
    std::string type_name = std::string(expr.name.lexeme);
    std::vector<Type> params;
    params.reserve(expr.param_count);

    for (size_t i = 0; i < expr.param_count; ++i) {
        if (!expr.params[i]) continue;
        if (auto* type_expr = dynamic_cast<TypeExpr*>(expr.params[i].get())) {
            params.push_back(analyze_type_expr(*type_expr));
        } else {
            error(expr.name, "Generic type parameters must be types.");
            params.push_back(make_error_type());
        }
    }

    if (type_name == "Option") {
        if (params.size() != 1) {
            error(expr.name, "The 'Option' type expects exactly one type parameter, but got " + std::to_string(params.size()) + ".");
        }
        if (params.empty()) {
            params.push_back(Type{Type::Kind::Unknown});
        }
        return Type{Type::Kind::Option, params};
    }

    if (type_name == "Result") {
        if (params.size() != 2) {
            error(expr.name, "The 'Result' type expects exactly two type parameters, but got " + std::to_string(params.size()) + ".");
        }
        while (params.size() < 2) {
            params.push_back(Type{Type::Kind::Unknown});
        }
        return Type{Type::Kind::Result, params};
    }

    Type base = type_from_token(expr.name);
    base.params = params;
    return base;
}

} // namespace frontend
} // namespace t81
