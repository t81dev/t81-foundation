#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

using namespace t81::frontend;

static bool analyzes(std::string_view source, const char* label = "t81lang_conformance_case") {
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) return false;
  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  if (analyzer.had_error()) {
    for (const auto& diag : analyzer.diagnostics()) {
      std::cerr << label << ": " << diag.message << "\n";
    }
  }
  return !analyzer.had_error();
}

static bool fails_semantic(std::string_view source,
                           const char* label = "t81lang_conformance_failure") {
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) return false;
  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  return analyzer.had_error();
}

static bool fails_semantic_with_message(std::string_view source, std::string_view needle,
                                        const char* label = "t81lang_conformance_failure") {
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) return false;
  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  if (!analyzer.had_error()) return false;
  for (const auto& diagnostic : analyzer.diagnostics()) {
    if (diagnostic.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static void require_true(bool condition, const char* label) {
  if (!condition) {
    std::cerr << "Conformance assertion failed: " << label << "\n";
    std::abort();
  }
}

static void test_baseline_supported_features() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let maybe: Option[i32] = Some(1);
      let v: i32 = match (maybe) {
        Some(x) => x;
        None => 0;
      };
      return v;
    }
  )";
  require_true(analyzes(source, "t81lang_supported_baseline"), "t81lang_supported_baseline");
}

static void test_tier_annotation_supported_for_functions() {
  constexpr const char* source = R"(
    @tier(2)
    fn main() -> i32 {
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_tier_annotation_supported"),
               "t81lang_tier_annotation_supported");
}

static void test_t81_numeric_types_bind_and_widen() {
  constexpr const char* source = R"(
    fn main() -> T81Float {
      let a: T81Float = 1.20t81;
      let b: T81BigInt = 12t81;
      let c: T81Float = a + b;
      return c;
    }
  )";
  require_true(analyzes(source, "t81lang_numeric_binding"), "t81lang_numeric_binding");
}

static void test_t81_mixed_numeric_widening_matrix() {
  constexpr const char* bigint_float = R"(
    fn main() -> T81Float {
      let i: T81BigInt = 3t81;
      let f: T81Float = 1.5t81;
      let out: T81Float = i * f;
      return out;
    }
  )";
  require_true(analyzes(bigint_float, "t81lang_widen_bigint_float"), "t81lang_widen_bigint_float");

  constexpr const char* bigint_fraction = R"(
    fn main() -> T81Fraction {
      let i: T81BigInt = 2t81;
      let q: T81Fraction = 22/7t81;
      let out: T81Fraction = i + q;
      return out;
    }
  )";
  require_true(analyzes(bigint_fraction, "t81lang_widen_bigint_fraction"),
               "t81lang_widen_bigint_fraction");
}

static void test_base81_integer_infers_bigint() {
  constexpr const char* source = R"(
    fn main() -> T81BigInt {
      let x = 12t81;
      let y: T81BigInt = x;
      return y;
    }
  )";
  require_true(analyzes(source, "t81lang_base81_integer_infers_bigint"),
               "t81lang_base81_integer_infers_bigint");
}

static void test_base81_integer_does_not_silently_narrow_to_i32() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let x = 12t81;
      let y: i32 = x;
      return y;
    }
  )";
  require_true(fails_semantic(source, "t81lang_base81_integer_no_narrow"),
               "t81lang_base81_integer_no_narrow");
}

static void test_t81_numeric_type_separation_rejects_invalid_mix() {
  constexpr const char* source = R"(
    fn main() -> i8 {
      let x: i2 = 1;
      return x + 1.5;
    }
  )";
  require_true(fails_semantic(source, "t81lang_numeric_type_separation"),
               "t81lang_numeric_type_separation");
}

static void test_base81_fraction_literal_is_native_t81fraction() {
  constexpr const char* source = R"(
    fn main() -> T81Fraction {
      let x = 22/7t81;
      let y: T81Fraction = x;
      return y;
    }
  )";
  require_true(analyzes(source, "t81lang_base81_fraction_native"),
               "t81lang_base81_fraction_native");
}

static void test_base81_fraction_literal_does_not_silently_narrow() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let x = 22/7t81;
      let y: i32 = x;
      return y;
    }
  )";
  require_true(fails_semantic(source, "t81lang_base81_fraction_no_narrow"),
               "t81lang_base81_fraction_no_narrow");
}

static void test_print_builtin_accepts_native_t81_numerics() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let i: T81BigInt = 12t81;
      let f: T81Float = 1.20t81;
      let q: T81Fraction = 22/7t81;
      print(i);
      print(f);
      print(q);
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_print_builtin_numerics"),
               "t81lang_print_builtin_numerics");
}

static void test_print_builtin_rejects_bad_arity() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      print();
      return 0;
    }
  )";
  require_true(fails_semantic(source, "t81lang_print_builtin_bad_arity"),
               "t81lang_print_builtin_bad_arity");
}

