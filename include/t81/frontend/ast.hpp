#ifndef T81_FRONTEND_AST_HPP
#define T81_FRONTEND_AST_HPP

#include <any>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include "t81/frontend/lexer.hpp"

namespace t81 {
namespace frontend {

// Forward declarations for visitors and all node types
class ExprVisitor;
class StmtVisitor;

struct Expr;
struct Stmt;
struct BinaryExpr;
struct UnaryExpr;
struct LiteralExpr;
struct GroupingExpr;
struct VariableExpr;
struct CallExpr;
struct AssignExpr;
struct MatchExpr;
struct VectorLiteralExpr;
struct FieldAccessExpr;
struct RecordLiteralExpr;
struct EnumLiteralExpr;
struct SymbolLiteralExpr;
struct InfiniteLiteralExpr;
struct IndexExpr;
struct BlockExpr;
struct IfExpr;
struct TypeExpr;         // Base class for type expressions
struct SimpleTypeExpr;   // For simple types like "T81Int"
struct GenericTypeExpr;  // For generic types like "Vector[T]"
struct InferExpr;
struct ExpressionStmt;
struct VarStmt;
struct LetStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ReflectStmt;
struct RecurseStmt;
struct DistributedStmt;
struct InfiniteStmt;
struct TrainStmt;
struct ReturnStmt;
struct BreakStmt;
struct ContinueStmt;
struct FunctionStmt;
struct LoopStmt;
struct TypeDecl;
struct RecordDecl;
struct EnumDecl;

// --- Base Classes ---

struct Expr {
  virtual ~Expr() = default;
  virtual std::any accept(ExprVisitor& visitor) const = 0;
};

struct Stmt {
  virtual ~Stmt() = default;
  virtual std::any accept(StmtVisitor& visitor) const = 0;
};

// --- Visitor Interfaces ---

class ExprVisitor {
public:
  virtual ~ExprVisitor() = default;
  virtual std::any visit(const BinaryExpr& expr) = 0;
  virtual std::any visit(const UnaryExpr& expr) = 0;
  virtual std::any visit(const LiteralExpr& expr) = 0;
  virtual std::any visit(const GroupingExpr& expr) = 0;
  virtual std::any visit(const VariableExpr& expr) = 0;
  virtual std::any visit(const CallExpr& expr) = 0;
  virtual std::any visit(const AssignExpr& expr) = 0;
  virtual std::any visit(const MatchExpr& expr) = 0;
  virtual std::any visit(const VectorLiteralExpr& expr) = 0;
  virtual std::any visit(const FieldAccessExpr& expr) = 0;
  virtual std::any visit(const RecordLiteralExpr& expr) = 0;
  virtual std::any visit(const EnumLiteralExpr& expr) = 0;
  virtual std::any visit(const SymbolLiteralExpr& expr) = 0;
  virtual std::any visit(const InfiniteLiteralExpr& expr) = 0;
  virtual std::any visit(const IndexExpr& expr) = 0;
  virtual std::any visit(const BlockExpr& expr) = 0;
  virtual std::any visit(const IfExpr& expr) = 0;
  virtual std::any visit(const SimpleTypeExpr& expr) = 0;
  virtual std::any visit(const GenericTypeExpr& expr) = 0;
  virtual std::any visit(const InferExpr& expr) = 0;
};

class StmtVisitor {
public:
  virtual ~StmtVisitor() = default;
  virtual std::any visit(const ExpressionStmt& stmt) = 0;
  virtual std::any visit(const VarStmt& stmt) = 0;
  virtual std::any visit(const LetStmt& stmt) = 0;
  virtual std::any visit(const BlockStmt& stmt) = 0;
  virtual std::any visit(const IfStmt& stmt) = 0;
  virtual std::any visit(const WhileStmt& stmt) = 0;
  virtual std::any visit(const ForStmt& stmt) = 0;
  virtual std::any visit(const ReflectStmt& stmt) = 0;
  virtual std::any visit(const RecurseStmt& stmt) = 0;
  virtual std::any visit(const DistributedStmt& stmt) = 0;
  virtual std::any visit(const InfiniteStmt& stmt) = 0;
  virtual std::any visit(const TrainStmt& stmt) = 0;
  virtual std::any visit(const LoopStmt& stmt) = 0;
  virtual std::any visit(const ReturnStmt& stmt) = 0;
  virtual std::any visit(const BreakStmt& stmt) = 0;
  virtual std::any visit(const ContinueStmt& stmt) = 0;
  virtual std::any visit(const FunctionStmt& stmt) = 0;
  virtual std::any visit(const TypeDecl& stmt) = 0;
  virtual std::any visit(const RecordDecl& stmt) = 0;
  virtual std::any visit(const EnumDecl& stmt) = 0;
};

// --- Expression Nodes ---

struct BinaryExpr : Expr {
  BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
      : left(std::move(left)), op(op), right(std::move(right)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> left;
  const Token op;
  const std::unique_ptr<Expr> right;
};

struct UnaryExpr : Expr {
  UnaryExpr(Token op, std::unique_ptr<Expr> right) : op(op), right(std::move(right)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token op;
  const std::unique_ptr<Expr> right;
};

struct LiteralExpr : Expr {
  LiteralExpr(Token value) : value(value) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token value;
};

struct VectorLiteralExpr : Expr {
  VectorLiteralExpr(Token token, std::vector<std::unique_ptr<Expr>> elements,
                    std::unique_ptr<Expr> repeat_count = nullptr)
      : token(token), elements(std::move(elements)), repeat_count(std::move(repeat_count)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token token;
  const std::vector<std::unique_ptr<Expr>> elements;
  const std::unique_ptr<Expr> repeat_count;
};

struct GroupingExpr : Expr {
  GroupingExpr(std::unique_ptr<Expr> expression) : expression(std::move(expression)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> expression;
};

struct VariableExpr : Expr {
  VariableExpr(Token name) : name(name) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
};

struct CallExpr : Expr {
  CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
      : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> callee;
  const Token paren;
  const std::vector<std::unique_ptr<Expr>> arguments;
};

struct FieldAccessExpr : Expr {
  FieldAccessExpr(std::unique_ptr<Expr> object, Token field)
      : object(std::move(object)), field(field) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> object;
  const Token field;
};

struct RecordLiteralExpr : Expr {
  RecordLiteralExpr(Token type_name, std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields)
      : type_name(type_name), fields(std::move(fields)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token type_name;
  const std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;
};

struct EnumLiteralExpr : Expr {
  EnumLiteralExpr(Token enum_name, Token variant, std::unique_ptr<Expr> payload)
      : enum_name(enum_name), variant(variant), payload(std::move(payload)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token enum_name;
  const Token variant;
  const std::unique_ptr<Expr> payload;
};

struct SymbolLiteralExpr : Expr {
  SymbolLiteralExpr(Token value) : value(value) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token value;
};

struct InfiniteLiteralExpr : Expr {
  InfiniteLiteralExpr(Token token, std::unique_ptr<Expr> seed)
      : token(token), seed(std::move(seed)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token token;
  const std::unique_ptr<Expr> seed;
};

struct IndexExpr : Expr {
  IndexExpr(std::unique_ptr<Expr> object, std::unique_ptr<Expr> index, Token bracket)
      : object(std::move(object)), index(std::move(index)), bracket(bracket) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> object;
  const std::unique_ptr<Expr> index;
  const Token bracket;
};

struct MatchPattern {
  enum class Kind { None, Identifier, Tuple, Record, Variant };

