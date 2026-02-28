// tests/cpp/frontend_ir_generator_test.cpp
// Robust integration tests for IRGenerator against the current frontend.

#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/ir.hpp"
#include "t81/isa/pretty_printer.hpp"

#include <cassert>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

using namespace t81::frontend;
using namespace t81::tisc::ir;

#define EXPECT(cond, msg)                                   \
  if (!(cond)) {                                            \
    std::cerr << "FAIL: " << msg << " (" << #cond << ")\n"; \
    std::exit(1);                                           \
  }

void test_simple_addition() {
  [[maybe_unused]] std::string source = "let x = 1 + 2;";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions");

  [[maybe_unused]] bool has_loadi = false;
  [[maybe_unused]] bool has_add = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::LOADI) has_loadi = true;
    if (inst.opcode == Opcode::ADD) has_add = true;
  }
  EXPECT(has_loadi, "Expected LOADI");
  EXPECT(has_add, "Expected ADD");

  std::cout << "IRGeneratorTest test_simple_addition passed!" << std::endl;
}

void test_if_statement() {
  [[maybe_unused]] std::string source = "if (1 < 2) { let x = 1; }";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for IfStmt");

  [[maybe_unused]] bool found_cmp = false;
  [[maybe_unused]] bool found_jz = false;
  [[maybe_unused]] bool found_label = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::CMP) found_cmp = true;
    if (inst.opcode == Opcode::JZ) found_jz = true;
    if (inst.opcode == Opcode::LABEL) found_label = true;
  }
  EXPECT(found_cmp, "Expected CMP for if condition");
  EXPECT(found_jz, "Expected JZ for if branch");
  EXPECT(found_label, "Expected LABEL for if end");

  std::cout << "IRGeneratorTest test_if_statement passed!" << std::endl;
}

void test_if_else_statement() {
  [[maybe_unused]] std::string source = "if (1 < 2) { let x = 1; } else { let y = 2; }";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for IfElseStmt");

  [[maybe_unused]] bool found_jz = false;
  [[maybe_unused]] bool found_jmp = false;
  [[maybe_unused]] int labels_count = 0;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::JZ) found_jz = true;
    if (inst.opcode == Opcode::JMP) found_jmp = true;
    if (inst.opcode == Opcode::LABEL) labels_count++;
  }
  EXPECT(found_jz, "Expected JZ for if branch");
  EXPECT(found_jmp, "Expected JMP to skip else branch");
  EXPECT(labels_count >= 2, "Expected at least 2 labels for if-else");

  std::cout << "IRGeneratorTest test_if_else_statement passed!" << std::endl;
}

void test_while_loop() {
  [[maybe_unused]] std::string source = "while (1 < 2) { let x = 1; }";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for WhileStmt");

  [[maybe_unused]] bool found_jz = false;
  [[maybe_unused]] bool found_jmp = false;
  [[maybe_unused]] int labels_count = 0;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::JZ) found_jz = true;
    if (inst.opcode == Opcode::JMP) found_jmp = true;
    if (inst.opcode == Opcode::LABEL) labels_count++;
  }
  EXPECT(found_jz, "Expected JZ for while condition");
  EXPECT(found_jmp, "Expected JMP back to condition");
  EXPECT(labels_count >= 2, "Expected at least 2 labels for while");

  std::cout << "IRGeneratorTest test_while_loop passed!" << std::endl;
}

void test_loop_statement() {
  [[maybe_unused]] std::string source = "@bounded(5) loop { let x = 1; }";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for LoopStmt");

  [[maybe_unused]] bool found_jmp = false;
  [[maybe_unused]] int labels_count = 0;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::JMP) found_jmp = true;
    if (inst.opcode == Opcode::LABEL) labels_count++;
  }
  EXPECT(found_jmp, "Expected JMP back to start of loop");
  EXPECT(labels_count >= 2, "Expected at least 2 labels for loop (entry/exit)");

  std::cout << "IRGeneratorTest test_loop_statement passed!" << std::endl;
}

void test_guarded_loop_statement() {
  [[maybe_unused]] std::string source = "var x = 0; @bounded(loop(x < 5)) loop { x = x + 1; }";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for Guarded LoopStmt");

  [[maybe_unused]] bool found_jz = false;
  [[maybe_unused]] bool found_jmp = false;
  [[maybe_unused]] int labels_count = 0;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::JZ) found_jz = true;
    if (inst.opcode == Opcode::JMP) found_jmp = true;
    if (inst.opcode == Opcode::LABEL) labels_count++;
  }
  EXPECT(found_jz, "Expected JZ for loop guard");
  EXPECT(found_jmp, "Expected JMP back to guard");
  EXPECT(labels_count >= 3, "Expected at least 3 labels for guarded loop (guard/entry/exit)");

  std::cout << "IRGeneratorTest test_guarded_loop_statement passed!" << std::endl;
}

