#if defined(_MSC_VER) && _MSC_VER < 1930
// MSVC before VS 2022 17.0 has broken thread_local in some contexts
static int type_to_string_depth = 0;
#define TYPE_TO_STRING_DEPTH type_to_string_depth
#else
thread_local int type_to_string_depth = 0;
#define TYPE_TO_STRING_DEPTH type_to_string_depth
#endif

#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/builtin_registry.hpp"
#include "t81/frontend/numeric_literals.hpp"
#include "t81/types/T81BigInt.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
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

bool is_base81_fraction_literal_expr(const t81::frontend::Expr& left,
                                     const t81::frontend::Expr& right) {
  using t81::frontend::LiteralExpr;
  using t81::frontend::TokenType;
  const auto* left_lit = dynamic_cast<const LiteralExpr*>(&left);
  const auto* right_lit = dynamic_cast<const LiteralExpr*>(&right);
  if (!left_lit || !right_lit) {
    return false;
  }
  // Accept any pair of integer literals (plain Integer or Base81Integer) as a fraction.
  auto is_int_token = [](TokenType t) {
    return t == TokenType::Integer || t == TokenType::Base81Integer;
  };
  return is_int_token(left_lit->value.type) && is_int_token(right_lit->value.type);
}

std::optional<long long> constant_integer_value(const t81::frontend::Expr& expr) {
  using t81::frontend::BinaryExpr;
  using t81::frontend::GroupingExpr;
  using t81::frontend::LiteralExpr;
  using t81::frontend::TokenType;
  using t81::frontend::UnaryExpr;

  auto safe_add = [](long long a, long long b) -> std::optional<long long> {
    if ((b > 0 && a > std::numeric_limits<long long>::max() - b) ||
        (b < 0 && a < std::numeric_limits<long long>::min() - b)) {
      return std::nullopt;
    }
    return a + b;
  };

  auto safe_sub = [](long long a, long long b) -> std::optional<long long> {
    if ((b > 0 && a < std::numeric_limits<long long>::min() + b) ||
        (b < 0 && a > std::numeric_limits<long long>::max() + b)) {
      return std::nullopt;
    }
    return a - b;
  };

  auto safe_mul = [](long long a, long long b) -> std::optional<long long> {
    if (a == 0 || b == 0) {
      return 0LL;
    }
    if (a == -1 && b == std::numeric_limits<long long>::min()) {
      return std::nullopt;
    }
    if (b == -1 && a == std::numeric_limits<long long>::min()) {
      return std::nullopt;
    }
    if (a > 0) {
      if (b > 0) {
        if (a > std::numeric_limits<long long>::max() / b) {
          return std::nullopt;
        }
      } else {
        if (b < std::numeric_limits<long long>::min() / a) {
          return std::nullopt;
        }
      }
    } else {
      if (b > 0) {
        if (a < std::numeric_limits<long long>::min() / b) {
          return std::nullopt;
        }
      } else {
        if (a != 0 && b < std::numeric_limits<long long>::max() / a) {
          return std::nullopt;
        }
      }
    }
    return a * b;
  };

  if (const auto* lit = dynamic_cast<const LiteralExpr*>(&expr)) {
    if (lit->value.type == TokenType::Integer) {
      try {
        return std::stoll(std::string(lit->value.lexeme));
      } catch (...) {
        return std::nullopt;
      }
    }
    if (lit->value.type == TokenType::Base81Integer) {
      try {
        auto bigint = t81::frontend::numeric_literals::parse_t81_bigint_literal(lit->value.lexeme);
        if (auto narrowed = bigint.maybe_int64(); narrowed.has_value()) {
          return *narrowed;
        } else {
          return std::nullopt;
        }
      } catch (const std::invalid_argument&) {
        return std::nullopt;
      } catch (...) {
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

  if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
    if (unary->op.type == TokenType::Minus) {
      auto nested = constant_integer_value(*unary->right);
      if (!nested.has_value()) {
        return std::nullopt;
      }
      if (*nested == std::numeric_limits<long long>::min()) {
        return std::nullopt;
      }
      return -*nested;
    }
    return std::nullopt;
  }

  if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
    auto left = constant_integer_value(*binary->left);
    auto right = constant_integer_value(*binary->right);
    if (!left.has_value() || !right.has_value()) {
      return std::nullopt;
    }

    switch (binary->op.type) {
      case TokenType::Plus:
        return safe_add(*left, *right);
      case TokenType::Minus:
        return safe_sub(*left, *right);
      case TokenType::Star:
        return safe_mul(*left, *right);
      case TokenType::Slash:
        if (*right == 0) {
          return std::nullopt;
        }
        if (*left == std::numeric_limits<long long>::min() && *right == -1) {
          return std::nullopt;
        }
        return *left / *right;
      case TokenType::Percent:
        if (*right == 0) {
          return std::nullopt;
        }
        if (*left == std::numeric_limits<long long>::min() && *right == -1) {
          return 0LL;
        }
        return *left % *right;
      default:
        return std::nullopt;
    }
  }

  if (const auto* grouping = dynamic_cast<const GroupingExpr*>(&expr)) {
    return constant_integer_value(*grouping->expression);
  }

  return std::nullopt;
}

std::optional<std::string> qualified_call_name(const t81::frontend::Expr& expr) {
  using t81::frontend::FieldAccessExpr;
  using t81::frontend::VariableExpr;
  if (const auto* var = dynamic_cast<const VariableExpr*>(&expr)) {
    return std::string(var->name.lexeme);
  }
  if (const auto* field = dynamic_cast<const FieldAccessExpr*>(&expr)) {
    auto head = qualified_call_name(*field->object);
    if (!head.has_value()) {
      return std::nullopt;
    }
    return *head + "." + std::string(field->field.lexeme);
  }
  return std::nullopt;
}

std::string canonical_stdlib_call_name(std::string_view name) {
  // Derived from the single source of truth: builtin_registry.hpp / kBuiltinTable.
  return std::string(t81::frontend::canonical_name_for(name));
}

std::optional<int> minimum_tier_for_call_surface(std::string_view canonical_name) {
  // Derived from kBuiltinTable.  Legacy alias "Tensor.load" preserved.
  if (const auto* def = t81::frontend::lookup_builtin_by_canonical(canonical_name)) {
    return def->min_tier;
  }
  if (canonical_name == "Tensor.load") return 2;  // legacy alias
  return std::nullopt;
}

bool is_effect_surface_call(std::string_view canonical_name) {
  // Derived from kBuiltinTable.  Legacy alias "Tensor.load" preserved.
  if (const auto* def = t81::frontend::lookup_builtin_by_canonical(canonical_name)) {
    return def->is_effect_surface;
  }
  if (canonical_name == "Tensor.load") return true;  // legacy alias
  return false;
}

int minimum_tier_for_stmt(const t81::frontend::Stmt& stmt) {
  using t81::frontend::DistributedStmt;
  using t81::frontend::InfiniteStmt;
  using t81::frontend::LoopStmt;
  using t81::frontend::RecurseStmt;
  using t81::frontend::ReflectStmt;
  using t81::frontend::TrainStmt;
  if (dynamic_cast<const InfiniteStmt*>(&stmt)) return 5;
  if (dynamic_cast<const DistributedStmt*>(&stmt) || dynamic_cast<const TrainStmt*>(&stmt))
    return 4;
  if (dynamic_cast<const RecurseStmt*>(&stmt)) return 3;
  if (dynamic_cast<const ReflectStmt*>(&stmt)) return 2;
  if (const auto* loop = dynamic_cast<const LoopStmt*>(&stmt)) {
    if (loop->bound_kind == LoopStmt::BoundKind::Infinite ||
        loop->bound_kind == LoopStmt::BoundKind::Guarded) {
      return 2;
    }
  }
  return 1;
}

const t81::frontend::Token* tier_anchor_token(const t81::frontend::Stmt& stmt) {
  using t81::frontend::DistributedStmt;
  using t81::frontend::ForStmt;
  using t81::frontend::InfiniteStmt;
  using t81::frontend::LoopStmt;
  using t81::frontend::RecurseStmt;
  using t81::frontend::ReflectStmt;
  using t81::frontend::TrainStmt;
  if (const auto* recurse = dynamic_cast<const RecurseStmt*>(&stmt)) return &recurse->keyword;
  if (const auto* reflect = dynamic_cast<const ReflectStmt*>(&stmt)) return &reflect->keyword;
  if (const auto* distributed = dynamic_cast<const DistributedStmt*>(&stmt))
    return &distributed->keyword;
  if (const auto* infinite = dynamic_cast<const InfiniteStmt*>(&stmt)) return &infinite->keyword;
  if (const auto* train = dynamic_cast<const TrainStmt*>(&stmt)) return &train->keyword;
  if (const auto* loop = dynamic_cast<const LoopStmt*>(&stmt)) return &loop->keyword;
  if (const auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) return &for_stmt->iterator;
  return nullptr;
}

void collect_tier_violations(const t81::frontend::Stmt& stmt, int declared_tier,
                             std::vector<std::pair<t81::frontend::Token, std::string>>& out) {
  using t81::frontend::BlockStmt;
  using t81::frontend::DistributedStmt;
  using t81::frontend::ForStmt;
  using t81::frontend::IfStmt;
  using t81::frontend::InfiniteStmt;
  using t81::frontend::LoopStmt;
  using t81::frontend::RecurseStmt;
  using t81::frontend::ReflectStmt;
  using t81::frontend::TrainStmt;
  using t81::frontend::WhileStmt;

  const int required_tier = minimum_tier_for_stmt(stmt);
  if (required_tier > declared_tier) {
    const auto* token = tier_anchor_token(stmt);
    if (token != nullptr) {
      std::ostringstream msg;
      msg << "Function tier @" << declared_tier << " violates required tier @" << required_tier
          << " behavior.";
      out.emplace_back(*token, msg.str());
    }
  }

  if (const auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
    for (const auto& nested : block->statements) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* loop = dynamic_cast<const LoopStmt*>(&stmt)) {
    for (const auto& nested : loop->body) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* reflect = dynamic_cast<const ReflectStmt*>(&stmt)) {
    for (const auto& nested : reflect->body) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* distributed = dynamic_cast<const DistributedStmt*>(&stmt)) {
    for (const auto& nested : distributed->body) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* infinite = dynamic_cast<const InfiniteStmt*>(&stmt)) {
    for (const auto& nested : infinite->body) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* train = dynamic_cast<const TrainStmt*>(&stmt)) {
    for (const auto& nested : train->body) {
      collect_tier_violations(*nested, declared_tier, out);
    }
    return;
  }
  if (const auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
    collect_tier_violations(*while_stmt->body, declared_tier, out);
    return;
  }
  if (const auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) {
    collect_tier_violations(*for_stmt->body, declared_tier, out);
    return;
  }
  if (const auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
    if (if_stmt->then_branch) {
      collect_tier_violations(*if_stmt->then_branch, declared_tier, out);
    }
    if (if_stmt->else_branch) {
      collect_tier_violations(*if_stmt->else_branch, declared_tier, out);
    }
    return;
  }
  if (dynamic_cast<const RecurseStmt*>(&stmt)) {
    return;
  }
}
}  // namespace

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
    : _statements(statements), _source_name(std::move(source_name)) {
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
        define_symbol(func->name, SymbolKind::Function, false);
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

void SemanticAnalyzer::analyze(const Stmt& stmt) { stmt.accept(*this); }

std::any SemanticAnalyzer::analyze(const Expr& expr) { return expr.accept(*this); }

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

void SemanticAnalyzer::enter_scope() { _scopes.emplace_back(); }

void SemanticAnalyzer::exit_scope() {
  if (!_scopes.empty()) {
    _scopes.pop_back();
  }
}

void SemanticAnalyzer::define_symbol(const Token& name, SymbolKind kind, bool is_mutable) {
  if (!_scopes.empty()) {
    std::string name_str = std::string(name.lexeme);
    _scopes.back()[name_str] =
        SemanticSymbol{kind, name, Type{}, {}, {}, std::nullopt, is_mutable, false};
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

Type SemanticAnalyzer::make_error_type() { return Type{Type::Kind::Error}; }

int SemanticAnalyzer::numeric_rank(const Type& type) const {
  switch (type.kind) {
    case Type::Kind::Qutrit:
      return 1;
    case Type::Kind::I2:
      return 2;
    case Type::Kind::I8:
      return 3;
    case Type::Kind::I16:
      return 4;
    case Type::Kind::I32:
      return 5;
    case Type::Kind::Uint:
      return 6;
    case Type::Kind::BigInt:
      return 7;
    case Type::Kind::Fraction:
      return 8;
    case Type::Kind::Fixed:
      return 9;
    case Type::Kind::Float:
      return 10;
    default:
      return 0;
  }
}

bool SemanticAnalyzer::is_numeric(const Type& type) const { return numeric_rank(type) > 0; }

bool SemanticAnalyzer::is_integer_type(const Type& type) const {
  switch (type.kind) {
    case Type::Kind::Qutrit:
    case Type::Kind::I2:
    case Type::Kind::I8:
    case Type::Kind::I16:
    case Type::Kind::I32:
    case Type::Kind::Uint:
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
  return is_integer_type(type) || is_float_type(type) || is_fraction_type(type) ||
         type.kind == Type::Kind::Fixed;
}

std::optional<Type> SemanticAnalyzer::deduce_numeric_type(const Type& left, const Type& right,
                                                          const Token& op) {
  if (left.kind == Type::Kind::Error || right.kind == Type::Kind::Error) {
    return make_error_type();
  }
  if (left.kind == Type::Kind::Unknown || right.kind == Type::Kind::Unknown) {
    return Type{Type::Kind::Unknown};
  }

  if (left.kind == Type::Kind::Complex && right.kind == Type::Kind::Complex) {
    return left;
  }

  if (left.kind == Type::Kind::Fixed && right.kind == Type::Kind::Fixed) {
    return left;
  }
  if (left.kind == Type::Kind::Fixed && is_integer_type(right)) {
    return left;
  }
  if (right.kind == Type::Kind::Fixed && is_integer_type(left)) {
    return right;
  }
  if (left.kind == Type::Kind::Fixed && is_float_type(right)) {
    return right;
  }
  if (right.kind == Type::Kind::Fixed && is_float_type(left)) {
    return left;
  }

  if (!is_primitive_numeric_type(left) || !is_primitive_numeric_type(right)) {
    error(op, "Operands must be primitive numeric types, got '" + type_to_string(left) + "' and '" +
                  type_to_string(right) + "'.");
    return std::nullopt;
  }

  if (is_integer_type(left) && is_integer_type(right)) {
    if (op.type == TokenType::Minus &&
        (left.kind == Type::Kind::Uint || right.kind == Type::Kind::Uint)) {
      // Preserve deterministic semantics for unsigned subtraction by promoting
      // to a signed wide domain instead of inferring wrap-prone unsigned.
      return Type{Type::Kind::BigInt};
    }
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

  error(op, "Operands must share a primitive numeric type (T81Int, T81Float, or T81Fraction) or "
            "widen deterministically from T81Int. Got '" +
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
    if (target.params[i].kind == Type::Kind::Unknown &&
        expected->params[i].kind != Type::Kind::Unknown) {
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
      } else if (target_param.kind == Type::Kind::Unknown ||
                 value_param.kind == Type::Kind::Unknown) {
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

Type SemanticAnalyzer::instantiate_alias(const AliasInfo& alias, const std::vector<Type>& params,
                                         const Token& location) {
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
  if (_current_type_env) {
    auto env_it = _current_type_env->find(std::string(name.lexeme));
    if (env_it != _current_type_env->end()) {
      return env_it->second;
    }
  }

  switch (name.type) {
    case TokenType::Void:
      return Type{Type::Kind::Void};
    case TokenType::Bool:
      return Type{Type::Kind::Bool};
    case TokenType::I2:
      return Type{Type::Kind::I2};
    case TokenType::I8:
      return Type{Type::Kind::I8};
    case TokenType::I16:
      return Type{Type::Kind::I16};
    case TokenType::I32:
      return Type{Type::Kind::I32};
    case TokenType::T81BigInt:
      return Type{Type::Kind::BigInt};
    case TokenType::T81Float:
      return Type{Type::Kind::Float};
    case TokenType::T81Fraction:
      return Type{Type::Kind::Fraction};
    case TokenType::T81Fixed:
      return Type{Type::Kind::Fixed};
    case TokenType::T81Complex:
      return Type{Type::Kind::Complex};
    case TokenType::T81Quaternion:
      return Type{Type::Kind::Quaternion};
    case TokenType::T81Prob:
      return Type{Type::Kind::Prob};
    case TokenType::Cell:
      return Type{Type::Kind::Cell};
    case TokenType::T81Qutrit:
      return Type{Type::Kind::Qutrit};
    case TokenType::T81Uint:
      return Type{Type::Kind::Uint};
    case TokenType::T81Vector:
      return Type{Type::Kind::Vector};
    case TokenType::Matrix:
      return Type{Type::Kind::Matrix};
    case TokenType::Tensor:
      return Type{Type::Kind::Tensor};
    case TokenType::Graph:
      return Type{Type::Kind::Graph};
    case TokenType::List:
      return Type{Type::Kind::List};
    case TokenType::Map:
      return Type{Type::Kind::Map};
    case TokenType::Set:
      return Type{Type::Kind::Set};
    case TokenType::Tree:
      return Type{Type::Kind::Tree};
    case TokenType::String:
      return Type{Type::Kind::String};
    case TokenType::ByteString:
      return Type{Type::Kind::Bytes};
    default:
      break;
  }

  std::string name_str{name.lexeme};
  if (name_str == "T81Quaternion") return Type{Type::Kind::Quaternion};
  if (name_str == "T81Prob") return Type{Type::Kind::Prob};
  if (name_str == "Cell") return Type{Type::Kind::Cell};
  if (name_str == "Option") return Type{Type::Kind::Option};
  if (name_str == "Result") return Type{Type::Kind::Result};
  if (name_str == "Vector") return Type{Type::Kind::Vector};
  if (name_str == "Matrix") return Type{Type::Kind::Matrix};
  if (name_str == "Tensor") return Type{Type::Kind::Tensor};
  if (name_str == "List") return Type{Type::Kind::List};
  if (name_str == "Map") return Type{Type::Kind::Map};
  if (name_str == "Set") return Type{Type::Kind::Set};
  if (name_str == "Tree") return Type{Type::Kind::Tree};
  if (name_str == "Graph") return Type{Type::Kind::Graph};
  if (name_str == "T81Bytes") return Type{Type::Kind::Bytes};
  if (name_str == "Symbol") return Type{Type::Kind::Symbol};
  if (name_str == "InfiniteCanonicalForm") return Type{Type::Kind::InfiniteCanonicalForm};
  return Type{Type::Kind::Custom, {}, name_str};
}

const std::vector<float>* SemanticAnalyzer::vector_literal_data(
    const VectorLiteralExpr* expr) const {
  auto it = _vector_literal_data.find(expr);
  if (it == _vector_literal_data.end()) return nullptr;
  return &it->second;
}

const std::vector<float>* SemanticAnalyzer::set_literal_data(
    const SetLiteralExpr* expr) const {
  auto it = _set_literal_data.find(expr);
  if (it == _set_literal_data.end()) return nullptr;
  return &it->second;
}

const std::vector<float>* SemanticAnalyzer::map_literal_data(
    const MapLiteralExpr* expr) const {
  auto it = _map_literal_data.find(expr);
  if (it == _map_literal_data.end()) return nullptr;
  return &it->second;
}

std::string SemanticAnalyzer::expr_to_string(const Expr& expr) const {
  if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) {
    return std::string(literal->value.lexeme);
  }
  if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) {
    return std::string(variable->name.lexeme);
  }
  if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
    return expr_to_string(*binary->left) + " " + std::string(binary->op.lexeme) + " " +
           expr_to_string(*binary->right);
  }
  if (auto* grouping = dynamic_cast<const GroupingExpr*>(&expr)) {
    // Strip parentheses in diagnostic strings; the grouping is semantically transparent.
    return expr_to_string(*grouping->expression);
  }
  if (auto* field = dynamic_cast<const FieldAccessExpr*>(&expr)) {
    return expr_to_string(*field->object) + "." + std::string(field->field.lexeme);
  }
  if (auto* index = dynamic_cast<const IndexExpr*>(&expr)) {
    return expr_to_string(*index->object) + "[" + expr_to_string(*index->index) + "]";
  }
  if (auto* call = dynamic_cast<const CallExpr*>(&expr)) {
    std::string result = expr_to_string(*call->callee);
    result += "(";
    bool first = true;
    for (const auto& arg : call->arguments) {
      if (!first) {
        result += ", ";
      }
      first = false;
      result += expr_to_string(*arg);
    }
    result += ")";
    return result;
  }
  if (auto* generic = dynamic_cast<const GenericTypeExpr*>(&expr)) {
    return type_expr_to_string(*generic);
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
                                         const std::unordered_map<std::string, Type>* env) {
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
    case Type::Kind::Void:
      result = "void";
      break;
    case Type::Kind::Bool:
      result = "bool";
      break;
    case Type::Kind::I2:
      result = "i2";
      break;
    case Type::Kind::I8:
      result = "i8";
      break;
    case Type::Kind::I16:
      result = "i16";
      break;
    case Type::Kind::I32:
      result = "i32";
      break;
    case Type::Kind::BigInt:
      result = "T81BigInt";
      break;
    case Type::Kind::Float:
      result = "T81Float";
      break;
    case Type::Kind::Fraction:
      result = "T81Fraction";
      break;
    case Type::Kind::Fixed:
      result = "T81Fixed";
      break;
    case Type::Kind::Complex:
      result = "T81Complex";
      break;
    case Type::Kind::Quaternion:
      result = "T81Quaternion";
      break;
    case Type::Kind::Prob:
      result = "T81Prob";
      break;
    case Type::Kind::Cell:
      result = "Cell";
      break;
    case Type::Kind::Qutrit:
      result = "T81Qutrit";
      break;
    case Type::Kind::Uint:
      result = "T81Uint";
      break;
    case Type::Kind::Vector:
    case Type::Kind::Matrix:
    case Type::Kind::Tensor:
    case Type::Kind::List:
    case Type::Kind::Map:
    case Type::Kind::Set:
    case Type::Kind::Tree: {
      if (type.kind == Type::Kind::Vector)
        result = "Vector";
      else if (type.kind == Type::Kind::Matrix)
        result = "Matrix";
      else if (type.kind == Type::Kind::Tensor)
        result = "Tensor";
      else if (type.kind == Type::Kind::List)
        result = "List";
      else if (type.kind == Type::Kind::Map)
        result = "Map";
      else if (type.kind == Type::Kind::Set)
        result = "Set";
      else if (type.kind == Type::Kind::Tree)
        result = "Tree";

      if (!type.params.empty()) {
        result += "[";
        for (size_t i = 0; i < type.params.size(); ++i) {
          if (i > 0) result += ", ";
          result += type_to_string(type.params[i]);
        }
        result += "]";
      }
      break;
    }
    case Type::Kind::Graph:
      result = "Graph";
      break;
    case Type::Kind::Symbol:
      result = "Symbol";
      break;
    case Type::Kind::InfiniteCanonicalForm:
      result = "InfiniteCanonicalForm";
      break;
    case Type::Kind::String:
      result = "T81String";
      break;
    case Type::Kind::Bytes:
      result = "T81Bytes";
      break;
    case Type::Kind::Constant:
      result = "const(" + type.custom_name + ")";
      break;
    case Type::Kind::Custom:
      result = type.custom_name;
      break;
    case Type::Kind::Unknown:
      result = "<unknown>";
      break;
    case Type::Kind::Error:
      result = "<error>";
      break;

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
    return is_assignable(target_success, value_success) && is_assignable(target_error, value_error);
  }

  if (target.kind == Type::Kind::Fixed && is_integer_type(value)) {
    return true;
  }
  if (target.kind == Type::Kind::Qutrit && is_integer_type(value)) {
    if (value.kind == Type::Kind::BigInt) {
      return false;
    }
    return true;
  }
  if (target.kind == Type::Kind::Quaternion && value.kind == Type::Kind::Quaternion) return true;
  if (target.kind == Type::Kind::Prob && value.kind == Type::Kind::Prob) return true;
  if (target.kind == Type::Kind::Prob && value.kind == Type::Kind::Float) return true;
  if (target.kind == Type::Kind::Cell && value.kind == Type::Kind::Cell) return true;

  if (is_numeric(target) && is_numeric(value)) {
    // Allow widening but prevent narrowing
    // Higher rank = more precise/larger numeric type
    return numeric_rank(target) >= numeric_rank(value);
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
    error(op, "Modulo requires integer operands, got '" + type_to_string(left) + "' and '" +
                  type_to_string(right) + "'.");
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
    // Annotate the AST node directly (typed AST) and also maintain the
    // legacy cache for backwards compatibility during the transition.
    expr.resolved_type    = casted;
    _expr_type_cache[&expr] = casted;
    return casted;
  } catch (const std::bad_any_cast&) {
    Type err = make_error_type();
    expr.resolved_type    = err;
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
  // Prefer the typed-AST annotation; fall back to the legacy cache.
  if (expr->resolved_type.kind != Type::Kind::Unknown) {
    return &expr->resolved_type;
  }
  auto it = _expr_type_cache.find(expr);
  if (it == _expr_type_cache.end()) return nullptr;
  return &it->second;
}

const SemanticAnalyzer::LoopMetadata* SemanticAnalyzer::loop_metadata_for(
    const LoopStmt& stmt) const {
  auto it = _loop_index.find(&stmt);
  if (it == _loop_index.end()) return nullptr;
  return &_loop_metadata[it->second];
}

const SemanticAnalyzer::MatchMetadata* SemanticAnalyzer::match_metadata_for(
    const MatchExpr& expr) const {
  auto it = _match_index.find(&expr);
  if (it == _match_index.end()) return nullptr;
  return &_match_metadata[it->second];
}

Type SemanticAnalyzer::expect_condition_bool(const Expr& expr, const Token& location) {
  Type cond_type = evaluate_expression(expr);
  if (!is_assignable(Type{Type::Kind::Bool}, cond_type)) {
    error(location, "Condition expression '" + expr_to_string(expr) + "' must be bool, found '" +
                        type_to_string(cond_type) + "'.");
    return make_error_type();
  }
  return Type{Type::Kind::Bool};
}

bool SemanticAnalyzer::validate_constrained_integer_assignment(const Type& target,
                                                               const Expr& value,
                                                               const Token& location) {
  auto maybe_value = constant_integer_value(value);
  if (!maybe_value.has_value()) {
    return true;
  }

  const long long v = *maybe_value;
  if (target.kind == Type::Kind::Uint && v < 0) {
    error(location, "T81Uint does not allow negative constants; got '" + std::to_string(v) + "'.");
    return false;
  }
  if (target.kind == Type::Kind::Qutrit && (v < -1 || v > 1)) {
    error(location, "T81Qutrit constants must be in [-1, 1], got '" + std::to_string(v) + "'.");
    return false;
  }
  return true;
}

void SemanticAnalyzer::register_function_signatures() {
  for (const auto& stmt : _statements) {
    const auto* func = dynamic_cast<const FunctionStmt*>(stmt.get());
    if (!func) continue;

    SemanticSymbol* symbol = resolve_symbol(func->name);
    if (!symbol) continue;

    std::unordered_map<std::string, Type> type_env;
    type_env.reserve(func->generic_params.size());
    for (const auto& generic_param : func->generic_params) {
      const std::string param_name(generic_param.lexeme);
      Type generic_type{Type::Kind::Custom};
      generic_type.custom_name = param_name;
      if (!type_env.emplace(param_name, generic_type).second) {
        error(generic_param,
              "Generic parameter '" + param_name + "' is already defined in this function.");
      }
    }

    std::vector<Type> param_types;
    bool param_error = false;
    for (const auto& param : func->params) {
      if (!param.type) {
        param_error = true;
        error(param.name,
              "Parameter '" + std::string(param.name.lexeme) + "' is missing a type annotation.");
        param_types.push_back(make_error_type());
        continue;
      }
      param_types.push_back(analyze_type_expr(*param.type, &type_env));
    }

    Type return_type = func->return_type ? analyze_type_expr(*func->return_type, &type_env)
                                         : Type{Type::Kind::Void};
    symbol->param_types = param_types;
    symbol->type = return_type;
    symbol->tier = func->tier;
    symbol->is_pure = func->is_pure;
    symbol->is_attention = func->is_attention;
    symbol->is_qmatmul = func->is_qmatmul;
    symbol->generic_params.clear();
    symbol->generic_params.reserve(func->generic_params.size());
    for (const auto& generic_param : func->generic_params) {
      symbol->generic_params.emplace_back(std::string(generic_param.lexeme));
    }
    symbol->is_defined = !param_error;
  }
}

Token SemanticAnalyzer::extract_token(const Expr& expr) const {
  if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) return binary->op;
  if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) return unary->op;
  if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) return literal->value;
  if (auto* sym = dynamic_cast<const SymbolLiteralExpr*>(&expr)) return sym->value;
  if (auto* inf = dynamic_cast<const InfiniteLiteralExpr*>(&expr)) return inf->token;
  if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) return variable->name;
  if (auto* field = dynamic_cast<const FieldAccessExpr*>(&expr)) return field->field;
  if (auto* record = dynamic_cast<const RecordLiteralExpr*>(&expr)) return record->type_name;
  if (auto* enum_literal = dynamic_cast<const EnumLiteralExpr*>(&expr))
    return enum_literal->variant;
  if (auto* index = dynamic_cast<const IndexExpr*>(&expr)) return index->bracket;
  if (auto* match = dynamic_cast<const MatchExpr*>(&expr)) return extract_token(*match->scrutinee);
  if (auto* vector = dynamic_cast<const VectorLiteralExpr*>(&expr)) return vector->token;
  if (auto* if_expr = dynamic_cast<const IfExpr*>(&expr)) return extract_token(*if_expr->condition);
  if (auto* block = dynamic_cast<const BlockExpr*>(&expr)) {
    if (block->final_expr) return extract_token(*block->final_expr);
    if (!block->statements.empty()) {
      if (auto* expr_stmt = dynamic_cast<const ExpressionStmt*>(block->statements.front().get())) {
        return extract_token(*expr_stmt->expression);
      }
    }
  }
  if (auto* simple_type = dynamic_cast<const SimpleTypeExpr*>(&expr)) return simple_type->name;
  if (auto* generic_type = dynamic_cast<const GenericTypeExpr*>(&expr)) return generic_type->name;
  if (auto* assign = dynamic_cast<const AssignExpr*>(&expr)) return extract_token(*assign->target);
  if (auto* call = dynamic_cast<const CallExpr*>(&expr)) return extract_token(*call->callee);
  if (auto* grouping = dynamic_cast<const GroupingExpr*>(&expr))
    return extract_token(*grouping->expression);

  return Token{TokenType::Illegal, "", 0, 0};
}

// --- Visitor Method Implementations ---

std::any SemanticAnalyzer::visit(const ExpressionStmt& stmt) {
  evaluate_expression(*stmt.expression);
  return {};
}

std::any SemanticAnalyzer::visit(const VarStmt& stmt) {
  if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
    error(stmt.name,
          "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
    return {};
  }

  Type declared_type = stmt.type ? analyze_type_expr(*stmt.type) : Type{Type::Kind::Unknown};
  Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer, &declared_type)
                                    : Type{Type::Kind::Unknown};

  if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
    error(stmt.name, "Variable '" + std::string(stmt.name.lexeme) +
                         "' requires a type annotation or initializer.");
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
  if (stmt.initializer && checked_declared.kind != Type::Kind::Unknown) {
    validate_constrained_integer_assignment(checked_declared, *stmt.initializer, stmt.name);
  }

  Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : checked_declared;
  define_symbol(stmt.name, SymbolKind::Variable, true);
  if (auto* symbol = resolve_symbol(stmt.name)) {
    symbol->type = final_type;
  }
  return final_type;
}

std::any SemanticAnalyzer::visit(const LetStmt& stmt) {
  if (is_defined_in_current_scope(std::string(stmt.name.lexeme))) {
    error(stmt.name,
          "Variable '" + std::string(stmt.name.lexeme) + "' is already defined in this scope.");
    return {};
  }

  Type declared_type = stmt.type ? analyze_type_expr(*stmt.type) : Type{Type::Kind::Unknown};
  Type init_type = stmt.initializer ? evaluate_expression(*stmt.initializer, &declared_type)
                                    : Type{Type::Kind::Unknown};

  if (declared_type.kind == Type::Kind::Unknown && init_type.kind == Type::Kind::Unknown) {
    error(stmt.name, "Constant '" + std::string(stmt.name.lexeme) +
                         "' requires a type annotation or initializer.");
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
  if (stmt.initializer && checked_declared.kind != Type::Kind::Unknown) {
    validate_constrained_integer_assignment(checked_declared, *stmt.initializer, stmt.name);
  }

  Type final_type = declared_type.kind == Type::Kind::Unknown ? init_type : checked_declared;
  define_symbol(stmt.name, SymbolKind::Variable, stmt.is_mutable);
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
  _loop_stack.push_back(nullptr);  // WhileStmt doesn't have metadata yet, but it's a loop
  analyze(*stmt.body);
  _loop_stack.pop_back();
  return {};
}

std::any SemanticAnalyzer::visit(const ForStmt& stmt) {
  evaluate_expression(*stmt.iterable);
  enter_scope();
  define_symbol(stmt.iterator, SymbolKind::Variable, false);
  if (auto* symbol = resolve_symbol(stmt.iterator)) {
    symbol->type = Type{Type::Kind::I32};
  }
  _loop_stack.push_back(nullptr);
  analyze(*stmt.body);
  _loop_stack.pop_back();
  exit_scope();
  return {};
}

std::any SemanticAnalyzer::visit(const ReflectStmt& stmt) {
  for (const auto& s : stmt.body) {
    analyze(*s);
  }
  return {};
}

std::any SemanticAnalyzer::visit(const DistributedStmt& stmt) {
  enter_scope();
  for (const auto& s : stmt.body) {
    analyze(*s);
  }
  exit_scope();
  return {};
}

std::any SemanticAnalyzer::visit(const InfiniteStmt& stmt) {
  enter_scope();
  for (const auto& s : stmt.body) {
    analyze(*s);
  }
  exit_scope();
  return {};
}

std::any SemanticAnalyzer::visit(const TrainStmt& stmt) {
  evaluate_expression(*stmt.model);
  enter_scope();
  for (const auto& s : stmt.body) {
    analyze(*s);
  }
  exit_scope();
  return {};
}

std::any SemanticAnalyzer::visit(const RecurseStmt& stmt) {
  SemanticSymbol* symbol = resolve_symbol(stmt.name);
  if (!symbol) {
    define_symbol(stmt.name, SymbolKind::Function, false);
    symbol = resolve_symbol(stmt.name);
  }

  enter_scope();
  // Recurse functions don't have explicit return type in syntax, so we infer or default.
  // For now, allow Unknown or infer from body if we implemented inference.
  _function_return_stack.push_back(symbol ? symbol->type : Type{Type::Kind::Unknown});

  for (const auto& param : stmt.params) {
    Type param_type = param.type ? analyze_type_expr(*param.type) : Type{Type::Kind::Unknown};
    if (is_defined_in_current_scope(std::string(param.name.lexeme))) {
      error(param.name, "Parameter '" + std::string(param.name.lexeme) + "' is already defined.");
    } else {
      define_symbol(param.name, SymbolKind::Variable, false);
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

std::any SemanticAnalyzer::visit(const AssertStmt& stmt) {
  auto expr_type = evaluate_expression(*stmt.expr);
  if (expr_type.kind != Type::Kind::Bool && expr_type.kind != Type::Kind::Error) {
    error(stmt.keyword, "assert expects a bool expression.");
  }
  return {};
}

std::any SemanticAnalyzer::visit(const BreakStmt& stmt) {
  if (_loop_stack.empty()) {
    error(stmt.keyword, "Break statement outside of a loop.");
  }
  return {};
}

std::any SemanticAnalyzer::visit(const ContinueStmt& stmt) {
  if (_loop_stack.empty()) {
    error(stmt.keyword, "Continue statement outside of a loop.");
  }
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
      error(stmt.keyword,
            "Return type mismatch: expected '" + type_to_string(expected) + "' but got 'void'.");
    }
    return expected;
  }

  Type value_type = evaluate_expression(*stmt.value, &expected);
  if (!is_assignable(expected, value_type)) {
    error(stmt.keyword, "Return type mismatch: expected '" + type_to_string(expected) +
                            "' but got '" + type_to_string(value_type) + "'.");
  }
  return expected;
}

std::any SemanticAnalyzer::visit(const FunctionStmt& stmt) {
  SemanticSymbol* symbol = resolve_symbol(stmt.name);
  if (!symbol) {
    define_symbol(stmt.name, SymbolKind::Function, false);
    symbol = resolve_symbol(stmt.name);
  }

  std::unordered_map<std::string, Type> type_env;
  type_env.reserve(stmt.generic_params.size());
  for (const auto& generic_param : stmt.generic_params) {
    const std::string param_name(generic_param.lexeme);
    Type generic_type{Type::Kind::Custom};
    generic_type.custom_name = param_name;
    if (!type_env.emplace(param_name, generic_type).second) {
      error(generic_param,
            "Generic parameter '" + param_name + "' is already defined in this function.");
    }
  }

  enter_scope();
  _function_return_stack.push_back(symbol ? symbol->type : Type{Type::Kind::Unknown});
  const std::optional<std::int64_t> active_tier =
      stmt.tier.has_value() ? stmt.tier : (symbol ? symbol->tier : std::nullopt);
  _function_tier_stack.push_back(active_tier);

  if (symbol && symbol->param_types.size() != stmt.params.size()) {
    error(stmt.name, "Function parameter count mismatch between declaration and definition.");
  }

  // RFC-0026 AI-M6: @attention / @qmatmul are Tier 2+ only.
  if (stmt.is_attention || stmt.is_qmatmul) {
    const int effective_tier = static_cast<int>(active_tier.value_or(1));
    if (effective_tier < 2) {
      const std::string attr_name = stmt.is_attention ? "@attention" : "@qmatmul";
      error(stmt.name, attr_name + " requires Tier 2 or higher (add @tier(2) annotation).");
    }
  }

  if (stmt.tier.has_value()) {
    std::vector<std::pair<Token, std::string>> tier_violations;
    for (const auto& statement : stmt.body) {
      collect_tier_violations(*statement, static_cast<int>(*stmt.tier), tier_violations);
    }
    for (const auto& [token, message] : tier_violations) {
      error(token, message);
    }
  }

  for (size_t i = 0; i < stmt.params.size(); ++i) {
    const auto& param = stmt.params[i];
    Type param_type = (symbol && i < symbol->param_types.size()) ? symbol->param_types[i]
                                                                 : Type{Type::Kind::Unknown};

    if (param.type && param_type.kind == Type::Kind::Unknown) {
      param_type = analyze_type_expr(*param.type, &type_env);
    }

    if (is_defined_in_current_scope(std::string(param.name.lexeme))) {
      error(param.name, "Parameter '" + std::string(param.name.lexeme) + "' is already defined.");
    } else {
      define_symbol(param.name, SymbolKind::Variable, false);
      if (auto* param_symbol = resolve_symbol(param.name)) {
        param_symbol->type = param_type;
      }
    }
  }

  const bool outer_pure = _in_pure_function;
  _in_pure_function = stmt.is_pure || _in_pure_function;

  for (const auto& statement : stmt.body) {
    analyze(*statement);
  }

  _in_pure_function = outer_pure;
  _function_tier_stack.pop_back();
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
    error(stmt.name, "Generic type '" + name_str + "' expects " + std::to_string(it->second) +
                         " parameters but got " + std::to_string(arity) + ".");
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
      error(field.name,
            "Field '" + field_name + "' is already declared in record '" + name_str + "'.");
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
  info.id = _next_enum_id++;
  bool had_error = false;

  for (const auto& variant : stmt.variants) {
    std::string variant_name(variant.name.lexeme);
    if (info.variants.find(variant_name) != info.variants.end()) {
      error(variant.name,
            "Variant '" + variant_name + "' already exists in enum '" + name_str + "'.");
      had_error = true;
      continue;
    }

    if (variant.payload) {
      Type payload_type = analyze_type_expr(*variant.payload);
      EnumVariantInfo variant_info;
      variant_info.payload = payload_type;
      variant_info.id = static_cast<int>(info.variant_order.size());
      info.variants.emplace(variant_name, variant_info);
    } else {
      EnumVariantInfo variant_info;
      variant_info.payload.reset();
      variant_info.id = static_cast<int>(info.variant_order.size());
      info.variants.emplace(variant_name, variant_info);
    }
    info.variant_order.push_back(variant_name);

    // Inject variant into scope
    if (variant.payload) {
      // Constructor function
      define_symbol(variant.name, SymbolKind::Function, false);
      if (auto* sym = resolve_symbol(variant.name)) {
        sym->type = Type{Type::Kind::Custom, {}, name_str};
        sym->param_types.push_back(analyze_type_expr(*variant.payload));
      }
    } else {
      // Constant variable
      define_symbol(variant.name, SymbolKind::Variable, false);
      if (auto* sym = resolve_symbol(variant.name)) {
        sym->type = Type{Type::Kind::Custom, {}, name_str};
      }
    }
  }

  if (!had_error) {
    if (stmt.schema_version.has_value() && *stmt.schema_version > 0) {
      info.schema_version = static_cast<std::uint32_t>(*stmt.schema_version);
    }
    info.module_path = stmt.module_path.has_value() ? *stmt.module_path : _source_name;
    _enum_definitions.emplace(name_str, std::move(info));

    // Inject the Enum name itself as a symbol to support `Enum.Variant` access.
    // We type it as the Enum type itself.
    define_symbol(stmt.name, SymbolKind::Variable, false);
    if (auto* sym = resolve_symbol(stmt.name)) {
      sym->type = Type{Type::Kind::Custom, {}, name_str};
    }
  }
  return {};
}

// RFC-0015 §3 — register agent and type-check all behavior signatures.
// The body of each behavior is analyzed in its own scope so type errors
// are reported at authoring time.  The agent name is injected into the
// enclosing scope as a symbol of Kind::Variable with a custom type equal
// to the agent name, so that `AgentName.behavior(...)` call-site resolution
// in visit(CallExpr) can look it up.
std::any SemanticAnalyzer::visit(const AgentDecl& stmt) {
  const std::string agent_name(stmt.name.lexeme);
  if (_agent_definitions.count(agent_name)) {
    error(stmt.name, "Agent '" + agent_name + "' is already defined.");
    return {};
  }

  AgentInfo info;
  info.name = agent_name;

  for (const auto& beh : stmt.behaviors) {
    const std::string beh_name(beh.name.lexeme);
    if (info.behavior_map.count(beh_name)) {
      error(beh.name,
            "Behavior '" + beh_name + "' already defined in agent '" + agent_name + "'.");
      continue;
    }

    // Resolve parameter and return types.
    AgentBehaviorInfo bi;
    bi.name = beh_name;
    for (const auto& param : beh.params) {
      if (param.type) {
        bi.param_types.push_back(analyze_type_expr(*param.type));
      } else {
        bi.param_types.push_back(Type{Type::Kind::Unknown});
      }
    }
    bi.return_type = beh.return_type ? analyze_type_expr(*beh.return_type)
                                     : Type{Type::Kind::Void};

    // Analyze body in a fresh scope with parameters injected.
    enter_scope();
    _function_return_stack.push_back(bi.return_type);
    _function_tier_stack.push_back(std::nullopt);
    for (std::size_t i = 0; i < beh.params.size(); ++i) {
      define_symbol(beh.params[i].name, SymbolKind::Variable, false);
      if (auto* sym = resolve_symbol(beh.params[i].name)) {
        sym->type = bi.param_types[i];
      }
    }
    for (const auto& s : beh.body) {
      if (s) analyze(*s);
    }
    _function_tier_stack.pop_back();
    _function_return_stack.pop_back();
    exit_scope();

    const std::size_t idx = info.behaviors.size();
    info.behavior_map.emplace(beh_name, idx);
    info.behaviors.push_back(std::move(bi));
  }

  // Inject agent name as a symbol so call-site resolution can find it.
  define_symbol(stmt.name, SymbolKind::Variable, false);
  if (auto* sym = resolve_symbol(stmt.name)) {
    sym->type = Type{Type::Kind::Custom, {}, agent_name};
  }

  _agent_definitions.emplace(agent_name, std::move(info));
  return {};
}

// RFC-0036 §3 — Register each `foreign {}` block's function signatures so that
// `foreign.<name>(...)` call-sites can be resolved during IR generation.
std::any SemanticAnalyzer::visit(const ForeignDecl& stmt) {
  for (const auto& ff : stmt.functions) {
    const std::string fn_name(ff.name.lexeme);
    if (_foreign_definitions.count(fn_name)) {
      error(ff.name, "Foreign function '" + fn_name + "' is already declared.");
      continue;
    }
    ForeignFnInfo info;
    info.name = fn_name;
    info.policy = stmt.policy;
    for (const auto& param : ff.params) {
      info.param_types.push_back(param.type ? analyze_type_expr(*param.type)
                                            : Type{Type::Kind::Unknown});
    }
    info.return_type =
        ff.return_type ? analyze_type_expr(*ff.return_type) : Type{Type::Kind::Void};
    _foreign_definitions.emplace(fn_name, std::move(info));
  }
  return {};
}

std::any SemanticAnalyzer::visit(const AssignExpr& expr) {
  Type target_type = Type{Type::Kind::Unknown};
  bool mutable_target = false;
  Token error_token;

  if (auto* variable = dynamic_cast<const VariableExpr*>(expr.target.get())) {
    error_token = variable->name;
    auto* symbol = resolve_symbol(variable->name);
    if (!symbol) {
      error(variable->name, "Undefined variable '" + std::string(variable->name.lexeme) + "'.");
      evaluate_expression(*expr.value);
      return make_error_type();
    }
    if (symbol->kind != SymbolKind::Variable) {
      error(variable->name,
            "Cannot assign to non-variable '" + std::string(variable->name.lexeme) + "'.");
    } else if (!symbol->is_mutable) {
      error(variable->name,
            "Cannot assign to immutable binding '" + std::string(variable->name.lexeme) + "'.");
    } else {
      mutable_target = true;
      target_type = symbol->type;
    }
  } else if (auto* index_expr = dynamic_cast<const IndexExpr*>(expr.target.get())) {
    error_token = index_expr->bracket;
    target_type = evaluate_expression(*expr.target);
    if (target_type.kind == Type::Kind::Error) {
      evaluate_expression(*expr.value);
      return make_error_type();
    }
    if (is_mutable_lvalue(*expr.target)) {
      mutable_target = true;
    } else {
      error(error_token,
            "Cannot assign to immutable index expression '" + expr_to_string(*expr.target) + "'.");
    }

  } else if (auto* field_expr = dynamic_cast<const FieldAccessExpr*>(expr.target.get())) {
    error_token = field_expr->field;
    target_type = evaluate_expression(*expr.target);
    if (target_type.kind == Type::Kind::Error) {
      evaluate_expression(*expr.value);
      return make_error_type();
    }
    if (is_mutable_lvalue(*expr.target)) {
      mutable_target = true;
    } else {
      error(error_token, "Cannot assign to immutable field '" +
                             std::string(field_expr->field.lexeme) + "' in target '" +
                             expr_to_string(*expr.target) + "'.");
    }
  } else {
    error(extract_token(*expr.target),
          "Invalid assignment target '" + expr_to_string(*expr.target) + "'.");
    return make_error_type();
  }

  Type value_type = evaluate_expression(*expr.value, &target_type);

  if (mutable_target) {
    if (!is_assignable(target_type, value_type)) {
      error(error_token, "Cannot assign expression '" + expr_to_string(*expr.value) +
                             "' of type '" + type_to_string(value_type) + "' to target '" +
                             expr_to_string(*expr.target) + "' of type '" +
                             type_to_string(target_type) + "'.");
    }
    validate_constrained_integer_assignment(target_type, *expr.value, error_token);
  }

  return target_type;
}

std::any SemanticAnalyzer::visit(const BinaryExpr& expr) {
  if (expr.op.type == TokenType::Slash &&
      is_base81_fraction_literal_expr(*expr.left, *expr.right)) {
    // Treat N/D as a fraction literal if the context expects T81Fraction,
    // or if the old condition (Base81Integer denominator) holds.
    const Type* ctx = current_expected_type();
    bool context_is_fraction = ctx && ctx->kind == Type::Kind::Fraction;
    using t81::frontend::LiteralExpr;
    const auto* right_lit = dynamic_cast<const LiteralExpr*>(expr.right.get());
    bool denom_is_base81 = right_lit && right_lit->value.type == TokenType::Base81Integer;
    if (context_is_fraction || denom_is_base81) {
      return Type{Type::Kind::Fraction};
    }
  }

  Type left_type = evaluate_expression(*expr.left);
  // For comparison operators: if left is Fraction, propagate Fraction context
  // to the right side so "frac_var == N/D" correctly interprets N/D as fraction.
  const bool is_comparison_op =
      (expr.op.type == TokenType::EqualEqual || expr.op.type == TokenType::BangEqual ||
       expr.op.type == TokenType::Less || expr.op.type == TokenType::LessEqual ||
       expr.op.type == TokenType::Greater || expr.op.type == TokenType::GreaterEqual);
  Type right_type;
  if (is_comparison_op && left_type.kind == Type::Kind::Fraction) {
    right_type = evaluate_expression(*expr.right, &left_type);
  } else {
    right_type = evaluate_expression(*expr.right);
    // Symmetric: if right is Fraction and left is not, re-evaluate left with context
    if (is_comparison_op && right_type.kind == Type::Kind::Fraction &&
        left_type.kind != Type::Kind::Fraction) {
      left_type = evaluate_expression(*expr.left, &right_type);
    }
  }

  switch (expr.op.type) {
    case TokenType::Plus:
    case TokenType::Minus:
    case TokenType::Star:
    case TokenType::Slash:
    case TokenType::Percent:
      return widen_numeric(left_type, right_type, expr.op);
    case TokenType::Amp:
    case TokenType::Pipe:
    case TokenType::Caret:
    case TokenType::LessLess:
    case TokenType::GreaterGreater:
    case TokenType::GreaterGreaterGreater:
      if (!is_integer_type(left_type) || !is_integer_type(right_type)) {
        error(expr.op, "Bitwise operators require integer operands, got '" +
                           type_to_string(left_type) + "' and '" + type_to_string(right_type) +
                           "'.");
        return make_error_type();
      }
      return widen_numeric(left_type, right_type, expr.op);
    case TokenType::StarStar:
      if ((left_type.kind == Type::Kind::Tensor || left_type.kind == Type::Kind::Matrix) &&
          (right_type.kind == Type::Kind::Tensor || right_type.kind == Type::Kind::Matrix)) {
        return left_type;
      }
      return widen_numeric(left_type, right_type, expr.op);
    case TokenType::Greater:
    case TokenType::GreaterEqual:
    case TokenType::Less:
    case TokenType::LessEqual:
      // Option[T] and Result[T,E] have total ordering: None < Some(x); Err < Ok(x)
      if (left_type.kind == Type::Kind::Option && right_type.kind == Type::Kind::Option) {
        return Type{Type::Kind::Bool};
      }
      if (left_type.kind == Type::Kind::Result && right_type.kind == Type::Kind::Result) {
        return Type{Type::Kind::Bool};
      }
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
      error(expr.op, "Invalid operands for equality check. Cannot compare '" +
                         type_to_string(left_type) + "' with '" + type_to_string(right_type) +
                         "'.");
      return make_error_type();
    }
    case TokenType::AmpAmp:
    case TokenType::PipePipe:
      if (!is_assignable(Type{Type::Kind::Bool}, left_type) ||
          !is_assignable(Type{Type::Kind::Bool}, right_type)) {
        error(expr.op, "Logical operators require boolean operands, got '" +
                           type_to_string(left_type) + "' and '" + type_to_string(right_type) +
                           "'.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    default:
      return make_error_type();
  }
}

std::any SemanticAnalyzer::visit(const CallExpr& expr) {
  if (auto callee_name = qualified_call_name(*expr.callee); callee_name.has_value()) {
    const std::string canonical = canonical_stdlib_call_name(*callee_name);
    const bool checks_empty_literal = canonical == "collections_first" ||
                                      canonical == "collections_last" ||
                                      canonical == "collections_pop";
    if (checks_empty_literal && expr.arguments.size() == 1) {
      if (auto* literal = dynamic_cast<const VectorLiteralExpr*>(expr.arguments[0].get())) {
        if (literal->elements.empty()) {
          Token call_token = extract_token(*expr.callee);
          if (canonical == "collections_first") {
            error(call_token, "std.collections.first does not accept empty vector literals.");
          } else if (canonical == "collections_last") {
            error(call_token, "std.collections.last does not accept empty vector literals.");
          } else {
            error(call_token, "std.collections.pop does not accept empty vector literals.");
          }
          return make_error_type();
        }
      }
    }
  }

  std::vector<Type> arg_types;
  arg_types.reserve(expr.arguments.size());
  for (const auto& arg : expr.arguments) {
    arg_types.push_back(evaluate_expression(*arg));
  }

  auto render_function_signature = [&](std::string_view name, const std::vector<Type>& params,
                                       const Type& ret) -> std::string {
    std::string sig(name);
    sig += "(";
    for (size_t i = 0; i < params.size(); ++i) {
      if (i > 0) sig += ", ";
      sig += type_to_string(params[i]);
    }
    sig += ") -> ";
    sig += type_to_string(ret);
    return sig;
  };

  auto emit_unbound_generic_error =
      [&](const Token& where, std::string_view func_name, const std::vector<std::string>& params,
          const std::unordered_map<std::string, Type>& bindings) -> bool {
    std::vector<std::string> missing;
    missing.reserve(params.size());
    for (const auto& param : params) {
      if (bindings.find(param) == bindings.end()) {
        missing.push_back(param);
      }
    }
    if (missing.empty()) {
      return false;
    }
    if (missing.size() == 1) {
      error(where, "Cannot infer generic parameter '" + missing[0] + "' for function '" +
                       std::string(func_name) + "'.");
      return true;
    }
    std::string joined;
    for (size_t i = 0; i < missing.size(); ++i) {
      if (i > 0) joined += ", ";
      joined += "'" + missing[i] + "'";
    }
    error(where, "Cannot infer generic parameters " + joined + " for function '" +
                     std::string(func_name) + "'.");
    return true;
  };

  if (auto callee_name = qualified_call_name(*expr.callee); callee_name.has_value()) {
    const std::string raw_name = *callee_name;
    std::string func_name = canonical_stdlib_call_name(raw_name);
    Token call_token = extract_token(*expr.callee);
    auto enforce_active_tier_minimum = [&](int required_tier, std::string_view surface) {
      if (_function_tier_stack.empty()) return;
      const auto& active = _function_tier_stack.back();
      if (!active.has_value()) return;
      if (*active < required_tier) {
        std::ostringstream msg;
        msg << "Function tier @" << *active << " cannot call '" << surface << "' (requires tier @"
            << required_tier << ").";
        error(call_token, msg.str());
      }
    };
    if (auto required = minimum_tier_for_call_surface(func_name); required.has_value()) {
      enforce_active_tier_minimum(*required, raw_name);
    }
    if (!_function_tier_stack.empty() && _function_tier_stack.back().has_value() &&
        *_function_tier_stack.back() <= 1 && is_effect_surface_call(func_name)) {
      std::ostringstream msg;
      msg << "Function tier @" << *_function_tier_stack.back() << " cannot use effect surface '"
          << raw_name << "'.";
      error(call_token, msg.str());
    }
    if (_in_pure_function && is_effect_surface_call(func_name)) {
      std::ostringstream msg;
      msg << "@pure function cannot call effect surface '" << raw_name << "'.";
      error(call_token, msg.str());
    }
    if (func_name.find('.') != std::string::npos) {
      size_t dot = func_name.find('.');
      std::string obj_name = func_name.substr(0, dot);
      std::string method_name = func_name.substr(dot + 1);

      auto* obj_symbol = resolve_symbol(Token{TokenType::Identifier, obj_name, 0, 0});
      // Cache the object expression's type so the IR generator can look it up
      // via type_of(fa->object.get()) — the SA dispatches via resolve_symbol, so
      // VarExpr("r") is never processed by evaluate_expression and would otherwise
      // remain absent from _expr_type_cache.
      if (obj_symbol) {
        if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
          _expr_type_cache[fa->object.get()] = obj_symbol->type;
        }
      }
      // RFC-0036: foreign function call dispatch — `foreign.<name>(args)`.
      if (obj_name == "foreign") {
        auto fit = _foreign_definitions.find(method_name);
        if (fit == _foreign_definitions.end()) {
          error(call_token, "No foreign function named '" + method_name + "' declared.");
          return make_error_type();
        }
        const auto& fi = fit->second;
        if (arg_types.size() != fi.param_types.size()) {
          error(call_token, "Foreign function '" + method_name + "' expects " +
                                std::to_string(fi.param_types.size()) + " argument(s), got " +
                                std::to_string(arg_types.size()) + ".");
          return make_error_type();
        }
        expr.resolved_type = fi.return_type;
        return fi.return_type;
      }
      // RFC-0015: agent behavior call dispatch — `AgentName.behaviorName(args)`.
      {
        auto ait = _agent_definitions.find(obj_name);
        if (ait != _agent_definitions.end()) {
          auto bit = ait->second.behavior_map.find(method_name);
          if (bit == ait->second.behavior_map.end()) {
            error(call_token, "Agent '" + obj_name + "' has no behavior named '" + method_name +
                                  "'.");
            return make_error_type();
          }
          const auto& beh = ait->second.behaviors[bit->second];
          // Type-check argument count.
          if (arg_types.size() != beh.param_types.size()) {
            error(call_token, "Agent '" + obj_name + "." + method_name + "' expects " +
                                  std::to_string(beh.param_types.size()) + " argument(s), got " +
                                  std::to_string(arg_types.size()) + ".");
            return make_error_type();
          }
          Type ret = beh.return_type;
          expr.resolved_type = ret;
          return ret;
        }
      }
      if (obj_symbol && (obj_symbol->type.kind == Type::Kind::Tensor ||
                         obj_symbol->type.kind == Type::Kind::I32)) {
        if (method_name == "matmul" || method_name == "vec_add") {
          if (arg_types.size() != 1) {
            error(call_token, method_name + " expects 1 argument.");
            return make_error_type();
          }
          return obj_symbol->type;
        }
      }
      // Built-in method dispatch for non-record types
      if (obj_symbol) {
        const Type& ot = obj_symbol->type;
        // .len() on Vector, String, Bytes → T81BigInt
        if (method_name == "len" && arg_types.empty() &&
            (ot.kind == Type::Kind::Vector || ot.kind == Type::Kind::String ||
             ot.kind == Type::Kind::Bytes || ot.kind == Type::Kind::Tensor)) {
          return Type{Type::Kind::BigInt};
        }
        // .is_some() / .is_none() on Option → bool
        if ((method_name == "is_some" || method_name == "is_none") && arg_types.empty() &&
            ot.kind == Type::Kind::Option) {
          return Type{Type::Kind::Bool};
        }
        // .is_ok() / .is_err() on Result → bool
        if ((method_name == "is_ok" || method_name == "is_err") && arg_types.empty() &&
            ot.kind == Type::Kind::Result) {
          return Type{Type::Kind::Bool};
        }
        // .unsigned_shr(n) on integer types → T81BigInt
        if (method_name == "unsigned_shr" &&
            (ot.kind == Type::Kind::BigInt || ot.kind == Type::Kind::I32 ||
             ot.kind == Type::Kind::I16 || ot.kind == Type::Kind::I8 ||
             ot.kind == Type::Kind::I2 || ot.kind == Type::Kind::Uint)) {
          return Type{Type::Kind::BigInt};
        }
        // .unwrap() on Option → inner type
        if (method_name == "unwrap" && ot.kind == Type::Kind::Option) {
          if (!ot.params.empty()) return ot.params[0];
          return Type{Type::Kind::BigInt};
        }
        // .unwrap() / .unwrap_ok() on Result → inner OK type
        if ((method_name == "unwrap" || method_name == "unwrap_ok") && ot.kind == Type::Kind::Result) {
          if (!ot.params.empty()) return ot.params[0];
          return Type{Type::Kind::BigInt};
        }
        // .unwrap_err() on Result → inner Err type
        if (method_name == "unwrap_err" && ot.kind == Type::Kind::Result) {
          if (ot.params.size() >= 2) return ot.params[1];
          return Type{Type::Kind::BigInt};
        }
      }
    }
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
        error(call_token, "The 'Some' constructor expects exactly one argument.");
        return make_error_type();
      }
      Type payload = arg_types[0];
      Type result{Type::Kind::Option, {payload}};
      if (expected && expected->kind == Type::Kind::Option) {
        Type expected_payload =
            expected->params.empty() ? Type{Type::Kind::Unknown} : expected->params[0];
        if (expected_payload.kind != Type::Kind::Unknown &&
            !is_assignable(expected_payload, payload)) {
          error(call_token,
                "The 'Some' constructor argument must match the contextual Option payload: "
                "expected '" +
                    type_to_string(expected_payload) + "' but got '" + type_to_string(payload) +
                    "'.");
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
        error(call_token, "The 'None' constructor does not take arguments.");
      }
      if (!expected || expected->kind != Type::Kind::Option) {
        error(call_token, "The 'None' constructor requires a contextual Option[T] type.");
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
        error(call_token, "The 'Ok' constructor expects exactly one argument.");
        return make_error_type();
      }
      if (!expected || expected->kind != Type::Kind::Result) {
        error(call_token, "The 'Ok' constructor requires a contextual Result[T, E] type.");
        return make_error_type();
      }
      Type result_type = build_result_template(expected);
      Type success_expected = result_type.params[0];
      Type success_arg = arg_types[0];

      if (!is_assignable(success_expected, success_arg)) {
        error(call_token,
              "The 'Ok' constructor argument must match the success type of the contextual "
              "Result: expected '" +
                  type_to_string(success_expected) + "' but got '" + type_to_string(success_arg) +
                  "'.");
      }
      result_type.params[0] =
          (success_expected.kind == Type::Kind::Unknown ? success_arg : success_expected);
      merge_expected_params(result_type, expected);
      return result_type;
    }
    if (func_name == "Err") {
      if (arg_types.size() != 1) {
        error(call_token, "The 'Err' constructor expects exactly one argument.");
        return make_error_type();
      }
      if (!expected || expected->kind != Type::Kind::Result) {
        error(call_token, "The 'Err' constructor requires a contextual Result[T, E] type.");
        return make_error_type();
      }
      Type result_type = build_result_template(expected);
      Type error_expected = result_type.params[1];
      Type error_arg = arg_types[0];

      if (!is_assignable(error_expected, error_arg)) {
        error(call_token,
              "The 'Err' constructor argument must match the error type of the contextual "
              "Result: expected '" +
                  type_to_string(error_expected) + "' but got '" + type_to_string(error_arg) +
                  "'.");
      }
      result_type.params[1] =
          (error_expected.kind == Type::Kind::Unknown ? error_arg : error_expected);
      merge_expected_params(result_type, expected);
      return result_type;
    }
    if (func_name == "T81Bytes") {
      if (arg_types.size() != 1) {
        error(call_token, "T81Bytes conversion expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String && arg_types[0].kind != Type::Kind::Bytes) {
        error(call_token, "T81Bytes conversion expects a T81String or T81Bytes argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bytes};
    }
    if (func_name == "T81Maybe") {
      if (!arg_types.empty()) {
        error(call_token, "T81Maybe constructor expects no arguments.");
        return make_error_type();
      }
      if (expected && expected->kind == Type::Kind::Option) {
        return *expected;
      }
      return Type{Type::Kind::Option, {Type{Type::Kind::Unknown}}};
    }
    if (func_name == "T81Promise") {
      if (!arg_types.empty()) {
        error(call_token, "T81Promise constructor expects no arguments.");
        return make_error_type();
      }
      if (expected && expected->kind == Type::Kind::Custom &&
          expected->custom_name == "T81Promise") {
        return *expected;
      }
      return Type{Type::Kind::Custom, {}, "T81Promise"};
    }
    if (func_name == "T81Agent" || func_name == "T81Polynomial" || func_name == "T81Symbolic" ||
        func_name == "T81Time" || func_name == "T81Entropy") {
      if (!arg_types.empty()) {
        error(call_token, func_name + " constructor expects no arguments.");
        return make_error_type();
      }
      if (expected && expected->kind == Type::Kind::Custom && expected->custom_name == func_name) {
        return *expected;
      }
      return Type{Type::Kind::Custom, {}, func_name};
    }
    if (func_name == "T81Uint" || func_name == "T81Qutrit") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " conversion expects exactly one argument.");
        return make_error_type();
      }
      const Type& argument_type = arg_types[0];
      if (!is_integer_type(argument_type)) {
        error(call_token, func_name + " conversion expects an integer argument, got '" +
                              type_to_string(argument_type) + "'.");
        return make_error_type();
      }
      Type target_type = type_from_token(call_token);
      if (target_type.kind == Type::Kind::Unknown && func_name == "T81Qutrit") {
        target_type = Type{Type::Kind::Qutrit};
      } else if (target_type.kind == Type::Kind::Unknown && func_name == "T81Uint") {
        target_type = Type{Type::Kind::Uint};
      }
      validate_constrained_integer_assignment(target_type, *expr.arguments[0], call_token);
      return target_type;
    }
    if (func_name == "weights.load" || func_name == "Tensor.load") {
      if (arg_types.size() != 1) {
        error(call_token, "The 'load' builtin expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String) {
        error(call_token, "The 'load' argument must be a string literal.");
        return make_error_type();
      }
      if (!dynamic_cast<const LiteralExpr*>(expr.arguments[0].get())) {
        error(call_token, "The 'load' argument must be a string literal.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "Tensor.from_list") {
      if (arg_types.size() != 1) {
        error(call_token, "Tensor.from_list expects a single argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector && arg_types[0].kind != Type::Kind::I32) {
        error(call_token, "Tensor.from_list expects a Vector or Tensor handle.");
        return make_error_type();
      }
      return Type{Type::Kind::Tensor};
    }
    if (func_name == "Tensor.matmul") {
      if (arg_types.size() != 2) {
        error(call_token, "Tensor.matmul expects two arguments.");
        return make_error_type();
      }
      const bool left_ok =
          arg_types[0].kind == Type::Kind::Tensor || arg_types[0].kind == Type::Kind::I32;
      const bool right_ok =
          arg_types[1].kind == Type::Kind::Tensor || arg_types[1].kind == Type::Kind::I32;
      if (!left_ok || !right_ok) {
        error(call_token, "Tensor.matmul expects Tensor or tensor-handle arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Tensor};
    }
    if (func_name == "Tensor.vec_add") {
      if (arg_types.size() != 2) {
        error(call_token, "Tensor.vec_add expects two arguments.");
        return make_error_type();
      }
      const bool left_ok =
          arg_types[0].kind == Type::Kind::Tensor || arg_types[0].kind == Type::Kind::I32;
      const bool right_ok =
          arg_types[1].kind == Type::Kind::Tensor || arg_types[1].kind == Type::Kind::I32;
      if (!left_ok || !right_ok) {
        error(call_token, "Tensor.vec_add expects Tensor or tensor-handle arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Tensor};
    }
    if (func_name == "Tensor.attention") {
      if (arg_types.size() != 3) {
        error(call_token, "Tensor.attention expects three arguments (q, k, v).");
        return make_error_type();
      }
      return Type{Type::Kind::Tensor};
    }
    if (func_name == "Tensor.qmatmul") {
      if (arg_types.size() != 3) {
        error(call_token, "Tensor.qmatmul expects three arguments (activations, weights, scale).");
        return make_error_type();
      }
      return Type{Type::Kind::Tensor};
    }
    if (func_name == "read_code") {
      if (arg_types.size() != 1) {
        error(call_token, "read_code expects 1 argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "write_code") {
      if (arg_types.size() != 2) {
        error(call_token, "write_code expects 2 arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "refine") {
      if (arg_types.size() != 2) {
        error(call_token, "refine expects 2 arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "observe_performance") {
      return Type{Type::Kind::I32};
    }
    if (func_name == "optimize") {
      return Type{Type::Kind::I32};
    }
    if (func_name == "sin" || func_name == "cos" || func_name == "tan" || func_name == "asin" ||
        func_name == "acos" || func_name == "atan" || func_name == "sinh" || func_name == "cosh" ||
        func_name == "tanh") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0])) {
        error(call_token, func_name + " argument must be convertible to T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "exp" || func_name == "log" || func_name == "sqrt") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0])) {
        error(call_token, func_name + " argument must be convertible to T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "pow") {
      if (arg_types.size() != 2) {
        error(call_token, "pow expects exactly two arguments.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0]) ||
          !is_assignable(Type{Type::Kind::Float}, arg_types[1])) {
        error(call_token, "pow arguments must be convertible to T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "clamp") {
      if (arg_types.size() != 3) {
        error(call_token, "clamp expects exactly three arguments.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0]) ||
          !is_assignable(Type{Type::Kind::Float}, arg_types[1]) ||
          !is_assignable(Type{Type::Kind::Float}, arg_types[2])) {
        error(call_token, "clamp arguments must be convertible to T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "abs") {
      if (arg_types.size() != 1) {
        error(call_token, "abs expects exactly one argument.");
        return make_error_type();
      }
      if (!is_numeric(arg_types[0])) {
        error(call_token, "abs argument must be numeric.");
        return make_error_type();
      }
      return arg_types[0];
    }
    if (func_name == "bigint_from_int") {
      if (arg_types.size() != 1) {
        error(call_token, "bigint.from_int expects exactly one argument.");
        return make_error_type();
      }
      if (!is_integer_type(arg_types[0])) {
        error(call_token, "bigint.from_int argument must be integer.");
        return make_error_type();
      }
      return Type{Type::Kind::BigInt};
    }
    if (func_name == "bigint_to_int") {
      if (arg_types.size() != 1) {
        error(call_token, "bigint.to_int expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::BigInt) {
        error(call_token, "bigint.to_int argument must be T81BigInt.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "bigint_add" || func_name == "bigint_mul") {
      if (arg_types.size() != 2) {
        error(call_token, func_name + " expects exactly two arguments.");
        return make_error_type();
      }
      if (!is_numeric(arg_types[0]) || !is_numeric(arg_types[1])) {
        error(call_token, func_name + " arguments must be numeric.");
        return make_error_type();
      }
      return Type{Type::Kind::BigInt};
    }
    if (func_name == "tensor_dot") {
      if (arg_types.size() != 2) {
        error(call_token, "tensor.dot_product expects exactly two arguments.");
        return make_error_type();
      }
      if ((arg_types[0].kind != Type::Kind::Tensor && arg_types[0].kind != Type::Kind::I32) ||
          (arg_types[1].kind != Type::Kind::Tensor && arg_types[1].kind != Type::Kind::I32)) {
        error(call_token, "tensor.dot_product arguments must be Tensor or tensor handles.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "frac_add" || func_name == "frac_sub" || func_name == "frac_mul" ||
        func_name == "frac_div") {
      if (arg_types.size() != 2) {
        error(call_token, func_name + " expects exactly two arguments.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Fraction}, arg_types[0]) ||
          !is_assignable(Type{Type::Kind::Fraction}, arg_types[1])) {
        error(call_token, func_name + " arguments must be T81Fraction.");
        return make_error_type();
      }
      return Type{Type::Kind::Fraction};
    }
    if (func_name == "frac_from_int") {
      if (arg_types.size() != 1) {
        error(call_token, "frac_from_int expects exactly one argument.");
        return make_error_type();
      }
      if (!is_integer_type(arg_types[0])) {
        error(call_token, "frac_from_int argument must be integer.");
        return make_error_type();
      }
      return Type{Type::Kind::Fraction};
    }
    if (func_name == "frac_to_int") {
      if (arg_types.size() != 1) {
        error(call_token, "frac_to_int expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Fraction}, arg_types[0])) {
        error(call_token, "frac_to_int argument must be T81Fraction.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "frac_from_float") {
      if (arg_types.size() != 1) {
        error(call_token, "frac_from_float expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0])) {
        error(call_token, "frac_from_float argument must be T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Fraction};
    }
    if (func_name == "frac_to_float") {
      if (arg_types.size() != 1) {
        error(call_token, "frac_to_float expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Fraction}, arg_types[0])) {
        error(call_token, "frac_to_float argument must be T81Fraction.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "sys_exit") {
      if (arg_types.size() != 1) {
        error(call_token, "sys_exit expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::I32) {
        error(call_token, "sys_exit expects an i32 exit code argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "sys_time") {
      if (!arg_types.empty()) {
        error(call_token, "sys_time expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Float};
    }
    if (func_name == "sys_entropy") {
      if (!arg_types.empty()) {
        error(call_token, "sys_entropy expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "sys_proof") {
      if (!arg_types.empty()) {
        error(call_token, "sys_proof expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "sys_reflect") {
      if (!arg_types.empty()) {
        error(call_token, "sys_reflect expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "async_yield") {
      if (!arg_types.empty()) {
        error(call_token, "async_yield expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "async_sleep") {
      if (arg_types.size() != 1) {
        error(call_token, "async_sleep expects exactly one argument.");
        return make_error_type();
      }
      if (!is_assignable(Type{Type::Kind::Float}, arg_types[0])) {
        error(call_token, "async_sleep duration must be convertible to T81Float.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "async_thread" || func_name == "async_promise") {
      if (!arg_types.empty()) {
        error(call_token, func_name + " expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "agent_self_reflect") {
      if (!arg_types.empty()) {
        error(call_token, "agent_self_reflect expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "symbolic_load" || func_name == "polynomial_load") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String) {
        error(call_token, func_name + " expects a T81String seed argument.");
        return make_error_type();
      }
      return Type{
          Type::Kind::Custom, {}, func_name == "symbolic_load" ? "T81Symbolic" : "T81Polynomial"};
    }
    if (func_name == "symbolic_rewrite" || func_name == "polynomial_rewrite") {
      if (arg_types.size() != 3) {
        error(call_token, func_name + " expects exactly three arguments.");
        return make_error_type();
      }
      const std::string root_name =
          (func_name == "symbolic_rewrite") ? "T81Symbolic" : "T81Polynomial";
      if (arg_types[0].kind != Type::Kind::Custom || arg_types[0].custom_name != root_name) {
        error(call_token, func_name + " first argument must be " + root_name + ".");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String || arg_types[2].kind != Type::Kind::String) {
        error(call_token, func_name + " rewrite operands must be T81String.");
        return make_error_type();
      }
      return Type{Type::Kind::Custom, {}, root_name};
    }
    if (func_name == "symbolic_canon" || func_name == "polynomial_canon") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      const std::string root_name =
          (func_name == "symbolic_canon") ? "T81Symbolic" : "T81Polynomial";
      if (arg_types[0].kind != Type::Kind::Custom || arg_types[0].custom_name != root_name) {
        error(call_token, func_name + " argument must be " + root_name + ".");
        return make_error_type();
      }
      return Type{Type::Kind::Custom, {}, root_name};
    }
    if (func_name == "symbolic_confluent" || func_name == "polynomial_confluent") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      const std::string root_name =
          (func_name == "symbolic_confluent") ? "T81Symbolic" : "T81Polynomial";
      if (arg_types[0].kind != Type::Kind::Custom || arg_types[0].custom_name != root_name) {
        error(call_token, func_name + " argument must be " + root_name + ".");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "print") {
      if (arg_types.size() != 1) {
        error(call_token, "The 'print' builtin expects exactly one argument.");
        return make_error_type();
      }
      const Type& arg = arg_types[0];
      const bool supported = is_primitive_numeric_type(arg) || arg.kind == Type::Kind::String ||
                             arg.kind == Type::Kind::Bytes || arg.kind == Type::Kind::Bool;
      if (!supported) {
        error(call_token,
              "The 'print' builtin requires a scalar T81 numeric, bool, string, or bytes "
              "argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "core_assert") {
      if (arg_types.size() != 1) {
        error(call_token, "core_assert expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bool) {
        error(call_token, "core_assert expects a bool argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Void};
    }
    if (func_name == "option_unwrap_or") {
      if (arg_types.size() != 2) {
        error(call_token, "option_unwrap_or expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Option) {
        error(call_token, "option_unwrap_or expects Option[T] as the first argument.");
        return make_error_type();
      }
      Type payload =
          arg_types[0].params.empty() ? Type{Type::Kind::Unknown} : arg_types[0].params[0];
      if (payload.kind != Type::Kind::Unknown && !is_assignable(payload, arg_types[1])) {
        error(call_token, "option_unwrap_or default value must match Option payload type '" +
                              type_to_string(payload) + "', got '" + type_to_string(arg_types[1]) +
                              "'.");
        return make_error_type();
      }
      if (payload.kind == Type::Kind::Unknown) {
        payload = arg_types[1];
      }
      return payload;
    }
    if (func_name == "option_is_some" || func_name == "option_is_none") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Option) {
        error(call_token, func_name + " expects an Option[T] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "option_unwrap") {
      if (arg_types.size() != 1) {
        error(call_token, "std.option.unwrap expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Option) {
        error(call_token, "std.option.unwrap expects an Option[T] argument.");
        return make_error_type();
      }
      return arg_types[0].params.empty() ? Type{Type::Kind::Unknown} : arg_types[0].params[0];
    }
    if (func_name == "result_is_ok" || func_name == "result_is_err") {
      if (arg_types.size() != 1) {
        error(call_token, func_name + " expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Result) {
        error(call_token, func_name + " expects a Result[T, E] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "result_unwrap") {
      if (arg_types.size() != 1) {
        error(call_token, "std.result.unwrap expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Result) {
        error(call_token, "std.result.unwrap expects a Result[T, E] argument.");
        return make_error_type();
      }
      return arg_types[0].params.size() >= 1 ? arg_types[0].params[0] : Type{Type::Kind::Unknown};
    }
    if (func_name == "result_unwrap_err") {
      if (arg_types.size() != 1) {
        error(call_token, "std.result.unwrap_err expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Result) {
        error(call_token, "std.result.unwrap_err expects a Result[T, E] argument.");
        return make_error_type();
      }
      return arg_types[0].params.size() >= 2 ? arg_types[0].params[1] : Type{Type::Kind::Unknown};
    }
    if (func_name == "str_len") {
      if (arg_types.size() != 1) {
        error(call_token, "str_len expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String) {
        error(call_token, "str_len expects a T81String argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "str_is_empty") {
      if (arg_types.size() != 1) {
        error(call_token, "str_is_empty expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String) {
        error(call_token, "str_is_empty expects a T81String argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "str_concat") {
      if (arg_types.size() != 2) {
        error(call_token, "str_concat expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_concat expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "str_starts_with") {
      if (arg_types.size() != 2) {
        error(call_token, "str_starts_with expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_starts_with expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "str_ends_with") {
      if (arg_types.size() != 2) {
        error(call_token, "str_ends_with expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_ends_with expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "str_contains") {
      if (arg_types.size() != 2) {
        error(call_token, "str_contains expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_contains expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "str_index_of") {
      if (arg_types.size() != 2) {
        error(call_token, "str_index_of expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_index_of expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "str_replace") {
      if (arg_types.size() != 3) {
        error(call_token, "str_replace expects exactly three arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String ||
          arg_types[2].kind != Type::Kind::String) {
        error(call_token, "str_replace expects T81String arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "str_to_string") {
      if (arg_types.size() != 1) {
        error(call_token, "str_to_string expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String && arg_types[0].kind != Type::Kind::Bytes) {
        error(call_token, "str_to_string expects a T81String or T81Bytes argument.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "str_split") {
      if (arg_types.size() != 2) {
        error(call_token, "str_split expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String || arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_split expects T81String arguments.");
        return make_error_type();
      }
      if (auto* sep_literal = dynamic_cast<const LiteralExpr*>(expr.arguments[1].get())) {
        if (sep_literal->value.type == TokenType::String && sep_literal->value.lexeme == "\"\"") {
          error(call_token, "str_split separator must not be empty.");
          return make_error_type();
        }
      }
      Type result{Type::Kind::Vector};
      result.params.push_back(Type{Type::Kind::String});
      return result;
    }
    if (func_name == "str_join") {
      if (arg_types.size() != 2) {
        error(call_token, "str_join expects exactly two arguments.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token, "str_join expects a Vector[T81String] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "str_join expects a T81String separator argument.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "bytes_len") {
      if (arg_types.size() != 1) {
        error(call_token, "bytes_len expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_len expects a T81Bytes argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "bytes_is_empty") {
      if (arg_types.size() != 1) {
        error(call_token, "bytes_is_empty expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_is_empty expects a T81Bytes argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "bytes_concat") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_concat expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_concat expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bytes};
    }
    if (func_name == "bytes_starts_with") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_starts_with expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_starts_with expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "bytes_ends_with") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_ends_with expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_ends_with expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "bytes_contains") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_contains expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_contains expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "bytes_index_of") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_index_of expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_index_of expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "bytes_replace") {
      if (arg_types.size() != 3) {
        error(call_token, "bytes_replace expects exactly three arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes ||
          arg_types[2].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_replace expects T81Bytes arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bytes};
    }
    if (func_name == "bytes_split") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_split expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Bytes || arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_split expects T81Bytes arguments.");
        return make_error_type();
      }
      if (auto* sep_constructor = dynamic_cast<const CallExpr*>(expr.arguments[1].get())) {
        bool is_bytes_constructor = false;
        if (auto* sep_type = dynamic_cast<const SimpleTypeExpr*>(sep_constructor->callee.get())) {
          is_bytes_constructor = sep_type->name.lexeme == "T81Bytes";
        } else if (auto* sep_var =
                       dynamic_cast<const VariableExpr*>(sep_constructor->callee.get())) {
          is_bytes_constructor = sep_var->name.lexeme == "T81Bytes";
        }
        if (is_bytes_constructor && sep_constructor->arguments.size() == 1) {
          if (auto* sep_literal =
                  dynamic_cast<const LiteralExpr*>(sep_constructor->arguments[0].get())) {
            if (sep_literal->value.type == TokenType::String &&
                sep_literal->value.lexeme == "\"\"") {
              error(call_token, "bytes_split separator must not be empty.");
              return make_error_type();
            }
          }
        }
      }
      Type result{Type::Kind::Vector};
      result.params.push_back(Type{Type::Kind::Bytes});
      return result;
    }
    if (func_name == "bytes_join") {
      if (arg_types.size() != 2) {
        error(call_token, "bytes_join expects exactly two arguments.");
        return make_error_type();
      }
      const bool is_bytes_vector = arg_types[0].kind == Type::Kind::Vector &&
                                   !arg_types[0].params.empty() &&
                                   arg_types[0].params[0].kind == Type::Kind::Bytes;
      if (!is_bytes_vector) {
        error(call_token, "bytes_join expects a Vector[T81Bytes] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::Bytes) {
        error(call_token, "bytes_join expects a T81Bytes separator argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bytes};
    }
    if (func_name == "collections_len") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.len expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector) {
        error(call_token, "std.collections.len expects a Vector[T] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "collections_is_empty") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.is_empty expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector) {
        error(call_token, "std.collections.is_empty expects a Vector[T] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "collections_first" || func_name == "collections_last") {
      const bool is_first = func_name == "collections_first";
      if (arg_types.size() != 1) {
        error(call_token, std::string(is_first ? "std.collections.first" : "std.collections.last") +
                              " expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector || arg_types[0].params.empty()) {
        error(call_token, std::string(is_first ? "std.collections.first" : "std.collections.last") +
                              " expects a Vector[T] argument.");
        return make_error_type();
      }
      if (auto* literal = dynamic_cast<const VectorLiteralExpr*>(expr.arguments[0].get())) {
        if (literal->elements.empty()) {
          error(call_token,
                std::string(is_first ? "std.collections.first" : "std.collections.last") +
                    " does not accept empty vector literals.");
          return make_error_type();
        }
      }
      return arg_types[0].params[0];
    }
    if (func_name == "collections_push") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.push expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector || arg_types[0].params.empty()) {
        error(call_token, "std.collections.push expects a Vector[T] first argument.");
        return make_error_type();
      }
      const Type& element_type = arg_types[0].params[0];
      if (!is_assignable(element_type, arg_types[1])) {
        error(call_token, "std.collections.push second argument must match Vector element type.");
        return make_error_type();
      }
      return arg_types[0];
    }
    if (func_name == "collections_pop") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.pop expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Vector || arg_types[0].params.empty()) {
        error(call_token, "std.collections.pop expects a Vector[T] argument.");
        return make_error_type();
      }
      if (auto* literal = dynamic_cast<const VectorLiteralExpr*>(expr.arguments[0].get())) {
        if (literal->elements.empty()) {
          error(call_token, "std.collections.pop does not accept empty vector literals.");
          return make_error_type();
        }
      }
      return arg_types[0];
    }
    if (func_name == "collections_list") {
      if (!arg_types.empty()) {
        error(call_token, "std.collections.list expects no arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::List};
      out.params.push_back(
          Type{Type::Kind::String});  // Default to String for polyfill compat? Or generic?
      return out;
    }
    if (func_name == "collections_map") {
      if (!arg_types.empty()) {
        error(call_token, "std.collections.map expects no arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::Map};
      out.params.push_back(Type{Type::Kind::String});  // Key
      out.params.push_back(Type{Type::Kind::String});  // Value
      return out;
    }
    if (func_name == "collections_map_size") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.map_size expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        error(call_token, "std.collections.map_size expects a Map argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "collections_map_has") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.map_has expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        error(call_token, "std.collections.map_has expects a Map first argument.");
        return make_error_type();
      }
      // Assuming key is string for now as per polyfill
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.map_has expects a T81String key argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "collections_map_put") {
      if (arg_types.size() != 3) {
        error(call_token, "std.collections.map_put expects exactly three arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        std::cerr << "DEBUG: map_put arg0 kind=" << (int)arg_types[0].kind
                  << " Map=" << (int)Type::Kind::Map << std::endl;
        error(call_token, "std.collections.map_put expects a Map first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String || arg_types[2].kind != Type::Kind::String) {
        error(call_token, "std.collections.map_put expects T81String key/value arguments.");
        return make_error_type();
      }
      return arg_types[0];  // Return the map
    }
    if (func_name == "collections_map_get") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.map_get expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        error(call_token, "std.collections.map_get expects a Map first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.map_get expects a T81String key argument.");
        return make_error_type();
      }
      Type out{Type::Kind::Option};
      // Payload type from Map value param
      if (arg_types[0].params.size() >= 2) {
        out.params.push_back(arg_types[0].params[1]);
      } else {
        out.params.push_back(Type{Type::Kind::Unknown});
      }
      return out;
    }
    if (func_name == "collections_map_remove") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.map_remove expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        error(call_token, "std.collections.map_remove expects a Map first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.map_remove expects a T81String key argument.");
        return make_error_type();
      }
      return arg_types[0];
    }
    if (func_name == "collections_map_keys") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.map_keys expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Map) {
        error(call_token, "std.collections.map_keys expects a Map argument.");
        return make_error_type();
      }
      Type out{Type::Kind::Vector};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_set") {
      if (!arg_types.empty()) {
        error(call_token, "std.collections.set expects no arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::Set};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_set_size") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.set_size expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Set) {
        error(call_token, "std.collections.set_size expects a Set argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "collections_set_has") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.set_has expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Set) {
        error(call_token, "std.collections.set_has expects a Set first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.set_has expects a T81String key argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "collections_set_add") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.set_add expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Set) {
        error(call_token, "std.collections.set_add expects a Set first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.set_add expects a T81String key argument.");
        return make_error_type();
      }
      return arg_types[0];
    }
    if (func_name == "collections_set_remove") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.set_remove expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Set) {
        error(call_token, "std.collections.set_remove expects a Set first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.set_remove expects a T81String key argument.");
        return make_error_type();
      }
      return arg_types[0];
    }
    if (func_name == "collections_tree" || func_name == "collections_graph") {
      if (!arg_types.empty()) {
        error(call_token, "std.collections container constructors expect no arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::Vector};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_graph_edge_count") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.graph_edge_count expects exactly one argument.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token, "std.collections.graph_edge_count expects a Vector[T81String] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::I32};
    }
    if (func_name == "collections_graph_has_edge") {
      if (arg_types.size() != 3) {
        error(call_token, "std.collections.graph_has_edge expects exactly three arguments.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token,
              "std.collections.graph_has_edge expects a Vector[T81String] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String || arg_types[2].kind != Type::Kind::String) {
        error(call_token, "std.collections.graph_has_edge expects T81String from/to arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }
    if (func_name == "collections_graph_add_edge") {
      if (arg_types.size() != 3) {
        error(call_token, "std.collections.graph_add_edge expects exactly three arguments.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token,
              "std.collections.graph_add_edge expects a Vector[T81String] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String || arg_types[2].kind != Type::Kind::String) {
        error(call_token, "std.collections.graph_add_edge expects T81String from/to arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::Vector};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_graph_remove_edge") {
      if (arg_types.size() != 3) {
        error(call_token, "std.collections.graph_remove_edge expects exactly three arguments.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token,
              "std.collections.graph_remove_edge expects a Vector[T81String] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String || arg_types[2].kind != Type::Kind::String) {
        error(call_token, "std.collections.graph_remove_edge expects T81String from/to arguments.");
        return make_error_type();
      }
      Type out{Type::Kind::Vector};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_graph_neighbors") {
      if (arg_types.size() != 2) {
        error(call_token, "std.collections.graph_neighbors expects exactly two arguments.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token,
              "std.collections.graph_neighbors expects a Vector[T81String] first argument.");
        return make_error_type();
      }
      if (arg_types[1].kind != Type::Kind::String) {
        error(call_token, "std.collections.graph_neighbors expects a T81String from argument.");
        return make_error_type();
      }
      Type out{Type::Kind::Vector};
      out.params.push_back(Type{Type::Kind::String});
      return out;
    }
    if (func_name == "collections_graph_canonical") {
      if (arg_types.size() != 1) {
        error(call_token, "std.collections.graph_canonical expects exactly one argument.");
        return make_error_type();
      }
      const bool is_string_vector = arg_types[0].kind == Type::Kind::Vector &&
                                    !arg_types[0].params.empty() &&
                                    arg_types[0].params[0].kind == Type::Kind::String;
      if (!is_string_vector) {
        error(call_token,
              "std.collections.graph_canonical expects a Vector[T81String] argument.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "io_stream" || func_name == "io_net") {
      if (!arg_types.empty()) {
        error(call_token, func_name + " expects no arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "symbol_intern") {
      if (arg_types.size() != 1) {
        error(call_token, "symbol_intern expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String) {
        error(call_token, "symbol_intern expects a T81String argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Symbol};
    }
    if (func_name == "symbol_to_string") {
      if (arg_types.size() != 1) {
        error(call_token, "symbol_to_string expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Symbol) {
        error(call_token, "symbol_to_string expects a Symbol argument.");
        return make_error_type();
      }
      return Type{Type::Kind::String};
    }
    if (func_name == "symbol_eq" || func_name == "symbol_ne") {
      if (arg_types.size() != 2) {
        error(call_token, func_name + " expects exactly two arguments.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::Symbol || arg_types[1].kind != Type::Kind::Symbol) {
        error(call_token, func_name + " expects Symbol arguments.");
        return make_error_type();
      }
      return Type{Type::Kind::Bool};
    }

    // ── Table-driven fallback ──────────────────────────────────────────────
    // Handles builtins not covered by the custom blocks above.  Custom blocks
    // always run first, so type-checking logic in them is never short-circuited.
    if (const auto* reg_def = t81::frontend::lookup_builtin_by_canonical(func_name);
        reg_def && !reg_def->needs_custom_sa_check) {
      const int8_t expected_arity = reg_def->arity;
      if (expected_arity != t81::frontend::kArityAny &&
          static_cast<int8_t>(arg_types.size()) != expected_arity) {
        error(call_token, std::string(func_name) + " expects " +
                              std::to_string(expected_arity) + " argument(s), got " +
                              std::to_string(arg_types.size()) + ".");
        return make_error_type();
      }
      if (reg_def->return_kind == Type::Kind::Custom &&
          !reg_def->return_custom_name.empty()) {
        Type ret{Type::Kind::Custom, {}, std::string(reg_def->return_custom_name)};
        _expr_type_cache[&expr] = ret;
        return ret;
      }
      if (reg_def->return_kind != Type::Kind::Unknown) {
        Type ret{reg_def->return_kind};
        _expr_type_cache[&expr] = ret;
        return ret;
      }
    }
    // ──────────────────────────────────────────────────────────────────────

    if (auto* var_expr = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
      auto* symbol = resolve_symbol(var_expr->name);
      if (!symbol) {
        error(var_expr->name, "Undefined function '" + func_name + "'.");
        return make_error_type();
      }
      if (symbol->kind != SymbolKind::Function) {
        error(var_expr->name, "'" + func_name + "' is not a function.");
        return make_error_type();
      }
      // Tier supervision: only block higher-tier functions calling lower-tier ones.
      // Lower-tier calling higher-tier is a valid tier boundary crossing — Axion
      // records the transition at runtime but the language allows it.
      if (!_function_tier_stack.empty() && _function_tier_stack.back().has_value() &&
          symbol->tier.has_value() && *_function_tier_stack.back() > *symbol->tier) {
        std::ostringstream msg;
        msg << "Function tier @" << *_function_tier_stack.back() << " cannot call '" << func_name
            << "' declared at tier @" << *symbol->tier << ".";
        error(var_expr->name, msg.str());
      }
      // @pure enforcement: only block effect surfaces (print, I/O, AXSET, etc.).
      // User-defined functions not annotated @pure may still be called from @pure
      // functions; Axion validates purity at runtime via trace inspection.

      if (symbol->param_types.size() != arg_types.size()) {
        error(var_expr->name, "Function '" + func_name + "' expects " +
                                  std::to_string(symbol->param_types.size()) +
                                  " arguments but got " + std::to_string(arg_types.size()) + ".");
        return symbol->type;
      }

      if (symbol->generic_params.empty()) {
        for (size_t i = 0; i < arg_types.size(); ++i) {
          if (!is_assignable(symbol->param_types[i], arg_types[i])) {
            error(var_expr->name, "Argument " + std::to_string(i) + " for function '" + func_name +
                                      "' expects '" + type_to_string(symbol->param_types[i]) +
                                      "' but got '" + type_to_string(arg_types[i]) + "'.");
          }
        }
        return symbol->type;
      }

      std::unordered_map<std::string, Type> generic_bindings;
      generic_bindings.reserve(symbol->generic_params.size());

      std::function<bool(const Type&, const Type&)> bind_generic = [&](const Type& pattern,
                                                                       const Type& actual) -> bool {
        if (pattern.kind == Type::Kind::Custom) {
          auto generic_it = std::find(symbol->generic_params.begin(), symbol->generic_params.end(),
                                      pattern.custom_name);
          if (generic_it != symbol->generic_params.end()) {
            auto [it, inserted] = generic_bindings.emplace(pattern.custom_name, actual);
            if (!inserted) {
              return it->second == actual;
            }
            return true;
          }
        }

        if (pattern.kind != actual.kind) {
          return is_assignable(pattern, actual);
        }

        if (pattern.kind == Type::Kind::Custom || pattern.kind == Type::Kind::Constant) {
          return pattern.custom_name == actual.custom_name;
        }

        if (pattern.params.size() != actual.params.size()) {
          return is_assignable(pattern, actual);
        }

        for (size_t i = 0; i < pattern.params.size(); ++i) {
          if (!bind_generic(pattern.params[i], actual.params[i])) {
            return false;
          }
        }
        return true;
      };

      std::function<Type(const Type&)> instantiate_generic = [&](const Type& in) -> Type {
        if (in.kind == Type::Kind::Custom) {
          auto it = generic_bindings.find(in.custom_name);
          if (it != generic_bindings.end()) {
            return it->second;
          }
          return in;
        }
        Type out = in;
        for (auto& param : out.params) {
          param = instantiate_generic(param);
        }
        return out;
      };

      for (size_t i = 0; i < arg_types.size(); ++i) {
        if (!bind_generic(symbol->param_types[i], arg_types[i])) {
          const Type instantiated_return = instantiate_generic(symbol->type);
          std::vector<Type> instantiated_params;
          instantiated_params.reserve(symbol->param_types.size());
          for (const auto& param : symbol->param_types) {
            instantiated_params.push_back(instantiate_generic(param));
          }
          const std::string instantiated_sig =
              render_function_signature(func_name, instantiated_params, instantiated_return);
          error(var_expr->name, "Argument " + std::to_string(i) + " for function '" + func_name +
                                    "' expects '" + type_to_string(symbol->param_types[i]) +
                                    "' but got '" + type_to_string(arg_types[i]) +
                                    "'. Instantiated signature: " + instantiated_sig + ".");
        }
      }

      if (emit_unbound_generic_error(var_expr->name, func_name, symbol->generic_params,
                                     generic_bindings)) {
        return make_error_type();
      }

      Type instantiated_return = instantiate_generic(symbol->type);
      return instantiated_return;
    }
  }

  if (auto* type_callee = dynamic_cast<const SimpleTypeExpr*>(expr.callee.get())) {
    const std::string callee_name(type_callee->name.lexeme);
    const Type* expected = current_expected_type();
    if (callee_name == "T81Bytes") {
      if (arg_types.size() != 1) {
        error(type_callee->name, "T81Bytes conversion expects exactly one argument.");
        return make_error_type();
      }
      if (arg_types[0].kind != Type::Kind::String && arg_types[0].kind != Type::Kind::Bytes) {
        error(type_callee->name, "T81Bytes conversion expects a T81String or T81Bytes argument.");
        return make_error_type();
      }
      return Type{Type::Kind::Bytes};
    }
    if (callee_name == "T81Uint" || callee_name == "T81Qutrit") {
      if (arg_types.size() != 1) {
        error(type_callee->name, callee_name + " conversion expects exactly one argument.");
        return make_error_type();
      }

      const Type& argument_type = arg_types[0];
      if (!is_integer_type(argument_type)) {
        error(type_callee->name, callee_name + " conversion expects an integer argument, got '" +
                                     type_to_string(argument_type) + "'.");
        return make_error_type();
      }

      const Type target_type = type_from_token(type_callee->name);
      validate_constrained_integer_assignment(target_type, *expr.arguments[0], type_callee->name);
      return target_type;
    }
    if (callee_name == "T81Maybe") {
      if (!arg_types.empty()) {
        error(type_callee->name, "T81Maybe constructor expects no arguments.");
        return make_error_type();
      }
      if (expected && expected->kind == Type::Kind::Option) {
        return *expected;
      }
      return Type{Type::Kind::Option, {Type{Type::Kind::Unknown}}};
    }
    if (callee_name == "T81Promise") {
      if (!arg_types.empty()) {
        error(type_callee->name, "T81Promise constructor expects no arguments.");
        return make_error_type();
      }
      if (expected && expected->kind == Type::Kind::Custom &&
          expected->custom_name == "T81Promise") {
        return *expected;
      }
      return Type{Type::Kind::Custom, {}, "T81Promise"};
    }
    if (callee_name == "T81Agent" || callee_name == "T81Polynomial" ||
        callee_name == "T81Symbolic" || callee_name == "T81Time" || callee_name == "T81Entropy" ||
        callee_name == "T81Quaternion" || callee_name == "T81Prob" || callee_name == "Cell") {
      if (!arg_types.empty()) {
        error(type_callee->name, callee_name + " constructor expects no arguments.");
        return make_error_type();
      }
      return type_from_token(type_callee->name);
    }
  }

  if (auto* generic_callee = dynamic_cast<const GenericTypeExpr*>(expr.callee.get())) {
    if (auto* symbol = resolve_symbol(generic_callee->name);
        symbol && symbol->kind == SymbolKind::Function) {
      const std::string func_name(generic_callee->name.lexeme);

      if (symbol->param_types.size() != arg_types.size()) {
        error(generic_callee->name,
              "Function '" + func_name + "' expects " + std::to_string(symbol->param_types.size()) +
                  " arguments but got " + std::to_string(arg_types.size()) + ".");
        return symbol->type;
      }

      if (symbol->generic_params.empty()) {
        error(generic_callee->name, "Function '" + func_name +
                                        "' is not generic and does not accept explicit type "
                                        "arguments.");
        return make_error_type();
      }

      std::vector<Type> explicit_type_args;
      explicit_type_args.reserve(generic_callee->param_count);
      for (size_t i = 0; i < generic_callee->param_count; ++i) {
        const Expr* raw = generic_callee->params[i].get();
        if (!raw) {
          error(generic_callee->name,
                "Explicit function type argument " + std::to_string(i) + " is missing.");
          return make_error_type();
        }
        auto* type_arg = dynamic_cast<const TypeExpr*>(raw);
        if (!type_arg) {
          error(generic_callee->name, "Explicit function type arguments must be types.");
          return make_error_type();
        }
        explicit_type_args.push_back(analyze_type_expr(*type_arg));
      }

      if (explicit_type_args.size() > symbol->generic_params.size()) {
        error(generic_callee->name, "Function '" + func_name + "' expects " +
                                        std::to_string(symbol->generic_params.size()) +
                                        " explicit type arguments at most but got " +
                                        std::to_string(explicit_type_args.size()) + ".");
        return make_error_type();
      }

      std::unordered_map<std::string, Type> generic_bindings;
      generic_bindings.reserve(symbol->generic_params.size());
      for (size_t i = 0; i < explicit_type_args.size(); ++i) {
        generic_bindings.emplace(symbol->generic_params[i], explicit_type_args[i]);
      }

      std::function<bool(const Type&, const Type&)> bind_generic = [&](const Type& pattern,
                                                                       const Type& actual) -> bool {
        if (pattern.kind == Type::Kind::Custom) {
          auto generic_it = std::find(symbol->generic_params.begin(), symbol->generic_params.end(),
                                      pattern.custom_name);
          if (generic_it != symbol->generic_params.end()) {
            auto [it, inserted] = generic_bindings.emplace(pattern.custom_name, actual);
            if (!inserted) {
              return it->second == actual;
            }
            return true;
          }
        }

        if (pattern.kind != actual.kind) {
          return is_assignable(pattern, actual);
        }

        if (pattern.kind == Type::Kind::Custom || pattern.kind == Type::Kind::Constant) {
          return pattern.custom_name == actual.custom_name;
        }

        if (pattern.params.size() != actual.params.size()) {
          return is_assignable(pattern, actual);
        }

        for (size_t i = 0; i < pattern.params.size(); ++i) {
          if (!bind_generic(pattern.params[i], actual.params[i])) {
            return false;
          }
        }
        return true;
      };

      std::function<Type(const Type&)> instantiate_generic = [&](const Type& in) -> Type {
        if (in.kind == Type::Kind::Custom) {
          auto it = generic_bindings.find(in.custom_name);
          if (it != generic_bindings.end()) {
            return it->second;
          }
          return in;
        }
        Type out = in;
        for (auto& param : out.params) {
          param = instantiate_generic(param);
        }
        return out;
      };

      for (size_t i = 0; i < arg_types.size(); ++i) {
        if (!bind_generic(symbol->param_types[i], arg_types[i])) {
          Type expected = instantiate_generic(symbol->param_types[i]);
          std::vector<Type> instantiated_params;
          instantiated_params.reserve(symbol->param_types.size());
          for (const auto& param : symbol->param_types) {
            instantiated_params.push_back(instantiate_generic(param));
          }
          const Type instantiated_return = instantiate_generic(symbol->type);
          const std::string instantiated_sig =
              render_function_signature(func_name, instantiated_params, instantiated_return);
          error(generic_callee->name, "Argument " + std::to_string(i) + " for function '" +
                                          func_name + "' expects '" + type_to_string(expected) +
                                          "' but got '" + type_to_string(arg_types[i]) +
                                          "'. Instantiated signature: " + instantiated_sig + ".");
        }
      }

      if (emit_unbound_generic_error(generic_callee->name, func_name, symbol->generic_params,
                                     generic_bindings)) {
        return make_error_type();
      }

      Type instantiated_return = instantiate_generic(symbol->type);
      return instantiated_return;
    }

    Type constructed_type = evaluate_expression(*generic_callee);
    if (constructed_type.kind == Type::Kind::Option &&
        std::string(generic_callee->name.lexeme) == "T81Maybe") {
      if (!arg_types.empty()) {
        error(generic_callee->name, "T81Maybe constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Custom &&
        constructed_type.custom_name == "T81Promise") {
      if (!arg_types.empty()) {
        error(generic_callee->name, "T81Promise constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Fixed) {
      if (arg_types.size() != 1) {
        error(generic_callee->name, "T81Fixed constructor expects exactly one argument.");
        return make_error_type();
      }
      if (!is_numeric(arg_types[0])) {
        error(generic_callee->name, "T81Fixed constructor argument must be numeric, got '" +
                                        type_to_string(arg_types[0]) + "'.");
        return make_error_type();
      }
      return constructed_type;
    }

    if (constructed_type.kind == Type::Kind::Complex) {
      if (arg_types.size() != 2) {
        error(generic_callee->name, "T81Complex constructor expects exactly two arguments.");
        return make_error_type();
      }
      if (!is_numeric(arg_types[0]) || !is_numeric(arg_types[1])) {
        error(generic_callee->name, "T81Complex constructor arguments must be numeric, got '" +
                                        type_to_string(arg_types[0]) + "' and '" +
                                        type_to_string(arg_types[1]) + "'.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Quaternion) {
      if (arg_types.size() != 0) {  // For simplicity, assume no-arg or require 4 args? Let's check
                                    // what's easiest. We can allow 0 args.
        error(generic_callee->name, "T81Quaternion constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Prob) {
      if (arg_types.size() != 0) {
        error(generic_callee->name, "T81Prob constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Cell) {
      if (arg_types.size() != 0) {
        error(generic_callee->name, "Cell constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
    if (constructed_type.kind == Type::Kind::Map || constructed_type.kind == Type::Kind::Set ||
        constructed_type.kind == Type::Kind::List || constructed_type.kind == Type::Kind::Tree) {
      if (!arg_types.empty()) {
        error(generic_callee->name, "Collection constructor expects no arguments.");
        return make_error_type();
      }
      return constructed_type;
    }
  }

  // Handle Enum.Variant(payload) constructor calls
  if (auto* field_expr = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
    Type object_type = evaluate_expression(*field_expr->object);
    if (object_type.kind == Type::Kind::Custom && !object_type.custom_name.empty()) {
      auto enum_it = _enum_definitions.find(object_type.custom_name);
      if (enum_it != _enum_definitions.end()) {
        std::string variant_name(field_expr->field.lexeme);
        auto variant_it = enum_it->second.variants.find(variant_name);
        if (variant_it != enum_it->second.variants.end()) {
          if (!variant_it->second.payload) {
            error(field_expr->field,
                  "Variant '" + variant_name + "' is not a constructor function.");
            return make_error_type();
          }

          if (arg_types.size() != 1) {
            error(field_expr->field,
                  "Enum constructor '" + variant_name + "' expects exactly one argument.");
            return make_error_type();
          }

          if (!is_assignable(*variant_it->second.payload, arg_types[0])) {
            error(field_expr->field, "Argument mismatch for enum constructor '" + variant_name +
                                         "': expected '" +
                                         type_to_string(*variant_it->second.payload) +
                                         "' but got '" + type_to_string(arg_types[0]) + "'.");
          }
          return object_type;
        }
      }
    }
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
    std::size_t id = 0;
    int enum_id = -1;
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
    Type payload =
        scrutinee_type.params.empty() ? Type{Type::Kind::Unknown} : scrutinee_type.params[0];
    allowed_variants.emplace("Some", VariantMeta{payload, 0});
    allowed_variants.emplace("None", VariantMeta{std::nullopt, 1});
    required_variants = {"Some", "None"};
  } else if (is_result) {
    match_label = "Result";
    Type success =
        scrutinee_type.params.size() >= 1 ? scrutinee_type.params[0] : Type{Type::Kind::Unknown};
    Type error =
        scrutinee_type.params.size() >= 2 ? scrutinee_type.params[1] : Type{Type::Kind::Unknown};
    allowed_variants.emplace("Ok", VariantMeta{success, 0});
    allowed_variants.emplace("Err", VariantMeta{error, 1});
    required_variants = {"Ok", "Err"};
  } else if (is_enum) {
    match_label = "Enum";
    auto enum_it = _enum_definitions.find(scrutinee_type.custom_name);
    if (enum_it == _enum_definitions.end()) {
      error(scrutinee_token, "Type '" + scrutinee_type.custom_name + "' is not a known enum.");
      return make_error_type();
    }
    const EnumInfo& info = enum_it->second;
    for (size_t idx = 0; idx < info.variant_order.size(); ++idx) {
      const auto& name = info.variant_order[idx];
      auto variant_it = info.variants.find(name);
      std::optional<Type> payload;
      if (variant_it != info.variants.end()) {
        payload = variant_it->second.payload;
      }
      allowed_variants.emplace(name, VariantMeta{payload, idx, info.id});
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
  std::unordered_set<std::string> variants_with_no_guard;
  std::vector<MatchMetadata::ArmInfo> arm_infos;

  for (const auto& arm : expr.arms) {
    std::string name{arm.keyword.lexeme};
    auto variant_it = allowed_variants.find(name);
    if (variant_it == allowed_variants.end()) {
      error(arm.keyword,
            "Variant '" + name + "' is not part of '" + type_to_string(scrutinee_type) + "'.");
      structural_error = true;
      continue;
    }
    if (name == "Some") saw_some = true;
    if (name == "None") saw_none = true;
    if (name == "Ok") saw_ok = true;
    if (name == "Err") saw_err = true;
    bool has_guard = arm.guard != nullptr;
    if (!has_guard) {
      if (!variants_with_no_guard.insert(name).second) {
        error(arm.keyword, "Duplicate match arm for '" + name + "' without a guard.");
        structural_error = true;
      }
    }
    seen_variants.insert(name);

    bool variant_has_payload = variant_it->second.payload.has_value();
    Type payload_type =
        variant_has_payload ? *variant_it->second.payload : Type{Type::Kind::Unknown};
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

    MatchMetadata::ArmInfo arm_info;
    arm_info.variant = name;
    arm_info.pattern_kind = pattern_kind;
    arm_info.variant_id = static_cast<int>(variant_it->second.id);
    arm_info.enum_id = variant_it->second.enum_id;
    arm_info.enum_name = type_to_string(scrutinee_type);
    if (variant_has_payload) {
      arm_info.payload_type = payload_type;
    }
    arm_info.has_guard = arm.guard != nullptr;

    if (arm.guard) {
      Token guard_token = extract_token(*arm.guard);
      expect_condition_bool(*arm.guard, guard_token);
      arm_info.guard_expression = expr_to_string(*arm.guard);
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
      // Try to coerce/widen for monadic ergonomics
      if (is_numeric(result_type) && is_numeric(arm_type)) {
        if (numeric_rank(arm_type) > numeric_rank(result_type)) {
          result_type = arm_type;
        }
      } else if (result_type.kind == Type::Kind::Result && arm_type.kind == Type::Kind::Result) {
        // Unify Result arms
        Type unified = result_type;
        bool changed = false;
        for (size_t i = 0; i < 2; ++i) {
          Type t1 =
              (i < result_type.params.size()) ? result_type.params[i] : Type{Type::Kind::Unknown};
          Type t2 = (i < arm_type.params.size()) ? arm_type.params[i] : Type{Type::Kind::Unknown};
          if (t1.kind == Type::Kind::Unknown && t2.kind != Type::Kind::Unknown) {
            unified.params[i] = t2;
            changed = true;
          } else if (is_numeric(t1) && is_numeric(t2) && numeric_rank(t2) > numeric_rank(t1)) {
            unified.params[i] = t2;
            changed = true;
          }
        }
        if (changed)
          result_type = unified;
        else if (!is_assignable(result_type, arm_type)) {
          error(arm.keyword, "All match arms must produce the same type: expected '" +
                                 type_to_string(result_type) + "' but got '" +
                                 type_to_string(arm_type) + "' for arm '" + name + "'.");
          structural_error = true;
        }
      } else {
        error(arm.keyword, "All match arms must produce the same type: expected '" +
                               type_to_string(result_type) + "' but got '" +
                               type_to_string(arm_type) + "' for arm '" + name + "'.");
        structural_error = true;
      }
    }

    // arm_info already configured above
    arm_info.arm_type = arm_type;
    arm_infos.push_back(std::move(arm_info));
  }

  MatchMetadata meta;
  meta.expr = &expr;
  meta.result_type = result_type;
  meta.kind = is_option   ? MatchMetadata::Kind::Option
              : is_result ? MatchMetadata::Kind::Result
                          : MatchMetadata::Kind::Enum;
  meta.has_none = saw_none;
  meta.has_some = saw_some;
  meta.has_ok = saw_ok;
  meta.has_err = saw_err;
  meta.guard_present =
      std::any_of(arm_infos.begin(), arm_infos.end(),
                  [](const MatchMetadata::ArmInfo& info) { return info.has_guard; });
  meta.arms = std::move(arm_infos);
  _match_index[&expr] = _match_metadata.size();
  _match_metadata.push_back(std::move(meta));

  auto describe_missing_arm = [&](const std::string& missing) {
    std::ostringstream oss;
    oss << match_label << " match on '" << type_to_string(scrutinee_type) << "' requires '"
        << missing << "' arm";
    return oss.str();
  };

  for (const auto& required : required_variants) {
    if (seen_variants.find(required) == seen_variants.end()) {
      error(scrutinee_token, describe_missing_arm(required) + ".");
      structural_error = true;
      continue;
    }
    if (variants_with_no_guard.find(required) == variants_with_no_guard.end()) {
      error(scrutinee_token, describe_missing_arm(required) + " without a guard.");
      structural_error = true;
    }
  }

  if (structural_error) {
    return make_error_type();
  }

  return result_type;
}

std::any SemanticAnalyzer::visit(const IndexExpr& expr) {
  Type obj_type = evaluate_expression(*expr.object);
  Type index_type = evaluate_expression(*expr.index);

  if (obj_type.kind == Type::Kind::Error || index_type.kind == Type::Kind::Error) {
    return make_error_type();
  }

  if (!is_integer_type(index_type)) {
    error(expr.bracket, "Index expression '" + expr_to_string(*expr.index) + "' for target '" +
                            expr_to_string(*expr.object) + "' must be an integer type, got '" +
                            type_to_string(index_type) + "'.");
    return make_error_type();
  }

  if (obj_type.kind == Type::Kind::Vector || obj_type.kind == Type::Kind::Tensor) {
    if (obj_type.params.empty()) {
      return Type{Type::Kind::Unknown};
    }
    if (obj_type.params.size() <= 2) {
      return obj_type.params[0];
    } else {
      Type result = obj_type;
      result.params.erase(result.params.begin() + 1);
      return result;
    }
  }

  // Matrix[T][i] → Vector[T] (a row)
  if (obj_type.kind == Type::Kind::Matrix) {
    Type row_type{Type::Kind::Vector};
    if (!obj_type.params.empty()) row_type.params.push_back(obj_type.params[0]);
    return row_type;
  }

  // Map[K,V][K] → V (map lookup; IR emits MapGet + OptionUnwrap)
  if (obj_type.kind == Type::Kind::Map) {
    if (obj_type.params.size() >= 2) return obj_type.params[1];
    return Type{Type::Kind::Unknown};
  }

  error(expr.bracket, "Expression '" + expr_to_string(*expr.object) + "' of type '" +
                          type_to_string(obj_type) + "' does not support indexing.");
  return make_error_type();
}

std::any SemanticAnalyzer::visit(const FieldAccessExpr& expr) {
  Type object_type = evaluate_expression(*expr.object);
  if (object_type.kind == Type::Kind::Error) {
    return make_error_type();
  }
  if (object_type.kind != Type::Kind::Custom || object_type.custom_name.empty()) {
    error(expr.field,
          "Field access requires a record value, found '" + type_to_string(object_type) + "'.");
    return make_error_type();
  }

  // Built-in AxionFault type: structural type with a 'reason' field
  if (object_type.kind == Type::Kind::Custom && object_type.custom_name == "AxionFault") {
    std::string field_name(expr.field.lexeme);
    if (field_name == "reason") {
      return Type{Type::Kind::String};
    }
    error(expr.field, "AxionFault has no field '" + field_name + "'.");
    return make_error_type();
  }

  auto record_it = _record_definitions.find(object_type.custom_name);
  if (record_it == _record_definitions.end()) {
    // Check if it's an Enum for variant access (e.g. Enum.Variant)
    auto enum_it = _enum_definitions.find(object_type.custom_name);
    if (enum_it != _enum_definitions.end()) {
      std::string field_name(expr.field.lexeme);
      auto variant_it = enum_it->second.variants.find(field_name);
      if (variant_it != enum_it->second.variants.end()) {
        if (variant_it->second.payload) {
          // Accessing constructor as a value - invalid in T81 as functions aren't first-class.
          // Note: If this is part of a CallExpr (e.g. Enum.Variant(arg)), CallExpr visitor
          // will handle it separately. If we are here, it means it's being used as a value.
          error(expr.field, "Cannot use enum constructor '" + field_name + "' as a value.");
          return make_error_type();
        }
        // Constant variant access (e.g. Enum.Variant) -> evaluates to Enum type
        return object_type;
      }
      error(expr.field,
            "Enum '" + object_type.custom_name + "' has no variant '" + field_name + "'.");
      return make_error_type();
    }

    error(expr.field, "Type '" + object_type.custom_name + "' has no record fields.");
    return make_error_type();
  }

  std::string field_name(expr.field.lexeme);
  auto field_it = record_it->second.field_map.find(field_name);
  if (field_it == record_it->second.field_map.end()) {
    error(expr.field,
          "Record '" + object_type.custom_name + "' has no field '" + field_name + "'.");
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
      error(field.first,
            "Field '" + field_name + "' is provided more than once in '" + type_name + "'.");
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
        error(expr.type_name,
              "Record literal for '" + type_name + "' is missing field '" + field_info.name + "'.");
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

  if (variant_it->second.payload.has_value()) {
    if (!expr.payload) {
      error(expr.variant,
            "Variant '" + variant_name + "' of enum '" + enum_name + "' requires a payload.");
      return make_error_type();
    }
    Type expected_type = *variant_it->second.payload;
    Type actual_type = evaluate_expression(*expr.payload, &expected_type);
    if (!is_assignable(expected_type, actual_type)) {
      error(expr.variant, "Enum payload for '" + variant_name + "' must be '" +
                              type_to_string(expected_type) + "', but got '" +
                              type_to_string(actual_type) + "'.");
      return make_error_type();
    }
  } else if (expr.payload) {
    Token location = extract_token(*expr.payload);
    error(location,
          "Variant '" + variant_name + "' of enum '" + enum_name + "' does not accept a payload.");
    return make_error_type();
  }

  Type result{Type::Kind::Custom};
  result.custom_name = enum_name;
  return result;
}

std::any SemanticAnalyzer::visit(const VectorLiteralExpr& expr) {
  const Type* expected = current_expected_type();

  if (expr.elements.empty()) {
    if (expected &&
        (expected->kind == Type::Kind::Vector || expected->kind == Type::Kind::Tensor)) {
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

  // Matrix[T] context: treat each element as a row (Vector[T])
  if (expected && expected->kind == Type::Kind::Matrix) {
    Type row_type{Type::Kind::Vector};
    if (!expected->params.empty()) row_type.params.push_back(expected->params[0]);
    for (const auto& element : expr.elements) {
      evaluate_expression(*element, &row_type);
    }
    return *expected;  // Matrix[T]
  }

  Type expected_element;
  bool has_expected_element = false;
  if (expected && (expected->kind == Type::Kind::Vector || expected->kind == Type::Kind::Tensor)) {
    if (!expected->params.empty()) {
      expected_element = expected->params[0];
      has_expected_element = true;
    }
  }

  for (const auto& element : expr.elements) {
    Type elem_type =
        evaluate_expression(*element, has_expected_element ? &expected_element : nullptr);
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

    // Relax check for non-numeric vectors (like Vector[String])
    // if (!is_numeric(element_type)) {
    //    error(expr.token, "Vector literal elements must be numeric.");
    //    return make_error_type();
    // }

    auto* literal = dynamic_cast<const LiteralExpr*>(element.get());
    if (literal) {
      auto parsed = parse_numeric_literal_value(literal->value);
      if (parsed.has_value()) {
        values.push_back(*parsed);
      }
    }
  }

  if (expr.repeat_count) {
    Type count_type = evaluate_expression(*expr.repeat_count);
    if (!is_integer_type(count_type)) {
      error(expr.token, "Vector repeat count must be an integer.");
      return make_error_type();
    }
  }

  Type result{Type::Kind::Vector};
  result.params.push_back(element_type.kind == Type::Kind::Unknown ? Type{Type::Kind::Unknown}
                                                                   : element_type);
  merge_expected_params(result, current_expected_type());

  bool all_literals = (values.size() == expr.elements.size());
  if (expr.repeat_count) all_literals = false;

  if (all_literals) {
    _vector_literal_data[&expr] = std::move(values);
  }
  return result;
}

std::any SemanticAnalyzer::visit(const SetLiteralExpr& expr) {
  const Type* expected = current_expected_type();

  if (expr.elements.empty()) {
    if (expected &&
        (expected->kind == Type::Kind::Set || expected->kind == Type::Kind::Tensor)) {
      Type result;
      if (expected->kind == Type::Kind::Set) {
        result = *expected;
      } else {
        result.kind = Type::Kind::Set;
        if (!expected->params.empty()) {
          result.params.push_back(expected->params[0]);
        } else {
          result.params.push_back(Type{Type::Kind::Unknown});
        }
      }
      _set_literal_data[&expr] = std::vector<float>{};
      return result;
    }
    // Empty set literal {} - infer as Set[T] where T is unknown
    Type result{Type::Kind::Set};
    if (expected && !expected->params.empty()) {
      result.params.push_back(expected->params[0]);
    } else {
      result.params.push_back(Type{Type::Kind::Unknown});
    }
    _set_literal_data[&expr] = std::vector<float>{};
    return result;
  }

  // Non-empty set literal: infer element type from elements
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
        error(expr.token, "Set literal elements must share a compatible type.");
        return make_error_type();
      }
    }

    // Collect numeric data like VectorLiteralExpr does
    auto* literal = dynamic_cast<const LiteralExpr*>(element.get());
    if (literal) {
      auto parsed = parse_numeric_literal_value(literal->value);
      if (parsed.has_value()) {
        values.push_back(*parsed);
      }
    }
  }

  Type result_type{Type::Kind::Set};
  result_type.params.push_back(element_type.kind == Type::Kind::Unknown ? Type{Type::Kind::Unknown}
                                                                   : element_type);
  merge_expected_params(result_type, current_expected_type());

  _set_literal_data[&expr] = std::move(values);
  return result_type;
}

std::any SemanticAnalyzer::visit(const MapLiteralExpr& expr) {
  const Type* expected = current_expected_type();

  if (expr.entries.empty()) {
    if (expected &&
        (expected->kind == Type::Kind::Map || expected->kind == Type::Kind::Tensor)) {
      Type result;
      if (expected->kind == Type::Kind::Map) {
        result = *expected;
      } else {
        result.kind = Type::Kind::Map;
        if (!expected->params.empty()) {
          result.params.push_back(expected->params[0]);
          result.params.push_back(expected->params[1]);
        } else {
          result.params.push_back(Type{Type::Kind::Unknown});
          result.params.push_back(Type{Type::Kind::Unknown});
        }
      }
      _map_literal_data[&expr] = std::vector<float>{};
      return result;
    }
    // Empty map literal {} - infer as Map[K,V] where K,V are unknown
    Type result{Type::Kind::Map};
    if (expected && expected->params.size() >= 2) {
      result.params.push_back(expected->params[0]);
      result.params.push_back(expected->params[1]);
    } else {
      result.params.push_back(Type{Type::Kind::Unknown});
      result.params.push_back(Type{Type::Kind::Unknown});
    }
    _map_literal_data[&expr] = std::vector<float>{};
    return result;
  }

  // Non-empty map literal: infer key and value types from entries
  Type key_type{Type::Kind::Unknown};
  Type value_type{Type::Kind::Unknown};
  std::vector<float> values;
  values.reserve(expr.entries.size() * 2); // Store key-value pairs
  
  for (const auto& [key, value] : expr.entries) {
    Type key_elem_type = evaluate_expression(*key);
    Type value_elem_type = evaluate_expression(*value);
    if (key_elem_type.kind == Type::Kind::Error || value_elem_type.kind == Type::Kind::Error) {
      return make_error_type();
    }

    if (key_type.kind == Type::Kind::Unknown) {
      key_type = key_elem_type;
    } else if (key_type != key_elem_type) {
      if (is_numeric(key_type) && is_numeric(key_elem_type)) {
        auto merged = deduce_numeric_type(key_type, key_elem_type, expr.token);
        if (!merged.has_value()) {
          return make_error_type();
        }
        key_type = *merged;
      } else {
        error(expr.token, "Map literal keys must share a compatible type.");
        return make_error_type();
      }
    }

    if (value_type.kind == Type::Kind::Unknown) {
      value_type = value_elem_type;
    } else if (value_type != value_elem_type) {
      if (is_numeric(value_type) && is_numeric(value_elem_type)) {
        auto merged = deduce_numeric_type(value_type, value_elem_type, expr.token);
        if (!merged.has_value()) {
          return make_error_type();
        }
        value_type = *merged;
      } else {
        error(expr.token, "Map literal values must share a compatible type.");
        return make_error_type();
      }
    }

    // Collect numeric data for keys and values
    auto* key_literal = dynamic_cast<const LiteralExpr*>(key.get());
    if (key_literal) {
      auto parsed = parse_numeric_literal_value(key_literal->value);
      if (parsed.has_value()) {
        values.push_back(*parsed);
      }
    }
    
    auto* value_literal = dynamic_cast<const LiteralExpr*>(value.get());
    if (value_literal) {
      auto parsed = parse_numeric_literal_value(value_literal->value);
      if (parsed.has_value()) {
        values.push_back(*parsed);
      }
    }
  }

  Type result_type{Type::Kind::Map};
  result_type.params.push_back(key_type.kind == Type::Kind::Unknown ? Type{Type::Kind::Unknown}
                                                                   : key_type);
  result_type.params.push_back(value_type.kind == Type::Kind::Unknown ? Type{Type::Kind::Unknown}
                                                                     : value_type);
  merge_expected_params(result_type, current_expected_type());

  _map_literal_data[&expr] = std::move(values);
  return result_type;
}

std::any SemanticAnalyzer::visit(const GroupingExpr& expr) {
  return evaluate_expression(*expr.expression);
}

std::any SemanticAnalyzer::visit(const LiteralExpr& expr) {
  switch (expr.value.type) {
    case TokenType::True:
    case TokenType::False:
      return Type{Type::Kind::Bool};
    case TokenType::Integer: {
      const Type* expected = current_expected_type();
      if (expected && expected->kind == Type::Kind::BigInt) {
        return Type{Type::Kind::BigInt};
      }
      return Type{Type::Kind::I32};
    }
    case TokenType::Base81Integer:
      // Validate `t81` integer literal and provide helpful error messages.
      try {
        auto bigint = t81::frontend::numeric_literals::parse_t81_bigint_literal(expr.value.lexeme);
        (void)bigint;
        return Type{Type::Kind::BigInt};
      } catch (const std::invalid_argument& e) {
        error(expr.value, "Invalid t81 integer literal: " + std::string(e.what()) + ".");
        return make_error_type();
      } catch (const std::out_of_range& e) {
        error(expr.value, "t81 integer literal '" + std::string(expr.value.lexeme) +
                              "' exceeds supported range: " + std::string(e.what()) + ".");
        return make_error_type();
      } catch (...) {
        error(expr.value, "Invalid t81 integer literal '" + std::string(expr.value.lexeme) + "'.");
        return make_error_type();
      }
    case TokenType::Float:
    case TokenType::Base81Float:
      return Type{Type::Kind::Float};
    case TokenType::String:
      return Type{Type::Kind::String};
    case TokenType::ByteString:
      return Type{Type::Kind::Bytes};
    case TokenType::Ternary: {
      // Trit literal: e.g. "1t", "0t", "-1t" — value must be in [-1, 0, 1]
      std::string text{expr.value.lexeme};
      // Strip trailing 't'
      if (!text.empty() && text.back() == 't') {
        text.pop_back();
      }
      try {
        std::int64_t v = std::stoll(text);
        if (v < -1 || v > 1) {
          error(expr.value, "Trit literal must be in [-1, 0, 1].");
          return make_error_type();
        }
      } catch (...) {
        error(expr.value, "Invalid trit literal.");
        return make_error_type();
      }
      return Type{Type::Kind::Qutrit};
    }
    case TokenType::T81Fixed: {
      // Fixed-point literal: e.g. "1.25fx" - value with 'fx' suffix
      return Type{Type::Kind::Fixed};
    }
    default:
      return Type{Type::Kind::Unknown};
  }
}

std::any SemanticAnalyzer::visit(const SymbolLiteralExpr& /*expr*/) {
  return Type{Type::Kind::Symbol};
}

std::any SemanticAnalyzer::visit(const InfiniteLiteralExpr& expr) {
  if (expr.seed) {
    evaluate_expression(*expr.seed);
  }
  return Type{Type::Kind::InfiniteCanonicalForm};
}

std::any SemanticAnalyzer::visit(const InferExpr& expr) {
  // RFC-0015 §3.2 — `infer AgentName(args)` is sugar for `AgentName.infer(args)`.
  // Detect the pattern: expression is a CallExpr whose callee is a VariableExpr
  // naming a declared agent that has an "infer" behavior.
  if (auto* call = dynamic_cast<const CallExpr*>(expr.expression.get())) {
    if (auto* var = dynamic_cast<const VariableExpr*>(call->callee.get())) {
      const std::string agent_name(var->name.lexeme);
      auto ait = _agent_definitions.find(agent_name);
      if (ait != _agent_definitions.end()) {
        // Validate the agent has an "infer" behavior.
        auto bit = ait->second.behavior_map.find("infer");
        if (bit == ait->second.behavior_map.end()) {
          error(var->name,
                "Agent '" + agent_name +
                    "' does not have an 'infer' behavior; "
                    "the 'infer' keyword requires an 'infer' behavior.");
          return make_error_type();
        }
        // Type-check arguments.
        const auto& beh = ait->second.behaviors[bit->second];
        if (call->arguments.size() != beh.param_types.size()) {
          error(call->paren,
                "Agent '" + agent_name + ".infer' expects " +
                    std::to_string(beh.param_types.size()) + " argument(s), got " +
                    std::to_string(call->arguments.size()) + ".");
          return make_error_type();
        }
        for (std::size_t i = 0; i < call->arguments.size(); ++i) {
          evaluate_expression(*call->arguments[i]);
        }
        Type ret = beh.return_type;
        expr.resolved_type = ret;
        return ret;
      }
    }
  }
  // Non-agent infer expression: evaluate inner expression and assume Tensor.
  evaluate_expression(*expr.expression);
  return Type{Type::Kind::Tensor};
}

std::any SemanticAnalyzer::visit(const UnaryExpr& expr) {
  Type right = evaluate_expression(*expr.right);
  if (expr.op.type == TokenType::Bang) {
    // T81Qutrit supports trit-NOT: !p flips +1↔-1, leaves 0 unchanged.
    if (right.kind == Type::Kind::Qutrit) {
      return Type{Type::Kind::Qutrit};
    }
    if (!is_assignable(Type{Type::Kind::Bool}, right)) {
      error(expr.op,
            "Logical not requires a boolean operand, got '" + type_to_string(right) + "'.");
      return make_error_type();
    }
    return Type{Type::Kind::Bool};
  }

  if (expr.op.type == TokenType::Minus) {
    if (!is_numeric(right)) {
      error(expr.op, "Unary minus requires a numeric operand.");
      return make_error_type();
    }
    if (right.kind == Type::Kind::Uint) {
      error(expr.op, "Unary minus is not allowed for T81Uint; use a signed type.");
      return make_error_type();
    }
    return right;
  }

  if (expr.op.type == TokenType::Tilde) {
    if (!is_integer_type(right)) {
      error(expr.op, "Bitwise NOT requires an integer operand.");
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
  _expr_type_cache[&expr] = symbol->type;
  return symbol->type;
}

std::any SemanticAnalyzer::visit(const SimpleTypeExpr& expr) {
  Type t = type_from_token(expr.name);
  if (t.kind == Type::Kind::Custom) {
    auto it = _type_aliases.find(t.custom_name);
    if (it != _type_aliases.end() && it->second.alias) {
      // If alias has NO parameters, we can resolve it directly.
      // If it has parameters, SimpleTypeExpr is missing them -> error or partial?
      // TypeDecl stores params in alias info.
      if (it->second.params.empty()) {
        return analyze_type_expr(*it->second.alias);
      }
    }
  }
  return t;
}

std::any SemanticAnalyzer::visit(const BlockExpr& expr) {
  enter_scope();
  for (const auto& statement : expr.statements) {
    analyze(*statement);
  }

  Type result{Type::Kind::Void};
  if (expr.final_expr) {
    // Pass expected type through to the final expression if available
    result = evaluate_expression(*expr.final_expr, current_expected_type());
  }

  exit_scope();
  return result;
}

std::any SemanticAnalyzer::visit(const IfExpr& expr) {
  Token cond_token = extract_token(*expr.condition);
  expect_condition_bool(*expr.condition, cond_token);

  const Type* expected = current_expected_type();
  Type then_type = evaluate_expression(*expr.then_branch, expected);
  Type else_type{Type::Kind::Void};

  if (expr.else_branch) {
    // If we have an expected type, use it for the else branch too.
    // Otherwise, use the then-branch type as the expectation/hint for the else branch.
    const Type* else_expected = expected ? expected : &then_type;
    else_type = evaluate_expression(*expr.else_branch, else_expected);
  } else {
    // If no else branch, the expression must evaluate to Void
    if (then_type.kind != Type::Kind::Void && then_type.kind != Type::Kind::Unknown &&
        then_type.kind != Type::Kind::Error) {
      error(cond_token, "'if' expression without 'else' must evaluate to Void, but found '" +
                            type_to_string(then_type) + "'.");
    }
    return Type{Type::Kind::Void};
  }

  if (then_type.kind == Type::Kind::Error || else_type.kind == Type::Kind::Error) {
    return make_error_type();
  }

  if (then_type.kind == Type::Kind::Unknown && else_type.kind == Type::Kind::Unknown) {
    return Type{Type::Kind::Unknown};
  }

  // Check compatibility
  if (is_assignable(then_type, else_type)) {
    return then_type;
  }
  if (is_assignable(else_type, then_type)) {
    return else_type;
  }

  // Try to find a common numeric type if applicable
  if (is_numeric(then_type) && is_numeric(else_type)) {
    if (numeric_rank(then_type) >= numeric_rank(else_type)) return then_type;
    return else_type;
  }

  error(cond_token, "'if' branches have incompatible types: '" + type_to_string(then_type) +
                        "' and '" + type_to_string(else_type) + "'.");
  return make_error_type();
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

    const Expr& raw = *expr.params[i];
    if (auto* type_expr = dynamic_cast<const TypeExpr*>(&raw)) {
      params.push_back(analyze_type_expr(*type_expr));
      continue;
    }

    if (auto type = type_from_expr(&raw)) {
      params.push_back(*type);
      continue;
    }

    if (i == 0) {
      // Allow expression if it's T81Fixed, T81Complex, etc.
      if (type_name == "T81Fixed" || type_name == "T81Complex" || type_name == "T81Matrix") {
        auto constant = constant_type_from_expr(raw);
        if (constant.has_value()) {
          params.push_back(*constant);
          continue;
        }
      }
      error(expr.name, "The first generic parameter must be a type.");
      params.push_back(make_error_type());
      continue;
    }

    auto constant = constant_type_from_expr(raw);
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
    if (type_name != "T81Fixed" && type_name != "T81Complex" && type_name != "T81Matrix") {
      error(expr.name, "The first generic parameter must be a type.");
      return make_error_type();
    }
  }

  if (type_name == "Option") {
    if (params.size() != 1) {
      error(expr.name, "The 'Option' type expects exactly one type parameter, but got " +
                           std::to_string(params.size()) + ".");
    }
    Type result{Type::Kind::Option, {params[0]}};
    merge_expected_params(result, expected);
    return result;
  }

  if (type_name == "T81Maybe") {
    if (params.size() != 1) {
      error(expr.name, "The 'T81Maybe' type expects exactly one type parameter, but got " +
                           std::to_string(params.size()) + ".");
    }
    Type result{Type::Kind::Option, {params.empty() ? Type{Type::Kind::Unknown} : params[0]}};
    merge_expected_params(result, expected);
    return result;
  }

  if (type_name == "T81Promise") {
    if (params.size() != 1) {
      error(expr.name, "The 'T81Promise' type expects exactly one type parameter, but got " +
                           std::to_string(params.size()) + ".");
    }
    Type result{Type::Kind::Custom, params, "T81Promise"};
    merge_expected_params(result, expected);
    return result;
  }

  if (type_name == "Result") {
    if (params.size() != 2) {
      error(expr.name, "The 'Result' type expects exactly two type parameters, but got " +
                           std::to_string(params.size()) + ".");
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
    if (literal->value.type == TokenType::Integer ||
        literal->value.type == TokenType::Base81Integer) {
      return Type::constant(std::string(literal->value.lexeme));
    }
  }
  if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) {
    return Type::constant(std::string(variable->name.lexeme));
  }
  return std::nullopt;
}

bool SemanticAnalyzer::bind_pattern_payload(const MatchPattern& pattern, const Type& payload_type,
                                            const Token& keyword) {
  switch (pattern.kind) {
    case MatchPattern::Kind::Identifier:
      if (!pattern.binding_is_wildcard) {
        return bind_pattern_symbol(pattern.identifier, payload_type);
      }
      return true;
    case MatchPattern::Kind::Tuple: {
      size_t expected_fields = pattern.tuple_bindings.size();
      if (payload_type.params.empty()) {
        error(keyword, "Tuple pattern for variant '" + std::string(keyword.lexeme) +
                           "' lacks payload type information.");
        return false;
      }
      if (payload_type.params.size() != expected_fields) {
        error(keyword, "Tuple pattern for variant '" + std::string(keyword.lexeme) + "' expects " +
                           std::to_string(expected_fields) + " fields but payload has " +
                           std::to_string(payload_type.params.size()) + ".");
        return false;
      }
      bool ok = true;
      for (size_t i = 0; i < expected_fields; ++i) {
        ok = bind_pattern_symbol(pattern.tuple_bindings[i], payload_type.params[i]) && ok;
      }
      return ok;
    }
    case MatchPattern::Kind::Record: {
      if (payload_type.kind != Type::Kind::Custom || payload_type.custom_name.empty()) {
        error(keyword, "Record pattern for variant '" + std::string(keyword.lexeme) +
                           "' requires a record payload.");
        return false;
      }
      auto record_it = _record_definitions.find(payload_type.custom_name);
      if (record_it == _record_definitions.end()) {
        error(keyword, "Variant '" + std::string(keyword.lexeme) + "' payload '" +
                           payload_type.custom_name + "' is not a known record.");
        return false;
      }
      const auto& info = record_it->second;
      bool ok = true;
      for (const auto& binding : pattern.record_bindings) {
        std::string field_name(binding.first.lexeme);
        auto field_it = info.field_map.find(field_name);
        if (field_it == info.field_map.end()) {
          error(binding.first,
                "Record '" + payload_type.custom_name + "' has no field '" + field_name + "'.");
          ok = false;
          continue;
        }
        ok = bind_pattern_symbol(binding.second, field_it->second) && ok;
      }
      return ok;
    }
    default:
      error(keyword, "Unsupported pattern kind for variant payload.");
      return false;
  }
}

bool SemanticAnalyzer::analyze_nested_variant(const MatchPattern& pattern,
                                              const Type& payload_type) {
  if (payload_type.kind != Type::Kind::Custom || payload_type.custom_name.empty()) {
    error(pattern.variant_name,
          "Variant '" + std::string(pattern.variant_name.lexeme) + "' requires an enum payload.");
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
    error(pattern.variant_name,
          "Variant '" + variant_name + "' is not part of '" + payload_type.custom_name + "'.");
    return false;
  }
  if (!pattern.variant_payload) {
    if (variant_it->second.payload.has_value()) {
      error(pattern.variant_name, "Variant '" + variant_name + "' requires a binding.");
      return false;
    }
    return true;
  }
  if (!variant_it->second.payload.has_value()) {
    error(pattern.variant_name, "Variant '" + variant_name + "' does not accept a binding.");
    return false;
  }
  return bind_pattern_payload(*pattern.variant_payload, *variant_it->second.payload,
                              pattern.variant_name);
}

bool SemanticAnalyzer::bind_pattern_symbol(const Token& name, const Type& type) {
  if (std::string_view{name.lexeme} == "_") {
    return true;
  }
  if (is_defined_in_current_scope(std::string(name.lexeme))) {
    error(name, "Pattern binding '" + std::string(name.lexeme) +
                    "' is already defined in this match arm.");
    return false;
  }
  define_symbol(name, SymbolKind::Variable, false);
  if (auto* symbol = resolve_symbol(name)) {
    symbol->type = type;
  }
  return true;
}

bool SemanticAnalyzer::is_mutable_lvalue(const Expr& expr) {
  if (auto* var = dynamic_cast<const VariableExpr*>(&expr)) {
    auto* symbol = resolve_symbol(var->name);
    return symbol && symbol->is_mutable;
  }
  if (auto* index = dynamic_cast<const IndexExpr*>(&expr)) {
    return is_mutable_lvalue(*index->object);
  }
  if (auto* field = dynamic_cast<const FieldAccessExpr*>(&expr)) {
    return is_mutable_lvalue(*field->object);
  }
  return false;
}

}  // namespace frontend
}  // namespace t81
