// include/t81/frontend/ir_generator.hpp
#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include <any>
#include <charconv>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <typeinfo>
#include <unordered_map>
#include "t81/enum_meta.hpp"
#include "t81/frontend/ast.hpp"
#include "t81/frontend/builtin_registry.hpp"
#include "t81/frontend/numeric_literals.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/isa/ir.hpp"
#include "t81/tensor.hpp"

namespace t81::frontend {

inline int hex_digit(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return 10 + (value - 'a');
  if (value >= 'A' && value <= 'F') return 10 + (value - 'A');
  return -1;
}

inline std::string decode_string_literal(const Token& token) {
  std::string_view view = token.lexeme;
  if (view.size() < 2 || view.front() != '"' || view.back() != '"') {
    return {};
  }
  std::string result;
  result.reserve(view.size() - 2);
  for (size_t i = 1; i + 1 < view.size(); ++i) {
    char c = view[i];
    if (c == '\\' && i + 1 < view.size() - 1) {
      ++i;
      char esc = view[i];
      switch (esc) {
        case '\\':
          result.push_back('\\');
          break;
        case '"':
          result.push_back('"');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'x': {
          if (i + 2 < view.size() - 1) {
            int hi = hex_digit(view[++i]);
            int lo = hex_digit(view[++i]);
            if (hi >= 0 && lo >= 0) {
              result.push_back(static_cast<char>((hi << 4) | lo));
            }
          }
          break;
        }
        default:
          result.push_back(esc);
          break;
      }
    } else {
      result.push_back(c);
    }
  }
  return result;
}

inline std::string escape_metadata_string(std::string_view input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

// Use std::from_chars for locale-independent, deterministic float parsing.
// Strips trailing 'f'/'F' suffix (C-style float literal suffix) before parsing.
inline double parse_canonical_float(std::string_view literal) {
  // Strip trailing 'f' or 'F' suffix (e.g. "1.0f" → "1.0")
  if (!literal.empty() && (literal.back() == 'f' || literal.back() == 'F')) {
    literal.remove_suffix(1);
  }
  double value = 0.0;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L && !defined(__APPLE__)
  auto res = std::from_chars(literal.data(), literal.data() + literal.size(), value);
  if (res.ec != std::errc()) {
    return 0.0;
  }
#else
  // Fallback for platforms without float std::from_chars (e.g. older libc++ on macOS)
  std::string s(literal);
  std::istringstream iss(s);
  iss.imbue(std::locale::classic());
  iss >> value;
  if (iss.fail()) return 0.0;
#endif
  return value;
}

inline int64_t parse_base81_integer_literal(std::string_view literal) {
  try {
    return numeric_literals::parse_t81_integer_literal(literal);
  } catch (const std::runtime_error& e) {
    throw std::runtime_error(e.what());
  } catch (const std::out_of_range&) {
    auto normalized = numeric_literals::normalize_decimal_integer_literal_text(literal, true);
    throw std::out_of_range("t81 integer literal '" + normalized.value_or(std::string(literal)) +
                            "' exceeds 64-bit range");
  }
}

// Parse an integer literal lexeme, stripping '_' separators and handling 0x hex prefix.
// Stops at unknown suffix characters (e.g. trit 't') so "1t" → 1.
inline int64_t parse_integer_literal_raw(std::string_view lexeme) {
  std::string s;
  s.reserve(lexeme.size());
  for (char c : lexeme) {
    if (c == '_') continue;  // skip digit separators
    s += c;
  }
  // base 0 auto-detects 0x prefix for hex literals; stoll stops at first invalid char.
  std::size_t idx = 0;
  return std::stoll(s, &idx, 0);
}

inline double parse_base81_float_literal(std::string_view literal) {
  std::string value = numeric_literals::strip_t81_suffix(literal);
  return parse_canonical_float(value);
}

inline std::optional<std::string> qualified_call_name(const Expr& expr) {
  if (const auto* var = dynamic_cast<const VariableExpr*>(&expr)) {
    return std::string(var->name.lexeme);
  }
  if (const auto* generic = dynamic_cast<const GenericTypeExpr*>(&expr)) {
    return std::string(generic->name.lexeme);
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

// ── Delegated to builtin_registry.hpp / kBuiltinTable ──────────────────────
// The old 377-line if-chain lived here.  It is now the single source of truth.
inline std::string canonical_stdlib_call_name(std::string_view name) {
  return std::string(t81::frontend::canonical_name_for(name));
}

class IRGenerator : public ExprVisitor, public StmtVisitor {
public:
  struct LoopInfo {
    int id = -1;
    tisc::ir::Label entry_label{};
    tisc::ir::Label exit_label{};
    int depth = 0;
    bool annotated = false;
  };

  tisc::ir::IntermediateProgram generate(const std::vector<std::unique_ptr<Stmt>>& statements);

  const std::vector<LoopInfo>& loop_infos() const;

  void attach_semantic_analyzer(const SemanticAnalyzer* analyzer);

  // Statements
  std::any visit(const ExpressionStmt& stmt) override;

  std::any visit(const BlockStmt& stmt) override;

  std::any visit(const VarStmt& stmt) override;
  std::any visit(const LetStmt& stmt) override;
  std::any visit(const IfStmt& stmt) override;

  std::any visit(const WhileStmt& stmt) override;
  std::any visit(const ForStmt& stmt) override;

  std::any visit(const ReflectStmt& stmt) override;

  std::any visit(const RecurseStmt& stmt) override;

  std::any visit(const DistributedStmt& stmt) override;

  std::any visit(const InfiniteStmt& stmt) override;

  std::any visit(const TrainStmt& stmt) override;

  std::any visit(const LoopStmt& stmt) override;
  std::any visit(const ReturnStmt& stmt) override;
  std::any visit(const AssertStmt& stmt) override;
  std::any visit(const BreakStmt&) override;
  std::any visit(const ContinueStmt&) override;
  std::any visit(const FunctionStmt& stmt) override;
  std::any visit(const TypeDecl& stmt) override;
  std::any visit(const RecordDecl& stmt) override;
  std::any visit(const EnumDecl& stmt) override;
  std::any visit(const AgentDecl& stmt) override;  // RFC-0015

  // Expressions
  std::any visit(const BinaryExpr& expr) override;

  std::any visit(const LiteralExpr& expr) override;

  std::any visit(const GroupingExpr& expr) override;

  std::any visit(const UnaryExpr& expr) override;
  std::any visit(const VariableExpr& expr) override;
  std::any visit(const CallExpr& expr) override;
  std::any visit(const AssignExpr& expr) override;
  std::any visit(const SimpleTypeExpr&) override;
  std::any visit(const GenericTypeExpr&) override;
  std::any visit(const SetLiteralExpr& expr) override;
  std::any visit(const MapLiteralExpr& expr) override;
  std::any visit(const BlockExpr& expr) override;

  std::any visit(const IfExpr& expr) override;

  std::any visit(const MatchExpr& expr) override;

  std::any visit(const FieldAccessExpr& expr) override;

  std::any visit(const RecordLiteralExpr& expr) override;

  std::any visit(const EnumLiteralExpr& expr) override;

  std::any visit(const SymbolLiteralExpr& expr) override;

  std::any visit(const InfiniteLiteralExpr& expr) override;

  std::any visit(const InferExpr& expr) override;

  std::any visit(const VectorLiteralExpr& expr) override;

  std::any visit(const IndexExpr& expr) override;

private:
  struct TypedRegister {
    tisc::ir::Register reg;
    tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Unknown;
  };

  enum class NumericCategory { Integer, Float, Fraction, Unknown };

  static tisc::ir::ComparisonRelation relation_from_token(TokenType type);

  NumericCategory categorize(const Type* type) const;

  tisc::ir::PrimitiveKind categorize_primitive(const Type* type) const;

  // Map a TypeExpr annotation (from the AST) to a PrimitiveKind for coercion.
  tisc::ir::PrimitiveKind primitive_kind_from_type_expr(const TypeExpr* te) const;

  tisc::ir::Opcode select_opcode(NumericCategory kind, tisc::ir::Opcode integer_op,
                                 tisc::ir::Opcode float_op, tisc::ir::Opcode fraction_op) const;

  const Type* typed_expr(const Expr* expr) const;

  void emit(tisc::ir::Instruction instr);

  void emit_simple(tisc::ir::Opcode opcode);

  void emit_label(tisc::ir::Label label);

  void emit_jump(tisc::ir::Label target);

  void emit_jump_if_zero(tisc::ir::Label target, const TypedRegister& cond);

  void emit_jump_if_not_zero(tisc::ir::Label target, const TypedRegister& cond);

  void emit_option_is_some(const TypedRegister& dest, const TypedRegister& source);

  void emit_option_unwrap(const TypedRegister& dest, const TypedRegister& source);

  void emit_result_is_ok(const TypedRegister& dest, const TypedRegister& source);

  void emit_result_unwrap_ok(const TypedRegister& dest, const TypedRegister& source);

  void emit_result_unwrap_err(const TypedRegister& dest, const TypedRegister& source);

  void emit_make_option_some(const TypedRegister& dest, const TypedRegister& payload);

  void emit_make_option_none(const TypedRegister& dest);

  void emit_make_result_ok(const TypedRegister& dest, const TypedRegister& payload);

  void emit_make_result_err(const TypedRegister& dest, const TypedRegister& payload);

  void emit_make_enum_variant(const TypedRegister& dest, int global_variant_id);

  void emit_make_enum_variant_payload(const TypedRegister& dest, const TypedRegister& payload,
                                      int global_variant_id);

  void emit_enum_is_variant(const TypedRegister& dest, const TypedRegister& source,
                            int global_variant_id);

  void emit_enum_unwrap_payload(const TypedRegister& dest, const TypedRegister& source);

  tisc::ir::Register new_register();

  tisc::ir::Label new_label();

  TypedRegister evaluate_expr(const Expr* expr);

  void record_result(const Expr* expr, TypedRegister reg);

  TypedRegister allocate_typed_register(tisc::ir::PrimitiveKind primitive);

  TypedRegister ensure_kind(TypedRegister source, tisc::ir::PrimitiveKind target);

  TypedRegister ensure_integer(TypedRegister source);

  TypedRegister ensure_expr_result(const Expr* expr) const;

  void copy_to_dest(TypedRegister source, TypedRegister dest);

  TypedRegister emit_integer_literal(std::int64_t value);

  TypedRegister emit_checked_i32_narrow(TypedRegister source);

  void bind_variable(const std::string& name, TypedRegister reg);

  std::optional<TypedRegister> lookup_variable(std::string_view name) const;

  void bind_variable_from_initializer(const Token& name_token, const Expr* initializer);

  void enter_pattern_scope();

  void exit_pattern_scope();

  void bind_pattern_variable(std::string name, const TypedRegister& reg);

  void bind_pattern_payload(const MatchPattern& pattern, const TypedRegister& reg);

  void bind_variant_payload(const MatchArm& arm, const TypedRegister& reg);

  std::string guard_metadata_reason(const SemanticAnalyzer::MatchMetadata::ArmInfo& info,
                                    std::optional<int> variant_id) const;

  void emit_guard_metadata(const SemanticAnalyzer::MatchMetadata::ArmInfo* info,
                           std::optional<int> variant_id);

  const EnumInfo* enum_info_for_name(std::string_view name) const;

  std::optional<int> global_variant_id_for(std::string_view enum_name, int variant_id) const;

  std::optional<int> global_variant_id_for(
      const SemanticAnalyzer::MatchMetadata::ArmInfo& arm) const;

  std::optional<int> resolve_variant_index(std::string_view enum_name,
                                           std::string_view variant_name) const;

  tisc::ir::IntermediateProgram _program;
  SymbolTable _symbols;
  const SemanticAnalyzer* _semantic = nullptr;
  int _register_count = 1;
  int _label_count = 0;
  std::unordered_map<const Expr*, TypedRegister> _expr_registers;
  std::unordered_map<std::string, TypedRegister> _variable_registers;
  std::unordered_map<std::string, tisc::ir::Label> _function_labels;
  // RFC-0026 AI-M6: maps @attention/@qmatmul function names to their AI IR opcode.
  std::unordered_map<std::string, tisc::ir::Opcode> _ai_intrinsic_map;
  std::vector<std::vector<std::pair<std::string, std::optional<TypedRegister>>>> _pattern_scopes;
  std::vector<LoopInfo> _loop_infos;
  std::vector<LoopInfo> _loop_stack;
};

}  // namespace t81::frontend

#endif  // T81_FRONTEND_IR_GENERATOR_HPP