static void test_std_namespace_builtin_aliases() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let angle: T81Float = std.math.sin(1.0);
      let c: T81Float = std.math.cos(angle);
      let t: T81Float = std.math.tan(c);
      let a1: T81Float = std.math.asin(0.5);
      let a2: T81Float = std.math.acos(0.5);
      let a3: T81Float = std.math.atan(1.0);
      let h1: T81Float = std.math.sinh(1.0);
      let h2: T81Float = std.math.cosh(1.0);
      let h3: T81Float = std.math.tanh(1.0);
      let root: T81Float = std.math.sqrt(4.0);
      let ex: T81Float = std.math.exp(1.0);
      let lg: T81Float = std.math.log(ex);
      let pw: T81Float = std.math.pow(2.0, 8.0);
      let cl: T81Float = std.math.clamp(-2.0, 0.0, 1.0);
      let ints: Vector[i32] = [1, 2, 3];
      let n: i32 = std.collections.len(ints);
      let e: bool = std.collections.is_empty(ints);
      let f: i32 = std.collections.first(ints);
      let l: i32 = std.collections.last(ints);
      let pushed: Vector[i32] = std.collections.push(ints, 4);
      let popped: Vector[i32] = std.collections.pop(pushed);
      std.core.assert(1 < 2);
      std.core.debug("dbg");
      let present: Option[i32] = Some(7);
      let keep: i32 = std.core.unwrap_or(present, 9);
      let _k = keep;
      std.io.println("ok");
      std.io.print_int(7);
      std.io.print_float(t);
      std.io.print_float(a1);
      std.io.print_float(a2);
      std.io.print_float(a3);
      std.io.print_float(h1);
      std.io.print_float(h2);
      std.io.print_float(h3);
      std.io.print_float(root);
      std.io.print_float(lg);
      std.io.print_float(pw);
      std.io.print_float(cl);
      if (e) {
        return n;
      }
      if (f == l) {
        return std.collections.len(popped);
      }
      let now: T81Float = std.sys.time();
      let ent: i32 = std.sys.entropy();
      let proof: T81String = std.sys.proof();
      std.sys.reflect();
      let stream_h: T81String = std.io.stream();
      let net_h: T81String = std.io.net();
      std.async.yield();
      std.async.sleep(now);
      let thread_h: T81String = std.async.thread();
      let promise_h: T81String = std.async.promise();
      let list_v: List[T81String] = std.collections.list();
      let map_v: Map[T81String, T81String] = std.collections.map();
      let map_flat: Map[T81String, T81String] = std.collections.map();
      let set_v: Set[T81String] = std.collections.set();
      let tree_v: Tree[T81String] = std.collections.tree();
      let graph_v: Graph[T81String] = std.collections.graph();
      std.agent.self_reflect();
      std.sys.exit(0);
      let handle: i32 = std.tensor.load("layer0.weight");
      let _ent = ent;
      let _proof = proof;
      let _stream_h = stream_h;
      let _net_h = net_h;
      let _thread_h = thread_h;
      let _promise_h = promise_h;
      let _list_h = 0;
      let _map_h = std.collections.map_size(map_v);
      let _map_pairs = std.collections.map_size(map_v);
      let _map_has_city = std.collections.map_has(map_v, "city");
      let map_updated: Map[T81String, T81String] = std.collections.map_put(map_v, "city", "oakland");
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
      let set_added: Set[T81String] = std.collections.set_add(set_v, "edge");
      let set_added_dup: Set[T81String] = std.collections.set_add(set_added, "city");
      let set_removed: Set[T81String] = std.collections.set_remove(set_added_dup, "lang");
      let _set_size = std.collections.set_size(set_v);
      let _set_has_city = std.collections.set_has(set_v, "city");
      let _set_added_size = std.collections.set_size(set_added);
      let _set_added_dup_size = std.collections.set_size(set_added_dup);
      let _set_removed_size = std.collections.set_size(set_removed);
      let _set_removed_has_lang = std.collections.set_has(set_removed, "lang");
      let _set_h = std.collections.set_size(set_v);
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
  require_true(analyzes(source, "t81lang_std_namespace_builtin_aliases"),
               "t81lang_std_namespace_builtin_aliases");

  constexpr const char* bad_print_arity = R"(
    fn main() -> i32 {
      std.io.print_int();
      return 0;
    }
  )";
  require_true(fails_semantic(bad_print_arity, "t81lang_std_print_int_bad_arity"),
               "t81lang_std_print_int_bad_arity");

  constexpr const char* bad_math_type = R"(
    fn main() -> i32 {
      let x: T81Float = std.math.cos("oops");
      let _ = x;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_math_type, "t81lang_std_math_cos_bad_type"),
               "t81lang_std_math_cos_bad_type");

  constexpr const char* bad_math_asin_arity = R"(
    fn main() -> i32 {
      let x: T81Float = std.math.asin(0.5, 0.2);
      let _ = x;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_math_asin_arity, "asin expects exactly one argument.",
                                  "t81lang_std_math_asin_bad_arity"),
      "t81lang_std_math_asin_bad_arity");

  constexpr const char* bad_math_pow_arity = R"(
    fn main() -> i32 {
      let x: T81Float = std.math.pow(2.0);
      let _ = x;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_math_pow_arity, "pow expects exactly two arguments.",
                                           "t81lang_std_math_pow_bad_arity"),
               "t81lang_std_math_pow_bad_arity");

  constexpr const char* bad_math_clamp_arity = R"(
    fn main() -> i32 {
      let x: T81Float = std.math.clamp(1.0, 0.0);
      let _ = x;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_math_clamp_arity, "clamp expects exactly three arguments.",
                                  "t81lang_std_math_clamp_bad_arity"),
      "t81lang_std_math_clamp_bad_arity");

  constexpr const char* bad_math_clamp_type = R"(
    fn main() -> i32 {
      let x: T81Float = std.math.clamp("oops", 0.0, 1.0);
      let _ = x;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_math_clamp_type,
                                           "clamp arguments must be convertible to T81Float.",
                                           "t81lang_std_math_clamp_bad_type"),
               "t81lang_std_math_clamp_bad_type");

  constexpr const char* bad_collections_len_type = R"(
    fn main() -> i32 {
      let n: i32 = std.collections.len(7);
      let _ = n;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_len_type,
                                           "std.collections.len expects a Vector[T] argument.",
                                           "t81lang_std_collections_len_bad_type"),
               "t81lang_std_collections_len_bad_type");

  constexpr const char* bad_collections_empty_arity = R"(
    fn main() -> i32 {
      let e: bool = std.collections.is_empty();
      let _ = e;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_empty_arity,
                                           "std.collections.is_empty expects exactly one argument.",
                                           "t81lang_std_collections_is_empty_bad_arity"),
               "t81lang_std_collections_is_empty_bad_arity");

  constexpr const char* bad_collections_first_empty_literal = R"(
    fn main() -> i32 {
      let first: i32 = std.collections.first([]);
      let _ = first;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_first_empty_literal,
                                  "std.collections.first does not accept empty vector literals.",
                                  "t81lang_std_collections_first_empty_literal"),
      "t81lang_std_collections_first_empty_literal");

  constexpr const char* bad_collections_push_type = R"(
    fn main() -> i32 {
      let ints: Vector[i32] = [1, 2, 3];
      let pushed: Vector[i32] = std.collections.push(ints, "oops");
      let _ = pushed;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_push_type,
                                  "std.collections.push second argument must match Vector element "
                                  "type.",
                                  "t81lang_std_collections_push_bad_type"),
      "t81lang_std_collections_push_bad_type");

  constexpr const char* bad_collections_pop_empty_literal = R"(
    fn main() -> i32 {
      let popped: Vector[i32] = std.collections.pop([]);
      let _ = popped;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_pop_empty_literal,
                                  "std.collections.pop does not accept empty vector literals.",
                                  "t81lang_std_collections_pop_empty_literal"),
      "t81lang_std_collections_pop_empty_literal");

  constexpr const char* bad_debug_type = R"(
    fn main() -> i32 {
      let t: Tensor = std.tensor.from_list([1, 2, 3]);
      std.core.debug(t);
      return 0;
    }
  )";
  require_true(fails_semantic(bad_debug_type, "t81lang_std_core_debug_bad_type"),
               "t81lang_std_core_debug_bad_type");

  constexpr const char* bad_assert_arity = R"(
    fn main() -> i32 {
      std.core.assert();
      return 0;
    }
  )";
  require_true(fails_semantic(bad_assert_arity, "t81lang_std_core_assert_bad_arity"),
               "t81lang_std_core_assert_bad_arity");

  constexpr const char* bad_assert_type = R"(
    fn main() -> i32 {
      std.core.assert(7);
      return 0;
    }
  )";
  require_true(fails_semantic(bad_assert_type, "t81lang_std_core_assert_bad_type"),
               "t81lang_std_core_assert_bad_type");

  constexpr const char* bad_unwrap_or_arity = R"(
    fn main() -> i32 {
      let value: i32 = std.core.unwrap_or(Some(1));
      let _ = value;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_unwrap_or_arity, "t81lang_std_core_unwrap_or_bad_arity"),
               "t81lang_std_core_unwrap_or_bad_arity");

  constexpr const char* bad_unwrap_or_type = R"(
    fn main() -> i32 {
      let value: i32 = std.core.unwrap_or(Some(1), "oops");
      let _ = value;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_unwrap_or_type, "t81lang_std_core_unwrap_or_bad_type"),
               "t81lang_std_core_unwrap_or_bad_type");

  constexpr const char* bad_sys_exit_type = R"(
    fn main() -> i32 {
      std.sys.exit(1.5);
      return 0;
    }
  )";
  require_true(fails_semantic(bad_sys_exit_type, "t81lang_std_sys_exit_bad_type"),
               "t81lang_std_sys_exit_bad_type");

  constexpr const char* bad_sys_time_arity = R"(
    fn main() -> i32 {
      let now: T81Float = std.sys.time(1);
      let _ = now;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_sys_time_arity, "t81lang_std_sys_time_bad_arity"),
               "t81lang_std_sys_time_bad_arity");

  constexpr const char* bad_sys_entropy_arity = R"(
    fn main() -> i32 {
      let e: i32 = std.sys.entropy(1);
      let _ = e;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_sys_entropy_arity, "t81lang_std_sys_entropy_bad_arity"),
               "t81lang_std_sys_entropy_bad_arity");

  constexpr const char* bad_sys_proof_arity = R"(
    fn main() -> i32 {
      let p: T81String = std.sys.proof(1);
      let _ = p;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_sys_proof_arity, "t81lang_std_sys_proof_bad_arity"),
               "t81lang_std_sys_proof_bad_arity");

  constexpr const char* bad_sys_reflect_arity = R"(
    fn main() -> i32 {
      std.sys.reflect(1);
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_sys_reflect_arity, "sys_reflect expects no arguments.",
                                  "t81lang_std_sys_reflect_bad_arity"),
      "t81lang_std_sys_reflect_bad_arity");

  constexpr const char* bad_io_stream_arity = R"(
    fn main() -> i32 {
      let s: T81String = std.io.stream(1);
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_io_stream_arity, "io_stream expects no arguments.",
                                           "t81lang_std_io_stream_bad_arity"),
               "t81lang_std_io_stream_bad_arity");

  constexpr const char* bad_async_thread_arity = R"(
    fn main() -> i32 {
      let t: T81String = std.async.thread(1);
      let _ = t;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_async_thread_arity, "async_thread expects no arguments.",
                                  "t81lang_std_async_thread_bad_arity"),
      "t81lang_std_async_thread_bad_arity");

  constexpr const char* bad_collections_container_arity = R"(
    fn main() -> i32 {
      let g: i32 = std.collections.graph(1);
      let _ = g;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_container_arity,
                                  "std.collections container constructors expect no arguments.",
                                  "t81lang_std_collections_container_bad_arity"),
      "t81lang_std_collections_container_bad_arity");

  constexpr const char* bad_collections_map_put_value_type = R"(
    fn main() -> i32 {
      let m: Map[T81String, T81String] = std.collections.map();
      let out: Map[T81String, T81String] = std.collections.map_put(m, "k", 7);
      let _ = out;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_map_put_value_type,
                                  "std.collections.map_put expects T81String key/value arguments.",
                                  "t81lang_std_collections_map_put_bad_value_type"),
      "t81lang_std_collections_map_put_bad_value_type");

  constexpr const char* bad_collections_map_size_type = R"(
    fn main() -> i32 {
      let n: i32 = std.collections.map_size([1, 2, 3]);
      let _ = n;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_map_size_type,
                                           "std.collections.map_size expects a Map argument.",
                                           "t81lang_std_collections_map_size_bad_type"),
               "t81lang_std_collections_map_size_bad_type");

  constexpr const char* bad_collections_map_has_key_type = R"(
    fn main() -> i32 {
      let ok: bool = std.collections.map_has(["k", "v"], 7);
      let _ = ok;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_map_has_key_type,
                                           "std.collections.map_has expects a Map first argument.",
                                           "t81lang_std_collections_map_has_bad_key_type"),
               "t81lang_std_collections_map_has_bad_key_type");

  constexpr const char* bad_collections_map_get_key_type = R"(
    fn main() -> i32 {
      let maybe: Option[T81String] = std.collections.map_get(["k", "v"], 7);
      let _ = maybe;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_map_get_key_type,
                                           "std.collections.map_get expects a Map first argument.",
                                           "t81lang_std_collections_map_get_bad_key_type"),
               "t81lang_std_collections_map_get_bad_key_type");

  constexpr const char* bad_collections_map_remove_arity = R"(
    fn main() -> i32 {
      let out: Map[T81String, T81String] = std.collections.map_remove(["k", "v"]);
      let _ = out;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_map_remove_arity,
                                  "std.collections.map_remove expects exactly two arguments.",
                                  "t81lang_std_collections_map_remove_bad_arity"),
      "t81lang_std_collections_map_remove_bad_arity");

  constexpr const char* bad_collections_map_keys_type = R"(
    fn main() -> i32 {
      let keys: Vector[T81String] = std.collections.map_keys([1, 2, 3]);
      let _ = keys;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_map_keys_type,
                                           "std.collections.map_keys expects a Map argument.",
                                           "t81lang_std_collections_map_keys_bad_type"),
               "t81lang_std_collections_map_keys_bad_type");

  constexpr const char* bad_collections_set_size_type = R"(
    fn main() -> i32 {
      let n: i32 = std.collections.set_size([1, 2, 3]);
      let _ = n;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_set_size_type,
                                           "std.collections.set_size expects a Set argument.",
                                           "t81lang_std_collections_set_size_bad_type"),
               "t81lang_std_collections_set_size_bad_type");

  constexpr const char* bad_collections_set_has_key_type = R"(
    fn main() -> i32 {
      let ok: bool = std.collections.set_has(["city"], 7);
      let _ = ok;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_set_has_key_type,
                                           "std.collections.set_has expects a Set first argument.",
                                           "t81lang_std_collections_set_has_bad_key_type"),
               "t81lang_std_collections_set_has_bad_key_type");

  constexpr const char* bad_collections_set_add_key_type = R"(
    fn main() -> i32 {
      let out: Set[T81String] = std.collections.set_add(["city"], 7);
      let _ = out;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_collections_set_add_key_type,
                                           "std.collections.set_add expects a Set first argument.",
                                           "t81lang_std_collections_set_add_bad_key_type"),
               "t81lang_std_collections_set_add_bad_key_type");

  constexpr const char* bad_collections_set_remove_arity = R"(
    fn main() -> i32 {
      let out: Set[T81String] = std.collections.set_remove(["city"]);
      let _ = out;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_set_remove_arity,
                                  "std.collections.set_remove expects exactly two arguments.",
                                  "t81lang_std_collections_set_remove_bad_arity"),
      "t81lang_std_collections_set_remove_bad_arity");

  constexpr const char* bad_collections_graph_edge_count_type = R"(
    fn main() -> i32 {
      let n: i32 = std.collections.graph_edge_count(std.collections.graph[T81String]());
      let _ = n;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_graph_edge_count_type,
                                  "std.collections.graph_edge_count expects a Graph argument.",
                                  "t81lang_std_collections_graph_edge_count_bad_type"),
      "t81lang_std_collections_graph_edge_count_bad_type");

  constexpr const char* bad_collections_graph_has_edge_arity = R"(
    fn main() -> i32 {
      let ok: bool = std.collections.graph_has_edge(std.collections.graph[T81String](), "a");
      let _ = ok;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_graph_has_edge_arity,
                                  "std.collections.graph_has_edge expects exactly three arguments.",
                                  "t81lang_std_collections_graph_has_edge_bad_arity"),
      "t81lang_std_collections_graph_has_edge_bad_arity");

  constexpr const char* bad_collections_graph_add_edge_type = R"(
    fn main() -> i32 {
      let out: Graph[T81String] = std.collections.graph_add_edge(["a", "b"], "a", 7);
      let _ = out;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_graph_add_edge_type,
                                  "std.collections.graph_add_edge expects a Graph first argument.",
                                  "t81lang_std_collections_graph_add_edge_bad_type"),
      "t81lang_std_collections_graph_add_edge_bad_type");

  constexpr const char* bad_collections_graph_remove_edge_arity = R"(
    fn main() -> i32 {
      let out: Graph[T81String] = std.collections.graph_remove_edge(["a", "b"], "a");
      let _ = out;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(
                   bad_collections_graph_remove_edge_arity,
                   "std.collections.graph_remove_edge expects exactly three arguments.",
                   "t81lang_std_collections_graph_remove_edge_bad_arity"),
               "t81lang_std_collections_graph_remove_edge_bad_arity");

  constexpr const char* bad_collections_graph_neighbors_type = R"(
    fn main() -> i32 {
      let out: Vector[T81String] = std.collections.graph_neighbors(["a", "b"], 7);
      let _ = out;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_collections_graph_neighbors_type,
                                  "std.collections.graph_neighbors expects a Graph first argument.",
                                  "t81lang_std_collections_graph_neighbors_bad_type"),
      "t81lang_std_collections_graph_neighbors_bad_type");

  constexpr const char* bad_async_sleep_type = R"(
    fn main() -> i32 {
      std.async.sleep("slow");
      return 0;
    }
  )";
  require_true(fails_semantic(bad_async_sleep_type, "t81lang_std_async_sleep_bad_type"),
               "t81lang_std_async_sleep_bad_type");

  constexpr const char* bad_agent_self_reflect_arity = R"(
    fn main() -> i32 {
      std.agent.self_reflect(1);
      return 0;
    }
  )";
  require_true(
      fails_semantic(bad_agent_self_reflect_arity, "t81lang_std_agent_self_reflect_bad_arity"),
      "t81lang_std_agent_self_reflect_bad_arity");
}

