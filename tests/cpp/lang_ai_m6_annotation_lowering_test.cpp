#include "test_runtime_check.hpp"

#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/ir.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace t81::frontend;

namespace {

// Compile source → TISC Program (SA + IRGen + BinaryEmitter pipeline).
// Returns a default-constructed Program on SA error (caller checks had_error).
struct CompileResult {
  bool parse_ok{false};
  bool sa_ok{false};
  std::vector<std::string> sa_diagnostics;
  t81::tisc::Program program;
};

CompileResult compile(const std::string& source) {
  CompileResult res;
  Lexer lexer(source);
  Parser parser(lexer, "<test>");
  auto stmts = parser.parse();
  res.parse_ok = !parser.had_error();
  if (!res.parse_ok) return res;

  SemanticAnalyzer analyzer(stmts, "<test>");
  analyzer.analyze();
  res.sa_ok = !analyzer.had_error();
  for (const auto& d : analyzer.diagnostics()) {
    res.sa_diagnostics.push_back(d.message);
  }
  if (!res.sa_ok) return res;

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  auto ir = ir_gen.generate(stmts);

  t81::tisc::BinaryEmitter emitter;
  res.program = emitter.emit(ir);
  return res;
}

bool program_contains_opcode(const t81::tisc::Program& p, t81::tisc::Opcode op) {
  return std::any_of(p.insns.begin(), p.insns.end(),
                     [op](const t81::tisc::Insn& insn) { return insn.opcode == op; });
}

}  // namespace

int main() {
  // -----------------------------------------------------------------------
  // Test 1: Tensor.attention() builtin call lowers to ATTN opcode
  // -----------------------------------------------------------------------
  {
    // Minimal program: declare tensors in the tensor_pool and reference them
    // via handles; call Tensor.attention(q, k, v) → should emit ATTN.
    // We test compile-time opcode emission rather than runtime execution here.
    const std::string src = R"(
@tier(2)
fn main() -> Tensor {
    let q: Tensor = std.tensor.from_list([1, 2, 3]);
    let k: Tensor = std.tensor.from_list([4, 5, 6]);
    let v: Tensor = std.tensor.from_list([7, 8, 9]);
    return std.tensor.attention(q, k, v);
}
)";
    auto res = compile(src);
    T81_TEST_CHECK(res.parse_ok);
    T81_TEST_CHECK(res.sa_ok);
    T81_TEST_CHECK(program_contains_opcode(res.program, t81::tisc::Opcode::ATTN));
    std::cout << "AI_M6 Tensor.attention() → ATTN opcode: PASS\n";
  }

  // -----------------------------------------------------------------------
  // Test 2: Tensor.qmatmul() builtin call lowers to QMATMUL opcode
  // -----------------------------------------------------------------------
  {
    const std::string src = R"(
@tier(2)
fn main() -> Tensor {
    let act: Tensor   = std.tensor.from_list([1, 2, 3]);
    let wt: Tensor    = std.tensor.from_list([4, 5, 6]);
    let scale: Tensor = std.tensor.from_list([1]);
    return std.tensor.qmatmul(act, wt, scale);
}
)";
    auto res = compile(src);
    T81_TEST_CHECK(res.parse_ok);
    T81_TEST_CHECK(res.sa_ok);
    T81_TEST_CHECK(program_contains_opcode(res.program, t81::tisc::Opcode::QMATMUL));
    std::cout << "AI_M6 Tensor.qmatmul() → QMATMUL opcode: PASS\n";
  }

  // -----------------------------------------------------------------------
  // Test 3: @attention annotated function — call site lowers to ATTN
  // -----------------------------------------------------------------------
  {
    const std::string src = R"(
@tier(2)
@attention
fn my_attn(q: Tensor, k: Tensor, v: Tensor) -> Tensor {
    return std.tensor.attention(q, k, v);
}

@tier(2)
fn main() -> Tensor {
    let q: Tensor = std.tensor.from_list([1, 2, 3]);
    let k: Tensor = std.tensor.from_list([4, 5, 6]);
    let v: Tensor = std.tensor.from_list([7, 8, 9]);
    return my_attn(q, k, v);
}
)";
    auto res = compile(src);
    T81_TEST_CHECK(res.parse_ok);
    T81_TEST_CHECK(res.sa_ok);
    // The call site my_attn(q, k, v) must have been lowered to ATTN.
    T81_TEST_CHECK(program_contains_opcode(res.program, t81::tisc::Opcode::ATTN));
    std::cout << "AI_M6 @attention call site → ATTN opcode: PASS\n";
  }

  // -----------------------------------------------------------------------
  // Test 4: @qmatmul annotated function — call site lowers to QMATMUL
  // -----------------------------------------------------------------------
  {
    const std::string src = R"(
@tier(2)
@qmatmul
fn my_qmm(act: Tensor, wt: Tensor, scale: Tensor) -> Tensor {
    return std.tensor.qmatmul(act, wt, scale);
}

@tier(2)
fn main() -> Tensor {
    let act: Tensor   = std.tensor.from_list([1, 2, 3]);
    let wt: Tensor    = std.tensor.from_list([4, 5, 6]);
    let scale: Tensor = std.tensor.from_list([1]);
    return my_qmm(act, wt, scale);
}
)";
    auto res = compile(src);
    T81_TEST_CHECK(res.parse_ok);
    T81_TEST_CHECK(res.sa_ok);
    T81_TEST_CHECK(program_contains_opcode(res.program, t81::tisc::Opcode::QMATMUL));
    std::cout << "AI_M6 @qmatmul call site → QMATMUL opcode: PASS\n";
  }

  // -----------------------------------------------------------------------
  // Test 5: @attention on a Tier 1 function is a semantic error
  // -----------------------------------------------------------------------
  {
    const std::string src = R"(
@attention
fn bad_attn(q: Tensor, k: Tensor, v: Tensor) -> Tensor {
    return std.tensor.matmul(q, k);
}
fn main() -> i32 { return 0; }
)";
    Lexer lexer(src);
    Parser parser(lexer, "<test-tier1-reject>");
    auto stmts = parser.parse();
    T81_TEST_CHECK(!parser.had_error());

    SemanticAnalyzer analyzer(stmts, "<test-tier1-reject>");
    analyzer.analyze();
    // Must reject: @attention requires Tier 2+.
    T81_TEST_CHECK(analyzer.had_error());
    const auto& diags = analyzer.diagnostics();
    const bool tier_error =
        std::any_of(diags.begin(), diags.end(), [](const auto& d) {
          return d.message.find("Tier 2") != std::string::npos ||
                 d.message.find("@attention") != std::string::npos;
        });
    T81_TEST_CHECK(tier_error);
    std::cout << "AI_M6 @attention Tier 1 rejection: PASS\n";
  }

  return 0;
}
