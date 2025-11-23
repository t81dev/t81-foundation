#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "t81/lang/types.hpp"

namespace t81::lang {
struct ExprLiteral {
  std::int64_t value{0};
};

struct ExprIdent {
  std::string name;
};

struct ExprBinary {
  enum class Op { Add, Sub, Mul } op{Op::Add};
  std::shared_ptr<struct Expr> lhs;
  std::shared_ptr<struct Expr> rhs;
};

struct Expr {
  std::variant<ExprLiteral, ExprIdent, ExprBinary> node;
};

struct StatementReturn {
  Expr expr;
};

struct StatementLet {
  std::string name;
  Expr expr;
};

struct StatementIf {
  Expr condition;
  std::vector<struct Statement> then_body;
  std::vector<struct Statement> else_body;
};

struct Statement {
  std::variant<StatementReturn, StatementLet, StatementIf> node;
};

struct Function {
  std::string name;
  Type return_type{Type::I64};
  std::vector<Type> params;
  std::vector<Statement> body;
};

struct Module {
  std::vector<Function> functions;
};
}  // namespace t81::lang
