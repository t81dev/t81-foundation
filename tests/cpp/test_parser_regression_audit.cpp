#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"

using namespace t81::frontend;

// Minimal AST Printer for Regression Audit
class ASTPrinter : public ExprVisitor {
public:
  std::string print(const Expr& expr) { return std::any_cast<std::string>(expr.accept(*this)); }

  std::any visit(const BinaryExpr& expr) override {
    return parenthesize(expr.op.lexeme, {expr.left.get(), expr.right.get()});
  }

  std::any visit(const UnaryExpr& expr) override {
    return parenthesize(expr.op.lexeme, {expr.right.get()});
  }

  std::any visit(const LiteralExpr& expr) override { return std::string(expr.value.lexeme); }

  std::any visit(const GroupingExpr& expr) override {
    return std::string("(group ") + print(*expr.expression) + ")";
  }

  std::any visit(const VariableExpr& expr) override { return std::string(expr.name.lexeme); }

  std::any visit(const AssignExpr& expr) override {
    return parenthesize("=", {expr.target.get(), expr.value.get()});
  }

  // Stub other visitors as we don't expect them in this specific corpus
  std::any visit(const CallExpr& expr) override { return std::string("call"); }
  std::any visit(const MatchExpr& expr) override { return std::string("match"); }
  std::any visit(const VectorLiteralExpr& expr) override { return std::string("vector"); }
  std::any visit(const FieldAccessExpr& expr) override { return std::string("field"); }
  std::any visit(const RecordLiteralExpr& expr) override { return std::string("record"); }
  std::any visit(const EnumLiteralExpr& expr) override { return std::string("enum"); }
  std::any visit(const SymbolLiteralExpr& expr) override { return std::string("symbol"); }
  std::any visit(const InfiniteLiteralExpr& expr) override { return std::string("infinite"); }
  std::any visit(const IndexExpr& expr) override { return std::string("index"); }
  std::any visit(const BlockExpr& expr) override { return std::string("block"); }
  std::any visit(const IfExpr& expr) override { return std::string("if"); }
  std::any visit(const SimpleTypeExpr& expr) override { return std::string("type"); }
  std::any visit(const GenericTypeExpr& expr) override { return std::string("generic_type"); }
  std::any visit(const InferExpr& expr) override { return std::string("infer"); }
  std::any visit(const SetLiteralExpr& expr) override { return std::string("set"); }
  std::any visit(const MapLiteralExpr& expr) override { return std::string("map"); }

private:
  std::string parenthesize(std::string_view name, std::vector<const Expr*> exprs) {
    std::stringstream ss;
    ss << "(" << name;
    for (const auto* expr : exprs) {
      ss << " " << print(*expr);
    }
    ss << ")";
    return ss.str();
  }
};

struct TestCase {
  std::string expression;
  std::string expected_ast;
};