void test_assignment() {
  [[maybe_unused]] std::string source = "let x = 1; x = 2;";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);

  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "Assignment should produce IR");

  [[maybe_unused]] bool has_loadi = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::LOADI) has_loadi = true;
  }
  EXPECT(has_loadi, "Expected LOADI");

  std::cout << "IRGeneratorTest test_assignment passed!" << std::endl;
}

void test_match_option() {
  std::string source = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(1);
            let v: i32 = match (maybe) {
                Some(x) => x + 1;
                None => 0;
            };
            return v;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "IRGenerator produced no instructions for match");

  [[maybe_unused]] bool has_option_is_some = false;
  [[maybe_unused]] bool has_option_unwrap = false;
  [[maybe_unused]] bool has_branch = false;
  [[maybe_unused]] bool has_jump = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::OPTION_IS_SOME) has_option_is_some = true;
    if (inst.opcode == Opcode::OPTION_UNWRAP) has_option_unwrap = true;
    if (inst.opcode == Opcode::JNZ) has_branch = true;
    if (inst.opcode == Opcode::JMP) has_jump = true;
  }

  EXPECT(has_option_is_some, "Option match should emit OPTION_IS_SOME");
  EXPECT(has_option_unwrap, "Option match should unwrap payload");
  EXPECT(has_branch, "Option match should branch");
  EXPECT(has_jump, "Option match should jump to end");

  std::cout << "IRGeneratorTest test_match_option passed!" << std::endl;
}

void test_match_result() {
  std::string source = R"(
        fn main() -> Result[i32, T81String] {
            let result: Result[i32, T81String] = Ok(1);
            return match (result) {
                Ok(x) => Ok(x + 1);
                Err(e) => Err(e);
            };
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  [[maybe_unused]] IRGenerator generator;
  [[maybe_unused]] auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "Result match should produce IR");

  [[maybe_unused]] bool has_result_is_ok = false;
  [[maybe_unused]] bool has_result_unwrap_ok = false;
  [[maybe_unused]] bool has_result_unwrap_err = false;
  [[maybe_unused]] bool has_branch = false;
  [[maybe_unused]] bool has_jump = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::RESULT_IS_OK) has_result_is_ok = true;
    if (inst.opcode == Opcode::RESULT_UNWRAP_OK) has_result_unwrap_ok = true;
    if (inst.opcode == Opcode::RESULT_UNWRAP_ERR) has_result_unwrap_err = true;
    if (inst.opcode == Opcode::JNZ) has_branch = true;
    if (inst.opcode == Opcode::JMP) has_jump = true;
  }

  EXPECT(has_result_is_ok, "Result match should emit RESULT_IS_OK");
  EXPECT(has_result_unwrap_ok, "Result match should unwrap Ok payload");
  EXPECT(has_result_unwrap_err, "Result match should unwrap Err payload");
  EXPECT(has_branch, "Result match should branch");
  EXPECT(has_jump, "Result match should jump to end");

  std::cout << "IRGeneratorTest test_match_result passed!" << std::endl;
}

void test_print_builtin_lowers_to_print_opcode() {
  std::string source = "print(1);";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();

  IRGenerator generator;
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  EXPECT(!instructions.empty(), "print should produce IR");

  bool has_print = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::PRINT) {
      has_print = true;
      break;
    }
  }
  EXPECT(has_print, "Expected PRINT opcode for print(...)");

  std::cout << "IRGeneratorTest test_print_builtin_lowers_to_print_opcode passed!" << std::endl;
}

void test_extended_numeric_types_lower_to_arithmetic_ir() {
  std::string source = R"(
        fn main() -> T81Uint {
            let q: T81Qutrit = 1;
            let q2: T81Qutrit = q + 1;
            let u: T81Uint = 7;
            let u2: T81Uint = u + 2;
            let f: T81Fixed[8, 4] = 3;
            let f2: T81Fixed[8, 4] = f + 4;
            var c1: T81Complex[18];
            var c2: T81Complex[18];
            c2 = c1 + c1;
            return q2 + u2;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for extended numeric types fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for extended numeric types fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "extended numeric fixture produced no IR");

  bool has_add = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::ADD) {
      has_add = true;
      break;
    }
  }
  EXPECT(has_add, "extended numeric fixture should lower arithmetic via ADD");
  std::cout << "IRGeneratorTest test_extended_numeric_types_lower_to_arithmetic_ir passed!"
            << std::endl;
}

void test_constructor_and_conversion_calls_lower() {
  std::string source = R"(
        fn main() -> T81Uint {
            let q: T81Qutrit = T81Qutrit(1);
            let u: T81Uint = T81Uint(3);
            let f: T81Fixed[8, 4] = T81Fixed[8, 4](u);
            let _c: T81Complex[18] = T81Complex[18](u, q);
            return u + 1;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for constructor/conversion fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for constructor/conversion fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "constructor/conversion fixture produced no IR");

  bool has_add = false;
  bool has_make_complex = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::ADD) {
      has_add = true;
    }
    if (inst.opcode == Opcode::MAKE_COMPLEX) {
      has_make_complex = true;
    }
  }
  EXPECT(has_add, "constructor/conversion fixture should include ADD");
  EXPECT(has_make_complex, "constructor/conversion fixture should include MAKE_COMPLEX");
  std::cout << "IRGeneratorTest test_constructor_and_conversion_calls_lower passed!" << std::endl;
}

