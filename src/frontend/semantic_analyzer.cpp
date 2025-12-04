#if defined(_MSC_VER) && _MSC_VER < 1930
// MSVC before VS 2022 17.0 has broken thread_local in some contexts
static int type_to_string_depth = 0;
#define TYPE_TO_STRING_DEPTH type_to_string_depth
#else
thread_local int type_to_string_depth = 0;
#define TYPE_TO_STRING_DEPTH type_to_string_depth
#endif

#include "t81/frontend/semantic_analyzer.hpp"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string_view>
#include <utility>

namespace {
std::optional<float> parse_numeric_literal_value(const t81::frontend::Token& token) {
    using t81::frontend::Token;
    using t81::frontend::TokenType;
    std::string lexeme(token.lexeme);
    switch (token.type) {
        case TokenType::Integer: {
            try {
                long long v = std::stoll(lexeme);
                return static_cast<float>(v);
            } catch (...) {
                return std::nullopt;
            }
        }
        case TokenType::Float: {
            char* end = nullptr;
            float value = std::strtof(lexeme.c_str(), &end);
            if (end != lexeme.c_str() + lexeme.size()) {
                return std::nullopt;
            }
            return value;
        }
        case TokenType::Base81Integer:
        case TokenType::Base81Float:
            return std::nullopt;
        default:
            return std::nullopt;
    }
}
} // namespace