static void test_std_tensor_from_list_alias() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let t: Tensor = std.tensor.from_list([1, 2, 3]);
      let _ = t;
      return 0;
    }
  )";
  require_true(analyzes(valid, "t81lang_std_tensor_from_list_alias"),
               "t81lang_std_tensor_from_list_alias");

  constexpr const char* bad_arity = R"(
    fn main() -> i32 {
      let t: Tensor = std.tensor.from_list();
      let _ = t;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_arity, "t81lang_std_tensor_from_list_bad_arity"),
               "t81lang_std_tensor_from_list_bad_arity");

  constexpr const char* bad_arg = R"(
    fn main() -> i32 {
      let t: Tensor = std.tensor.from_list("abc");
      let _ = t;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_arg, "t81lang_std_tensor_from_list_bad_arg"),
               "t81lang_std_tensor_from_list_bad_arg");
}

static void test_std_tensor_matmul_alias() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let a: Tensor = std.tensor.from_list([1, 2, 3]);
      let b: Tensor = std.tensor.from_list([4, 5, 6]);
      let c: Tensor = std.tensor.matmul(a, b);
      let _ = c;
      return 0;
    }
  )";
  require_true(analyzes(valid, "t81lang_std_tensor_matmul_alias"),
               "t81lang_std_tensor_matmul_alias");

  constexpr const char* bad_arity = R"(
    fn main() -> i32 {
      let a: Tensor = std.tensor.from_list([1, 2, 3]);
      let c: Tensor = std.tensor.matmul(a);
      let _ = c;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_arity, "t81lang_std_tensor_matmul_bad_arity"),
               "t81lang_std_tensor_matmul_bad_arity");
}