void test_generic_call_with_explicit_type_args_lowers_to_call() {
  std::string source = R"(
        fn id[T](x: T) -> T {
            return x;
        }
        fn main() -> i32 {
            let v: i32 = id[i32](7);
            return v;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for explicit generic call lowering fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(),
         "semantic analyzer failed for explicit generic call lowering fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "explicit generic call lowering fixture produced no IR");

  int call_count = 0;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::CALL) {
      ++call_count;
    }
  }
  EXPECT(call_count >= 2, "Expected CALL for startup main and explicit generic function call");
  std::cout << "IRGeneratorTest test_generic_call_with_explicit_type_args_lowers_to_call passed!"
            << std::endl;
}

void test_std_namespace_aliases_lower_to_builtin_opcodes() {
  std::string source = R"(
        fn main() -> i32 {
            let angle: T81Float = std.math.sin(1.0);
            let c: T81Float = std.math.cos(angle);
            let t: T81Float = std.math.tan(c);
            let asn: T81Float = std.math.asin(0.5);
            let acs: T81Float = std.math.acos(0.5);
            let atn: T81Float = std.math.atan(1.0);
            let sh: T81Float = std.math.sinh(1.0);
            let ch: T81Float = std.math.cosh(1.0);
            let th: T81Float = std.math.tanh(1.0);
            let root: T81Float = std.math.sqrt(4.0);
            let ex: T81Float = std.math.exp(1.0);
            let lg: T81Float = std.math.log(ex);
            let pw: T81Float = std.math.pow(2.0, 8.0);
            let clamped: T81Float = std.math.clamp(-2.0, 0.0, 1.0);
            let n: i32 = std.text.str_len("hello");
            let e: bool = std.text.str_is_empty("");
            let joined: T81String = std.text.concat("he", "llo");
            let sw: bool = std.text.starts_with(joined, "he");
            let ew: bool = std.text.ends_with(joined, "lo");
            let has_mid: bool = std.text.contains(joined, "ell");
            let idx: i32 = std.text.index_of(joined, "ll");
            let replaced: T81String = std.text.replace(joined, "ll", "yy");
            let rendered: T81String = std.text.to_string(replaced);
            let rendered_bytes: T81String = std.text.to_string(T81Bytes("beta"));
            let rendered_from_bytes: T81String = std.text.from_bytes(T81Bytes("beta"));
            let parts: Vector[T81String] = std.text.split("a,,b", ",");
            let joined_again: T81String = std.text.join(parts, ",");
            let joined_literal: T81String = std.text.join(["x", "y"], "-");
            let int_parts: Vector[i32] = [1, 2, 3];
            let part_count: i32 = std.collections.len(int_parts);
            let part_empty: bool = std.collections.is_empty(int_parts);
            let part_first: i32 = std.collections.first(int_parts);
            let part_last: i32 = std.collections.last(int_parts);
            let part_pushed: Vector[i32] = std.collections.push(int_parts, 4);
            let part_popped: Vector[i32] = std.collections.pop(part_pushed);
            let str_part_count: i32 = std.collections.len(parts);
            let str_part_empty: bool = std.collections.is_empty(parts);
            let str_first: T81String = std.collections.first(parts);
            let str_last: T81String = std.collections.last(parts);
            let str_pushed: Vector[T81String] = std.collections.push(parts, "tail");
            let str_popped: Vector[T81String] = std.collections.pop(str_pushed);
            let sym: Symbol = std.symbol.intern("omega");
            let rendered_sym: T81String = std.symbol.to_string(sym);
            let s_omega: Symbol = std.symbol.intern("omega");
            let s_alpha: Symbol = std.symbol.intern("alpha");
            let same: bool = std.symbol.eq(sym, s_omega);
            let diff: bool = std.symbol.ne(sym, s_alpha);
            std.core.assert(1 < 2);
            std.core.debug("debug");
            let maybe: Option[i32] = Some(7);
            let kept: i32 = std.core.unwrap_or(maybe, 9);
            let now: T81Float = std.sys.time();
            let entropy: i32 = std.sys.entropy();
            let proof: T81String = std.sys.proof();
            let stream_h: T81String = std.io.stream();
            let net_h: T81String = std.io.net();
            std.async.yield();
            std.async.sleep(now);
            let thread_h: T81String = std.async.thread();
            let promise_h: T81String = std.async.promise();
            let list_v: List[T81String] = std.collections.list();
            let map_v: Map[T81String, T81String] = std.collections.map();
            let map_flat: Map[T81String, T81String] = std.collections.map();
            let _mp1: Map[T81String, T81String] = std.collections.map_put(map_flat, "city", "sf");
            let set_v: Set[T81String] = std.collections.set();
            let tree_v: Tree[T81String] = std.collections.tree();
            let graph_v: Graph[T81String] = std.collections.graph();
            std.agent.self_reflect();
            std.sys.exit(0);
            std.io.println("hello");
            std.io.print_int(7);
            std.io.print_float(t);
            let model: i32 = std.tensor.load("encoder.weight");
            let _ = angle;
            let _n = n;
            let _e = e;
            let _t = t;
            let _asn = asn;
            let _acs = acs;
            let _atn = atn;
            let _sh = sh;
            let _ch = ch;
            let _th = th;
            let _rt = root;
            let _ex = ex;
            let _lg = lg;
            let _pw = pw;
            let _cl = clamped;
            let _m = model;
            let _sw = sw;
            let _ew = ew;
            let _hm = has_mid;
            let _idx = idx;
            let _rp = replaced;
            let _rd = rendered;
            let _rdb = rendered_bytes;
            let _rdfb = rendered_from_bytes;
            let _ja = joined_again;
            let _jl = joined_literal;
            let _pc = part_count;
            let _pe = part_empty;
            let _pf = part_first;
            let _pl = part_last;
            let _pp = part_popped;
            let _spc = str_part_count;
            let _spe = str_part_empty;
            let _sf = str_first;
            let _sl = str_last;
            let _spp = str_popped;
            let _sym = sym;
            let _rs = rendered_sym;
            let _same = same;
            let _diff = diff;
            let _kp = kept;
            let _entropy = entropy;
            let _proof = proof;
            std.sys.reflect();
            let _stream_h = stream_h;
            let _net_h = net_h;
            let _thread_h = thread_h;
            let _promise_h = promise_h;
            // Removed len checks for list/map/set because new analyzer enforces strict Vector[T] for len
            let _map_pairs = std.collections.map_size(map_v);
            let _map_has_city = std.collections.map_has(map_v, "city");
            let map_updated: Map[T81String, T81String] = std.collections.map_put(map_flat, "city", "oakland");
            let map_keys: Vector[T81String] = std.collections.map_keys(map_updated);
            let map_removed: Map[T81String, T81String] = std.collections.map_remove(map_updated, "lang");
            let map_lookup: Option[T81String] = std.collections.map_get(map_removed, "city");
            let _map_keys_len = std.collections.len(map_keys);
            let _map_removed_pairs = std.collections.map_size(map_removed);
            let _map_lookup_has = std.collections.map_has(map_removed, "city");
            let _map_lookup_value = match (map_lookup) {
                Some(v) => v;
                None => "none";
            };
            let set_flat: Set[T81String] = std.collections.set();
            let set_added: Set[T81String] = std.collections.set_add(set_flat, "edge");
            let set_added_dup: Set[T81String] = std.collections.set_add(set_added, "city");
            let set_removed: Set[T81String] = std.collections.set_remove(set_added_dup, "lang");
            let _set_size = std.collections.set_size(set_v);
            let _set_has_city = std.collections.set_has(set_v, "city");
            let _set_added_size = std.collections.set_size(set_added);
            let _set_added_dup_size = std.collections.set_size(set_added_dup);
            let _set_removed_size = std.collections.set_size(set_removed);
            let _set_removed_has_lang = std.collections.set_has(set_removed, "lang");
            let _tree_h = 0;
            let _graph_h = std.collections.graph_edge_count(graph_v);
            let graph_edges: Graph[T81String] = std.collections.graph_add_edge(graph_v, "a", "b");
            let graph_edges_dup: Graph[T81String] = std.collections.graph_add_edge(graph_edges, "a", "b");
            let graph_edges_removed: Graph[T81String] = std.collections.graph_remove_edge(graph_edges_dup, "a", "b");
            let graph_neighbors_b: Vector[T81String] = std.collections.graph_neighbors(graph_edges_dup, "b");
            let _graph_edge_count = std.collections.graph_edge_count(graph_edges_dup);
            let _graph_edge_count_removed = std.collections.graph_edge_count(graph_edges_removed);
            let _graph_has_ab = std.collections.graph_has_edge(graph_edges_dup, "a", "b");
            let _graph_has_ab_removed = std.collections.graph_has_edge(graph_edges_removed, "a", "b");
            let _graph_has_ba = std.collections.graph_has_edge(graph_edges_dup, "b", "a");
            let _graph_neighbors_b_len = std.collections.len(graph_neighbors_b);
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std namespace alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std namespace alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std namespace alias fixture produced no IR");

  bool has_fsin = false;
  bool has_fcos = false;
  bool has_ftan = false;
  bool has_fasin = false;
  bool has_facos = false;
  bool has_fatan = false;
  bool has_fsinh = false;
  bool has_fcosh = false;
  bool has_ftanh = false;
  bool has_fsqrt = false;
  bool has_fexp = false;
  bool has_flog = false;
  bool has_fpow = false;
  bool has_strlen = false;
  bool has_strempty = false;
  bool has_strconcat = false;
  bool has_strstartswith = false;
  bool has_strendswith = false;
  bool has_strcontains = false;
  bool has_strindexof = false;
  bool has_strreplace = false;
  bool has_strsplit = false;
  bool has_strjoin = false;
  bool has_strvecnew = false;
  bool has_strvecpush = false;
  bool has_veclen = false;
  bool has_vecempty = false;
  bool has_vecfirst = false;
  bool has_veclast = false;
  bool has_vecpush = false;
  bool has_vecpop = false;
  bool has_cmp = false;
  bool has_print = false;
  bool has_trap = false;
  bool has_meta_reflect = false;
  bool has_option_is_some = false;
  bool has_option_unwrap = false;
  bool has_weights_load = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::FSIN) {
      has_fsin = true;
    } else if (inst.opcode == Opcode::FCOS) {
      has_fcos = true;
    } else if (inst.opcode == Opcode::FTAN) {
      has_ftan = true;
    } else if (inst.opcode == Opcode::FASIN) {
      has_fasin = true;
    } else if (inst.opcode == Opcode::FACOS) {
      has_facos = true;
    } else if (inst.opcode == Opcode::FATAN) {
      has_fatan = true;
    } else if (inst.opcode == Opcode::FSINH) {
      has_fsinh = true;
    } else if (inst.opcode == Opcode::FCOSH) {
      has_fcosh = true;
    } else if (inst.opcode == Opcode::FTANH) {
      has_ftanh = true;
    } else if (inst.opcode == Opcode::FSQRT) {
      has_fsqrt = true;
    } else if (inst.opcode == Opcode::FEXP) {
      has_fexp = true;
    } else if (inst.opcode == Opcode::FLOG) {
      has_flog = true;
    } else if (inst.opcode == Opcode::FPOW) {
      has_fpow = true;
    } else if (inst.opcode == Opcode::STRLEN) {
      has_strlen = true;
    } else if (inst.opcode == Opcode::STREMPTY) {
      has_strempty = true;
    } else if (inst.opcode == Opcode::STRCONCAT) {
      has_strconcat = true;
    } else if (inst.opcode == Opcode::STRSTARTSWITH) {
      has_strstartswith = true;
    } else if (inst.opcode == Opcode::STRENDSWITH) {
      has_strendswith = true;
    } else if (inst.opcode == Opcode::STRCONTAINS) {
      has_strcontains = true;
    } else if (inst.opcode == Opcode::STRINDEXOF) {
      has_strindexof = true;
    } else if (inst.opcode == Opcode::STRREPLACE) {
      has_strreplace = true;
    } else if (inst.opcode == Opcode::STRSPLIT) {
      has_strsplit = true;
    } else if (inst.opcode == Opcode::STRJOIN) {
      has_strjoin = true;
    } else if (inst.opcode == Opcode::STRVECNEW) {
      has_strvecnew = true;
    } else if (inst.opcode == Opcode::STRVECPUSH) {
      has_strvecpush = true;
    } else if (inst.opcode == Opcode::VECLEN) {
      has_veclen = true;
    } else if (inst.opcode == Opcode::VECEMPTY) {
      has_vecempty = true;
    } else if (inst.opcode == Opcode::VECFIRST) {
      has_vecfirst = true;
    } else if (inst.opcode == Opcode::VECLAST) {
      has_veclast = true;
    } else if (inst.opcode == Opcode::VECPUSH) {
      has_vecpush = true;
    } else if (inst.opcode == Opcode::VECPOP) {
      has_vecpop = true;
    } else if (inst.opcode == Opcode::CMP) {
      has_cmp = true;
    } else if (inst.opcode == Opcode::PRINT) {
      has_print = true;
    } else if (inst.opcode == Opcode::TRAP) {
      has_trap = true;
    } else if (inst.opcode == Opcode::META_REFLECT) {
      has_meta_reflect = true;
    } else if (inst.opcode == Opcode::OPTION_IS_SOME) {
      has_option_is_some = true;
    } else if (inst.opcode == Opcode::OPTION_UNWRAP) {
      has_option_unwrap = true;
    } else if (inst.opcode == Opcode::WEIGHTS_LOAD) {
      has_weights_load = true;
    }
  }

  EXPECT(has_fsin, "std.math.sin should lower to FSIN");
  EXPECT(has_fcos, "std.math.cos should lower to FCOS");
  EXPECT(has_ftan, "std.math.tan should lower to FTAN");
  EXPECT(has_fasin, "std.math.asin should lower to FASIN");
  EXPECT(has_facos, "std.math.acos should lower to FACOS");
  EXPECT(has_fatan, "std.math.atan should lower to FATAN");
  EXPECT(has_fsinh, "std.math.sinh should lower to FSINH");
  EXPECT(has_fcosh, "std.math.cosh should lower to FCOSH");
  EXPECT(has_ftanh, "std.math.tanh should lower to FTANH");
  EXPECT(has_fsqrt, "std.math.sqrt should lower to FSQRT");
  EXPECT(has_fexp, "std.math.exp should lower to FEXP");
  EXPECT(has_flog, "std.math.log should lower to FLOG");
  EXPECT(has_fpow, "std.math.pow should lower to FPOW");
  EXPECT(has_strlen, "std.text.str_len should lower to STRLEN");
  EXPECT(has_strempty, "std.text.str_is_empty should lower to STREMPTY");
  EXPECT(has_strconcat, "std.text.concat should lower to STRCONCAT");
  EXPECT(has_strstartswith, "std.text.starts_with should lower to STRSTARTSWITH");
  EXPECT(has_strendswith, "std.text.ends_with should lower to STRENDSWITH");
  EXPECT(has_strcontains, "std.text.contains should lower to STRCONTAINS");
  EXPECT(has_strindexof, "std.text.index_of should lower to STRINDEXOF");
  EXPECT(has_strreplace, "std.text.replace should lower to STRREPLACE");
  EXPECT(has_strsplit, "std.text.split should lower to STRSPLIT");
  EXPECT(has_strjoin, "std.text.join should lower to STRJOIN");
  EXPECT(has_strvecnew, "Vector[T81String] literal should lower to STRVECNEW");
  EXPECT(has_strvecpush, "Vector[T81String] literal should lower to STRVECPUSH");
  EXPECT(has_veclen, "std.collections.len should lower to VECLEN");
  EXPECT(has_vecempty, "std.collections.is_empty should lower to VECEMPTY");
  EXPECT(has_vecfirst, "std.collections.first should lower to VECFIRST");
  EXPECT(has_veclast, "std.collections.last should lower to VECLAST");
  EXPECT(has_vecpush, "std.collections.push should lower to VECPUSH");
  EXPECT(has_vecpop, "std.collections.pop should lower to VECPOP");
  EXPECT(has_cmp, "std.symbol.eq/ne should lower to CMP");
  EXPECT(has_print, "std.core.debug/std.io.* should lower to PRINT");
  EXPECT(has_trap, "std.core.assert/std.sys.exit should lower to TRAP");
  EXPECT(has_meta_reflect, "std.agent.self_reflect should lower to META_REFLECT");
  EXPECT(has_option_is_some, "std.core.unwrap_or should lower to OPTION_IS_SOME");
  EXPECT(has_option_unwrap, "std.core.unwrap_or should lower to OPTION_UNWRAP");
  EXPECT(has_weights_load, "std.tensor.load should lower to WEIGHTS_LOAD");
  std::cout << "IRGeneratorTest test_std_namespace_aliases_lower_to_builtin_opcodes passed!"
            << std::endl;
}