namespace t81 {
namespace frontend {

Type Type::constant(std::string repr) {
    Type t;
    t.kind = Kind::Constant;
    t.custom_name = std::move(repr);
    return t;
}

bool Type::operator==(const Type& other) const {
    if (kind != other.kind) return false;
    if (kind == Kind::Custom || kind == Kind::Constant) {
        return custom_name == other.custom_name;
    }
    return params == other.params;
}

SemanticAnalyzer::SemanticAnalyzer(const std::vector<std::unique_ptr<Stmt>>& statements,
                                   std::string source_name)
    : _statements(statements),
      _source_name(std::move(source_name)) {
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
    _diagnostics.push_back(Diagnostic{_source_name, token.line, token.column, message});
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

bool SemanticAnalyzer::is_integer_type(const Type& type) const {
    switch (type.kind) {
        case Type::Kind::I2:
        case Type::Kind::I8:
        case Type::Kind::I16:
        case Type::Kind::I32:
        case Type::Kind::BigInt:
            return true;
        default:
            return false;
    }
}

bool SemanticAnalyzer::is_float_type(const Type& type) const {
    return type.kind == Type::Kind::Float;
}

bool SemanticAnalyzer::is_fraction_type(const Type& type) const {
    return type.kind == Type::Kind::Fraction;
}

bool SemanticAnalyzer::is_primitive_numeric_type(const Type& type) const {
    return is_integer_type(type) || is_float_type(type) || is_fraction_type(type);
}

std::optional<Type> SemanticAnalyzer::deduce_numeric_type(const Type& left, const Type& right, const Token& op) {
    if (left.kind == Type::Kind::Error || right.kind == Type::Kind::Error) {
        return make_error_type();
    }
    if (left.kind == Type::Kind::Unknown || right.kind == Type::Kind::Unknown) {
        return Type{Type::Kind::Unknown};
    }
    if (!is_primitive_numeric_type(left) || !is_primitive_numeric_type(right)) {
        error(op, "Operands must be primitive numeric types, got '" + type_to_string(left) +
                  "' and '" + type_to_string(right) + "'.");
        return std::nullopt;
    }

    if (is_integer_type(left) && is_integer_type(right)) {
        return numeric_rank(left) >= numeric_rank(right) ? left : right;
    }
    if (is_integer_type(left) && is_float_type(right)) {
        return right;
    }
    if (is_integer_type(right) && is_float_type(left)) {
        return left;
    }
    if (is_integer_type(left) && is_fraction_type(right)) {
        return right;
    }
    if (is_integer_type(right) && is_fraction_type(left)) {
        return left;
    }
    if (left.kind == Type::Kind::Float && right.kind == Type::Kind::Float) {
        return left;
    }
    if (left.kind == Type::Kind::Fraction && right.kind == Type::Kind::Fraction) {
        return left;
    }

    error(op, "Operands must share a primitive numeric type (T81Int, T81Float, or T81Fraction) or widen deterministically from T81Int. Got '" +
                  type_to_string(left) + "' and '" + type_to_string(right) + "'.");
    return std::nullopt;
}

Type SemanticAnalyzer::refine_generic_type(const Type& declared, const Type& initializer) const {
    if (declared.kind == Type::Kind::Option && initializer.kind == Type::Kind::Option) {
        Type result = declared;
        if (initializer.params.size() >= 1) {
            if (result.params.empty()) {
                result.params = initializer.params;
            } else if (result.params[0].kind == Type::Kind::Unknown) {
                result.params[0] = initializer.params[0];
            }
        }
        return result;
    }
    if (declared.kind == Type::Kind::Result && initializer.kind == Type::Kind::Result) {
        Type result = declared;
        if (result.params.size() < 2) {
            result.params.resize(2, Type{Type::Kind::Unknown});
        }
        for (size_t i = 0; i < 2 && i < initializer.params.size(); ++i) {
            if (result.params[i].kind == Type::Kind::Unknown) {
                result.params[i] = initializer.params[i];
            }
        }
        return result;
    }
    if (declared.kind == initializer.kind && declared.kind != Type::Kind::Unknown) {
        Type result = declared;
        size_t max_params = std::max(result.params.size(), initializer.params.size());
        result.params.resize(max_params, Type{Type::Kind::Unknown});
        for (size_t i = 0; i < initializer.params.size(); ++i) {
            if (result.params[i].kind == Type::Kind::Unknown) {
                result.params[i] = initializer.params[i];
            }
        }
        return result;
    }
    return declared;
}

void SemanticAnalyzer::merge_expected_params(Type& target, const Type* expected) const {
    if (!expected || target.kind != expected->kind) {
        return;
    }
    if (target.kind == Type::Kind::Custom && target.custom_name != expected->custom_name) {
        return;
    }
    if (target.params.empty() && !expected->params.empty()) {
        target.params = expected->params;
        return;
    }
    size_t max_params = std::max(target.params.size(), expected->params.size());
    target.params.resize(max_params, Type{Type::Kind::Unknown});
    for (size_t i = 0; i < expected->params.size(); ++i) {
        if (target.params[i].kind == Type::Kind::Unknown && expected->params[i].kind != Type::Kind::Unknown) {
            target.params[i] = expected->params[i];
        }
    }
}

bool SemanticAnalyzer::structural_params_assignable(const Type& target, const Type& value) const {
    size_t count = std::max(target.params.size(), value.params.size());
    size_t target_defined = target.params.size();
    size_t value_defined = value.params.size();

    if (target_defined && value_defined && target_defined != value_defined) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        Type target_param = (i < target.params.size()) ? target.params[i] : Type{Type::Kind::Unknown};
        Type value_param = (i < value.params.size()) ? value.params[i] : Type{Type::Kind::Unknown};

        if (target_param.kind == Type::Kind::Constant || value_param.kind == Type::Kind::Constant) {
            if (target_param.kind == Type::Kind::Constant && value_param.kind == Type::Kind::Constant) {
                if (target_param.custom_name != value_param.custom_name) {
                    return false;
                }
            } else if (target_param.kind == Type::Kind::Unknown || value_param.kind == Type::Kind::Unknown) {
                // Allow unspecified parameters to align with constants.
            } else {
                return false;
            }
            continue;
        }

        if (!is_assignable(target_param, value_param)) {
            return false;
        }
    }
    return true;
}

Type SemanticAnalyzer::instantiate_alias(const AliasInfo& alias,
                                        const std::vector<Type>& params,
                                        const Token& location)
{
    if (alias.params.size() != params.size()) {
        error(location, "Generic type '" + std::string(location.lexeme) + "' expects " +
                        std::to_string(alias.params.size()) + " parameters but got " +
                        std::to_string(params.size()) + ".");
        return make_error_type();
    }
    std::unordered_map<std::string, Type> env;
    for (size_t i = 0; i < alias.params.size(); ++i) {
        env[alias.params[i]] = params[i];
    }
    if (!alias.alias) {
        return make_error_type();
    }
    return analyze_type_expr(*alias.alias, &env);
}

void SemanticAnalyzer::enforce_generic_arity(const Type& type, const Token& location) {
    if (type.kind != Type::Kind::Custom) return;
    size_t arity = type.params.size();
    auto it = _generic_arities.find(type.custom_name);
    if (it == _generic_arities.end()) {
        _generic_arities[type.custom_name] = arity;
        return;
    }
    if (it->second != arity) {
        error(location, "Generic type '" + type.custom_name + "' expects " +
                        std::to_string(it->second) + " parameters but got " +
                        std::to_string(arity) + ".");
    }
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

const std::vector<float>* SemanticAnalyzer::vector_literal_data(const VectorLiteralExpr* expr) const {
    auto it = _vector_literal_data.find(expr);
    if (it == _vector_literal_data.end()) return nullptr;
    return &it->second;
}

std::string SemanticAnalyzer::expr_to_string(const Expr& expr) const {
    if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) {
        return std::string(literal->value.lexeme);
    }
    if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) {
        return std::string(variable->name.lexeme);
    }
    return "<expr>";
}

std::string SemanticAnalyzer::type_expr_to_string(const TypeExpr& expr) const {
    if (auto* simple = dynamic_cast<const SimpleTypeExpr*>(&expr)) {
        return std::string(simple->name.lexeme);
    }
    if (auto* generic = dynamic_cast<const GenericTypeExpr*>(&expr)) {
        std::string result = std::string(generic->name.lexeme) + "[";
        for (size_t i = 0; i < generic->param_count; ++i) {
            if (i > 0) result += ", ";
            if (!generic->params[i]) {
                result += "<missing>";
                continue;
            }
            Expr* raw = generic->params[i].get();
            if (auto* type_expr = dynamic_cast<TypeExpr*>(raw)) {
                result += type_expr_to_string(*type_expr);
            } else {
                result += expr_to_string(*raw);
            }
        }
        result += "]";
        return result;
    }
    return "<unknown>";
}

Type SemanticAnalyzer::analyze_type_expr(const TypeExpr& expr,
                                        const std::unordered_map<std::string, Type>* env)
{
    auto prev_env = _current_type_env;
    _current_type_env = env;
    auto result = expr.accept(*this);
    _current_type_env = prev_env;
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
    // Thread-local depth guard — zero overhead, total safety
    thread_local int depth = 0;
    if (++depth > 32) {
        depth--;
        return "...";
    }

    std::string result;

    switch (type.kind) {
        case Type::Kind::Void:     result = "void"; break;
        case Type::Kind::Bool:     result = "bool"; break;
        case Type::Kind::I2:       result = "i2"; break;
        case Type::Kind::I8:       result = "i8"; break;
        case Type::Kind::I16:      result = "i16"; break;
        case Type::Kind::I32:      result = "i32"; break;
        case Type::Kind::BigInt:   result = "T81BigInt"; break;
        case Type::Kind::Float:    result = "T81Float"; break;
        case Type::Kind::Fraction: result = "T81Fraction"; break;
        case Type::Kind::Vector:   result = "Vector"; break;
        case Type::Kind::Matrix:   result = "Matrix"; break;
        case Type::Kind::Tensor:   result = "Tensor"; break;
        case Type::Kind::Graph:    result = "Graph"; break;
        case Type::Kind::String:   result = "T81String"; break;
        case Type::Kind::Constant: result = "const(" + type.custom_name + ")"; break;
        case Type::Kind::Custom:   result = type.custom_name; break;
        case Type::Kind::Unknown:  result = "<unknown>"; break;
        case Type::Kind::Error:    result = "<error>"; break;

        case Type::Kind::Option:
        case Type::Kind::Result: {
            std::ostringstream oss;
            oss << (type.kind == Type::Kind::Option ? "Option" : "Result");

            if (!type.params.empty()) {
                oss << '[';
                for (size_t i = 0; i < type.params.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << type_to_string(type.params[i]);  // now safe
                }
                oss << ']';
            }
            result = oss.str();
            break;
        }
    }

    depth--;
    return result;
}

bool SemanticAnalyzer::is_assignable(const Type& target, const Type& value) const {
    if (target.kind == Type::Kind::Error || value.kind == Type::Kind::Error) return true;
    if (target.kind == Type::Kind::Unknown || value.kind == Type::Kind::Unknown) return true;
    if (target == value) return true;

    if (target.kind == Type::Kind::Option && value.kind == Type::Kind::Option) {
        Type target_param = target.params.empty() ? Type{Type::Kind::Unknown} : target.params[0];
        Type value_param = value.params.empty() ? Type{Type::Kind::Unknown} : value.params[0];
        return is_assignable(target_param, value_param);
    }

    if (target.kind == Type::Kind::Result && value.kind == Type::Kind::Result) {
        Type target_success = target.params.size() > 0 ? target.params[0] : Type{Type::Kind::Unknown};
        Type target_error = target.params.size() > 1 ? target.params[1] : Type{Type::Kind::Unknown};
        Type value_success = value.params.size() > 0 ? value.params[0] : Type{Type::Kind::Unknown};
        Type value_error = value.params.size() > 1 ? value.params[1] : Type{Type::Kind::Unknown};
        return is_assignable(target_success, value_success) &&
               is_assignable(target_error, value_error);
    }

    if (is_numeric(target) && is_numeric(value)) {
        return numeric_rank(value) <= numeric_rank(target);
    }

    if (target.kind == value.kind && (!target.params.empty() || !value.params.empty())) {
        if (target.kind == Type::Kind::Custom && target.custom_name != value.custom_name) {
            return false;
        }
        return structural_params_assignable(target, value);
    }

    if (target.kind == Type::Kind::Custom && value.kind == Type::Kind::Custom) {
        return target.custom_name == value.custom_name;
    }
    if (target.kind == Type::Kind::Constant && value.kind == Type::Kind::Constant) {
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
    if (op.type == TokenType::Percent && (!is_integer_type(left) || !is_integer_type(right))) {
        error(op, "Modulo requires integer operands, got '" + type_to_string(left) + "' and '" + type_to_string(right) + "'.");
        return make_error_type();
    }

    auto deduced = deduce_numeric_type(left, right, op);
    if (!deduced.has_value()) {
        return make_error_type();
    }
    if (op.type == TokenType::Percent && !is_integer_type(*deduced)) {
        return make_error_type();
    }
    return *deduced;
}

Type SemanticAnalyzer::evaluate_expression(const Expr& expr, const Type* expected) {
    _expected_type_stack.push_back(expected);
    auto result = analyze(expr);
    _expected_type_stack.pop_back();
    if (!result.has_value()) {
        return Type{Type::Kind::Unknown};
    }
    try {
        Type casted = std::any_cast<Type>(result);
        _expr_type_cache[&expr] = casted;
        return casted;
    } catch (const std::bad_any_cast&) {
        Type err = make_error_type();
        _expr_type_cache[&expr] = err;
        return err;
    }
}

const Type* SemanticAnalyzer::current_expected_type() const {
    if (_expected_type_stack.empty()) {
        return nullptr;
    }
    return _expected_type_stack.back();
}

const Type* SemanticAnalyzer::type_of(const Expr* expr) const {
    if (!expr) return nullptr;
    auto it = _expr_type_cache.find(expr);
    if (it == _expr_type_cache.end()) return nullptr;
    return &it->second;
}

const SemanticAnalyzer::LoopMetadata* SemanticAnalyzer::loop_metadata_for(const LoopStmt& stmt) const {
    auto it = _loop_index.find(&stmt);
    if (it == _loop_index.end()) return nullptr;
    return &_loop_metadata[it->second];
}

const SemanticAnalyzer::MatchMetadata* SemanticAnalyzer::match_metadata_for(const MatchExpr& expr) const {
    auto it = _match_index.find(&expr);
    if (it == _match_index.end()) return nullptr;
    return &_match_metadata[it->second];
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
    Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer, &declared_type) : Type{Type::Kind::Unknown};

    if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
        error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) + "' requires a type annotation or initializer.");
    }

    Type checked_declared = declared_type;
    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown) {
        checked_declared = refine_generic_type(declared_type, init_type);
    }

    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown &&
        !is_assignable(checked_declared, init_type)) {
        error(stmt.name, "Cannot assign initializer of type '" + type_to_string(init_type) +
                              "' to variable of type '" + type_to_string(declared_type) + "'.");
    }

    Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : checked_declared;
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
    Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer, &declared_type) : Type{Type::Kind::Unknown};

    if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
        error(stmt.name, "Constant '" + std::string(stmt.name.lexeme) + "' requires a type annotation or initializer.");
    }

    Type checked_declared = declared_type;
    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown) {
        checked_declared = refine_generic_type(declared_type, init_type);
    }

    if (declared_type.kind != Type::Kind::Unknown && init_type.kind != Type::Kind::Unknown &&
        !is_assignable(checked_declared, init_type)) {
        error(stmt.name, "Cannot assign initializer of type '" + type_to_string(init_type) +
                              "' to constant of type '" + type_to_string(declared_type) + "'.");
    }

    Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : checked_declared;
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

