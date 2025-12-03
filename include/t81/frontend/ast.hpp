#ifndef T81_FRONTEND_AST_HPP
#define T81_FRONTEND_AST_HPP

#include "t81/frontend/lexer.hpp"
#include <any>
#include <memory>
#include <vector>
#include <array>
#include <utility>
#include <optional>
#include <cstdint>

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
struct TypeExpr;      // Base class for type expressions
struct SimpleTypeExpr; // For simple types like "T81Int"
struct GenericTypeExpr; // For generic types like "Vector[T]"
struct ExpressionStmt;
struct VarStmt;
struct LetStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;
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
    virtual std::any visit(const SimpleTypeExpr& expr) = 0;
    virtual std::any visit(const GenericTypeExpr& expr) =
 0;
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
    virtual std::any visit(const LoopStmt& stmt) = 0;
    virtual std::any visit(const ReturnStmt& stmt) = 0;
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
    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(op), right(std::move(right)) {}

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
    VectorLiteralExpr(Token token, std::vector<std::unique_ptr<Expr>> elements)
        : token(token), elements(std::move(elements)) {}

    std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

    const Token token;
    const std::vector<std::unique_ptr<Expr>> elements;
};

struct GroupingExpr : Expr {
    GroupingExpr(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

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

struct MatchArm {
    enum class Variant {
        Some,
        None,
        Ok,
        Err
    };

    MatchArm(Variant variant, Token keyword, bool has_binding,
             Token binding, bool binding_is_wildcard, std::unique_ptr<Expr> expression)
        : variant(variant),
          keyword(keyword),
          has_binding(has_binding),
          binding(binding),
          binding_is_wildcard(binding_is_wildcard),
          expression(std::move(expression)) {}

    MatchArm(const MatchArm&) = delete;
    MatchArm& operator=(const MatchArm&) = delete;
    MatchArm(MatchArm&&) = default;
    MatchArm& operator=(MatchArm&&) = default;

    Variant variant;
    Token keyword;
    bool has_binding;
    Token binding;
    bool binding_is_wildcard;
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
    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}

    std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }

    const Token name;
    const std::unique_ptr<Expr> value;
};

// --- Statement Nodes ---

struct ExpressionStmt : Stmt {
    ExpressionStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

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
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const std::vector<std::unique_ptr<Stmt>> statements;
};

struct IfStmt : Stmt {
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
        : condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

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

struct ReturnStmt : Stmt {
    ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
        : keyword(keyword), value(std::move(value)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const Token keyword;
    const std::unique_ptr<Expr> value;
};

struct Parameter {
    Token name;
    std::unique_ptr<TypeExpr> type;
};

struct FunctionStmt : Stmt {
    FunctionStmt(Token name, std::vector<Parameter> params, std::unique_ptr<TypeExpr> return_type, std::vector<std::unique_ptr<Stmt>> body)
        : name(name), params(std::move(params)), return_type(std::move(return_type)), body(std::move(body)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const Token name;
    const std::vector<Parameter> params;
    const std::unique_ptr<TypeExpr> return_type;
    const std::vector<std::unique_ptr<Stmt>> body;
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

    RecordDecl(Token name, std::vector<Field> fields)
        : name(name), fields(std::move(fields)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const Token name;
    const std::vector<Field> fields;
};

struct EnumDecl : Stmt {
    struct Variant {
        Token name;
        std::unique_ptr<TypeExpr> payload;
    };

    EnumDecl(Token name, std::vector<Variant> variants)
        : name(name), variants(std::move(variants)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const Token name;
    const std::vector<Variant> variants;
};

struct LoopStmt : Stmt {
    enum class BoundKind {
        None,
        Infinite,
        Static
    };

    LoopStmt(Token keyword, BoundKind bound_kind, std::optional<std::int64_t> bound_value,
             std::vector<std::unique_ptr<Stmt>> body)
        : keyword(keyword),
          bound_kind(bound_kind),
          bound_value(bound_value),
          body(std::move(body)) {}

    std::any accept(StmtVisitor& visitor) const override { return visitor.visit(*this); }

    const Token keyword;
    const BoundKind bound_kind;
    const std::optional<std::int64_t> bound_value;
    const std::vector<std::unique_ptr<Stmt>> body;
};


} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_AST_HPP
