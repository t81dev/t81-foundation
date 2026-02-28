#undef NDEBUG
#include <cassert>
#include <iostream>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/pretty_printer.hpp"

using namespace t81;
using namespace t81::frontend;

void verify_collection_lowering(const std::string& source) {
  Lexer lexer(source);
  Parser parser(lexer, "test_source");
  auto statements = parser.parse();
  assert(!statements.empty());

  SemanticAnalyzer analyzer(statements, "test_source");
  analyzer.analyze();
  assert(!analyzer.had_error());

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  auto program = ir_gen.generate(statements);

  std::cout << "--- Source ---\n" << source << "\n";
  std::cout << "--- IR ---\n" << t81::tisc::pretty_print(program) << "\n";

  // Verify STRVECNEW opcode presence for List/Map/Set/Tree
  bool found_alloc = false;
  for (const auto& instr : program.instructions()) {
    if (instr.opcode == t81::tisc::ir::Opcode::STRVECNEW) {
      found_alloc = true;
      break;
    }
  }
  assert(found_alloc);
}

void test_list_lowering() {
  verify_collection_lowering(R"(
    fn main() {
      let l: List[i32] = List[i32]();
    }
  )");
  std::cout << "test_list_lowering PASSED\n";
}

void test_map_lowering() {
  verify_collection_lowering(R"(
    fn main() {
      let m: Map[i32] = Map[i32]();
    }
  )");
  std::cout << "test_map_lowering PASSED\n";
}

void test_set_lowering() {
  verify_collection_lowering(R"(
    fn main() {
      let s: Set[i32] = Set[i32]();
    }
  )");
  std::cout << "test_set_lowering PASSED\n";
}

void test_tree_lowering() {
  verify_collection_lowering(R"(
    fn main() {
      let t: Tree[i32] = Tree[i32]();
    }
  )");
  std::cout << "test_tree_lowering PASSED\n";
}

void verify_ir_stable(const std::string& source) {
  // Verify IR instruction count is identical across two independent lowering passes.
  Lexer lexer1(source);
  Parser parser1(lexer1, "test_source");
  auto stmts1 = parser1.parse();
  SemanticAnalyzer analyzer1(stmts1, "test_source");
  analyzer1.analyze();
  assert(!analyzer1.had_error());
  IRGenerator gen1;
  gen1.attach_semantic_analyzer(&analyzer1);
  auto prog1 = gen1.generate(stmts1);

  Lexer lexer2(source);
  Parser parser2(lexer2, "test_source");
  auto stmts2 = parser2.parse();
  SemanticAnalyzer analyzer2(stmts2, "test_source");
  analyzer2.analyze();
  IRGenerator gen2;
  gen2.attach_semantic_analyzer(&analyzer2);
  auto prog2 = gen2.generate(stmts2);

  assert(prog1.instructions().size() == prog2.instructions().size());
}

void test_list_populated_determinism() {
  verify_ir_stable(R"(
    fn main() {
      let v0: Vector[i32] = [10, 20, 30];
      let v1: Vector[i32] = std.collections.push(v0, 40);
      let v2: Vector[i32] = std.collections.pop(v1);
      let n: i32 = std.collections.len(v2);
      let f: i32 = std.collections.first(v2);
      let l: i32 = std.collections.last(v2);
    }
  )");
  std::cout << "test_list_populated_determinism PASSED\n";
}

void test_tree_populated_determinism() {
  verify_ir_stable(R"(
    fn main() {
      let v0: Vector[i32] = [5, 15, 25];
      let v1: Vector[i32] = std.collections.push(v0, 35);
      let v2: Vector[i32] = std.collections.pop(v1);
      let n: i32 = std.collections.len(v2);
      let f: i32 = std.collections.first(v2);
      let l: i32 = std.collections.last(v2);
    }
  )");
  std::cout << "test_tree_populated_determinism PASSED\n";
}

void test_graph_lowering() {
  verify_collection_lowering(R"(
    fn main() {
      let g: Vector[T81String] = std.collections.graph();
    }
  )");
  std::cout << "test_graph_lowering PASSED\n";
}

void test_graph_canonical_lowering() {
  // Verify graph_canonical lowers to IR containing STRJOIN.
  const std::string source = R"(
    fn main() {
      let g0: Vector[T81String] = std.collections.graph();
      let g1: Vector[T81String] = std.collections.graph_add_edge(g0, "x", "y");
      let s: T81String = std.collections.graph_canonical(g1);
    }
  )";
  Lexer lexer(source);
  Parser parser(lexer, "test_source");
  auto statements = parser.parse();
  assert(!statements.empty());

  SemanticAnalyzer analyzer(statements, "test_source");
  analyzer.analyze();
  assert(!analyzer.had_error());

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  auto program = ir_gen.generate(statements);

  bool found_strjoin = false;
  for (const auto& instr : program.instructions()) {
    if (instr.opcode == t81::tisc::ir::Opcode::STRJOIN) {
      found_strjoin = true;
      break;
    }
  }
  assert(found_strjoin);

  // Verify IR is stable across two independent lowering passes (determinism).
  IRGenerator ir_gen2;
  ir_gen2.attach_semantic_analyzer(&analyzer);
  auto program2 = ir_gen2.generate(statements);
  assert(program.instructions().size() == program2.instructions().size());

  std::cout << "test_graph_canonical_lowering PASSED\n";
}

int main() {
  test_list_lowering();
  test_map_lowering();
  test_set_lowering();
  test_tree_lowering();
  test_list_populated_determinism();
  test_tree_populated_determinism();
  test_graph_lowering();
  test_graph_canonical_lowering();
  std::cout << "All Collection Determinism Tests PASSED!\n";
  return 0;
}
