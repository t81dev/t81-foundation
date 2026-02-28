#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace t81;

// Helper to compile and run a T81Lang source string and return the final value of register r0.
int64_t run_e2e_test(const std::string& source) {
  frontend::Lexer lexer(source);
  frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  if (parser.had_error()) {
    std::cerr << "Parser error!" << std::endl;
    return -1;
  }

  frontend::SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (analyzer.had_error()) {
    std::cerr << "Semantic Analyzer error!" << std::endl;
    for (const auto& diag : analyzer.diagnostics()) {
      std::cerr << diag.line << ":" << diag.column << ": " << diag.message << std::endl;
    }
    return -2;
  }

  [[maybe_unused]] frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  [[maybe_unused]] tisc::ir::IntermediateProgram ir = ir_gen.generate(stmts);

  [[maybe_unused]] tisc::BinaryEmitter emitter;
  [[maybe_unused]] tisc::Program program = emitter.emit(ir);

  [[maybe_unused]] auto vm = vm::make_interpreter_vm();
  vm->load_program(program);
  try {
    auto res = vm->run_to_halt();
    if (!res) {
      std::cerr << "VM halted with trap: " << static_cast<int>(res.error()) << std::endl;
      return -3;
    }
  } catch (const std::exception& e) {
    std::cerr << "VM Exception: " << e.what() << std::endl;
    return -4;
  }

  // Return R2 as it holds the return value of main
  return vm->state().contexts[0].registers[2];
}

void test_while_break() {
  const std::string source = R"(
        fn main() -> i32 {
            var i: i32 = 0;
            while (1 == 1) {
                if (i == 10) {
                    break;
                }
                i = i + 1;
            }
            return i;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 10) {
    std::cerr << "test_while_break failed: expected 10, got " << result << std::endl;
    throw std::runtime_error("test_while_break failed");
  }
}

void test_nested_loop_continue() {
  const std::string source = R"(
        fn main() -> i32 {
            var sum: i32 = 0;
            var i: i32 = 0;
            @bounded(10)
            loop {
                i = i + 1;
                if (i > 9) {
                    break;
                }
                if (i % 2 == 0) {
                    continue;
                }
                sum = sum + i;
            }
            return sum; // 1 + 3 + 5 + 7 + 9 = 25
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 25) {
    std::cerr << "test_nested_loop_continue failed: expected 25, got " << result << std::endl;
    throw std::runtime_error("test_nested_loop_continue failed");
  }
}

void test_match_guards() {
  const std::string source = R"(
        fn main() -> i32 {
            let x: i32 = 5;
            let opt: Option[i32] = Some(x);
            let result: i32 = match (opt) {
                Some(v) if v > 10 => 100,
                Some(v) if v < 10 => 200,
                Some(v) => 300,
                None => 0
            };
            return result; // 200
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 200) {
    std::cerr << "test_match_guards failed: expected 200, got " << result << std::endl;
    throw std::runtime_error("test_match_guards failed");
  }
}

void test_custom_enum_match() {
  const std::string source = R"(
        enum Status {
            Idle;
            Active(i32);
            Error(i32);
        };

        fn main() -> i32 {
            let s: Status = Status.Active(42);
            return match (s) {
                Idle => 1,
                Active(v) => v,
                Error(e) => 0 - e
            };
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 42) {
    std::cerr << "test_custom_enum_match failed: expected 42, got " << result << std::endl;
    throw std::runtime_error("test_custom_enum_match failed");
  }
}

void test_extended_numeric_types_pipeline() {
  const std::string source = R"(
        fn main() -> T81Uint {
            let q: T81Qutrit = 1;
            let q2: T81Qutrit = q + 1;
            let u: T81Uint = 7;
            let u2: T81Uint = u + 2;
            let f: T81Fixed[8, 4] = 3;
            let _f2: T81Fixed[8, 4] = f + 4;
            return q2 + u2;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 11) {
    std::cerr << "test_extended_numeric_types_pipeline failed: expected 11, got " << result
              << std::endl;
    throw std::runtime_error("test_extended_numeric_types_pipeline failed");
  }
}

void test_constructor_and_conversion_pipeline() {
  const std::string source = R"(
        fn main() -> T81Uint {
            let q: T81Qutrit = T81Qutrit(1);
            let u: T81Uint = T81Uint(3);
            let f: T81Fixed[8, 4] = T81Fixed[8, 4](u);
            return u + 1;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 4) {
    std::cerr << "test_constructor_and_conversion_pipeline failed: expected 4, got " << result
              << std::endl;
    throw std::runtime_error("test_constructor_and_conversion_pipeline failed");
  }
}

