// tests/cpp/test_ir_snapshot_audit.cpp
//
// Targeted IR Snapshot Audit for T81Lang Bitwise Precedence
// Validates that the parser's precedence rules are correctly lowered to IR structure.
// Specifically checks relative ordering of operations to confirm grouping.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/ir.hpp"

using namespace t81::frontend;
using namespace t81::tisc::ir;

// Returns true if the sequence of opcodes in `expected` appears in `instructions`
// in the same relative order (not necessarily contiguous).
bool check_opcode_sequence(const std::vector<Instruction>& instructions,
                           const std::vector<Opcode>& expected) {
  auto it = instructions.begin();
  for (const auto& op : expected) {
    it = std::find_if(it, instructions.end(),
                      [&](const Instruction& inst) { return inst.opcode == op; });
    if (it == instructions.end()) {
      return false;
    }
    // Advance to next instruction to ensure strict forward ordering
    it++;
  }
  return true;
}

void run_test(const std::string& name, const std::string& source,
              const std::vector<Opcode>& expected_sequence,
              const std::vector<Opcode>& forbidden_sequence = {}) {
  std::cout << "Running test: " << name << " ... ";

  // Wrap expression in a function to make it a valid program
  std::string full_source = "fn main() -> i32 { " + source + "; return 0; }";

  Lexer lexer(full_source);
  Parser parser(lexer);
  auto stmts = parser.parse();

  if (parser.had_error()) {
    std::cout << "[FAIL] Parse error" << std::endl;
    exit(1);
  }

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();

  // Note: Semantic analysis might fail if variables are not defined.
  // However, for structural checks, we might get away with it if we declare them.
  // Or we can just use literals for simplicity where possible, or declare variables.

  IRGenerator generator;
  // We attach analyzer if we want type info, but for simple structural checks of ops,
  // maybe we can skip if the generator handles untyped AST?
  // IRGenerator usually requires typed AST for things like method calls, but maybe not for basic
  // ops. Let's try attaching it.
  generator.attach_semantic_analyzer(&analyzer);

  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  bool passed = true;

  // Check expected sequence
  if (!check_opcode_sequence(instructions, expected_sequence)) {
    std::cout << "[FAIL] Expected opcode sequence not found." << std::endl;
    passed = false;
  }

  // Check forbidden sequence (if provided) to ensure we didn't get the wrong precedence
  if (!forbidden_sequence.empty() && check_opcode_sequence(instructions, forbidden_sequence)) {
    std::cout << "[FAIL] Forbidden opcode sequence found (indicates wrong precedence)."
              << std::endl;
    passed = false;
  }

  if (passed) {
    std::cout << "[PASS]" << std::endl;
  } else {
    std::cout << "  Source: " << source << std::endl;
    std::cout << "  Generated IR Opcodes: ";
    for (const auto& inst : instructions) {
      // We don't have a to_string for Opcode handy here easily without including extra headers or
      // implementing it. So just print raw int for now or skip.
      std::cout << (int)inst.opcode << " ";
    }
    std::cout << std::endl;
    exit(1);
  }
}

int main() {
  // 1. Bitwise AND vs Equality
  // a & b == c  ->  (a & b) == c  ->  (a & b) evaluated first -> BITAND before CMP
  // Context: need variables.
  std::string decls = "let a = 1; let b = 2; let c = 3; ";
  run_test("Precedence: & vs ==", decls + "let _ = a & b == c",
           {Opcode::BITAND, Opcode::CMP},   // Expected: Bitwise first
           {Opcode::CMP, Opcode::BITAND});  // Forbidden: Equality first

  // 2. Shift vs Add
  // a << b + c  ->  a << (b + c)  ->  (b + c) evaluated first -> ADD before BITSHL
  run_test("Precedence: << vs +", decls + "let _ = a << b + c", {Opcode::ADD, Opcode::BITSHL},
           {Opcode::BITSHL, Opcode::ADD});

  // 3. Unary ~ vs Add
  // ~a + 1  ->  (~a) + 1  ->  ~a evaluated first -> BITNOT before ADD
  run_test("Precedence: ~ vs +", decls + "let _ = ~a + 1", {Opcode::BITNOT, Opcode::ADD},
           {Opcode::ADD, Opcode::BITNOT});

  // 4. Bitwise OR vs Logical AND
  // SKIP: IRGenerator does not support logical && / || yet.
  // run_test("Precedence: | vs &&", ...);
  std::cout
      << "Skipping test: Precedence: | vs && (Logical operators not implemented in IRGenerator)"
      << std::endl;

  // 5. Comparison vs Shift
  // a < b << c  ->  a < (b << c)
  // Shift is tighter than Comparison.
  // So BITSHL before CMP.
  run_test("Precedence: < vs <<", decls + "let _ = a < b << c", {Opcode::BITSHL, Opcode::CMP},
           {Opcode::CMP, Opcode::BITSHL});

  // 6. Right Shift vs Unsigned Right Shift
  // a >> b  -> BITSHR
  run_test("Opcode: >>", decls + "let _ = a >> b", {Opcode::BITSHR});

  // a >>> b -> BITUSHR
  run_test("Opcode: >>>", decls + "let _ = a >>> b", {Opcode::BITUSHR});

  std::cout << "All IR Snapshot Audit tests passed." << std::endl;
  return 0;
}