void test_std_sys_entropy_alias_lowers_to_integer_zero() {
  std::string source = R"(
        fn main() -> i32 {
            let e: i32 = std.sys.entropy();
            return e;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std.sys.entropy alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std.sys.entropy alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std.sys.entropy alias fixture produced no IR");

  bool has_entropy_zero_load = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::LOADI && inst.operands.size() >= 2) {
      if (const auto* imm = std::get_if<t81::tisc::ir::Immediate>(&inst.operands[1]);
          imm != nullptr && imm->value == 0) {
        has_entropy_zero_load = true;
        break;
      }
    }
  }

  EXPECT(has_entropy_zero_load, "std.sys.entropy should lower to deterministic integer zero");
  std::cout << "IRGeneratorTest test_std_sys_entropy_alias_lowers_to_integer_zero passed!"
            << std::endl;
}

void test_std_runtime_handle_aliases_lower_to_deterministic_tokens() {
  std::string source = R"(
        fn main() -> i32 {
            let proof: T81String = std.sys.proof();
            let stream_h: T81String = std.io.stream();
            let net_h: T81String = std.io.net();
            let thread_h: T81String = std.async.thread();
            let promise_h: T81String = std.async.promise();
            let list_v: List[T81String] = std.collections.list();
            let map_v: Map[T81String, T81String] = std.collections.map();
            let set_v: Set[T81String] = std.collections.set();
            let tree_v: Tree[T81String] = std.collections.tree();
            let graph_v: Graph[T81String] = std.collections.graph();
            let _proof = proof;
            let _stream_h = stream_h;
            let _net_h = net_h;
            let _thread_h = thread_h;
            let _promise_h = promise_h;
            // Removed len() on non-Vectors as they are now strictly checked
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std runtime handle alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std runtime handle alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std runtime handle alias fixture produced no IR");

  std::set<std::string> seen_tokens;
  int strvecnew_count = 0;
  bool has_map_new = false;
  bool has_set_new = false;
  for (const auto& inst : instructions) {
    if (inst.opcode != Opcode::LOADI) {
      if (inst.opcode == Opcode::STRVECNEW) {
        ++strvecnew_count;
      } else if (inst.opcode == Opcode::MapNew) {
        has_map_new = true;
      } else if (inst.opcode == Opcode::SetNew) {
        has_set_new = true;
      }
      continue;
    }
    if (inst.literal_kind == t81::tisc::LiteralKind::SymbolHandle &&
        inst.text_literal.has_value()) {
      seen_tokens.insert(*inst.text_literal);
    }
  }

  EXPECT(seen_tokens.count("std.sys.proof") == 1,
         "std.sys.proof should lower to deterministic symbol token");
  EXPECT(seen_tokens.count("std.io.stream") == 1,
         "std.io.stream should lower to deterministic symbol token");
  EXPECT(seen_tokens.count("std.io.net") == 1,
         "std.io.net should lower to deterministic symbol token");
  EXPECT(seen_tokens.count("std.async.thread") == 1,
         "std.async.thread should lower to deterministic symbol token");
  EXPECT(seen_tokens.count("std.async.promise") == 1,
         "std.async.promise should lower to deterministic symbol token");
  EXPECT(strvecnew_count >= 3,
         "std.collections.list/tree/graph should lower to STRVECNEW-backed vector "
         "construction");
  EXPECT(has_map_new, "std.collections.map should lower to MapNew");
  EXPECT(has_set_new, "std.collections.set should lower to SetNew");
  std::cout << "IRGeneratorTest test_std_runtime_handle_aliases_lower_to_deterministic_tokens "
               "passed!"
            << std::endl;
}

void test_std_tensor_from_list_alias_lowers_vector_literal_to_tensor_handle() {
  std::string source = R"(
        fn main() -> i32 {
            let t: Tensor = std.tensor.from_list([1, 2, 3]);
            let _ = t;
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std.tensor.from_list alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std.tensor.from_list alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std.tensor.from_list alias fixture produced no IR");

  bool has_tensor_handle_load = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::LOADI && inst.literal_kind == t81::tisc::LiteralKind::TensorHandle) {
      has_tensor_handle_load = true;
      break;
    }
  }
  EXPECT(has_tensor_handle_load,
         "std.tensor.from_list([..]) should lower vector literal to TensorHandle LOADI");
  std::cout
      << "IRGeneratorTest test_std_tensor_from_list_alias_lowers_vector_literal_to_tensor_handle "
         "passed!"
      << std::endl;
}