static void test_std_tensor_vec_add_alias() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let a: Tensor = std.tensor.from_list([1, 2, 3]);
      let b: Tensor = std.tensor.from_list([4, 5, 6]);
      let c: Tensor = std.tensor.vec_add(a, b);
      let _ = c;
      return 0;
    }
  )";
  require_true(analyzes(valid, "t81lang_std_tensor_vec_add_alias"),
               "t81lang_std_tensor_vec_add_alias");

  constexpr const char* bad_arity = R"(
    fn main() -> i32 {
      let a: Tensor = std.tensor.from_list([1, 2, 3]);
      let c: Tensor = std.tensor.vec_add(a);
      let _ = c;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_arity, "t81lang_std_tensor_vec_add_bad_arity"),
               "t81lang_std_tensor_vec_add_bad_arity");
}

static void test_std_math_tensor_module_wrappers() {
  constexpr const char* source = R"(
    fn sin(x: T81Float) -> T81Float {
      return std.math.sin(x);
    }
    fn cos(x: T81Float) -> T81Float {
      return std.math.cos(x);
    }
    fn tan(x: T81Float) -> T81Float {
      return std.math.tan(x);
    }
    fn asin(x: T81Float) -> T81Float {
      return std.math.asin(x);
    }
    fn acos(x: T81Float) -> T81Float {
      return std.math.acos(x);
    }
    fn atan(x: T81Float) -> T81Float {
      return std.math.atan(x);
    }
    fn sinh(x: T81Float) -> T81Float {
      return std.math.sinh(x);
    }
    fn cosh(x: T81Float) -> T81Float {
      return std.math.cosh(x);
    }
    fn tanh(x: T81Float) -> T81Float {
      return std.math.tanh(x);
    }
    fn exp(x: T81Float) -> T81Float {
      return std.math.exp(x);
    }
    fn log(x: T81Float) -> T81Float {
      return std.math.log(x);
    }
    fn sqrt(x: T81Float) -> T81Float {
      return std.math.sqrt(x);
    }
    fn pow(b: T81Float, e: T81Float) -> T81Float {
      return std.math.pow(b, e);
    }
    fn clamp(v: T81Float, minv: T81Float, maxv: T81Float) -> T81Float {
      return std.math.clamp(v, minv, maxv);
    }
    fn len_wrap[T](v: Vector[T]) -> i32 {
      return std.collections.len(v);
    }
    fn is_empty_wrap[T](v: Vector[T]) -> bool {
      return std.collections.is_empty(v);
    }
    fn first_wrap[T](v: Vector[T]) -> T {
      return std.collections.first(v);
    }
    fn last_wrap[T](v: Vector[T]) -> T {
      return std.collections.last(v);
    }
    fn push_wrap[T](v: Vector[T], value: T) -> Vector[T] {
      return std.collections.push(v, value);
    }
    fn pop_wrap[T](v: Vector[T]) -> Vector[T] {
      return std.collections.pop(v);
    }
    fn from_list(values: Vector[i32]) -> Tensor {
      return std.tensor.from_list(values);
    }
    fn matmul(a: Tensor, b: Tensor) -> Tensor {
      return std.tensor.matmul(a, b);
    }
    fn vec_add(a: Tensor, b: Tensor) -> Tensor {
      return std.tensor.vec_add(a, b);
    }
    fn main() -> i32 {
      let angle: T81Float = sin(1.0);
      let c: T81Float = cos(angle);
      let _t: T81Float = tan(c);
      let _asin: T81Float = asin(0.5);
      let _acos: T81Float = acos(0.5);
      let _atan: T81Float = atan(1.0);
      let _sinh: T81Float = sinh(1.0);
      let _cosh: T81Float = cosh(1.0);
      let _tanh: T81Float = tanh(1.0);
      let ex: T81Float = exp(1.0);
      let _lg: T81Float = log(ex);
      let _rt: T81Float = sqrt(4.0);
      let _pw: T81Float = pow(2.0, 8.0);
      let _cp: T81Float = clamp(-2.0, 0.0, 1.0);
      let ints: Vector[i32] = [1, 2, 3];
      let _len: i32 = len_wrap(ints);
      let _empty: bool = is_empty_wrap(ints);
      let _first: i32 = first_wrap(ints);
      let _last: i32 = last_wrap(ints);
      let pushed: Vector[i32] = push_wrap(ints, 4);
      let _popped: Vector[i32] = pop_wrap(pushed);
      let a: Tensor = from_list([1, 2, 3]);
      let b: Tensor = from_list([4, 5, 6]);
      let _m: Tensor = matmul(a, b);
      let _v: Tensor = vec_add(a, b);
      let _h: i32 = std.tensor.load("encoder.weight");
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_std_math_tensor_module_wrappers"),
               "t81lang_std_math_tensor_module_wrappers");
}

static void test_std_sys_async_agent_module_wrappers() {
  constexpr const char* source = R"(
    fn exit(code: i32) -> void {
      std.sys.exit(code);
    }
    fn time() -> T81Float {
      return std.sys.time();
    }
    fn yield() -> void {
      std.async.yield();
    }
    fn sleep(duration: T81Float) -> void {
      std.async.sleep(duration);
    }
    fn self_reflect() -> void {
      std.agent.self_reflect();
    }
    fn main() -> i32 {
      let now: T81Float = time();
      yield();
      sleep(now);
      self_reflect();
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_std_sys_async_agent_module_wrappers"),
               "t81lang_std_sys_async_agent_module_wrappers");
}