std::any SemanticAnalyzer::visit(const LoopStmt& stmt) {
    if (stmt.bound_kind == LoopStmt::BoundKind::None) {
        error(stmt.keyword, "Loops must be annotated with '@bounded(...)'.");
    }
    if (stmt.bound_kind == LoopStmt::BoundKind::Static) {
        if (!stmt.bound_value || *stmt.bound_value <= 0) {
            error(stmt.keyword, "Static loop bounds must be a positive integer.");
        }
    }
    if (stmt.bound_kind == LoopStmt::BoundKind::Guarded) {
        if (!stmt.guard_expression) {
            error(stmt.keyword, "Guarded loops must provide a guard expression.");
        } else {
            expect_condition_bool(*stmt.guard_expression, stmt.keyword);
        }
    }
    int depth = static_cast<int>(_loop_stack.size());
    LoopMetadata meta;
    meta.stmt = &stmt;
    meta.keyword = stmt.keyword;
    meta.bound_kind = stmt.bound_kind;
    meta.bound_value = stmt.bound_value;
    if (stmt.bound_kind == LoopStmt::BoundKind::Guarded) {
        meta.guard_present = true;
    }
    meta.depth = depth;
    meta.id = _next_loop_id++;
    meta.source_file = _source_name;
    _loop_index[&stmt] = _loop_metadata.size();
    _loop_metadata.push_back(meta);
    _loop_stack.push_back(&stmt);
    for (const auto& statement : stmt.body) {
        analyze(*statement);
    }
    _loop_stack.pop_back();
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

    Type value_type = evaluate_expression(*stmt.value, &expected);
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

std::any SemanticAnalyzer::visit(const TypeDecl& stmt) {
    std::string name_str = std::string(stmt.name.lexeme);
    size_t arity = stmt.params.size();
    auto it = _generic_arities.find(name_str);
    if (it == _generic_arities.end()) {
        _generic_arities[name_str] = arity;
    } else if (it->second != arity) {
        error(stmt.name, "Generic type '" + name_str + "' expects " +
                         std::to_string(it->second) + " parameters but got " +
                         std::to_string(arity) + ".");
    }

    if (!_defined_generics.insert(name_str).second) {
        error(stmt.name, "Generic type '" + name_str + "' is already defined.");
    }

    if (stmt.alias) {
        AliasInfo info;
        info.alias = stmt.alias.get();
        for (const auto& param : stmt.params) {
            info.params.emplace_back(std::string(param.lexeme));
        }
        _type_aliases[name_str] = info;
        analyze_type_expr(*stmt.alias);
    }
    return {};
}

std::any SemanticAnalyzer::visit(const RecordDecl& stmt) {
    std::string name_str = std::string(stmt.name.lexeme);
    if (_record_definitions.find(name_str) != _record_definitions.end()) {
        error(stmt.name, "Record '" + name_str + "' is already defined.");
        return {};
    }

    RecordInfo info;
    info.fields.reserve(stmt.fields.size());
    bool had_error = false;

    for (const auto& field : stmt.fields) {
        std::string field_name(field.name.lexeme);
        if (!field.type) {
            error(field.name, "Field '" + field_name + "' requires a type.");
            had_error = true;
            continue;
        }
        if (info.field_map.find(field_name) != info.field_map.end()) {
            error(field.name, "Field '" + field_name + "' is already declared in record '" + name_str + "'.");
            had_error = true;
            continue;
        }

        Type field_type = analyze_type_expr(*field.type);
        info.fields.push_back({field_name, field_type, field.name});
        info.field_map.emplace(field_name, field_type);
    }

    if (!had_error) {
        if (stmt.schema_version.has_value() && *stmt.schema_version > 0) {
            info.schema_version = static_cast<std::uint32_t>(*stmt.schema_version);
        }
        info.module_path = stmt.module_path.has_value() ? *stmt.module_path : _source_name;
        _record_definitions.emplace(name_str, std::move(info));
    }
    return {};
}

std::any SemanticAnalyzer::visit(const EnumDecl& stmt) {
    std::string name_str = std::string(stmt.name.lexeme);
    if (_enum_definitions.find(name_str) != _enum_definitions.end()) {
        error(stmt.name, "Enum '" + name_str + "' is already defined.");
        return {};
    }

    EnumInfo info;
    bool had_error = false;

    for (const auto& variant : stmt.variants) {
        std::string variant_name(variant.name.lexeme);
        if (info.variants.find(variant_name) != info.variants.end()) {
            error(variant.name, "Variant '" + variant_name + "' already exists in enum '" + name_str + "'.");
            had_error = true;
            continue;
        }

        if (variant.payload) {
            Type payload_type = analyze_type_expr(*variant.payload);
            info.variants.emplace(variant_name, payload_type);
        } else {
            info.variants.emplace(variant_name, std::nullopt);
        }
        info.variant_order.push_back(variant_name);
    }

    if (!had_error) {
        if (stmt.schema_version.has_value() && *stmt.schema_version > 0) {
            info.schema_version = static_cast<std::uint32_t>(*stmt.schema_version);
        }
        info.module_path = stmt.module_path.has_value() ? *stmt.module_path : _source_name;
        _enum_definitions.emplace(name_str, std::move(info));
    }
    return {};
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

    Type value_type = evaluate_expression(*expr.value, &symbol->type);
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
            if (!deduce_numeric_type(left_type, right_type, expr.op).has_value()) {
                return make_error_type();
            }
            return Type{Type::Kind::Bool};
        case TokenType::EqualEqual:
        case TokenType::BangEqual: {
            if (left_type == right_type) return Type{Type::Kind::Bool};
            if (deduce_numeric_type(left_type, right_type, expr.op).has_value()) {
                return Type{Type::Kind::Bool};
            }
            error(expr.op, "Invalid operands for equality check. Cannot compare '" + type_to_string(left_type) +
                             "' with '" + type_to_string(right_type) + "'.");
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
        const Type* expected = current_expected_type();

        auto build_result_template = [&](const Type* context) {
            Type result{Type::Kind::Result};
            if (context && context->kind == Type::Kind::Result) {
                result = *context;
            }
            if (result.params.size() < 2) {
                result.params.resize(2, Type{Type::Kind::Unknown});
            }
            return result;
        };

        if (func_name == "Some") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Some' constructor expects exactly one argument.");
                return make_error_type();
            }
            Type payload = arg_types[0];
            Type result{Type::Kind::Option, {payload}};
            if (expected && expected->kind == Type::Kind::Option) {
                Type expected_payload = expected->params.empty() ? Type{Type::Kind::Unknown} : expected->params[0];
                if (expected_payload.kind != Type::Kind::Unknown &&
                    !is_assignable(expected_payload, payload)) {
                    error(var_expr->name, "The 'Some' constructor argument must match the contextual Option payload ('" +
                                         type_to_string(expected_payload) + "').");
                } else if (expected_payload.kind != Type::Kind::Unknown) {
                    result.params[0] = expected_payload;
                } else {
                    result.params[0] = payload;
                }
                merge_expected_params(result, expected);
            }
            return result;
        }
        if (func_name == "None") {
            if (!arg_types.empty()) {
                error(var_expr->name, "The 'None' constructor does not take arguments.");
            }
            if (!expected || expected->kind != Type::Kind::Option) {
                error(var_expr->name, "The 'None' constructor requires a contextual Option[T] type.");
                return make_error_type();
            }
            Type option_type = *expected;
            if (option_type.params.empty()) {
                option_type.params.emplace_back(Type{Type::Kind::Unknown});
            }
            return option_type;
        }
        if (func_name == "Ok") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Ok' constructor expects exactly one argument.");
                return make_error_type();
            }
            if (!expected || expected->kind != Type::Kind::Result) {
                error(var_expr->name, "The 'Ok' constructor requires a contextual Result[T, E] type.");
                return make_error_type();
            }
            Type result_type = build_result_template(expected);
            Type success_expected = result_type.params[0];
            Type success_arg = arg_types[0];

            if (!is_assignable(success_expected, success_arg)) {
                error(var_expr->name, "The 'Ok' constructor argument must match the success type of the contextual Result.");
            }
            result_type.params[0] =
                (success_expected.kind == Type::Kind::Unknown ? success_arg : success_expected);
            merge_expected_params(result_type, expected);
            return result_type;
        }
        if (func_name == "Err") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'Err' constructor expects exactly one argument.");
                return make_error_type();
            }
            if (!expected || expected->kind != Type::Kind::Result) {
                error(var_expr->name, "The 'Err' constructor requires a contextual Result[T, E] type.");
                return make_error_type();
            }
            Type result_type = build_result_template(expected);
            Type error_expected = result_type.params[1];
            Type error_arg = arg_types[0];

            if (!is_assignable(error_expected, error_arg)) {
                error(var_expr->name, "The 'Err' constructor argument must match the error type of the contextual Result.");
            }
            result_type.params[1] =
                (error_expected.kind == Type::Kind::Unknown ? error_arg : error_expected);
            merge_expected_params(result_type, expected);
            return result_type;
        }
        if (func_name == "weights.load") {
            if (arg_types.size() != 1) {
                error(var_expr->name, "The 'weights.load' builtin expects exactly one argument.");
                return make_error_type();
            }
            if (arg_types[0].kind != Type::Kind::String) {
                error(var_expr->name, "The 'weights.load' argument must be a string literal.");
                return make_error_type();
            }
            if (!dynamic_cast<const LiteralExpr*>(expr.arguments[0].get())) {
                error(var_expr->name, "The 'weights.load' argument must be a string literal.");
                return make_error_type();
            }
            return Type{Type::Kind::I32};
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

std::any SemanticAnalyzer::visit(const MatchExpr& expr) {
    Type scrutinee_type = evaluate_expression(*expr.scrutinee);
    Token scrutinee_token = extract_token(*expr.scrutinee);
    bool is_option = scrutinee_type.kind == Type::Kind::Option;
    bool is_result = scrutinee_type.kind == Type::Kind::Result;
    bool is_enum = scrutinee_type.kind == Type::Kind::Custom;

    struct VariantMeta {
        std::optional<Type> payload;
    };

    std::unordered_map<std::string, VariantMeta> allowed_variants;
    std::vector<std::string> required_variants;
    bool saw_some = false;
    bool saw_none = false;
    bool saw_ok = false;
    bool saw_err = false;
    std::string match_label = "Match";

    if (is_option) {
        match_label = "Option";
        Type payload = scrutinee_type.params.empty() ? Type{Type::Kind::Unknown} : scrutinee_type.params[0];
        allowed_variants.emplace("Some", VariantMeta{payload});
        allowed_variants.emplace("None", VariantMeta{std::nullopt});
        required_variants = {"Some", "None"};
    } else if (is_result) {
        match_label = "Result";
        Type success = scrutinee_type.params.size() >= 1 ? scrutinee_type.params[0] : Type{Type::Kind::Unknown};
        Type error = scrutinee_type.params.size() >= 2 ? scrutinee_type.params[1] : Type{Type::Kind::Unknown};
        allowed_variants.emplace("Ok", VariantMeta{success});
        allowed_variants.emplace("Err", VariantMeta{error});
        required_variants = {"Ok", "Err"};
    } else if (is_enum) {
        match_label = "Enum";
        auto enum_it = _enum_definitions.find(scrutinee_type.custom_name);
        if (enum_it == _enum_definitions.end()) {
            error(scrutinee_token, "Type '" + scrutinee_type.custom_name + "' is not a known enum.");
            return make_error_type();
        }
        const EnumInfo& info = enum_it->second;
        for (const auto& name : info.variant_order) {
            auto variant_it = info.variants.find(name);
            std::optional<Type> payload;
            if (variant_it != info.variants.end()) {
                payload = variant_it->second;
            }
            allowed_variants.emplace(name, VariantMeta{payload});
            required_variants.push_back(name);
        }
    } else {
        error(scrutinee_token, "Match expressions require Option[T], Result[T, E], or enum values.");
        return make_error_type();
    }

    const Type* contextual_expected = current_expected_type();
    Type result_type = contextual_expected ? *contextual_expected : Type{Type::Kind::Unknown};
    bool result_type_locked = contextual_expected && contextual_expected->kind != Type::Kind::Unknown;
    bool structural_error = false;
    std::unordered_set<std::string> seen_variants;
    std::vector<MatchMetadata::ArmInfo> arm_infos;

    for (const auto& arm : expr.arms) {
        std::string name{arm.keyword.lexeme};
        auto variant_it = allowed_variants.find(name);
        if (variant_it == allowed_variants.end()) {
            error(arm.keyword, "Variant '" + name + "' is not part of '" + type_to_string(scrutinee_type) + "'.");
            structural_error = true;
            continue;
        }
        if (name == "Some") saw_some = true;
        if (name == "None") saw_none = true;
        if (name == "Ok") saw_ok = true;
        if (name == "Err") saw_err = true;
        if (!seen_variants.insert(name).second) {
            error(arm.keyword, "Duplicate match arm for '" + name + "'.");
            structural_error = true;
        }

        bool variant_has_payload = variant_it->second.payload.has_value();
        Type payload_type = variant_has_payload ? *variant_it->second.payload : Type{Type::Kind::Unknown};
        MatchPattern::Kind pattern_kind = arm.pattern.kind;

        if (variant_has_payload && pattern_kind == MatchPattern::Kind::None) {
            error(arm.keyword, "Variant '" + name + "' requires a binding.");
            structural_error = true;
            continue;
        }

        if (!variant_has_payload && pattern_kind != MatchPattern::Kind::None) {
            error(arm.keyword, "Variant '" + name + "' does not accept a binding.");
            structural_error = true;
            continue;
        }

        enter_scope();
        bool pattern_valid = true;

        if (variant_has_payload && pattern_kind == MatchPattern::Kind::Variant) {
            pattern_valid = analyze_nested_variant(arm.pattern, payload_type);
        } else if (variant_has_payload && pattern_kind != MatchPattern::Kind::None) {
            pattern_valid = bind_pattern_payload(arm.pattern, payload_type, arm.keyword);
        }

        if (!pattern_valid) {
            exit_scope();
            structural_error = true;
            continue;
        }

        if (arm.guard) {
            Token guard_token = extract_token(*arm.guard);
            expect_condition_bool(*arm.guard, guard_token);
        }

        const Type* arm_expected = result_type_locked ? &result_type : nullptr;
        Type arm_type = evaluate_expression(*arm.expression, arm_expected);
        exit_scope();

        if (!result_type_locked && arm_type.kind != Type::Kind::Unknown) {
            result_type = arm_type;
            result_type_locked = true;
        }

        if (result_type_locked && arm_type.kind != Type::Kind::Unknown &&
                !is_assignable(result_type, arm_type)) {
            error(arm.keyword, "All match arms must produce the same type.");
            structural_error = true;
        }

        MatchMetadata::ArmInfo arm_info;
        arm_info.variant = name;
        arm_info.pattern_kind = pattern_kind;
        arm_info.has_guard = arm.guard != nullptr;
        if (variant_has_payload) {
            arm_info.payload_type = payload_type;
        }
        arm_info.arm_type = arm_type;
        arm_infos.push_back(std::move(arm_info));
    }

    MatchMetadata meta;
    meta.expr = &expr;
    meta.result_type = result_type;
    meta.kind = is_option ? MatchMetadata::Kind::Option
                         : is_result ? MatchMetadata::Kind::Result
                         : MatchMetadata::Kind::Enum;
    meta.has_none = saw_none;
    meta.has_some = saw_some;
    meta.has_ok = saw_ok;
    meta.has_err = saw_err;
    meta.guard_present = std::any_of(arm_infos.begin(), arm_infos.end(), [](const MatchMetadata::ArmInfo& info) {
        return info.has_guard;
    });
    meta.arms = std::move(arm_infos);
    _match_index[&expr] = _match_metadata.size();
    _match_metadata.push_back(std::move(meta));


    auto describe_missing_arm = [&](const std::string& missing) {
        std::ostringstream oss;
        oss << match_label << " match on '" << type_to_string(scrutinee_type)
            << "' requires '" << missing << "' arm";
        return oss.str();
    };

    for (const auto& required : required_variants) {
        if (seen_variants.find(required) == seen_variants.end()) {
            error(scrutinee_token, describe_missing_arm(required) + ".");
            structural_error = true;
        }
    }

    if (structural_error) {
        return make_error_type();
    }

    return result_type;
}

std::any SemanticAnalyzer::visit(const FieldAccessExpr& expr) {
    Type object_type = evaluate_expression(*expr.object);
    if (object_type.kind != Type::Kind::Custom || object_type.custom_name.empty()) {
        error(expr.field, "Field access requires a record value.");
        return make_error_type();
    }

    auto record_it = _record_definitions.find(object_type.custom_name);
    if (record_it == _record_definitions.end()) {
        error(expr.field, "Type '" + object_type.custom_name + "' has no record fields.");
        return make_error_type();
    }

    std::string field_name(expr.field.lexeme);
    auto field_it = record_it->second.field_map.find(field_name);
    if (field_it == record_it->second.field_map.end()) {
        error(expr.field, "Record '" + object_type.custom_name + "' has no field '" + field_name + "'.");
        return make_error_type();
    }

    return field_it->second;
}

std::any SemanticAnalyzer::visit(const RecordLiteralExpr& expr) {
    std::string type_name(expr.type_name.lexeme);
    auto record_it = _record_definitions.find(type_name);
    if (record_it == _record_definitions.end()) {
        error(expr.type_name, "Undefined record type '" + type_name + "'.");
        return make_error_type();
    }

    const RecordInfo& info = record_it->second;
    bool had_error = false;
    std::unordered_set<std::string> seen_fields;

    for (const auto& field : expr.fields) {
        std::string field_name(field.first.lexeme);
        auto expected_it = info.field_map.find(field_name);
        if (expected_it == info.field_map.end()) {
            error(field.first, "Record '" + type_name + "' has no field '" + field_name + "'.");
            had_error = true;
            continue;
        }

        if (!seen_fields.insert(field_name).second) {
            error(field.first, "Field '" + field_name + "' is provided more than once in '" + type_name + "'.");
            had_error = true;
        }

        Type expected_type = expected_it->second;
        Type actual_type = evaluate_expression(*field.second, &expected_type);
        if (!is_assignable(expected_type, actual_type)) {
            error(field.first, "Cannot assign '" + type_to_string(actual_type) + "' to field '" +
                                field_name + "' of type '" + type_to_string(expected_type) + "'.");
            had_error = true;
        }
    }

    if (seen_fields.size() != info.fields.size()) {
        for (const auto& field_info : info.fields) {
            if (seen_fields.find(field_info.name) == seen_fields.end()) {
                error(expr.type_name, "Record literal for '" + type_name + "' is missing field '" + field_info.name + "'.");
                had_error = true;
            }
        }
    }

    if (had_error) {
        return make_error_type();
    }

    Type result{Type::Kind::Custom};
    result.custom_name = type_name;
    return result;
}

std::any SemanticAnalyzer::visit(const EnumLiteralExpr& expr) {
    std::string enum_name(expr.enum_name.lexeme);
    auto enum_it = _enum_definitions.find(enum_name);
    if (enum_it == _enum_definitions.end()) {
        error(expr.enum_name, "Undefined enum '" + enum_name + "'.");
        return make_error_type();
    }

    std::string variant_name(expr.variant.lexeme);
    auto variant_it = enum_it->second.variants.find(variant_name);
    if (variant_it == enum_it->second.variants.end()) {
        error(expr.variant, "Enum '" + enum_name + "' has no variant '" + variant_name + "'.");
        return make_error_type();
    }

    if (variant_it->second.has_value()) {
        if (!expr.payload) {
            error(expr.variant, "Variant '" + variant_name + "' of enum '" + enum_name + "' requires a payload.");
            return make_error_type();
        }
        Type expected_type = *variant_it->second;
        Type actual_type = evaluate_expression(*expr.payload, &expected_type);
        if (!is_assignable(expected_type, actual_type)) {
            error(expr.variant, "Enum payload for '" + variant_name + "' must be '" + type_to_string(expected_type) + "'.");
            return make_error_type();
        }
    } else if (expr.payload) {
        Token location = extract_token(*expr.payload);
        error(location, "Variant '" + variant_name + "' of enum '" + enum_name + "' does not accept a payload.");
        return make_error_type();
    }

    Type result{Type::Kind::Custom};
    result.custom_name = enum_name;
    return result;
}

std::any SemanticAnalyzer::visit(const VectorLiteralExpr& expr) {
    if (expr.elements.empty()) {
        const Type* expected = current_expected_type();
        if (expected && (expected->kind == Type::Kind::Vector || expected->kind == Type::Kind::Tensor)) {
            Type result;
            if (expected->kind == Type::Kind::Vector) {
                result = *expected;
            } else {
                result.kind = Type::Kind::Vector;
                if (!expected->params.empty()) {
                    result.params.push_back(expected->params[0]);
                } else {
                    result.params.push_back(Type{Type::Kind::Unknown});
                }
            }
            _vector_literal_data[&expr] = {};
            return result;
        }
        error(expr.token, "Empty vector literal requires a contextual Vector[T] type.");
        return make_error_type();
    }

    Type element_type{Type::Kind::Unknown};
    std::vector<float> values;
    values.reserve(expr.elements.size());

    for (const auto& element : expr.elements) {
        Type elem_type = evaluate_expression(*element);
        if (elem_type.kind == Type::Kind::Error) {
            return make_error_type();
        }

        if (element_type.kind == Type::Kind::Unknown) {
            element_type = elem_type;
        } else if (element_type != elem_type) {
            if (is_numeric(element_type) && is_numeric(elem_type)) {
                auto merged = deduce_numeric_type(element_type, elem_type, expr.token);
                if (!merged.has_value()) {
                    return make_error_type();
                }
                element_type = *merged;
            } else {
                error(expr.token, "Vector literal elements must share a numeric type.");
                return make_error_type();
            }
        }

        if (!is_numeric(element_type)) {
            error(expr.token, "Vector literal elements must be numeric.");
            return make_error_type();
        }

        auto* literal = dynamic_cast<const LiteralExpr*>(element.get());
        if (!literal) {
            error(expr.token, "Vector literal elements must be literal numerics.");
            return make_error_type();
        }

        auto parsed = parse_numeric_literal_value(literal->value);
        if (!parsed.has_value()) {
            error(literal->value, "Numeric literal expected in vector literal.");
            return make_error_type();
        }
        values.push_back(*parsed);
    }

    Type result{Type::Kind::Vector};
    result.params.push_back(
        element_type.kind == Type::Kind::Unknown ? Type{Type::Kind::Unknown} : element_type);
    merge_expected_params(result, current_expected_type());
    _vector_literal_data[&expr] = std::move(values);
    return result;
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
    auto type_from_expr = [&](const Expr* raw) -> std::optional<Type> {
        if (!raw) return std::nullopt;
        if (auto* variable = dynamic_cast<const VariableExpr*>(raw)) {
            Type type = type_from_token(variable->name);
            if (type.kind != Type::Kind::Unknown && type.kind != Type::Kind::Constant) {
                return type;
            }
        }
        return std::nullopt;
    };

    for (size_t i = 0; i < expr.param_count; ++i) {
        if (!expr.params[i]) {
            error(expr.name, "Generic parameter " + std::to_string(i) + " is missing.");
            params.push_back(make_error_type());
            continue;
        }

        Expr* raw = expr.params[i].get();
        if (auto* type_expr = dynamic_cast<TypeExpr*>(raw)) {
            params.push_back(analyze_type_expr(*type_expr));
            continue;
        }

        if (i == 0) {
            error(expr.name, "The first generic parameter must be a type.");
            params.push_back(make_error_type());
            continue;
        }

        auto constant = constant_type_from_expr(*raw);
        if (!constant.has_value()) {
            error(expr.name, "Generic constant parameters must be integer literals or identifiers.");
            params.push_back(make_error_type());
            continue;
        }

        params.push_back(*constant);
    }

    Type base_expected;
    const Type* expected = current_expected_type();

    if (params.empty()) {
        error(expr.name, "Generic type requires at least one parameter.");
        return make_error_type();
    }

    if (params[0].kind == Type::Kind::Constant) {
        error(expr.name, "The first generic parameter must be a type.");
        return make_error_type();
    }

    if (type_name == "Option") {
        if (params.size() != 1) {
            error(expr.name, "The 'Option' type expects exactly one type parameter, but got " + std::to_string(params.size()) + ".");
        }
        Type result{Type::Kind::Option, {params[0]}};
        merge_expected_params(result, expected);
        return result;
    }

    if (type_name == "Result") {
        if (params.size() != 2) {
            error(expr.name, "The 'Result' type expects exactly two type parameters, but got " + std::to_string(params.size()) + ".");
        }
        if (params.size() > 1 && params[1].kind == Type::Kind::Constant) {
            if (auto normalized = type_from_expr(expr.params[1].get())) {
                params[1] = *normalized;
            }
        }
        Type success = params.size() > 0 ? params[0] : Type{Type::Kind::Unknown};
        Type err = params.size() > 1 ? params[1] : Type{Type::Kind::Unknown};
        Type result{Type::Kind::Result, {success, err}};
        merge_expected_params(result, expected);
        return result;
    }

    if (auto alias_it = _type_aliases.find(type_name); alias_it != _type_aliases.end()) {
        Type alias_type = instantiate_alias(alias_it->second, params, expr.name);
        merge_expected_params(alias_type, expected);
        enforce_generic_arity(alias_type, expr.name);
        return alias_type;
    }

    Type base = type_from_token(expr.name);
    base.params = params;
    merge_expected_params(base, expected);
    enforce_generic_arity(base, expr.name);
    return base;
}

std::optional<Type> SemanticAnalyzer::constant_type_from_expr(const Expr& expr) {
    if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) {
        if (literal->value.type == TokenType::Integer || literal->value.type == TokenType::Base81Integer) {
            return Type::constant(std::string(literal->value.lexeme));
        }
    }
    if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) {
        return Type::constant(std::string(variable->name.lexeme));
    }
    return std::nullopt;
}

