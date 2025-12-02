#pragma once

#include <vector>
#include <memory>
#include <any>
#include <string>
#include <unordered_map>
#include <optional>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"

namespace t81 {
namespace frontend {

class IRGenerator;

// Simple type system representation for semantic analysis
struct Type {
    enum class Kind {
        Void,
        Bool,
        I2,
        I8,
        I16,
        I32,
        BigInt,
        Float,
        Fraction,
        Vector,
        Matrix,
        Tensor,
        Graph,
        Option,
        Result,
        String,
        Constant,
        Custom,
        Unknown,
        Error
    };

    Kind kind = Kind::Unknown;
    std::vector<Type> params;
    std::string custom_name;
    [[nodiscard]] static Type constant(std::string repr);

    [[nodiscard]] bool operator==(const Type& other) const;
    [[nodiscard]] bool operator!=(const Type& other) const { return !(*this == other); }
};

// Simple symbol information for semantic analysis
enum class SymbolKind {
    Variable,
    Function
};

struct SemanticSymbol {
    SymbolKind kind;
    Token declaration;  // Token where the symbol was declared
    Type type;                      // Variable type or function return type
    std::vector<Type> param_types;  // Only used for functions
    bool is_defined = false;        // Functions get declared first, defined later
};

struct Diagnostic {
    std::string file;
    int line = 0;
    int column = 0;
    std::string message;
};

class SemanticAnalyzer : public StmtVisitor, public ExprVisitor {
    friend class IRGenerator;
public:
    explicit SemanticAnalyzer(const std::vector<std::unique_ptr<Stmt>>& statements,
                              std::string source_name = {});
    void analyze();
    bool had_error() const { return _had_error; }
    const std::vector<Diagnostic>& diagnostics() const { return _diagnostics; }
    const std::string& source_name() const { return _source_name; }

    // Visitor methods for statements
    std::any visit(const ExpressionStmt& stmt) override;
    std::any visit(const VarStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const LoopStmt& stmt) override;
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
    std::any visit(const MatchExpr& expr) override;
    std::any visit(const SimpleTypeExpr& expr) override;
    std::any visit(const GenericTypeExpr& expr) override;

    struct LoopMetadata {
        const LoopStmt* stmt = nullptr;
        Token keyword{};
        LoopStmt::BoundKind bound_kind = LoopStmt::BoundKind::None;
        std::optional<std::int64_t> bound_value;
        int depth = 0;
        int id = 0;
        std::string source_file;

        bool annotated() const { return bound_kind != LoopStmt::BoundKind::None; }
        bool bound_infinite() const { return bound_kind == LoopStmt::BoundKind::Infinite; }
    };

    const std::vector<LoopMetadata>& loop_metadata() const { return _loop_metadata; }
    const LoopMetadata* loop_metadata_for(const LoopStmt& stmt) const;

private:
    const std::vector<std::unique_ptr<Stmt>>& _statements;
    bool _had_error = false;
    std::vector<Type> _function_return_stack;
    std::vector<Diagnostic> _diagnostics;
    std::string _source_name;

    std::vector<LoopMetadata> _loop_metadata;
    std::unordered_map<const LoopStmt*, size_t> _loop_index;
    std::vector<const LoopStmt*> _loop_stack;
    int _next_loop_id = 0;

    // Scoped symbol table
    using Scope = std::unordered_map<std::string, SemanticSymbol>;
    std::vector<Scope> _scopes;
    std::vector<const Type*> _expected_type_stack;
    std::unordered_map<const Expr*, Type> _expr_type_cache;

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

    // Type helpers
    Type make_error_type();
    Type type_from_token(const Token& name);
    Type analyze_type_expr(const TypeExpr& expr);
    bool is_numeric(const Type& type) const;
    int numeric_rank(const Type& type) const;
    Type widen_numeric(const Type& left, const Type& right, const Token& op);
    bool is_assignable(const Type& target, const Type& value);
    std::string type_to_string(const Type& type) const;
    Type expect_condition_bool(const Expr& expr, const Token& location);
    Type evaluate_expression(const Expr& expr, const Type* expected = nullptr);
    const Type* current_expected_type() const;
    void register_function_signatures();
    Token extract_token(const Expr& expr) const;
    std::optional<Type> constant_type_from_expr(const Expr& expr);
    const Type* type_of(const Expr* expr) const;

    bool is_integer_type(const Type& type) const;
    bool is_float_type(const Type& type) const;
    bool is_fraction_type(const Type& type) const;
    bool is_primitive_numeric_type(const Type& type) const;
    std::optional<Type> deduce_numeric_type(const Type& left, const Type& right, const Token& op);
    Type refine_generic_type(const Type& declared, const Type& initializer) const;
    void merge_expected_params(Type& target, const Type* expected) const;
};

} // namespace frontend
} // namespace t81