static void test_std_text_aliases() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let n: i32 = std.text.str_len("alpha");
      let e: bool = std.text.str_is_empty("");
      let c: T81String = std.text.concat("al", "pha");
      let sw: bool = std.text.starts_with(c, "al");
      let ew: bool = std.text.ends_with(c, "ha");
      let has_mid: bool = std.text.contains(c, "lp");
      let idx: i32 = std.text.index_of(c, "ph");
      let repl: T81String = std.text.replace(c, "ph", "zz");
      let rendered: T81String = std.text.to_string(repl);
      let rendered_bytes: T81String = std.text.to_string(T81Bytes("beta"));
      let rendered_from_bytes: T81String = std.text.from_bytes(T81Bytes("beta"));
      let repl_idx: i32 = std.text.index_of(repl, "zz");
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
  require_true(analyzes(valid, "t81lang_std_text_aliases"), "t81lang_std_text_aliases");

  constexpr const char* bad_type = R"(
    fn main() -> i32 {
      let n: i32 = std.text.str_len(7);
      return n;
    }
  )";
  require_true(fails_semantic(bad_type, "t81lang_std_text_str_len_bad_type"),
               "t81lang_std_text_str_len_bad_type");

  constexpr const char* bad_arity = R"(
    fn main() -> i32 {
      let n: i32 = std.text.str_is_empty();
      return n;
    }
  )";
  require_true(fails_semantic(bad_arity, "t81lang_std_text_str_is_empty_bad_arity"),
               "t81lang_std_text_str_is_empty_bad_arity");

  constexpr const char* bad_concat_type = R"(
    fn main() -> i32 {
      let c: T81String = std.text.concat("alpha", 7);
      let _ = c;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_concat_type, "t81lang_std_text_concat_bad_type"),
               "t81lang_std_text_concat_bad_type");

  constexpr const char* bad_starts_with_arity = R"(
    fn main() -> i32 {
      let b: bool = std.text.starts_with("alpha");
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_starts_with_arity, "t81lang_std_text_starts_with_bad_arity"),
               "t81lang_std_text_starts_with_bad_arity");

  constexpr const char* bad_ends_with_type = R"(
    fn main() -> i32 {
      let b: bool = std.text.ends_with(7, "a");
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_ends_with_type, "t81lang_std_text_ends_with_bad_type"),
               "t81lang_std_text_ends_with_bad_type");

  constexpr const char* bad_contains_arity = R"(
    fn main() -> i32 {
      let b: bool = std.text.contains("alpha");
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_contains_arity, "t81lang_std_text_contains_bad_arity"),
               "t81lang_std_text_contains_bad_arity");

  constexpr const char* bad_index_of_type = R"(
    fn main() -> i32 {
      let i: i32 = std.text.index_of("alpha", 7);
      let _ = i;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_index_of_type, "t81lang_std_text_index_of_bad_type"),
               "t81lang_std_text_index_of_bad_type");

  constexpr const char* bad_replace_arity = R"(
    fn main() -> i32 {
      let s: T81String = std.text.replace("alpha", "a");
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_replace_arity, "t81lang_std_text_replace_bad_arity"),
               "t81lang_std_text_replace_bad_arity");

  constexpr const char* bad_replace_type = R"(
    fn main() -> i32 {
      let s: T81String = std.text.replace("alpha", 7, "b");
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_replace_type, "t81lang_std_text_replace_bad_type"),
               "t81lang_std_text_replace_bad_type");

  constexpr const char* bad_to_string_arity = R"(
    fn main() -> i32 {
      let s: T81String = std.text.to_string();
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_to_string_arity, "t81lang_std_text_to_string_bad_arity"),
               "t81lang_std_text_to_string_bad_arity");

  constexpr const char* bad_to_string_type = R"(
    fn main() -> i32 {
      let s: T81String = std.text.to_string(7);
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_to_string_type, "t81lang_std_text_to_string_bad_type"),
               "t81lang_std_text_to_string_bad_type");

  constexpr const char* split_valid = R"(
    fn main() -> i32 {
      let parts: Vector[T81String] = std.text.split("a,b", ",");
      let joined: T81String = std.text.join(parts, ",");
      let n: i32 = std.text.str_len(joined);
      if (n == 3) {
        return 1;
      }
      return 0;
    }
  )";
  require_true(analyzes(split_valid, "t81lang_std_text_split_valid"),
               "t81lang_std_text_split_valid");

  constexpr const char* join_valid_literal_vector = R"(
    fn main() -> i32 {
      let joined: T81String = std.text.join(["a", "b"], ",");
      let n: i32 = std.text.str_len(joined);
      return n;
    }
  )";
  require_true(analyzes(join_valid_literal_vector, "t81lang_std_text_join_valid_literal_vector"),
               "t81lang_std_text_join_valid_literal_vector");

  constexpr const char* bad_split_arity = R"(
    fn main() -> i32 {
      let parts: Vector[T81String] = std.text.split("alpha");
      let _ = parts;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_split_arity, "str_split expects exactly two arguments.",
                                  "t81lang_std_text_split_bad_arity"),
      "t81lang_std_text_split_bad_arity");

  constexpr const char* bad_split_type = R"(
    fn main() -> i32 {
      let parts: Vector[T81String] = std.text.split("alpha", 7);
      let _ = parts;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_split_type, "str_split expects T81String arguments.",
                                           "t81lang_std_text_split_bad_type"),
               "t81lang_std_text_split_bad_type");

  constexpr const char* bad_split_empty_separator = R"(
    fn main() -> i32 {
      let parts: Vector[T81String] = std.text.split("alpha", "");
      let _ = parts;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_split_empty_separator,
                                           "str_split separator must not be empty.",
                                           "t81lang_std_text_split_empty_separator"),
               "t81lang_std_text_split_empty_separator");

  constexpr const char* split_variable_separator_valid = R"(
    fn main() -> i32 {
      let sep: T81String = ",";
      let parts: Vector[T81String] = std.text.split("alpha", sep);
      let joined: T81String = std.text.join(parts, sep);
      let _ = joined;
      return 0;
    }
  )";
  require_true(
      analyzes(split_variable_separator_valid, "t81lang_std_text_split_variable_separator_valid"),
      "t81lang_std_text_split_variable_separator_valid");

  constexpr const char* bad_join_arity = R"(
    fn main() -> i32 {
      let joined: T81String = std.text.join(["a", "b"]);
      let _ = joined;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_join_arity, "str_join expects exactly two arguments.",
                                  "t81lang_std_text_join_bad_arity"),
      "t81lang_std_text_join_bad_arity");

  constexpr const char* bad_join_parts_type = R"(
    fn main() -> i32 {
      let joined: T81String = std.text.join("abc", ",");
      let _ = joined;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_join_parts_type,
                                           "str_join expects a Vector[T81String] first argument.",
                                           "t81lang_std_text_join_bad_parts_type"),
               "t81lang_std_text_join_bad_parts_type");

  constexpr const char* bad_join_sep_type = R"(
    fn main() -> i32 {
      let joined: T81String = std.text.join(["a", "b"], 7);
      let _ = joined;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_join_sep_type,
                                           "str_join expects a T81String separator argument.",
                                           "t81lang_std_text_join_bad_separator_type"),
               "t81lang_std_text_join_bad_separator_type");

  constexpr const char* bad_from_bytes_arity = R"(
    fn main() -> i32 {
      let s: T81String = std.text.from_bytes();
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_from_bytes_arity, "t81lang_std_text_from_bytes_bad_arity"),
               "t81lang_std_text_from_bytes_bad_arity");

  constexpr const char* bad_from_bytes_type = R"(
    fn main() -> i32 {
      let s: T81String = std.text.from_bytes(7);
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_from_bytes_type, "t81lang_std_text_from_bytes_bad_type"),
               "t81lang_std_text_from_bytes_bad_type");

  constexpr const char* bad_from_bytes_arity_extra = R"(
    fn main() -> i32 {
      let s: T81String = std.text.from_bytes(T81Bytes("alpha"), T81Bytes("beta"));
      let _ = s;
      return 0;
    }
  )";
  require_true(
      fails_semantic(bad_from_bytes_arity_extra, "t81lang_std_text_from_bytes_bad_arity_extra"),
      "t81lang_std_text_from_bytes_bad_arity_extra");
}