bool SemanticAnalyzer::bind_pattern_payload(const MatchPattern& pattern,
                                            const Type& payload_type,
                                            const Token& keyword) {
    switch (pattern.kind) {
        case MatchPattern::Kind::Identifier:
            if (!pattern.binding_is_wildcard) {
                bind_pattern_symbol(pattern.identifier, payload_type);
            }
            return true;
        case MatchPattern::Kind::Tuple: {
            size_t expected_fields = pattern.tuple_bindings.size();
            if (payload_type.params.empty()) {
                error(keyword, "Tuple pattern for variant '" + std::string(keyword.lexeme) + "' lacks payload type information.");
                return false;
            }
            if (payload_type.params.size() != expected_fields) {
                error(keyword, "Tuple pattern for variant '" + std::string(keyword.lexeme) + "' expects " +
                               std::to_string(expected_fields) + " fields but payload has " +
                               std::to_string(payload_type.params.size()) + ".");
                return false;
            }
            for (size_t i = 0; i < expected_fields; ++i) {
                bind_pattern_symbol(pattern.tuple_bindings[i], payload_type.params[i]);
            }
            return true;
        }
        case MatchPattern::Kind::Record: {
            if (payload_type.kind != Type::Kind::Custom || payload_type.custom_name.empty()) {
                error(keyword, "Record pattern for variant '" + std::string(keyword.lexeme) + "' requires a record payload.");
                return false;
            }
            auto record_it = _record_definitions.find(payload_type.custom_name);
            if (record_it == _record_definitions.end()) {
                error(keyword, "Variant '" + std::string(keyword.lexeme) + "' payload '" + payload_type.custom_name + "' is not a known record.");
                return false;
            }
            const auto& info = record_it->second;
            bool ok = true;
            for (const auto& binding : pattern.record_bindings) {
                std::string field_name(binding.first.lexeme);
                auto field_it = info.field_map.find(field_name);
                if (field_it == info.field_map.end()) {
                    error(binding.first, "Record '" + payload_type.custom_name + "' has no field '" + field_name + "'.");
                    ok = false;
                    continue;
                }
                bind_pattern_symbol(binding.second, field_it->second);
            }
            return ok;
        }
        default:
            error(keyword, "Unsupported pattern kind for variant payload.");
            return false;
    }
}

