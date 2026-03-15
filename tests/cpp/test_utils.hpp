#ifndef T81_TEST_UTILS_HPP
#define T81_TEST_UTILS_HPP

#include <any>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

using namespace t81::frontend;

class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
  std::string print(const Stmt& stmt) { return std::any_cast<std::string>(stmt.accept(*this)); }

  std::string print(const Expr& expr) { return std::any_cast<std::string>(expr.accept(*this)); }

  std::any visit(const ExpressionStmt& stmt) override {
    return parenthesize(";", {&stmt.expression});
  }

  std::any visit(const VarStmt& stmt) override {
    [[maybe_unused]] std::string name = "var " + std::string(stmt.name.lexeme);
    if (stmt.type) {
      name += ": " + print(*stmt.type);
    }
    if (stmt.initializer) {
      return parenthesize(name, {&stmt.initializer});
    }
    return std::string("(" + name + ")");
  }

  std::any visit(const LetStmt& stmt) override {
    [[maybe_unused]] std::string name = "let " + std::string(stmt.name.lexeme);
    if (stmt.type) {
      name += ": " + print(*stmt.type);
    }
    name += " =";
    return parenthesize(name, {&stmt.initializer});
  }

  std::any visit(const BlockStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(block";
    for (const auto& statement : stmt.statements) {
      ss << " " << print(*statement);
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const IfStmt& stmt) override {
    if (stmt.else_branch) {
      return parenthesize("if-else", {&stmt.condition, &stmt.then_branch, &stmt.else_branch});
    }
    return parenthesize("if", {&stmt.condition, &stmt.then_branch});
  }

  std::any visit(const WhileStmt& stmt) override {
    return parenthesize("while", {&stmt.condition, &stmt.body});
  }

  std::any visit(const ForStmt& stmt) override {
    return parenthesize("for " + std::string(stmt.iterator.lexeme) + " in",
                        {&stmt.iterable, &stmt.body});
  }

  std::any visit(const ReflectStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(reflect (block";
    for (const auto& s : stmt.body) {
      ss << " " << print(*s);
    }
    ss << "))";
    return ss.str();
  }

  std::any visit(const LoopStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(loop";
    if (stmt.bound_kind == LoopStmt::BoundKind::Infinite) {
      ss << " @bounded(infinite)";
    } else if (stmt.bound_kind == LoopStmt::BoundKind::Static) {
      ss << " @bounded(" << stmt.bound_value.value_or(0) << ")";
    }
    ss << " (block";
    for (const auto& statement : stmt.body) {
      ss << " " << print(*statement);
    }
    ss << "))";
    return ss.str();
  }

  std::any visit(const ReturnStmt& stmt) override {
    if (stmt.value) {
      return parenthesize("return", {&stmt.value});
    }
    return std::string("(return)");
  }

  std::any visit(const AssertStmt& stmt) override {
    return parenthesize("assert", std::vector<const Expr*>{stmt.expr.get()});
  }

  std::any visit(const BreakStmt& /*stmt*/) override { return std::string("(break)"); }

  std::any visit(const ContinueStmt& /*stmt*/) override { return std::string("(continue)"); }

  std::any visit(const RecurseStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(recurse " << stmt.name.lexeme;
    ss << " (";
    for (size_t i = 0; i < stmt.params.size(); ++i) {
      ss << stmt.params[i].name.lexeme;
      if (stmt.params[i].type) {
        ss << ": " << print(*stmt.params[i].type);
      }
      if (i < stmt.params.size() - 1) {
        ss << " ";
      }
    }
    ss << " )";
    ss << " ";
    ss << "(block";
    for (const auto& statement : stmt.body) {
      ss << " " << print(*statement);
    }
    ss << ")";
    ss << ")";
    return ss.str();
  }

  std::any visit(const DistributedStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(distributed (block";
    for (const auto& s : stmt.body) {
      ss << " " << print(*s);
    }
    ss << "))";
    return ss.str();
  }

  std::any visit(const InfiniteStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(infinite (block";
    for (const auto& s : stmt.body) {
      ss << " " << print(*s);
    }
    ss << "))";
    return ss.str();
  }

  std::any visit(const TrainStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(train " << print(*stmt.model) << " (block";
    for (const auto& s : stmt.body) {
      ss << " " << print(*s);
    }
    ss << "))";
    return ss.str();
  }

  std::any visit(const FunctionStmt& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(fn " << stmt.name.lexeme;
    if (!stmt.generic_params.empty()) {
      ss << " [";
      for (size_t i = 0; i < stmt.generic_params.size(); ++i) {
        ss << stmt.generic_params[i].lexeme;
        if (i + 1 < stmt.generic_params.size()) {
          ss << ", ";
        }
      }
      ss << "]";
    }
    ss << " (";
    for (size_t i = 0; i < stmt.params.size(); ++i) {
      ss << stmt.params[i].name.lexeme << ": " << print(*stmt.params[i].type);
      if (i < stmt.params.size() - 1) {
        ss << " ";
      }
    }
    ss << " )";
    if (stmt.return_type) {
      ss << " -> " << print(*stmt.return_type);
    }
    ss << " ";
    ss << "(block";
    for (const auto& statement : stmt.body) {
      ss << " " << print(*statement);
    }
    ss << ")";
    ss << ")";
    return ss.str();
  }

  std::any visit(const TypeDecl& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(type " << stmt.name.lexeme << " [";
    for (size_t i = 0; i < stmt.params.size(); ++i) {
      ss << stmt.params[i].lexeme;
      if (i + 1 < stmt.params.size()) {
        ss << ", ";
      }
    }
    ss << "] = " << print(*stmt.alias);
    ss << ")";
    return ss.str();
  }

  std::any visit(const RecordDecl& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(record " << stmt.name.lexeme;
    for (const auto& field : stmt.fields) {
      ss << " " << field.name.lexeme << ": ";
      if (field.type) {
        ss << print(*field.type);
      } else {
        ss << "<unknown>";
      }
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const EnumDecl& stmt) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(enum " << stmt.name.lexeme;
    for (const auto& variant : stmt.variants) {
      ss << " " << variant.name.lexeme;
      if (variant.payload) {
        ss << "(" << print(*variant.payload) << ")";
      }
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const BinaryExpr& expr) override {
    return parenthesize(expr.op.lexeme, {std::any(&expr.left), std::any(&expr.right)});
  }

  std::any visit(const UnaryExpr& expr) override {
    return parenthesize(expr.op.lexeme, {&expr.right});
  }

  std::any visit(const LiteralExpr& expr) override { return std::string(expr.value.lexeme); }

  std::any visit(const SymbolLiteralExpr& expr) override { return std::string(expr.value.lexeme); }

  std::any visit(const InfiniteLiteralExpr& expr) override {
    if (expr.seed) {
      return "(infinite_literal " + print(*expr.seed) + ")";
    }
    return std::string("(infinite_literal)");
  }

  std::any visit(const InferExpr& expr) override {
    return "(infer " + print(*expr.expression) + ")";
  }

  std::any visit(const VectorLiteralExpr& expr) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < expr.elements.size(); ++i) {
      ss << print(*expr.elements[i]);
      if (i + 1 < expr.elements.size()) ss << ", ";
    }
    ss << "]";
    return ss.str();
  }

  std::any visit(const GroupingExpr& expr) override {
    return parenthesize("group", {&expr.expression});
  }

  std::any visit(const VariableExpr& expr) override { return std::string(expr.name.lexeme); }

  std::any visit(const CallExpr& expr) override {
    [[maybe_unused]] std::vector<std::any> parts;
    parts.push_back(&expr.callee);
    for (const auto& arg : expr.arguments) {
      parts.push_back(&arg);
    }
    return parenthesize("call", parts);
  }

  std::any visit(const AssignExpr& expr) override {
    return parenthesize("=", std::vector<const Expr*>{expr.target.get(), expr.value.get()});
  }

  std::any visit(const MatchExpr& expr) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(match " << print(*expr.scrutinee);
    for (const auto& arm : expr.arms) {
      ss << " (" << arm.keyword.lexeme;
      switch (arm.pattern.kind) {
        case MatchPattern::Kind::Identifier:
          if (!arm.pattern.binding_is_wildcard) {
            ss << " " << arm.pattern.identifier.lexeme;
          }
          break;
        case MatchPattern::Kind::Tuple:
          for (const auto& binding : arm.pattern.tuple_bindings) {
            ss << " " << binding.lexeme;
          }
          break;
        case MatchPattern::Kind::Record:
          ss << " {";
          for (const auto& binding : arm.pattern.record_bindings) {
            ss << binding.first.lexeme;
            if (!(binding.second.lexeme == binding.first.lexeme)) {
              ss << ":" << binding.second.lexeme;
            }
            ss << ",";
          }
          ss << " }";
          break;
        default:
          break;
      }
      if (arm.guard) {
        ss << " if " << print(*arm.guard);
      }
      ss << " => " << print(*arm.expression);
      ss << ")";
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const IndexExpr& expr) override {
    return parenthesize("index", {std::any(&expr.object), std::any(&expr.index)});
  }

  std::any visit(const FieldAccessExpr& expr) override {
    return parenthesize("field " + std::string(expr.field.lexeme),
                        std::vector<const Expr*>{expr.object.get()});
  }

  std::any visit(const RecordLiteralExpr& expr) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(recordlit " << expr.type_name.lexeme;
    for (const auto& field : expr.fields) {
      ss << " " << field.first.lexeme;
      if (field.second) {
        ss << ": " << print(*field.second);
      }
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const EnumLiteralExpr& expr) override {
    [[maybe_unused]] std::stringstream ss;
    ss << "(enumlit " << expr.enum_name.lexeme << "." << expr.variant.lexeme;
    if (expr.payload) {
      ss << " " << print(*expr.payload);
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const SimpleTypeExpr& expr) override { return std::string(expr.name.lexeme); }

  std::any visit(const GenericTypeExpr& expr) override {
    std::vector<const Expr*> params;
    for (size_t i = 0; i < expr.param_count; ++i) {
      params.push_back(expr.params[i].get());
    }
    return parenthesize("generic " + std::string(expr.name.lexeme), params);
  }

  std::any visit(const BlockExpr& expr) override {
    std::stringstream ss;
    ss << "(block";
    for (const auto& statement : expr.statements) {
      ss << " " << print(*statement);
    }
    if (expr.final_expr) {
      ss << " " << print(*expr.final_expr);
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const IfExpr& expr) override {
    std::stringstream ss;
    ss << "(if " << print(*expr.condition) << " " << print(*expr.then_branch);
    if (expr.else_branch) {
      ss << " " << print(*expr.else_branch);
    }
    ss << ")";
    return ss.str();
  }

  std::any visit(const SetLiteralExpr& /*expr*/) override { return std::string("set"); }
  std::any visit(const MapLiteralExpr& /*expr*/) override { return std::string("map"); }

private:
  std::string parenthesize(std::string_view name, const std::vector<const Expr*>& exprs) {
    [[maybe_unused]] std::stringstream ss;
    ss << "(" << name;
    for (const auto& expr : exprs) {
      ss << " " << print(*expr);
    }
    ss << ")";
    return ss.str();
  }

  std::string parenthesize(std::string_view name, const std::vector<std::any>& parts) {
    [[maybe_unused]] std::stringstream ss;
    ss << "(" << name;
    for (const auto& part : parts) {
      ss << " ";
      if (part.type() == typeid(const std::unique_ptr<Expr>*)) {
        ss << print(**std::any_cast<const std::unique_ptr<Expr>*>(part));
      } else if (part.type() == typeid(const std::unique_ptr<Stmt>*)) {
        ss << print(**std::any_cast<const std::unique_ptr<Stmt>*>(part));
      }
    }
    ss << ")";
    return ss.str();
  }
};

inline void expect_semantic_success(const std::string& source, const char* label) {
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  if (parser.had_error()) {
    std::cerr << "Parse error in '" << label << "'" << std::endl;
  }
  assert(!parser.had_error() && label);

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (analyzer.had_error()) {
    std::cerr << "Semantic error in '" << label << "'" << std::endl;
    for (const auto& diag : analyzer.diagnostics()) {
      std::cerr << diag.message << std::endl;
    }
  }
  assert(!analyzer.had_error() && label);
}

inline void expect_semantic_failure(const std::string& source, const char* label,
                                    const std::string& expected_error = "") {
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  if (parser.had_error()) return;

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  assert(analyzer.had_error() && label);

  if (!expected_error.empty()) {
    for (const auto& diag : analyzer.diagnostics()) {
      if (diag.message.find(expected_error) != std::string::npos) {
        return;
      }
    }
    std::cerr << "Test '" << label << "' failed. Expected error containing: '" << expected_error
              << "', but no matching diagnostic was found." << std::endl;
    assert(false && "Expected diagnostic not found.");
  }
}

#endif  // T81_TEST_UTILS_HPP