  MatchPattern() = default;
  MatchPattern(Token identifier, bool wildcard)
      : kind(Kind::Identifier), identifier(identifier), binding_is_wildcard(wildcard) {}

  Kind kind = Kind::None;
  Token identifier{};
  bool binding_is_wildcard = false;
  std::vector<Token> tuple_bindings;
  std::vector<std::pair<Token, Token>> record_bindings;
  Token variant_name{};
  std::unique_ptr<MatchPattern> variant_payload;
};

struct MatchArm {
  MatchArm(Token keyword, MatchPattern pattern, std::unique_ptr<Expr> guard,
           std::unique_ptr<Expr> expression)
      : keyword(keyword),
        pattern(std::move(pattern)),
        guard(std::move(guard)),
        expression(std::move(expression)) {}

  MatchArm(const MatchArm&) = delete;
  MatchArm& operator=(const MatchArm&) = delete;
  MatchArm(MatchArm&&) = default;
  MatchArm& operator=(MatchArm&&) = default;

  Token keyword;
  MatchPattern pattern;
  std::unique_ptr<Expr> guard;
  std::unique_ptr<Expr> expression;
};

struct MatchExpr : Expr {
  MatchExpr(std::unique_ptr<Expr> scrutinee, std::vector<MatchArm> arms)
      : scrutinee(std::move(scrutinee)), arms(std::move(arms)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> scrutinee;
  const std::vector<MatchArm> arms;
};

struct AssignExpr : Expr {
  AssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value)
      : target(std::move(target)), value(std::move(value)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> target;
  const std::unique_ptr<Expr> value;
};

struct BlockExpr : Expr {
  BlockExpr(std::vector<std::unique_ptr<Stmt>> statements, std::unique_ptr<Expr> final_expr)
      : statements(std::move(statements)), final_expr(std::move(final_expr)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::vector<std::unique_ptr<Stmt>> statements;
  const std::unique_ptr<Expr> final_expr;
};

struct IfExpr : Expr {
  IfExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> then_branch,
         std::unique_ptr<Expr> else_branch)
      : condition(std::move(condition)),
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> condition;
  const std::unique_ptr<Expr> then_branch;
  const std::unique_ptr<Expr> else_branch;
};

struct InferExpr : Expr {
  InferExpr(Token keyword, std::unique_ptr<Expr> expression)
      : keyword(keyword), expression(std::move(expression)) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::unique_ptr<Expr> expression;
};

// --- Statement Nodes ---

struct ExpressionStmt : Stmt {
  ExpressionStmt(std::unique_ptr<Expr> expression) : expression(std::move(expression)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> expression;
};

// --- Type Expression Nodes ---

// Base class for all type expressions.
struct TypeExpr : Expr {
  // The accept method will be implemented by subclasses.
  virtual std::any accept(ExprVisitor& visitor) const = 0;
};

// Represents a simple, non-generic type like `T81Int`.
struct SimpleTypeExpr : TypeExpr {
  SimpleTypeExpr(Token name) : name(name) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
};

// Represents a generic type instantiation, e.g., `Vector[T]`.
struct GenericTypeExpr : TypeExpr {
  GenericTypeExpr(Token name, std::array<std::unique_ptr<Expr>, 8> params, size_t param_count)
      : name(name), params(std::move(params)), param_count(param_count) {}

  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::array<std::unique_ptr<Expr>, 8> params;
  const size_t param_count;
};

struct VarStmt : Stmt {
  VarStmt(Token name, std::unique_ptr<TypeExpr> type, std::unique_ptr<Expr> initializer)
      : name(name), type(std::move(type)), initializer(std::move(initializer)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::unique_ptr<TypeExpr> type;
  const std::unique_ptr<Expr> initializer;
};

struct LetStmt : Stmt {
  LetStmt(Token name, std::unique_ptr<TypeExpr> type, std::unique_ptr<Expr> initializer)
      : name(name), type(std::move(type)), initializer(std::move(initializer)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::unique_ptr<TypeExpr> type;
  const std::unique_ptr<Expr> initializer;
};

struct BlockStmt : Stmt {
  BlockStmt(std::vector<std::unique_ptr<Stmt>> statements) : statements(std::move(statements)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const std::vector<std::unique_ptr<Stmt>> statements;
};

struct IfStmt : Stmt {
  IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch,
         std::unique_ptr<Stmt> else_branch)
      : condition(std::move(condition)),
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> condition;
  const std::unique_ptr<Stmt> then_branch;
  const std::unique_ptr<Stmt> else_branch;
};

struct WhileStmt : Stmt {
  WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
      : condition(std::move(condition)), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const std::unique_ptr<Expr> condition;
  const std::unique_ptr<Stmt> body;
};

struct LoopStmt : Stmt {
  enum class BoundKind { None, Infinite, Static, Guarded };

