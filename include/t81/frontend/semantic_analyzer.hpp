#pragma once

#include <any>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/types.hpp"

namespace t81 {
namespace frontend {

class IRGenerator;

// Type is now defined in types.hpp — included above.

// Simple symbol information for semantic analysis
enum class SymbolKind { Variable, Function };

struct SemanticSymbol {
  SymbolKind kind;
  Token declaration;                        // Token where the symbol was declared
  Type type;                                // Variable type or function return type
  std::vector<Type> param_types;            // Only used for functions
  std::vector<std::string> generic_params;  // Generic function parameter names
  std::optional<std::int64_t> tier;         // Optional @tier(n) intent on functions
  bool is_mutable = true;   // `let` bindings are immutable, `var` bindings are mutable
  bool is_defined = false;  // Functions get declared first, defined later
  bool is_pure = false;      // @pure annotation: function must have no observable side effects
  bool is_attention = false; // @attention — RFC-0026 AI-M6: call sites lower to ATTN opcode
  bool is_qmatmul = false;   // @qmatmul  — RFC-0026 AI-M6: call sites lower to QMATMUL opcode
};

struct Diagnostic {
  std::string file;
  int line = 0;
  int column = 0;
  std::string message;
};

struct RecordInfo {
  struct Field {
    std::string name;
    Type type;
    Token token;
  };

  std::vector<Field> fields;
  std::unordered_map<std::string, Type> field_map;
  std::uint32_t schema_version = 1;
  std::string module_path;
};

struct EnumVariantInfo {
  std::optional<Type> payload;
  int id = -1;
};

struct EnumInfo {
  std::unordered_map<std::string, EnumVariantInfo> variants;
  std::vector<std::string> variant_order;
  std::uint32_t schema_version = 1;
  std::string module_path;
  int id = -1;
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
  std::any visit(const ForStmt& stmt) override;
  std::any visit(const ReflectStmt& stmt) override;
  std::any visit(const LoopStmt& stmt) override;
  std::any visit(const ReturnStmt& stmt) override;
  std::any visit(const AssertStmt& stmt) override;
  std::any visit(const BreakStmt& stmt) override;
  std::any visit(const ContinueStmt& stmt) override;
  std::any visit(const RecurseStmt& stmt) override;
  std::any visit(const DistributedStmt& stmt) override;
  std::any visit(const InfiniteStmt& stmt) override;
  std::any visit(const TrainStmt& stmt) override;
  std::any visit(const FunctionStmt& stmt) override;
  std::any visit(const TypeDecl& stmt) override;
  std::any visit(const RecordDecl& stmt) override;
  std::any visit(const EnumDecl& stmt) override;

  // Visitor methods for expressions
  std::any visit(const FieldAccessExpr& expr) override;
  std::any visit(const RecordLiteralExpr& expr) override;
  std::any visit(const EnumLiteralExpr& expr) override;
  std::any visit(const SymbolLiteralExpr& expr) override;
  std::any visit(const InfiniteLiteralExpr& expr) override;
  std::any visit(const IndexExpr& expr) override;
  std::any visit(const AssignExpr& expr) override;
  std::any visit(const BinaryExpr& expr) override;
  std::any visit(const CallExpr& expr) override;
  std::any visit(const GroupingExpr& expr) override;
  std::any visit(const LiteralExpr& expr) override;
  std::any visit(const UnaryExpr& expr) override;
  std::any visit(const VariableExpr& expr) override;
  std::any visit(const MatchExpr& expr) override;
  std::any visit(const VectorLiteralExpr& expr) override;
  std::any visit(const SetLiteralExpr& expr) override;
  std::any visit(const MapLiteralExpr& expr) override;
  std::any visit(const BlockExpr& expr) override;
  std::any visit(const IfExpr& expr) override;
  std::any visit(const SimpleTypeExpr& expr) override;
  std::any visit(const GenericTypeExpr& expr) override;
  std::any visit(const InferExpr& expr) override;

  struct MatchMetadata {
    const MatchExpr* expr = nullptr;
    Type result_type;
    enum class Kind {
      Unknown,
      Option,
      Result,
      Enum,
    };
    Kind kind = Kind::Unknown;
    bool has_some = false;
    bool has_none = false;
    bool has_ok = false;
    bool has_err = false;

    struct ArmInfo {
      std::string variant;
      MatchPattern::Kind pattern_kind = MatchPattern::Kind::None;
      bool has_guard = false;
      Type payload_type;
      Type arm_type;
      int variant_id = -1;
      int enum_id = -1;
      std::string enum_name;
      std::string guard_expression;
    };
    std::vector<ArmInfo> arms;
    bool guard_present = false;
  };

  struct LoopMetadata {
    const LoopStmt* stmt = nullptr;
    Token keyword{};
    LoopStmt::BoundKind bound_kind = LoopStmt::BoundKind::None;
    std::optional<std::int64_t> bound_value;
    int depth = 0;
    int id = 0;
    std::string source_file;
    bool guard_present = false;

    bool annotated() const { return bound_kind != LoopStmt::BoundKind::None; }
    bool bound_infinite() const { return bound_kind == LoopStmt::BoundKind::Infinite; }
  };

