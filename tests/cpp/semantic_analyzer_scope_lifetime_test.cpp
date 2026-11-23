#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace t81::frontend {
class ScopeLifetimeTestPeer {
public:
  static void enter_scope(SemanticAnalyzer& sa) { sa.enter_scope(); }
  static void exit_scope(SemanticAnalyzer& sa) { sa.exit_scope(); }
  static void define_symbol(SemanticAnalyzer& sa, const Token& name, SymbolKind kind,
                            bool is_mutable = true) {
    sa.define_symbol(name, kind, is_mutable);
  }
  static SemanticSymbol* resolve_symbol(SemanticAnalyzer& sa, const Token& name) {
    return sa.resolve_symbol(name);
  }
};
}  // namespace t81::frontend

using namespace t81::frontend;

#define RELEASE_ASSERT(cond, msg)                                                              \
  do {                                                                                         \
    if (!(cond)) {                                                                             \
      std::cerr << "RELEASE_ASSERT failed: " << (msg) << " at " << __FILE__ << ":" << __LINE__ \
                << std::endl;                                                                  \
      std::exit(1);                                                                            \
    }                                                                                          \
  } while (false)

static std::vector<std::unique_ptr<Stmt>> parse_source(std::string_view source) {
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  RELEASE_ASSERT(!parser.had_error(), "Parser reported unexpected syntax error");
  return stmts;
}

// 1. Direct address stability test across deep scope growth.
// Verifies that a pointer to a symbol in an outer scope retains its exact
// memory address and contents while scopes are pushed past vector capacity.
static void test_symbol_address_stability_across_scope_growth() {
  std::vector<std::unique_ptr<Stmt>> dummy_stmts;
  SemanticAnalyzer sa(dummy_stmts, "unit_test");

  // Global scope is index 0. Define symbol "target_fn".
  Token fn_token{TokenType::Identifier, "target_fn", 1, 1};
  ScopeLifetimeTestPeer::define_symbol(sa, fn_token, SymbolKind::Function, false);
  SemanticSymbol* original_ptr = ScopeLifetimeTestPeer::resolve_symbol(sa, fn_token);
  RELEASE_ASSERT(original_ptr != nullptr, "Failed to resolve initial symbol");

  original_ptr->type = Type{Type::Kind::I32};
  original_ptr->param_types.push_back(Type{Type::Kind::I32});
  original_ptr->param_types.push_back(Type{Type::Kind::BigInt});
  original_ptr->tier = 2;
  original_ptr->is_pure = true;

  // Pre-generate variable names so that std::string_view tokens in the symbol table remain valid.
  constexpr int kDeepNestLevels = 256;
  std::vector<std::string> var_names;
  var_names.reserve(kDeepNestLevels);
  for (int i = 0; i < kDeepNestLevels; ++i) {
    var_names.push_back("local_var_" + std::to_string(i));
  }

  // Push 256 nested scopes. A std::vector would reallocate multiple times
  // (typically capacities: 1, 2, 4, 8, 16, 32, 64, 128, 256).
  for (int i = 0; i < kDeepNestLevels; ++i) {
    ScopeLifetimeTestPeer::enter_scope(sa);
    // In each nested scope, define a local variable to ensure the map allocates nodes.
    Token var_token{TokenType::Identifier, var_names[i], i + 2, 1};
    ScopeLifetimeTestPeer::define_symbol(sa, var_token, SymbolKind::Variable, true);

    // Verify pointer address remains bit-identical.
    SemanticSymbol* current_ptr = ScopeLifetimeTestPeer::resolve_symbol(sa, fn_token);
    RELEASE_ASSERT(current_ptr == original_ptr,
                   "Symbol pointer address changed during enter_scope!");
    RELEASE_ASSERT(current_ptr->type.kind == Type::Kind::I32,
                   "Symbol type corrupted during scope growth");
    RELEASE_ASSERT(current_ptr->param_types.size() == 2,
                   "Symbol param_types count corrupted during scope growth");
    RELEASE_ASSERT(current_ptr->tier.has_value() && *current_ptr->tier == 2,
                   "Symbol tier corrupted during scope growth");
    RELEASE_ASSERT(current_ptr->is_pure, "Symbol is_pure corrupted during scope growth");
  }

  // Pop back all nested scopes.
  for (int i = 0; i < kDeepNestLevels; ++i) {
    ScopeLifetimeTestPeer::exit_scope(sa);
    SemanticSymbol* current_ptr = ScopeLifetimeTestPeer::resolve_symbol(sa, fn_token);
    RELEASE_ASSERT(current_ptr == original_ptr,
                   "Symbol pointer address changed during exit_scope!");
  }
}