static void test_std_text_module_wrappers() {
  constexpr const char* source = R"(
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
      let joined: T81String = concat("ze", "ta");
      let n: i32 = str_len(joined);
      let e: bool = str_is_empty("");
      let sw: bool = starts_with(joined, "ze");
      let ew: bool = ends_with(joined, "ta");
      let has_mid: bool = contains(joined, "et");
      let idx: i32 = index_of(joined, "ta");
      let replaced: T81String = replace(joined, "ta", "xo");
      let rendered: T81String = to_string(replaced);
      let rendered_bytes: T81String = from_bytes(T81Bytes("beta"));
      let rendered_from_bytes: T81String = from_bytes_alias(T81Bytes("beta"));
      let repl_idx: i32 = index_of(replaced, "xo");
      if (e) {
        if (sw) {
          if (ew) {
            if (has_mid) {
              if (idx == 2) {
                if (repl_idx == 2) {
                  if (str_len(rendered) == 4) {
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
  require_true(analyzes(source, "t81lang_std_text_module_wrappers"),
               "t81lang_std_text_module_wrappers");
}

static void test_std_text_split_join_wrappers() {
  constexpr const char* split_wrapper = R"(
    fn split_wrapper(s: T81String, sep: T81String) -> Vector[T81String] {
      return std.text.split(s, sep);
    }
    fn main() -> i32 {
      let parts: Vector[T81String] = split_wrapper("a,b", ",");
      let _ = parts;
      return 0;
    }
  )";
  require_true(analyzes(split_wrapper, "t81lang_std_text_split_wrapper_valid"),
               "t81lang_std_text_split_wrapper_valid");

  constexpr const char* join_wrapper = R"(
    fn join_wrapper(parts: Vector[T81String], sep: T81String) -> T81String {
      return std.text.join(parts, sep);
    }
    fn main() -> i32 {
      let joined: T81String = join_wrapper(["a", "b"], ",");
      let _ = joined;
      return 0;
    }
  )";
  require_true(analyzes(join_wrapper, "t81lang_std_text_join_wrapper_valid"),
               "t81lang_std_text_join_wrapper_valid");
}

static void test_std_bytes_aliases() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let alpha: T81Bytes = T81Bytes("alpha");
      let empty: T81Bytes = T81Bytes("");
      let c: T81Bytes = std.bytes.concat(T81Bytes("al"), T81Bytes("pha"));
      let n: i32 = std.bytes.len(alpha);
      let e: bool = std.bytes.is_empty(empty);
      let sw: bool = std.bytes.starts_with(c, T81Bytes("al"));
      let ew: bool = std.bytes.ends_with(c, T81Bytes("ha"));
      let has_mid: bool = std.bytes.contains(c, T81Bytes("lp"));
      let idx: i32 = std.bytes.index_of(c, T81Bytes("ph"));
      let repl: T81Bytes = std.bytes.replace(c, T81Bytes("ph"), T81Bytes("zz"));
      let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("a,,b"), T81Bytes(","));
      let roundtrip: T81Bytes = std.bytes.join(parts, T81Bytes(","));
      let rendered: T81String = std.bytes.to_string(repl);
      let from_text: T81Bytes = std.bytes.from_string("alpha");
      let from_text_len: i32 = std.bytes.len(from_text);
      let repl_idx: i32 = std.bytes.index_of(repl, T81Bytes("zz"));
      let roundtrip_idx: i32 = std.bytes.index_of(roundtrip, T81Bytes(",,"));
      let m: i32 = std.bytes.len(c);
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
  require_true(analyzes(valid, "t81lang_std_bytes_aliases"), "t81lang_std_bytes_aliases");

  constexpr const char* bad_type = R"(
    fn main() -> i32 {
      let n: i32 = std.bytes.len(7);
      return n;
    }
  )";
  require_true(fails_semantic(bad_type, "t81lang_std_bytes_len_bad_type"),
               "t81lang_std_bytes_len_bad_type");

  constexpr const char* bad_string_without_conversion = R"(
    fn main() -> i32 {
      let n: i32 = std.bytes.len("alpha");
      return n;
    }
  )";
  require_true(
      fails_semantic(bad_string_without_conversion, "t81lang_std_bytes_len_requires_bytes"),
      "t81lang_std_bytes_len_requires_bytes");

  constexpr const char* bad_bytes_to_text = R"(
    fn main() -> i32 {
      let b: T81Bytes = T81Bytes("alpha");
      let n: i32 = std.text.str_len(b);
      return n;
    }
  )";
  require_true(fails_semantic(bad_bytes_to_text, "t81lang_std_text_rejects_bytes"),
               "t81lang_std_text_rejects_bytes");

  constexpr const char* bad_arity = R"(
    fn main() -> i32 {
      let n: i32 = std.bytes.is_empty();
      return n;
    }
  )";
  require_true(fails_semantic(bad_arity, "t81lang_std_bytes_is_empty_bad_arity"),
               "t81lang_std_bytes_is_empty_bad_arity");

  constexpr const char* bad_concat_type = R"(
    fn main() -> i32 {
      let c: T81Bytes = std.bytes.concat(T81Bytes("alpha"), 7);
      let _ = c;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_concat_type, "t81lang_std_bytes_concat_bad_type"),
               "t81lang_std_bytes_concat_bad_type");

  constexpr const char* bad_starts_with_arity = R"(
    fn main() -> i32 {
      let b: bool = std.bytes.starts_with(T81Bytes("alpha"));
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_starts_with_arity, "t81lang_std_bytes_starts_with_bad_arity"),
               "t81lang_std_bytes_starts_with_bad_arity");

  constexpr const char* bad_ends_with_type = R"(
    fn main() -> i32 {
      let b: bool = std.bytes.ends_with(7, T81Bytes("a"));
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_ends_with_type, "t81lang_std_bytes_ends_with_bad_type"),
               "t81lang_std_bytes_ends_with_bad_type");

  constexpr const char* bad_contains_arity = R"(
    fn main() -> i32 {
      let b: bool = std.bytes.contains(T81Bytes("alpha"));
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_contains_arity, "t81lang_std_bytes_contains_bad_arity"),
               "t81lang_std_bytes_contains_bad_arity");

  constexpr const char* bad_index_of_type = R"(
    fn main() -> i32 {
      let i: i32 = std.bytes.index_of(T81Bytes("alpha"), 7);
      let _ = i;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_index_of_type, "t81lang_std_bytes_index_of_bad_type"),
               "t81lang_std_bytes_index_of_bad_type");

  constexpr const char* bad_replace_arity = R"(
    fn main() -> i32 {
      let s: T81Bytes = std.bytes.replace(T81Bytes("alpha"), T81Bytes("a"));
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_replace_arity, "t81lang_std_bytes_replace_bad_arity"),
               "t81lang_std_bytes_replace_bad_arity");

  constexpr const char* bad_replace_type = R"(
    fn main() -> i32 {
      let s: T81Bytes = std.bytes.replace(T81Bytes("alpha"), 7, T81Bytes("b"));
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_replace_type, "t81lang_std_bytes_replace_bad_type"),
               "t81lang_std_bytes_replace_bad_type");

  constexpr const char* bad_split_arity = R"(
    fn main() -> i32 {
      let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("alpha"));
      let _ = parts;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_split_arity, "bytes_split expects exactly two arguments.",
                                  "t81lang_std_bytes_split_bad_arity"),
      "t81lang_std_bytes_split_bad_arity");

  constexpr const char* bad_split_type = R"(
    fn main() -> i32 {
      let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("alpha"), 7);
      let _ = parts;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_split_type, "bytes_split expects T81Bytes arguments.",
                                  "t81lang_std_bytes_split_bad_type"),
      "t81lang_std_bytes_split_bad_type");

  constexpr const char* bad_split_empty_separator = R"(
    fn main() -> i32 {
      let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("alpha"), T81Bytes(""));
      let _ = parts;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_split_empty_separator,
                                           "bytes_split separator must not be empty.",
                                           "t81lang_std_bytes_split_bad_empty_separator"),
               "t81lang_std_bytes_split_bad_empty_separator");

  constexpr const char* bad_join_arity = R"(
    fn main() -> i32 {
      let joined: T81Bytes = std.bytes.join([T81Bytes("a"), T81Bytes("b")]);
      let _ = joined;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_join_arity, "bytes_join expects exactly two arguments.",
                                  "t81lang_std_bytes_join_bad_arity"),
      "t81lang_std_bytes_join_bad_arity");

  constexpr const char* bad_join_parts_type = R"(
    fn main() -> i32 {
      let joined: T81Bytes = std.bytes.join(T81Bytes("abc"), T81Bytes(","));
      let _ = joined;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_join_parts_type,
                                           "bytes_join expects a Vector[T81Bytes] first argument.",
                                           "t81lang_std_bytes_join_bad_parts_type"),
               "t81lang_std_bytes_join_bad_parts_type");

  constexpr const char* bad_join_sep_type = R"(
    fn main() -> i32 {
      let joined: T81Bytes = std.bytes.join([T81Bytes("a"), T81Bytes("b")], 7);
      let _ = joined;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_join_sep_type,
                                           "bytes_join expects a T81Bytes separator argument.",
                                           "t81lang_std_bytes_join_bad_separator_type"),
               "t81lang_std_bytes_join_bad_separator_type");

  constexpr const char* bad_from_string_type = R"(
    fn main() -> i32 {
      let b: T81Bytes = std.bytes.from_string(7);
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_from_string_type, "t81lang_std_bytes_from_string_bad_type"),
               "t81lang_std_bytes_from_string_bad_type");

  constexpr const char* bad_from_string_arity = R"(
    fn main() -> i32 {
      let b: T81Bytes = std.bytes.from_string();
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_from_string_arity, "t81lang_std_bytes_from_string_bad_arity"),
               "t81lang_std_bytes_from_string_bad_arity");

  constexpr const char* bad_from_string_arity_extra = R"(
    fn main() -> i32 {
      let b: T81Bytes = std.bytes.from_string("alpha", "beta");
      let _ = b;
      return 0;
    }
  )";
  require_true(
      fails_semantic(bad_from_string_arity_extra, "t81lang_std_bytes_from_string_bad_arity_extra"),
      "t81lang_std_bytes_from_string_bad_arity_extra");

  constexpr const char* bad_to_string_arity = R"(
    fn main() -> i32 {
      let s: T81String = std.bytes.to_string();
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_to_string_arity, "t81lang_std_bytes_to_string_bad_arity"),
               "t81lang_std_bytes_to_string_bad_arity");

  constexpr const char* bad_to_string_type = R"(
    fn main() -> i32 {
      let s: T81String = std.bytes.to_string(7);
      let _ = s;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_to_string_type, "t81lang_std_bytes_to_string_bad_type"),
               "t81lang_std_bytes_to_string_bad_type");

  constexpr const char* bad_to_string_arity_extra = R"(
    fn main() -> i32 {
      let s: T81String = std.bytes.to_string(T81Bytes("alpha"), T81Bytes("beta"));
      let _ = s;
      return 0;
    }
  )";
  require_true(
      fails_semantic(bad_to_string_arity_extra, "t81lang_std_bytes_to_string_bad_arity_extra"),
      "t81lang_std_bytes_to_string_bad_arity_extra");

  constexpr const char* valid_constructor = R"(
    fn main() -> i32 {
      let direct: T81Bytes = T81Bytes("alpha");
      let passthrough: T81Bytes = T81Bytes(direct);
      if (std.bytes.len(passthrough) == 5) {
        return 5;
      }
      return 0;
    }
  )";
  require_true(analyzes(valid_constructor, "t81lang_t81bytes_constructor_valid"),
               "t81lang_t81bytes_constructor_valid");

  constexpr const char* bad_constructor_type = R"(
    fn main() -> i32 {
      let b: T81Bytes = T81Bytes(7);
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_constructor_type, "t81lang_t81bytes_constructor_bad_type"),
               "t81lang_t81bytes_constructor_bad_type");

  constexpr const char* bad_constructor_arity = R"(
    fn main() -> i32 {
      let b: T81Bytes = T81Bytes();
      let _ = b;
      return 0;
    }
  )";
  require_true(fails_semantic(bad_constructor_arity, "t81lang_t81bytes_constructor_bad_arity"),
               "t81lang_t81bytes_constructor_bad_arity");

  constexpr const char* bad_constructor_arity_extra = R"(
    fn main() -> i32 {
      let b: T81Bytes = T81Bytes("alpha", "beta");
      let _ = b;
      return 0;
    }
  )";
  require_true(
      fails_semantic(bad_constructor_arity_extra, "t81lang_t81bytes_constructor_bad_arity_extra"),
      "t81lang_t81bytes_constructor_bad_arity_extra");
}