  const std::vector<LoopMetadata>& loop_metadata() const { return _loop_metadata; }
  const LoopMetadata* loop_metadata_for(const LoopStmt& stmt) const;
  const MatchMetadata* match_metadata_for(const MatchExpr& expr) const;
  const std::vector<MatchMetadata>& match_metadata() const { return _match_metadata; }
  [[nodiscard]] std::string type_name(const Type& type) const { return type_to_string(type); }
  const std::unordered_map<std::string, EnumInfo>& enum_definitions() const {
    return _enum_definitions;
  }

private:
  const std::vector<std::unique_ptr<Stmt>>& _statements;
  bool _had_error = false;
  std::vector<Type> _function_return_stack;
  std::vector<std::optional<std::int64_t>> _function_tier_stack;
  bool _in_pure_function{false};  // true while analyzing the body of an @pure function
  std::vector<Diagnostic> _diagnostics;
  std::string _source_name;

  std::vector<LoopMetadata> _loop_metadata;
  std::unordered_map<const LoopStmt*, size_t> _loop_index;
  std::vector<const LoopStmt*> _loop_stack;
  int _next_loop_id = 0;
  std::vector<MatchMetadata> _match_metadata;
  std::unordered_map<const MatchExpr*, size_t> _match_index;
  int _next_enum_id = 0;

  // Scoped symbol table
  using Scope = std::unordered_map<std::string, SemanticSymbol>;
  std::vector<Scope> _scopes;
  std::vector<const Type*> _expected_type_stack;
  std::unordered_map<const Expr*, Type> _expr_type_cache;
  std::unordered_map<std::string, size_t> _generic_arities;
  std::unordered_set<std::string> _defined_generics;
  struct AliasInfo {
    std::vector<std::string> params;
    const TypeExpr* alias = nullptr;
  };
  std::unordered_map<std::string, AliasInfo> _type_aliases;
  std::unordered_map<const VectorLiteralExpr*, std::vector<float>> _vector_literal_data;
  std::unordered_map<const SetLiteralExpr*, std::vector<float>> _set_literal_data;
  std::unordered_map<const MapLiteralExpr*, std::vector<float>> _map_literal_data;
  std::unordered_map<std::string, RecordInfo> _record_definitions;
  std::unordered_map<std::string, EnumInfo> _enum_definitions;
  const std::unordered_map<std::string, Type>* _current_type_env = nullptr;

  void analyze(const Stmt& stmt);

public:
  std::any analyze(const Expr& expr);

private:
  void error(const Token& token, const std::string& message);
  void error_at(const Token& token, const std::string& message);

  // Symbol table operations
  void enter_scope();
  void exit_scope();
  void define_symbol(const Token& name, SymbolKind kind, bool is_mutable = true);
  SemanticSymbol* resolve_symbol(const Token& name);
  bool is_defined_in_current_scope(const std::string& name) const;

  // Type helpers
  Type make_error_type();
  Type type_from_token(const Token& name);
  Type analyze_type_expr(const TypeExpr& expr,
                         const std::unordered_map<std::string, Type>* env = nullptr);
  bool is_numeric(const Type& type) const;
  int numeric_rank(const Type& type) const;
  Type widen_numeric(const Type& left, const Type& right, const Token& op);
  bool is_assignable(const Type& target, const Type& value) const;
  std::string type_to_string(const Type& type) const;
  Type expect_condition_bool(const Expr& expr, const Token& location);
  Type evaluate_expression(const Expr& expr, const Type* expected = nullptr);
  const Type* current_expected_type() const;
  void register_function_signatures();
  Token extract_token(const Expr& expr) const;
  std::optional<Type> constant_type_from_expr(const Expr& expr);
  const Type* type_of(const Expr* expr) const;
  const std::unordered_map<std::string, AliasInfo>& type_aliases() const { return _type_aliases; }
  const std::unordered_map<std::string, RecordInfo>& record_definitions() const {
    return _record_definitions;
  }
  std::string type_expr_to_string(const TypeExpr& expr) const;
  std::string expr_to_string(const Expr& expr) const;
  bool validate_constrained_integer_assignment(const Type& target, const Expr& value,
                                               const Token& location);

  bool is_integer_type(const Type& type) const;
  bool is_float_type(const Type& type) const;
  bool is_fraction_type(const Type& type) const;
  bool is_primitive_numeric_type(const Type& type) const;
  std::optional<Type> deduce_numeric_type(const Type& left, const Type& right, const Token& op);
  Type refine_generic_type(const Type& declared, const Type& initializer) const;
  void merge_expected_params(Type& target, const Type* expected) const;
  void enforce_generic_arity(const Type& type, const Token& location);
  bool structural_params_assignable(const Type& target, const Type& value) const;
  Type instantiate_alias(const AliasInfo& alias, const std::vector<Type>& params,
                         const Token& location);
  const std::vector<float>* vector_literal_data(const VectorLiteralExpr* expr) const;
  const std::vector<float>* set_literal_data(const SetLiteralExpr* expr) const;
  const std::vector<float>* map_literal_data(const MapLiteralExpr* expr) const;
  bool bind_pattern_payload(const MatchPattern& pattern, const Type& payload_type,
                            const Token& keyword);
  bool analyze_nested_variant(const MatchPattern& pattern, const Type& payload_type);
  bool bind_pattern_symbol(const Token& name, const Type& type);
  bool is_mutable_lvalue(const Expr& expr);
};

}  // namespace frontend
}  // namespace t81