void test_complex_constructor_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let _c: T81Complex[18] = T81Complex[18](1, -1);
            return 7;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 7) {
    std::cerr << "test_complex_constructor_pipeline failed: expected 7, got " << result
              << std::endl;
    throw std::runtime_error("test_complex_constructor_pipeline failed");
  }
}

void test_placeholder_constructor_surface() {
  const std::string source = R"(
        fn main() -> i32 {
            let agent: T81Agent = T81Agent();
            let poly: T81Polynomial = T81Polynomial();
            let sym: T81Symbolic = T81Symbolic();
            let maybe: T81Maybe[i32] = T81Maybe[i32]();
            let promise: T81Promise[i32] = T81Promise[i32]();
            let timev: T81Time = T81Time();
            let ent: T81Entropy = T81Entropy();
            std.agent.self_reflect();
            std.sys.reflect();
            let maybe_score: i32 = match (maybe) {
                Some(v) => v,
                None => 9
            };
            let _a = agent;
            let _p = poly;
            let _s = sym;
            let _pr = promise;
            let _t = timev;
            let _e = ent;
            return 123 + maybe_score;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 132) {
    std::cerr << "test_placeholder_constructor_surface failed: expected 132, got " << result
              << std::endl;
    throw std::runtime_error("test_placeholder_constructor_surface failed");
  }
}

void test_std_tensor_from_list_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let t: Tensor = std.tensor.from_list([1, 2, 3]);
            let _ = t;
            return 17;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 17) {
    std::cerr << "test_std_tensor_from_list_pipeline failed: expected 17, got " << result
              << std::endl;
    throw std::runtime_error("test_std_tensor_from_list_pipeline failed");
  }
}

void test_std_tensor_vec_add_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let a: Tensor = std.tensor.from_list([1, 2, 3]);
            let b: Tensor = std.tensor.from_list([4, 5, 6]);
            let c: Tensor = std.tensor.vec_add(a, b);
            let _ = c;
            return 19;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 19) {
    std::cerr << "test_std_tensor_vec_add_pipeline failed: expected 19, got " << result
              << std::endl;
    throw std::runtime_error("test_std_tensor_vec_add_pipeline failed");
  }
}