static void test_std_bytes_module_wrappers() {
  constexpr const char* source = R"(
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
      let merged: T81Bytes = concat(T81Bytes("ze"), T81Bytes("ta"));
      let from_text: T81Bytes = from_string("zeta");
      let n: i32 = len(merged);
      let e: bool = is_empty(T81Bytes(""));
      let sw: bool = starts_with(merged, T81Bytes("ze"));
      let ew: bool = ends_with(merged, T81Bytes("ta"));
      let has_mid: bool = contains(merged, T81Bytes("et"));
      let idx: i32 = index_of(merged, T81Bytes("ta"));
      let replaced: T81Bytes = replace(merged, T81Bytes("ta"), T81Bytes("xo"));
      let parts: Vector[T81Bytes] = split(T81Bytes("x,,z"), T81Bytes(","));
      let roundtrip: T81Bytes = join(parts, T81Bytes(","));
      let rendered: T81String = to_string(replaced);
      let from_text_len: i32 = len(from_text);
      let repl_idx: i32 = index_of(replaced, T81Bytes("xo"));
      let roundtrip_idx: i32 = index_of(roundtrip, T81Bytes(",,"));
      let m: i32 = len(merged);
      if (e) {
        if (n == 4) {
          if (m == 4) {
            if (from_text_len == 4) {
              if (sw) {
                if (ew) {
                  if (has_mid) {
                    if (idx == 2) {
                      if (repl_idx == 2) {
                        if (std.text.str_len(rendered) == 4) {
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
  require_true(analyzes(source, "t81lang_std_bytes_module_wrappers"),
               "t81lang_std_bytes_module_wrappers");
}

static void test_std_symbol_aliases() {
  constexpr const char* valid = R"(
    fn main() -> i32 {
      let sym: Symbol = std.symbol.intern("omega");
      let rendered: T81String = std.symbol.to_string(sym);
      let same: bool = std.symbol.eq(sym, std.symbol.intern("omega"));
      let diff: bool = std.symbol.ne(sym, std.symbol.intern("alpha"));
      if (std.text.str_len(rendered) == 5) {
        if (same) {
          if (diff) {
            return 5;
          }
        }
      }
      return 0;
    }
  )";
  require_true(analyzes(valid, "t81lang_std_symbol_aliases_valid"),
               "t81lang_std_symbol_aliases_valid");

  constexpr const char* bad_intern_arity = R"(
    fn main() -> i32 {
      let sym: Symbol = std.symbol.intern();
      let _ = sym;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_intern_arity, "symbol_intern expects exactly one argument.",
                                  "t81lang_std_symbol_intern_bad_arity"),
      "t81lang_std_symbol_intern_bad_arity");

  constexpr const char* bad_intern_type = R"(
    fn main() -> i32 {
      let sym: Symbol = std.symbol.intern(7);
      let _ = sym;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_intern_type, "symbol_intern expects a T81String argument.",
                                  "t81lang_std_symbol_intern_bad_type"),
      "t81lang_std_symbol_intern_bad_type");

  constexpr const char* bad_to_string_type = R"(
    fn main() -> i32 {
      let rendered: T81String = std.symbol.to_string(7);
      let _ = rendered;
      return 0;
    }
  )";
  require_true(
      fails_semantic_with_message(bad_to_string_type, "symbol_to_string expects a Symbol argument.",
                                  "t81lang_std_symbol_to_string_bad_type"),
      "t81lang_std_symbol_to_string_bad_type");

  constexpr const char* bad_eq_arity = R"(
    fn main() -> i32 {
      let same: bool = std.symbol.eq(std.symbol.intern("a"));
      let _ = same;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_eq_arity, "symbol_eq expects exactly two arguments.",
                                           "t81lang_std_symbol_eq_bad_arity"),
               "t81lang_std_symbol_eq_bad_arity");

  constexpr const char* bad_eq_type = R"(
    fn main() -> i32 {
      let same: bool = std.symbol.eq(std.symbol.intern("a"), 7);
      let _ = same;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(bad_eq_type, "symbol_eq expects Symbol arguments.",
                                           "t81lang_std_symbol_eq_bad_type"),
               "t81lang_std_symbol_eq_bad_type");
}

static void test_generic_function_inference() {
  constexpr const char* valid = R"(
    fn choose[T](a: T, b: T) -> T {
      return b;
    }
    fn main() -> i32 {
      let out: i32 = choose(7, 9);
      return out;
    }
  )";
  require_true(analyzes(valid, "t81lang_generic_function_inference_valid"),
               "t81lang_generic_function_inference_valid");

  constexpr const char* mismatched_args = R"(
    fn choose[T](a: T, b: T) -> T {
      return a;
    }
    fn main() -> i32 {
      let out: i32 = choose(7, "oops");
      return out;
    }
  )";
  require_true(fails_semantic_with_message(mismatched_args,
                                           "Argument 1 for function 'choose' expects 'T' but got "
                                           "'T81String'.",
                                           "t81lang_generic_function_inference_mismatched_args"),
               "t81lang_generic_function_inference_mismatched_args");

  constexpr const char* mismatched_return_assignment = R"(
    fn first_or[T](a: T, fallback: T) -> T {
      return a;
    }
    fn main() -> i32 {
      let out: T81String = first_or(7, 9);
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(mismatched_return_assignment,
                                           "Cannot assign initializer of type 'i32' to constant "
                                           "of type 'T81String'.",
                                           "t81lang_generic_function_inference_bad_return_use"),
               "t81lang_generic_function_inference_bad_return_use");

  constexpr const char* explicit_type_args = R"(
    fn id[T](x: T) -> T {
      return x;
    }
    fn main() -> i32 {
      let out: i32 = id[i32](7);
      return out;
    }
  )";
  require_true(analyzes(explicit_type_args, "t81lang_generic_function_explicit_type_args"),
               "t81lang_generic_function_explicit_type_args");

  constexpr const char* partial_explicit_type_args = R"(
    fn first[T, U](a: T, b: U) -> T {
      return a;
    }
    fn main() -> i32 {
      let out: i32 = first[i32](7, "tail");
      return out;
    }
  )";
  require_true(
      analyzes(partial_explicit_type_args, "t81lang_generic_function_partial_explicit_type_args"),
      "t81lang_generic_function_partial_explicit_type_args");

  constexpr const char* explicit_type_arg_count_mismatch = R"(
    fn id[T](x: T) -> T {
      return x;
    }
    fn main() -> i32 {
      let out: i32 = id[i32, T81String](7);
      return out;
    }
  )";
  require_true(fails_semantic_with_message(explicit_type_arg_count_mismatch,
                                           "expects 1 explicit type arguments at most but got 2",
                                           "t81lang_generic_function_explicit_type_args_bad_arity"),
               "t81lang_generic_function_explicit_type_args_bad_arity");

  constexpr const char* unresolved_generic_inference = R"(
    fn none_of[T]() -> Option[T] {
      return None;
    }
    fn main() -> i32 {
      let out = none_of();
      let _ = out;
      return 1;
    }
  )";
  require_true(
      fails_semantic_with_message(unresolved_generic_inference,
                                  "Cannot infer generic parameter 'T' for function 'none_of'.",
                                  "t81lang_generic_function_unresolved_inference"),
      "t81lang_generic_function_unresolved_inference");

  constexpr const char* unresolved_multiple_generic_inference = R"(
    fn none_pair[T, U]() -> Option[Result[T, U]] {
      return None;
    }
    fn main() -> i32 {
      let out = none_pair();
      let _ = out;
      return 1;
    }
  )";
  require_true(fails_semantic_with_message(
                   unresolved_multiple_generic_inference,
                   "Cannot infer generic parameters 'T', 'U' for function 'none_pair'.",
                   "t81lang_generic_function_unresolved_multiple_inference"),
               "t81lang_generic_function_unresolved_multiple_inference");
}

