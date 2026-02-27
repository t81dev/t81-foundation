#include <iostream>
#include <memory>
#include <vector>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "test_runtime_check.hpp"

using namespace t81::frontend;

void test_symbol_intern_type() {
  // We need to parse a small script that uses std.symbol.intern and check the type
  // This requires setting up the full pipeline or at least SemanticAnalyzer with some mock
  // statements.

  // Easier approach: Use SemanticAnalyzer directly on a CallExpr.

  // Construct AST: std.symbol.intern("foo")
  auto callee = std::make_unique<FieldAccessExpr>(
      std::make_unique<FieldAccessExpr>(
          std::make_unique<VariableExpr>(Token{TokenType::Identifier, "std", 0, 0}),
          Token{TokenType::Identifier, "symbol", 0, 0}),
      Token{TokenType::Identifier, "intern", 0, 0});

  std::vector<std::unique_ptr<Expr>> args;
  args.push_back(std::make_unique<LiteralExpr>(Token{TokenType::String, "\"foo\"", 0, 0}));

  CallExpr call(std::move(callee), Token{TokenType::RParen, ")", 0, 0}, std::move(args));

  std::vector<std::unique_ptr<Stmt>> stmts;
  // We need an empty semantic analyzer context
  SemanticAnalyzer analyzer(stmts, "test_source");

  // Analyze
  auto result_any = analyzer.analyze(call);

  try {
    Type result_type = std::any_cast<Type>(result_any);
    std::cout << "Intern return type kind: " << (int)result_type.kind << std::endl;

    // Check if it is Type::Kind::Symbol
    T81_TEST_CHECK(result_type.kind == Type::Kind::Symbol);

  } catch (const std::bad_any_cast&) {
    std::cerr << "Analysis failed to return a type." << std::endl;
    std::exit(1);
  }
}

void test_symbol_equality_type() {
  // Check type of std.symbol.eq(sym1, sym2) -> bool
  // And argument types must be Symbol

  auto callee = std::make_unique<FieldAccessExpr>(
      std::make_unique<FieldAccessExpr>(
          std::make_unique<VariableExpr>(Token{TokenType::Identifier, "std", 0, 0}),
          Token{TokenType::Identifier, "symbol", 0, 0}),
      Token{TokenType::Identifier, "eq", 0, 0});

  // We need expressions that evaluate to Symbol.
  // Let's use SymbolLiteralExpr for arguments: :foo
  std::vector<std::unique_ptr<Expr>> args;
  args.push_back(std::make_unique<SymbolLiteralExpr>(Token{TokenType::Symbol, ":foo", 0, 0}));
  args.push_back(std::make_unique<SymbolLiteralExpr>(Token{TokenType::Symbol, ":bar", 0, 0}));

  CallExpr call(std::move(callee), Token{TokenType::RParen, ")", 0, 0}, std::move(args));

  std::vector<std::unique_ptr<Stmt>> stmts;
  SemanticAnalyzer analyzer(stmts, "test_source");

  auto result_any = analyzer.analyze(call);

  try {
    Type result_type = std::any_cast<Type>(result_any);
    T81_TEST_CHECK(result_type.kind == Type::Kind::Bool);
  } catch (const std::bad_any_cast&) {
    std::cerr << "Analysis failed for symbol.eq" << std::endl;
    std::exit(1);
  }
}

int main() {
  try {
    test_symbol_intern_type();
    test_symbol_equality_type();
    std::cout << "All symbol type tests passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