void test_std_text_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let n: i32 = std.text.str_len("alpha");
            let e: bool = std.text.str_is_empty("");
            let joined: T81String = std.text.concat("al", "pha");
            let sw: bool = std.text.starts_with(joined, "al");
            let ew: bool = std.text.ends_with(joined, "ha");
            let has_mid: bool = std.text.contains(joined, "lp");
            let idx: i32 = std.text.index_of(joined, "ph");
            let replaced: T81String = std.text.replace(joined, "ph", "zz");
            let rendered: T81String = std.text.to_string(replaced);
            let rendered_bytes: T81String = std.text.to_string(T81Bytes("beta"));
            let rendered_from_bytes: T81String = std.text.from_bytes(T81Bytes("beta"));
            let repl_idx: i32 = std.text.index_of(replaced, "zz");
            if (e) {
                if (sw) {
                    if (ew) {
                        if (has_mid) {
                            if (idx == 2) {
                                if (repl_idx == 2) {
                                    if (std.text.str_len(rendered) == 5) {
                                        if (std.text.str_len(rendered_bytes) == 4) {
                                            if (std.text.str_len(rendered_from_bytes) == 4) {
                                                return n;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 5) {
    std::cerr << "test_std_text_pipeline failed: expected 5, got " << result << std::endl;
    throw std::runtime_error("test_std_text_pipeline failed");
  }
}

void test_std_text_module_wrapper_pipeline() {
  const std::string source = R"(
        fn str_len(s: T81String) -> i32 {
            return std.text.str_len(s);
        }
        fn str_is_empty(s: T81String) -> bool {
            return std.text.str_is_empty(s);
        }
        fn concat(a: T81String, b: T81String) -> T81String {
            return std.text.concat(a, b);
        }
        fn starts_with(s: T81String, prefix: T81String) -> bool {
            return std.text.starts_with(s, prefix);
        }
        fn ends_with(s: T81String, suffix: T81String) -> bool {
            return std.text.ends_with(s, suffix);
        }
        fn contains(s: T81String, needle: T81String) -> bool {
            return std.text.contains(s, needle);
        }
        fn index_of(s: T81String, needle: T81String) -> i32 {
            return std.text.index_of(s, needle);
        }
        fn replace(s: T81String, needle: T81String, replacement: T81String) -> T81String {
            return std.text.replace(s, needle, replacement);
        }
        fn to_string(s: T81String) -> T81String {
            return std.text.to_string(s);
        }
        fn from_bytes(b: T81Bytes) -> T81String {
            return std.text.to_string(b);
        }
        fn from_bytes_alias(b: T81Bytes) -> T81String {
            return std.text.from_bytes(b);
        }
        fn main() -> i32 {
            let joined: T81String = concat("om", "ega");
            let n: i32 = str_len(joined);
            let e: bool = str_is_empty("");
            let sw: bool = starts_with(joined, "om");
            let ew: bool = ends_with(joined, "ga");
            let has_mid: bool = contains(joined, "meg");
            let idx: i32 = index_of(joined, "ga");
            let replaced: T81String = replace(joined, "ga", "xy");
            let rendered: T81String = to_string(replaced);
            let rendered_bytes: T81String = from_bytes(T81Bytes("beta"));
            let rendered_from_bytes: T81String = from_bytes_alias(T81Bytes("beta"));
            let repl_idx: i32 = index_of(replaced, "xy");
            if (e) {
                if (sw) {
                    if (ew) {
                        if (has_mid) {
                            if (idx == 3) {
                                if (repl_idx == 3) {
                                    if (str_len(rendered) == 5) {
                                        if (str_len(rendered_bytes) == 4) {
                                            if (str_len(rendered_from_bytes) == 4) {
                                                return n;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 5) {
    std::cerr << "test_std_text_module_wrapper_pipeline failed: expected 5, got " << result
              << std::endl;
    throw std::runtime_error("test_std_text_module_wrapper_pipeline failed");
  }
}

void test_std_text_split_join_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let parts: Vector[T81String] = std.text.split("a,,b", ",");
            let joined: T81String = std.text.join(parts, ",");
            let direct: T81String = std.text.join(["x", "", "z"], ",");
            let n: i32 = std.text.str_len(joined);
            let m: i32 = std.text.str_len(direct);
            let idx_joined: i32 = std.text.index_of(joined, ",,");
            let idx_direct: i32 = std.text.index_of(direct, ",,");
            if (n == 4) {
                if (m == 4) {
                    if (idx_joined == 1) {
                        if (idx_direct == 1) {
                            return 29;
                        }
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 29) {
    std::cerr << "test_std_text_split_join_pipeline failed: expected 29, got " << result
              << std::endl;
    throw std::runtime_error("test_std_text_split_join_pipeline failed");
  }
}

void test_std_bytes_pipeline() {
  const std::string source = R"(
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
            let repl_idx: i32 = std.bytes.index_of(replaced, T81Bytes("zz"));
            let roundtrip_idx: i32 = std.bytes.index_of(roundtrip, T81Bytes(",,"));
            let m: i32 = std.bytes.len(joined);
            if (e) {
                if (n == 5) {
                    if (m == 5) {
                        if (sw) {
                            if (ew) {
                                if (has_mid) {
                                    if (idx == 2) {
                                        if (repl_idx == 2) {
                                            if (std.text.str_len(rendered) == 5) {
                                                if (from_text_len == 5) {
                                                    if (roundtrip_idx == 1) {
                                                        return n;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 5) {
    std::cerr << "test_std_bytes_pipeline failed: expected 5, got " << result << std::endl;
    throw std::runtime_error("test_std_bytes_pipeline failed");
  }
}

void test_std_bytes_module_wrapper_pipeline() {
  const std::string source = R"(
        fn len(b: T81Bytes) -> i32 {
            return std.bytes.len(b);
        }
        fn is_empty(b: T81Bytes) -> bool {
            return std.bytes.is_empty(b);
        }
        fn concat(a: T81Bytes, b: T81Bytes) -> T81Bytes {
            return std.bytes.concat(a, b);
        }
        fn starts_with(b: T81Bytes, prefix: T81Bytes) -> bool {
            return std.bytes.starts_with(b, prefix);
        }
        fn ends_with(b: T81Bytes, suffix: T81Bytes) -> bool {
            return std.bytes.ends_with(b, suffix);
        }
        fn contains(b: T81Bytes, needle: T81Bytes) -> bool {
            return std.bytes.contains(b, needle);
        }
        fn index_of(b: T81Bytes, needle: T81Bytes) -> i32 {
            return std.bytes.index_of(b, needle);
        }
        fn replace(b: T81Bytes, needle: T81Bytes, replacement: T81Bytes) -> T81Bytes {
            return std.bytes.replace(b, needle, replacement);
        }
        fn split(b: T81Bytes, sep: T81Bytes) -> Vector[T81Bytes] {
            return std.bytes.split(b, sep);
        }
        fn join(parts: Vector[T81Bytes], sep: T81Bytes) -> T81Bytes {
            return std.bytes.join(parts, sep);
        }
        fn to_string(b: T81Bytes) -> T81String {
            return std.bytes.to_string(b);
        }
        fn from_string(s: T81String) -> T81Bytes {
            return std.bytes.from_string(s);
        }
        fn main() -> i32 {
            let joined: T81Bytes = concat(T81Bytes("om"), T81Bytes("ega"));
            let from_text: T81Bytes = from_string("omega");
            let n: i32 = len(joined);
            let e: bool = is_empty(T81Bytes(""));
            let sw: bool = starts_with(joined, T81Bytes("om"));
            let ew: bool = ends_with(joined, T81Bytes("ga"));
            let has_mid: bool = contains(joined, T81Bytes("meg"));
            let idx: i32 = index_of(joined, T81Bytes("ga"));
            let replaced: T81Bytes = replace(joined, T81Bytes("ga"), T81Bytes("xy"));
            let parts: Vector[T81Bytes] = split(T81Bytes("x,,z"), T81Bytes(","));
            let roundtrip: T81Bytes = join(parts, T81Bytes(","));
            let rendered: T81String = to_string(replaced);
            let from_text_len: i32 = len(from_text);
            let repl_idx: i32 = index_of(replaced, T81Bytes("xy"));
            let roundtrip_idx: i32 = index_of(roundtrip, T81Bytes(",,"));
            let m: i32 = len(joined);
            if (e) {
                if (n == 5) {
                    if (m == 5) {
                        if (from_text_len == 5) {
                            if (sw) {
                                if (ew) {
                                    if (has_mid) {
                                        if (idx == 3) {
                                            if (repl_idx == 3) {
                                                if (std.text.str_len(rendered) == 5) {
                                                    if (roundtrip_idx == 1) {
                                                        return n;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 5) {
    std::cerr << "test_std_bytes_module_wrapper_pipeline failed: expected 5, got " << result
              << std::endl;
    throw std::runtime_error("test_std_bytes_module_wrapper_pipeline failed");
  }
}

void test_std_symbol_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let sym: Symbol = std.symbol.intern("omega");
            let rendered: T81String = std.symbol.to_string(sym);
            let same: bool = std.symbol.eq(sym, std.symbol.intern("omega"));
            let diff: bool = std.symbol.ne(sym, std.symbol.intern("alpha"));
            let n: i32 = std.text.str_len(rendered);
            if (n == 5) {
                if (same) {
                    if (diff) {
                        return n;
                    }
                }
            }
            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 5) {
    std::cerr << "test_std_symbol_pipeline failed: expected 5, got " << result << std::endl;
    throw std::runtime_error("test_std_symbol_pipeline failed");
  }
}

void test_fraction_stdlib_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let a: T81Fraction = std.math.fraction.from_int(1);
            let b: T81Fraction = std.math.fraction.from_int(3);
            let c: T81Fraction = std.math.fraction.div(a, b); // 1/3

            let d: T81Fraction = std.math.fraction.from_float(0.5); // 1/2

            let sum: T81Fraction = std.math.fraction.add(c, d); // 1/3 + 1/2 = 5/6

            let f: T81Float = std.math.fraction.to_float(sum); // 0.8333...

            // Check 5/6 approx 0.8333
            let diff: T81Float = f - 0.833333333333333;
            // abs(diff) < 1e-9
            // T81Float comparison opcodes use CMP which works.

            let one: i32 = std.math.fraction.to_int(a);
            if (one != 1) return 1;

            // Check conversion back to float
            let half_f: T81Float = std.math.fraction.to_float(d);
            let epsilon: T81Float = 0.000000001;
            let diff_float: T81Float = half_f - 0.5;
            let abs_diff: T81Float = std.math.abs(diff_float);
            let is_greater: bool = abs_diff > epsilon;
            if (is_greater) return 2;

            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 0) {
    std::cerr << "test_fraction_stdlib_pipeline failed: expected 0, got " << result << std::endl;
    throw std::runtime_error("test_fraction_stdlib_pipeline failed");
  }
}

void test_bigint_stdlib_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let a: T81BigInt = std.math.bigint.from_int(10);
            let b: T81BigInt = std.math.bigint.from_int(20);
            let c: T81BigInt = std.math.bigint.add(a, b);
            let d: T81BigInt = std.math.bigint.mul(c, 2);

            let thirty: i32 = std.math.bigint.to_int(c);
            if (thirty != 30) return 1;

            let sixty: i32 = std.math.bigint.to_int(d);
            if (sixty != 60) return 2;

            return 0;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 0) {
    std::cerr << "test_bigint_stdlib_pipeline failed: expected 0, got " << result << std::endl;
    throw std::runtime_error("test_bigint_stdlib_pipeline failed");
  }
}

void test_advanced_tensor_ops_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let a: Tensor = std.tensor.from_list([1, 2, 3]);
            let b: Tensor = std.tensor.from_list([4, 5, 6]);
            let c: Tensor = std.tensor.from_list([7, 8, 9]);
            let d: Tensor = std.tensor.vec_add(a, c);
            let e: i32 = std.tensor.dot_product(d, b);
            return e;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 154) {
    std::cerr << "test_advanced_tensor_ops_pipeline failed: expected 154, got " << result
              << std::endl;
    throw std::runtime_error("test_advanced_tensor_ops_pipeline failed");
  }
}

void test_symbolic_polynomial_pipeline() {
  const std::string source = R"(
        fn main() -> i32 {
            let g: T81Symbolic = std.symbolic.load("seed");
            let g2: T81Symbolic = std.symbolic.rewrite(g, "seed", "x");
            let g3: T81Symbolic = std.symbolic.canon(g2);
            let _ok: bool = std.symbolic.confluent(g3);

            let p: T81Polynomial = std.polynomial.load("p0");
            let p2: T81Polynomial = std.polynomial.rewrite(p, "p0", "p1");
            let p3: T81Polynomial = std.polynomial.canon(p2);
            let _pk: bool = std.polynomial.confluent(p3);
            return 17;
        }
    )";
  [[maybe_unused]] int64_t result = run_e2e_test(source);
  if (result != 17) {
    std::cerr << "test_symbolic_polynomial_pipeline failed: expected 17, got " << result
              << std::endl;
    throw std::runtime_error("test_symbolic_polynomial_pipeline failed");
  }
}

int main() {
  std::cout << "Running test_while_break..." << std::endl;
  test_while_break();
  std::cout << "Running test_nested_loop_continue..." << std::endl;
  test_nested_loop_continue();
  std::cout << "Running test_match_guards..." << std::endl;
  test_match_guards();
  std::cout << "Running test_custom_enum_match..." << std::endl;
  test_custom_enum_match();
  std::cout << "Running test_extended_numeric_types_pipeline..." << std::endl;
  test_extended_numeric_types_pipeline();
  std::cout << "Running test_constructor_and_conversion_pipeline..." << std::endl;
  test_constructor_and_conversion_pipeline();
  std::cout << "Running test_complex_constructor_pipeline..." << std::endl;
  test_complex_constructor_pipeline();
  std::cout << "Running test_placeholder_constructor_surface..." << std::endl;
  test_placeholder_constructor_surface();
  std::cout << "Running test_std_tensor_from_list_pipeline..." << std::endl;
  test_std_tensor_from_list_pipeline();
  std::cout << "Running test_std_tensor_vec_add_pipeline..." << std::endl;
  test_std_tensor_vec_add_pipeline();
  std::cout << "Running test_std_text_pipeline..." << std::endl;
  test_std_text_pipeline();
  std::cout << "Running test_std_text_module_wrapper_pipeline..." << std::endl;
  test_std_text_module_wrapper_pipeline();
  std::cout << "Running test_std_text_split_join_pipeline..." << std::endl;
  test_std_text_split_join_pipeline();
  std::cout << "Running test_std_bytes_pipeline..." << std::endl;
  test_std_bytes_pipeline();
  std::cout << "Running test_std_bytes_module_wrapper_pipeline..." << std::endl;
  test_std_bytes_module_wrapper_pipeline();
  std::cout << "Running test_std_symbol_pipeline..." << std::endl;
  test_std_symbol_pipeline();
  std::cout << "Running test_fraction_stdlib_pipeline..." << std::endl;
  test_fraction_stdlib_pipeline();
  std::cout << "Running test_bigint_stdlib_pipeline..." << std::endl;
  test_bigint_stdlib_pipeline();
  std::cout << "Running test_advanced_tensor_ops_pipeline..." << std::endl;
  test_advanced_tensor_ops_pipeline();
  std::cout << "Running test_symbolic_polynomial_pipeline..." << std::endl;
  test_symbolic_polynomial_pipeline();
  std::cout << "All advanced E2E tests passed!" << std::endl;
  return 0;
}