void test_std_tensor_matmul_alias_lowers_to_tmatmul() {
  std::string source = R"(
        fn main() -> i32 {
            let a: Tensor = std.tensor.from_list([1, 2, 3]);
            let b: Tensor = std.tensor.from_list([4, 5, 6]);
            let c: Tensor = std.tensor.matmul(a, b);
            let _ = c;
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std.tensor.matmul alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std.tensor.matmul alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std.tensor.matmul alias fixture produced no IR");

  bool has_tmatmul = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::TMATMUL) {
      has_tmatmul = true;
      break;
    }
  }
  EXPECT(has_tmatmul, "std.tensor.matmul(a, b) should lower to TMATMUL");
  std::cout << "IRGeneratorTest test_std_tensor_matmul_alias_lowers_to_tmatmul passed!"
            << std::endl;
}

void test_std_tensor_vec_add_alias_lowers_to_tvecadd() {
  std::string source = R"(
        fn main() -> i32 {
            let a: Tensor = std.tensor.from_list([1, 2, 3]);
            let b: Tensor = std.tensor.from_list([4, 5, 6]);
            let c: Tensor = std.tensor.vec_add(a, b);
            let _ = c;
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std.tensor.vec_add alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std.tensor.vec_add alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std.tensor.vec_add alias fixture produced no IR");

  bool has_tvecadd = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::TVECADD) {
      has_tvecadd = true;
      break;
    }
  }
  EXPECT(has_tvecadd, "std.tensor.vec_add(a, b) should lower to TVECADD");
  std::cout << "IRGeneratorTest test_std_tensor_vec_add_alias_lowers_to_tvecadd passed!"
            << std::endl;
}