bool SemanticAnalyzer::analyze_nested_variant(const MatchPattern& pattern, const Type& payload_type) {
    if (payload_type.kind != Type::Kind::Custom || payload_type.custom_name.empty()) {
        error(pattern.variant_name, "Variant '" + std::string(pattern.variant_name.lexeme) +
                                    "' requires an enum payload.");
        return false;
    }
    auto enum_it = _enum_definitions.find(payload_type.custom_name);
    if (enum_it == _enum_definitions.end()) {
        error(pattern.variant_name, "Enum '" + payload_type.custom_name + "' is not defined.");
        return false;
    }
    const auto& variants = enum_it->second.variants;
    std::string variant_name(pattern.variant_name.lexeme);
    auto variant_it = variants.find(variant_name);
    if (variant_it == variants.end()) {
        error(pattern.variant_name, "Variant '" + variant_name + "' is not part of '" + payload_type.custom_name + "'.");
        return false;
    }
    if (!pattern.variant_payload) {
        if (variant_it->second.has_value()) {
            error(pattern.variant_name, "Variant '" + variant_name + "' requires a binding.");
            return false;
        }
        return true;
    }
    if (!variant_it->second.has_value()) {
        error(pattern.variant_name, "Variant '" + variant_name + "' does not accept a binding.");
        return false;
    }
    return bind_pattern_payload(*pattern.variant_payload, *variant_it->second, pattern.variant_name);
}

void SemanticAnalyzer::bind_pattern_symbol(const Token& name, const Type& type) {
    if (std::string_view{name.lexeme} == "_") {
        return;
    }
    define_symbol(name, SymbolKind::Variable);
    if (auto* symbol = resolve_symbol(name)) {
        symbol->type = type;
    }
}

} // namespace frontend
} // namespace t81