  LoopStmt(Token keyword, BoundKind bound_kind, std::optional<std::int64_t> bound_value,
           std::unique_ptr<Expr> guard_expression, std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword),
        bound_kind(bound_kind),
        bound_value(bound_value),
        guard_expression(std::move(guard_expression)),
        body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const BoundKind bound_kind;
  const std::optional<std::int64_t> bound_value;
  const std::unique_ptr<Expr> guard_expression;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct ForStmt : Stmt {
  ForStmt(Token iterator, std::unique_ptr<Expr> iterable, std::unique_ptr<Stmt> body,
          LoopStmt::BoundKind bound_kind = LoopStmt::BoundKind::None,
          std::optional<std::int64_t> bound_value = std::nullopt)
      : iterator(iterator),
        iterable(std::move(iterable)),
        body(std::move(body)),
        bound_kind(bound_kind),
        bound_value(bound_value) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token iterator;
  const std::unique_ptr<Expr> iterable;
  const std::unique_ptr<Stmt> body;
  const LoopStmt::BoundKind bound_kind;
  const std::optional<std::int64_t> bound_value;
};

struct ReflectStmt : Stmt {
  ReflectStmt(Token keyword, std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct DistributedStmt : Stmt {
  DistributedStmt(Token keyword, std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct InfiniteStmt : Stmt {
  InfiniteStmt(Token keyword, std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct TrainStmt : Stmt {
  TrainStmt(Token keyword, std::unique_ptr<Expr> model, std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword), model(std::move(model)), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::unique_ptr<Expr> model;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct ReturnStmt : Stmt {
  ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
      : keyword(keyword), value(std::move(value)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const std::unique_ptr<Expr> value;
};

struct BreakStmt : Stmt {
  BreakStmt(Token keyword) : keyword(keyword) {}
  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }
  const Token keyword;
};

struct ContinueStmt : Stmt {
  ContinueStmt(Token keyword) : keyword(keyword) {}
  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }
  const Token keyword;
};

struct Parameter {
  Token name;
  std::unique_ptr<TypeExpr> type;
};

struct RecurseStmt : Stmt {
  RecurseStmt(Token keyword, Token name, std::vector<Parameter> params,
              std::vector<std::unique_ptr<Stmt>> body)
      : keyword(keyword), name(name), params(std::move(params)), body(std::move(body)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token keyword;
  const Token name;
  const std::vector<Parameter> params;
  const std::vector<std::unique_ptr<Stmt>> body;
};

struct FunctionStmt : Stmt {
  FunctionStmt(Token name, std::vector<Token> generic_params, std::vector<Parameter> params,
               std::unique_ptr<TypeExpr> return_type, std::vector<std::unique_ptr<Stmt>> body,
               std::optional<std::int64_t> tier = std::nullopt, bool is_pure = false)
      : name(name),
        generic_params(std::move(generic_params)),
        params(std::move(params)),
        return_type(std::move(return_type)),
        body(std::move(body)),
        tier(tier),
        is_pure(is_pure) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::vector<Token> generic_params;
  const std::vector<Parameter> params;
  const std::unique_ptr<TypeExpr> return_type;
  const std::vector<std::unique_ptr<Stmt>> body;
  const std::optional<std::int64_t> tier;
  const bool is_pure{false};
};

struct TypeDecl : Stmt {
  TypeDecl(Token name, std::vector<Token> params, std::unique_ptr<TypeExpr> alias)
      : name(name), params(std::move(params)), alias(std::move(alias)) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::vector<Token> params;
  const std::unique_ptr<TypeExpr> alias;
};

struct RecordDecl : Stmt {
  struct Field {
    Token name;
    std::unique_ptr<TypeExpr> type;
  };

  RecordDecl(Token name, std::vector<Field> fields,
             std::optional<std::int64_t> schema_version = std::nullopt,
             std::optional<std::string> module_path = std::nullopt)
      : name(name),
        fields(std::move(fields)),
        schema_version(schema_version),
        module_path(module_path) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::vector<Field> fields;
  const std::optional<std::int64_t> schema_version;
  const std::optional<std::string> module_path;
};

struct EnumDecl : Stmt {
  struct Variant {
    Token name;
    std::unique_ptr<TypeExpr> payload;
  };

  EnumDecl(Token name, std::vector<Variant> variants,
           std::optional<std::int64_t> schema_version = std::nullopt,
           std::optional<std::string> module_path = std::nullopt)
      : name(name),
        variants(std::move(variants)),
        schema_version(schema_version),
        module_path(module_path) {}

  std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

  const Token name;
  const std::vector<Variant> variants;
  const std::optional<std::int64_t> schema_version;
  const std::optional<std::string> module_path;
};

}  // namespace frontend
}  // namespace t81

#endif  // T81_FRONTEND_AST_HPP