static void test_let_is_immutable() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let x: i32 = 1;
      x = 2;
      return x;
    }
  )";
  require_true(fails_semantic(source, "t81lang_let_immutable"), "t81lang_let_immutable");
}

static void test_var_is_mutable() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      var x: i32 = 1;
      x = 2;
      return x;
    }
  )";
  require_true(analyzes(source, "t81lang_var_mutable"), "t81lang_var_mutable");
}

static void test_vector_literal_is_immutable_lvalue() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      [1, 2, 3][0] = 5;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(source, "Cannot assign to immutable index expression",
                                           "t81lang_vector_literal_assignment_rejected"),
               "t81lang_vector_literal_assignment_rejected");
}

static void test_immutable_vector_variable_assignment_rejected() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let v: Vector[i32] = [1, 2, 3];
      v[0] = 5;
      return 0;
    }
  )";
  require_true(fails_semantic_with_message(source, "Cannot assign to immutable index expression",
                                           "t81lang_immutable_vector_variable_assignment_rejected"),
               "t81lang_immutable_vector_variable_assignment_rejected");
}

static void test_mutable_vector_variable_assignment_accepted() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      var v: Vector[i32] = [1, 2, 3];
      v[0] = 5;
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_mutable_vector_variable_assignment_accepted"),
               "t81lang_mutable_vector_variable_assignment_accepted");
}

int main() {
  test_baseline_supported_features();
  test_tier_annotation_supported_for_functions();
  test_t81_numeric_types_bind_and_widen();
  test_t81_mixed_numeric_widening_matrix();
  test_base81_integer_infers_bigint();
  test_base81_integer_does_not_silently_narrow_to_i32();
  test_base81_fraction_literal_is_native_t81fraction();
  test_base81_fraction_literal_does_not_silently_narrow();
  test_print_builtin_accepts_native_t81_numerics();
  test_print_builtin_rejects_bad_arity();
  test_std_namespace_builtin_aliases();
  test_std_tensor_from_list_alias();
  test_std_tensor_matmul_alias();
  test_std_tensor_vec_add_alias();
  test_std_math_tensor_module_wrappers();
  test_std_sys_async_agent_module_wrappers();
  test_std_text_aliases();
  test_std_text_module_wrappers();
  test_std_text_split_join_wrappers();
  test_std_bytes_aliases();
  test_std_bytes_module_wrappers();
  test_std_symbol_aliases();
  test_generic_function_inference();
  test_t81_numeric_type_separation_rejects_invalid_mix();
  test_let_is_immutable();
  test_var_is_mutable();
  test_vector_literal_is_immutable_lvalue();
  test_immutable_vector_variable_assignment_rejected();
  test_mutable_vector_variable_assignment_accepted();
  std::cout << "t81lang conformance baseline tests passed!\n";
  return 0;
}
