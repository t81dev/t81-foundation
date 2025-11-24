#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "t81/lang/types.hpp"

namespace t81::lang {
struct LiteralValue {
  enum class Kind { Int, Float, Fraction, Symbol };
  Kind kind{Kind::Int};
  std::int64_t int_value{0};
  std::string text;
};

struct ExprLiteral {
  LiteralValue value;
};

struct ExprIdent {
  std::string name;
};

struct ExprCall {
  std::string callee;
  std::vector<struct Expr> args;
};

struct ExprBinary {
  enum class Op {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Land,
    Lor,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge
  } op{Op::Add};
  std::shared_ptr<struct Expr> lhs;
  std::shared_ptr<struct Expr> rhs;
};

struct ExprUnary {
  enum class Op { Neg, Not } op{Op::Neg};
  std::shared_ptr<struct Expr> expr;
};

struct MatchPattern {
  enum class Kind {
    OptionSome,
    OptionNone,
    ResultOk,
    ResultErr,
  } kind{Kind::OptionSome};
  std::optional<std::string> binding;
};

struct MatchArm {
  MatchPattern pattern;
  std::shared_ptr<struct Expr> expr;
};

struct ExprMatch {
  std::shared_ptr<struct Expr> value;
  std::vector<MatchArm> arms;
};

struct Expr {
  std::variant<ExprLiteral, ExprIdent, ExprCall, ExprBinary, ExprUnary, ExprMatch> node;
};

struct StatementReturn {
  Expr expr;
};

struct StatementLet {
  std::string name;
  std::optional<Type> declared_type;
  Expr expr;
};

struct StatementAssign {
  std::string name;
  Expr expr;
};

struct StatementIf {
  Expr condition;
  std::vector<struct Statement> then_body;
  std::vector<struct Statement> else_body;
};

struct StatementLoop {
  std::vector<struct Statement> body;
};

struct StatementExpr {
  Expr expr;
};

struct Statement {
  std::variant<StatementReturn, StatementLet, StatementAssign, StatementIf, StatementLoop, StatementExpr> node;
};

struct Parameter {
  std::string name;
  Type type{Type::primitive(Type::Kind::T81Int)};
};

struct Function {
  std::string name;
  Type return_type{Type::primitive(Type::Kind::T81Int)};
  std::vector<Parameter> params;
  std::vector<Statement> body;
};

struct Module {
  std::vector<Function> functions;
};
}  // namespace t81::lang