// 2. Deeply nested block and loop statements exceeding vector capacity.
static void test_deeply_nested_scope_analysis() {
  // Construct a program with 100 levels of nested blocks inside a function.
  std::ostringstream oss;
  oss << "fn test_deep_nesting(x: i32) -> i32 {\n";
  constexpr int kLevels = 100;
  for (int i = 0; i < kLevels; ++i) {
    oss << std::string(i + 1, ' ') << "{\n";
    oss << std::string(i + 2, ' ') << "let v" << i << ": i32 = " << i << ";\n";
  }
  oss << std::string(kLevels + 1, ' ') << "return x;\n";
  for (int i = kLevels - 1; i >= 0; --i) {
    oss << std::string(i + 1, ' ') << "}\n";
  }
  oss << "}\n";

  std::string source = oss.str();
  auto stmts = parse_source(source);
  SemanticAnalyzer sa(stmts, "deep_blocks");
  sa.analyze();
  RELEASE_ASSERT(!sa.had_error(), "Semantic analysis failed on deep block nesting");
}

// 3. FunctionStmt parameter and signature validation across nested scope growth.
static void test_function_signatures_and_recurse_scope_growth() {
  const std::string prog = R"(
    fn fib(n: i32) -> i32 {
      if (n <= 1) {
        return n;
      }
      {
        {
          {
            {
              return fib(n - 1) + fib(n - 2);
            }
          }
        }
      }
    }

    fn compute(a: i32, b: i32) -> i32 {
      @bounded(10)
      loop {
        {
          {
            {
              let temp: i32 = a + b;
              if (temp > 0) {
                return temp;
              }
            }
          }
        }
      }
      return 0;
    }

    recurse simple_recurse() {
      {
        {
          {
            return 42;
          }
        }
      }
    }

    fn caller() -> i32 {
      return compute(2, 3) + fib(5) + simple_recurse();
    }
  )";

  auto stmts = parse_source(prog);
  SemanticAnalyzer sa(stmts, "fn_and_recurse");
  sa.analyze();
  RELEASE_ASSERT(!sa.had_error(), "Semantic analysis failed on function and recurse calls");
}

// 4. Function parameter count mismatch diagnostic preserved across scopes.
static void test_function_diagnostics_preserved() {
  const std::string bad_prog = R"(
    fn add(x: i32, y: i32) -> i32 {
      return x + y;
    }

    fn test() -> i32 {
      return add(1);
    }
  )";

  auto stmts = parse_source(bad_prog);
  SemanticAnalyzer sa(stmts, "diagnostic_test");
  sa.analyze();
  RELEASE_ASSERT(sa.had_error(), "Expected error for parameter count mismatch");
  bool found_arg_err = false;
  for (const auto& diag : sa.diagnostics()) {
    if (diag.message.find("expects 2 arguments but got 1") != std::string::npos ||
        diag.message.find("argument") != std::string::npos) {
      found_arg_err = true;
      break;
    }
  }
  RELEASE_ASSERT(found_arg_err, "Expected argument mismatch diagnostic not found");
}

int main() {
  std::cout << "Running semantic analyzer scope lifetime regression tests..." << std::endl;
  test_symbol_address_stability_across_scope_growth();
  test_deeply_nested_scope_analysis();
  test_function_signatures_and_recurse_scope_growth();
  test_function_diagnostics_preserved();
  std::cout << "All semantic analyzer scope lifetime regression tests passed!" << std::endl;
  return 0;
}