int main() {
  std::vector<TestCase> cases = {// 1. Bitwise Precedence
                                 {"a & b | c", "(| (& a b) c)"},
                                 {"a | b & c", "(| a (& b c))"},
                                 {"a ^ b | c", "(| (^ a b) c)"},
                                 {"a | b ^ c", "(| a (^ b c))"},
                                 {"a & b ^ c", "(^ (& a b) c)"},
                                 {"a ^ b & c", "(^ a (& b c))"},

                                 // 2. Unary ~
                                 {"~a & b", "(& (~ a) b)"},
                                 {"~a + 1", "(+ (~ a) 1)"},

                                 // 3. Shifts vs Arithmetic
                                 {"a << b + c", "(<< a (+ b c))"},
                                 {"a + b << c", "(<< (+ a b) c)"},
                                 {"a >> b - c", "(>> a (- b c))"},
                                 {"a >>> b * c", "(>>> a (* b c))"},

                                 // 4. Shifts vs Comparison
                                 {"a < b << c", "(< a (<< b c))"},
                                 {"a << b < c", "(< (<< a b) c)"},

                                 // 5. Bitwise vs Equality
                                 {"a & b == c", "(== (& a b) c)"},
                                 {"a == b & c", "(== a (& b c))"},

                                 // 6. Custom Operators (->, ..)
                                 {"a -> b & c", "(-> a (& b c))"},
                                 {"a & b -> c", "(-> (& a b) c)"},
                                 {"a .. b -> c", "(-> (.. a b) c)"},
                                 {"a -> b .. c", "(-> a (.. b c))"},

                                 // 7. Logical vs Bitwise
                                 {"a && b | c", "(&& a (| b c))"},
                                 {"a | b && c", "(&& (| a b) c)"},
                                 {"a || b && c", "(|| a (&& b c))"},
                                 {"a && b || c", "(|| (&& a b) c)"},

                                 // 8. Assignment
                                 {"x = a | b", "(= x (| a b))"},
                                 {"x = a & b", "(= x (& a b))"},
                                 {"x = a << b", "(= x (<< a b))"},

                                 // 9. Associativity
                                 {"a + b + c", "(+ (+ a b) c)"},
                                 {"a << b << c", "(<< (<< a b) c)"},
                                 {"a -> b -> c", "(-> (-> a b) c)"},
                                 {"a & b & c", "(& (& a b) c)"},
                                 {"a ** b ** c", "(** a (** b c))"},

                                 // 10. Unary Chaining
                                 {"~-a", "(~ (- a))"},
                                 {"-~a", "(- (~ a))"},
                                 {"!~a", "(! (~ a))"},

                                 // 11. Existing Arithmetic Regressions
                                 {"a + b * c", "(+ a (* b c))"},
                                 {"a * b + c", "(+ (* a b) c)"},
                                 {"(a + b) * c", "(* (group (+ a b)) c)"}};

  int passed = 0;
  int failed = 0;
  ASTPrinter printer;

  for (const auto& test : cases) {
    t81::frontend::Lexer lexer(test.expression);
    t81::frontend::Parser parser(lexer);

    // We need to parse an expression, not a statement list.
    // The parser only exposes `parse()` which returns statements.
    // However, `Parser::expression()` is private.
    // But `parse()` parses `declaration` -> `statement` -> `expression_statement`.
    // So if we parse "expr;", we get an ExpressionStmt containing the Expr.
    // Or just "expr" might be treated as ExpressionStmt if implicit?
    // Looking at Parser::parse(): `while (!is_at_end) statements.push_back(declaration());`
    // declaration -> ... -> statement.
    // statement -> ... -> expression_statement (if no keywords).
    // expression_statement -> expression ";".
    // So we need to append ";" to the expression string to be a valid statement.

    std::string source = test.expression + ";";
    t81::frontend::Lexer stmt_lexer(source);
    t81::frontend::Parser stmt_parser(stmt_lexer);

    try {
      auto stmts = stmt_parser.parse();
      if (stmt_parser.had_error() || stmts.empty()) {
        std::cerr << "[FAIL] Parse error for: " << test.expression << "\n";
        failed++;
        continue;
      }

      // Extract the expression from the statement
      const auto& stmt = stmts[0];
      // stmt is unique_ptr<Stmt>. We need to cast it to ExpressionStmt.
      // But we can't RTTI easily if types are not fully polymorphic or we don't know the exact
      // layout. Actually Stmt has `accept(StmtVisitor)`. We can use a visitor to extract the
      // expression.

      class ExtractExprVisitor : public StmtVisitor {
      public:
        const Expr* result = nullptr;
        std::any visit(const ExpressionStmt& stmt) override {
          result = stmt.expression.get();
          return {};
        }
        // Others ignore
        std::any visit(const VarStmt&) override { return {}; }
        std::any visit(const LetStmt&) override { return {}; }
        std::any visit(const BlockStmt&) override { return {}; }
        std::any visit(const IfStmt&) override { return {}; }
        std::any visit(const WhileStmt&) override { return {}; }
        std::any visit(const ForStmt&) override { return {}; }
        std::any visit(const ReflectStmt&) override { return {}; }
        std::any visit(const RecurseStmt&) override { return {}; }
        std::any visit(const DistributedStmt&) override { return {}; }
        std::any visit(const InfiniteStmt&) override { return {}; }
        std::any visit(const TrainStmt&) override { return {}; }
        std::any visit(const LoopStmt&) override { return {}; }
        std::any visit(const ReturnStmt&) override { return {}; }
        std::any visit(const AssertStmt&) override { return {}; }
        std::any visit(const BreakStmt&) override { return {}; }
        std::any visit(const ContinueStmt&) override { return {}; }
        std::any visit(const FunctionStmt&) override { return {}; }
        std::any visit(const TypeDecl&) override { return {}; }
        std::any visit(const RecordDecl&) override { return {}; }
        std::any visit(const EnumDecl&) override { return {}; }
      };

      ExtractExprVisitor extractor;
      stmt->accept(extractor);

      if (!extractor.result) {
        std::cerr << "[FAIL] Could not extract expression from statement for: " << test.expression
                  << "\n";
        failed++;
        continue;
      }

      std::string result = printer.print(*extractor.result);
      if (result == test.expected_ast) {
        // std::cout << "[PASS] " << test.expression << " -> " << result << "\n";
        passed++;
      } else {
        std::cerr << "[FAIL] " << test.expression << "\n";
        std::cerr << "  Expected: " << test.expected_ast << "\n";
        std::cerr << "  Got:      " << result << "\n";
        failed++;
      }

    } catch (const std::exception& e) {
      std::cerr << "[FAIL] Exception for: " << test.expression << " : " << e.what() << "\n";
      failed++;
    }
  }

  std::cout << "\nResults: " << passed << " passed, " << failed << " failed.\n";
  return failed == 0 ? 0 : 1;
}