void test_std_bytes_aliases_lower_to_string_opcodes() {
  std::string source = R"(
        fn main() -> i32 {
            let alpha: T81Bytes = T81Bytes("alpha");
            let empty: T81Bytes = T81Bytes("");
            let joined: T81Bytes = std.bytes.concat(T81Bytes("al"), T81Bytes("pha"));
            let n: i32 = std.bytes.len(alpha);
            let e: bool = std.bytes.is_empty(empty);
            let sw: bool = std.bytes.starts_with(joined, T81Bytes("al"));
            let ew: bool = std.bytes.ends_with(joined, T81Bytes("ha"));
            let has_mid: bool = std.bytes.contains(joined, T81Bytes("lp"));
            let idx: i32 = std.bytes.index_of(joined, T81Bytes("ph"));
            let replaced: T81Bytes = std.bytes.replace(joined, T81Bytes("ph"), T81Bytes("zz"));
            let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("a,,b"), T81Bytes(","));
            let roundtrip: T81Bytes = std.bytes.join(parts, T81Bytes(","));
            let rendered: T81String = std.bytes.to_string(replaced);
            let from_text: T81Bytes = std.bytes.from_string("alpha");
            let from_text_len: i32 = std.bytes.len(from_text);
            let _n = n;
            let _e = e;
            let _j = joined;
            let _sw = sw;
            let _ew = ew;
            let _hm = has_mid;
            let _idx = idx;
            let _rp = replaced;
            let _rt = roundtrip;
            let _rd = rendered;
            let _ftl = from_text_len;
            return 0;
        }
    )";
  Lexer lexer(source);
  Parser parser(lexer);
  auto stmts = parser.parse();
  EXPECT(!parser.had_error(), "parser failed for std.bytes alias fixture");

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  EXPECT(!analyzer.had_error(), "semantic analyzer failed for std.bytes alias fixture");

  IRGenerator generator;
  generator.attach_semantic_analyzer(&analyzer);
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();
  EXPECT(!instructions.empty(), "std.bytes alias fixture produced no IR");

  bool has_strlen = false;
  bool has_strempty = false;
  bool has_strconcat = false;
  bool has_strstartswith = false;
  bool has_strendswith = false;
  bool has_strcontains = false;
  bool has_strindexof = false;
  bool has_strreplace = false;
  bool has_strsplit = false;
  bool has_strjoin = false;
  for (const auto& inst : instructions) {
    if (inst.opcode == Opcode::STRLEN) {
      has_strlen = true;
    } else if (inst.opcode == Opcode::STREMPTY) {
      has_strempty = true;
    } else if (inst.opcode == Opcode::STRCONCAT) {
      has_strconcat = true;
    } else if (inst.opcode == Opcode::STRSTARTSWITH) {
      has_strstartswith = true;
    } else if (inst.opcode == Opcode::STRENDSWITH) {
      has_strendswith = true;
    } else if (inst.opcode == Opcode::STRCONTAINS) {
      has_strcontains = true;
    } else if (inst.opcode == Opcode::STRINDEXOF) {
      has_strindexof = true;
    } else if (inst.opcode == Opcode::STRREPLACE) {
      has_strreplace = true;
    } else if (inst.opcode == Opcode::STRSPLIT) {
      has_strsplit = true;
    } else if (inst.opcode == Opcode::STRJOIN) {
      has_strjoin = true;
    }
  }

  EXPECT(has_strlen, "std.bytes.len should lower to STRLEN");
  EXPECT(has_strempty, "std.bytes.is_empty should lower to STREMPTY");
  EXPECT(has_strconcat, "std.bytes.concat should lower to STRCONCAT");
  EXPECT(has_strstartswith, "std.bytes.starts_with should lower to STRSTARTSWITH");
  EXPECT(has_strendswith, "std.bytes.ends_with should lower to STRENDSWITH");
  EXPECT(has_strcontains, "std.bytes.contains should lower to STRCONTAINS");
  EXPECT(has_strindexof, "std.bytes.index_of should lower to STRINDEXOF");
  EXPECT(has_strreplace, "std.bytes.replace should lower to STRREPLACE");
  EXPECT(has_strsplit, "std.bytes.split should lower to STRSPLIT");
  EXPECT(has_strjoin, "std.bytes.join should lower to STRJOIN");
  std::cout << "IRGeneratorTest test_std_bytes_aliases_lower_to_string_opcodes passed!"
            << std::endl;
}

int main() {
  test_simple_addition();
  test_if_statement();
  test_if_else_statement();
  test_while_loop();
  test_loop_statement();
  test_guarded_loop_statement();
  test_assignment();
  test_match_option();
  test_match_result();
  test_print_builtin_lowers_to_print_opcode();
  test_extended_numeric_types_lower_to_arithmetic_ir();
  test_constructor_and_conversion_calls_lower();
  test_generic_call_with_explicit_type_args_lowers_to_call();
  test_std_namespace_aliases_lower_to_builtin_opcodes();
  test_std_sys_entropy_alias_lowers_to_integer_zero();
  test_std_runtime_handle_aliases_lower_to_deterministic_tokens();
  test_std_tensor_from_list_alias_lowers_vector_literal_to_tensor_handle();
  test_std_tensor_matmul_alias_lowers_to_tmatmul();
  test_std_tensor_vec_add_alias_lowers_to_tvecadd();
  test_std_bytes_aliases_lower_to_string_opcodes();

  std::cout << "All IRGenerator integration tests completed!" << std::endl;
  return 0;
}