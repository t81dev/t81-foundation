#include "t81/cli/driver.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {
fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
  static std::mt19937_64 rng{std::random_device{}()};
  [[maybe_unused]] std::uniform_int_distribution<uint64_t> dist;
  return fs::temp_directory_path() / (prefix + "-" + std::to_string(dist(rng)) + extension);
}

void write_source(const fs::path& path, std::string_view contents) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Failed to open source file: " + path.string());
  }
  out << contents;
  out.flush();
}
}  // namespace

int main() {
  const std::string minimal_program = R"(
        fn main() -> i32 {
            return 0;
        }
    )";

  [[maybe_unused]] auto source_path = make_temp_path("t81-check", ".t81");
  write_source(source_path, minimal_program);
  assert(t81::cli::check_syntax(source_path) == 0);
  fs::remove(source_path);

  const std::string broken_program = R"(
        fn main() -> i32 {
            let bad: i8 = 1.5;
            return 0;
        }
    )";

  [[maybe_unused]] auto broken_path = make_temp_path("t81-check-fail", ".t81");
  write_source(broken_path, broken_program);

  [[maybe_unused]] std::ostringstream captured;
  auto* old_buf = std::cerr.rdbuf(captured.rdbuf());
  [[maybe_unused]] int rc = t81::cli::check_syntax(broken_path);
  std::cerr.rdbuf(old_buf);

  if (rc == 0) {
    std::cerr << "Expected `t81 check` to fail on invalid input\n";
    return 1;
  }
  [[maybe_unused]] std::string output = captured.str();
  assert(output.find(broken_path.string()) != std::string::npos);
  assert(output.find("Cannot assign initializer") != std::string::npos);

  fs::remove(broken_path);

  const std::string split_program = R"(
        fn main() -> i32 {
            let parts: Vector[T81String] = std.text.split("alpha", ",");
            let _ = parts;
            return 0;
        }
    )";

  [[maybe_unused]] auto split_path = make_temp_path("t81-check-split-fail", ".t81");
  write_source(split_path, split_program);

  [[maybe_unused]] std::ostringstream split_captured;
  old_buf = std::cerr.rdbuf(split_captured.rdbuf());
  [[maybe_unused]] int split_rc = t81::cli::check_syntax(split_path);
  std::cerr.rdbuf(old_buf);

  if (split_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on valid split input\n";
    std::cerr << split_captured.str() << "\n";
    return 1;
  }

  fs::remove(split_path);

  const std::string join_program = R"(
        fn main() -> i32 {
            let joined: T81String = std.text.join(["a", "b"], ",");
            let _ = joined;
            return 0;
        }
    )";

  [[maybe_unused]] auto join_path = make_temp_path("t81-check-join-fail", ".t81");
  write_source(join_path, join_program);

  [[maybe_unused]] std::ostringstream join_captured;
  old_buf = std::cerr.rdbuf(join_captured.rdbuf());
  [[maybe_unused]] int join_rc = t81::cli::check_syntax(join_path);
  std::cerr.rdbuf(old_buf);

  if (join_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on valid join input\n";
    std::cerr << join_captured.str() << "\n";
    return 1;
  }

  fs::remove(join_path);

  const std::string bytes_split_join_program = R"(
        fn main() -> i32 {
            let parts: Vector[T81Bytes] = std.bytes.split(T81Bytes("a,,b"), T81Bytes(","));
            let joined: T81Bytes = std.bytes.join(parts, T81Bytes(","));
            let _ = joined;
            return 0;
        }
    )";

  [[maybe_unused]] auto bytes_split_join_path =
      make_temp_path("t81-check-bytes-split-join", ".t81");
  write_source(bytes_split_join_path, bytes_split_join_program);

  [[maybe_unused]] std::ostringstream bytes_split_join_captured;
  old_buf = std::cerr.rdbuf(bytes_split_join_captured.rdbuf());
  [[maybe_unused]] int bytes_split_join_rc = t81::cli::check_syntax(bytes_split_join_path);
  std::cerr.rdbuf(old_buf);

  if (bytes_split_join_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on valid bytes split/join input\n";
    std::cerr << bytes_split_join_captured.str() << "\n";
    return 1;
  }

  fs::remove(bytes_split_join_path);

  const std::string symbol_program = R"(
        fn main() -> i32 {
            let sym: Symbol = std.symbol.intern("omega");
            let rendered: T81String = std.symbol.to_string(sym);
            let s_omega: Symbol = std.symbol.intern("omega");
            let s_alpha: Symbol = std.symbol.intern("alpha");
            let same: bool = std.symbol.eq(sym, s_omega);
            let diff: bool = std.symbol.ne(sym, s_alpha);
            std.core.assert(same);
            std.core.debug(rendered);
            let present: Option[i32] = Some(7);
            let keep: i32 = std.core.unwrap_or(present, 9);
            let _ = same;
            let _d = diff;
            let _r = rendered;
            let _k = keep;
            return 0;
        }
    )";

  [[maybe_unused]] auto symbol_path = make_temp_path("t81-check-symbol", ".t81");
  write_source(symbol_path, symbol_program);

  [[maybe_unused]] std::ostringstream symbol_captured;
  old_buf = std::cerr.rdbuf(symbol_captured.rdbuf());
  [[maybe_unused]] int symbol_rc = t81::cli::check_syntax(symbol_path);
  std::cerr.rdbuf(old_buf);

  if (symbol_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on valid symbol input\n";
    std::cerr << symbol_captured.str() << "\n";
    return 1;
  }

  fs::remove(symbol_path);

  const std::string sys_async_agent_program = R"(
        fn main() -> i32 {
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
            let list_v: Vector[T81String] = std.collections.list();
            let map_v: Vector[T81String] = std.collections.map();
            let map_flat: Vector[T81String] = ["city", "sf", "lang", "t81"];
            let set_v: Vector[T81String] = std.collections.set();
            let tree_v: Vector[T81String] = std.collections.tree();
            let graph_v: Vector[T81String] = std.collections.graph();
            std.agent.self_reflect();
            std.sys.exit(0);
            let _ent = ent;
            let _proof = proof;
            let _stream_h = stream_h;
            let _net_h = net_h;
            let _thread_h = thread_h;
            let _promise_h = promise_h;
            let _list_h = std.collections.len(list_v);
            let _map_h = std.collections.len(map_v);
            let _map_pairs = std.collections.map_size(map_flat);
            let _map_has_city = std.collections.map_has(map_flat, "city");
            let map_updated: Vector[T81String] = std.collections.map_put(map_flat, "city", "oakland");
            let map_keys: Vector[T81String] = std.collections.map_keys(map_updated);
            let map_removed: Vector[T81String] = std.collections.map_remove(map_updated, "lang");
            let map_lookup: Option[T81String] = std.collections.map_get(map_removed, "city");
            let _map_keys_len = std.collections.len(map_keys);
            let _map_removed_pairs = std.collections.map_size(map_removed);
            let _map_lookup_has = std.collections.map_has(map_removed, "city");
            let _map_lookup_value = match (map_lookup) {
                Some(v) => v;
                None => "none";
            };
            let set_flat: Vector[T81String] = ["city", "lang", "city"];
            let set_added: Vector[T81String] = std.collections.set_add(set_flat, "edge");
            let set_added_dup: Vector[T81String] = std.collections.set_add(set_added, "city");
            let set_removed: Vector[T81String] = std.collections.set_remove(set_added_dup, "lang");
            let _set_size = std.collections.set_size(set_flat);
            let _set_has_city = std.collections.set_has(set_flat, "city");
            let _set_added_size = std.collections.set_size(set_added);
            let _set_added_dup_size = std.collections.set_size(set_added_dup);
            let _set_removed_size = std.collections.set_size(set_removed);
            let _set_removed_has_lang = std.collections.set_has(set_removed, "lang");
            let _set_h = std.collections.len(set_v);
            let _tree_h = std.collections.len(tree_v);
            let _graph_h = std.collections.len(graph_v);
            let graph_edges: Vector[T81String] = std.collections.graph_add_edge(graph_v, "a", "b");
            let graph_edges_dup: Vector[T81String] = std.collections.graph_add_edge(graph_edges, "a", "b");
            let graph_edges_removed: Vector[T81String] = std.collections.graph_remove_edge(graph_edges_dup, "a", "b");
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

  [[maybe_unused]] auto sys_async_agent_path = make_temp_path("t81-check-sys-async-agent", ".t81");
  write_source(sys_async_agent_path, sys_async_agent_program);

  [[maybe_unused]] std::ostringstream sys_async_agent_captured;
  old_buf = std::cerr.rdbuf(sys_async_agent_captured.rdbuf());
  [[maybe_unused]] int sys_async_agent_rc = t81::cli::check_syntax(sys_async_agent_path);
  std::cerr.rdbuf(old_buf);

  if (sys_async_agent_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on std.sys/std.async/std.agent input\n";
    std::cerr << sys_async_agent_captured.str() << "\n";
    return 1;
  }

  fs::remove(sys_async_agent_path);

  const std::string bad_sys_entropy_arity_program = R"(
        fn main() -> i32 {
            let ent: i32 = std.sys.entropy(1);
            let _ = ent;
            return 0;
        }
    )";

  [[maybe_unused]] auto bad_sys_entropy_arity_path =
      make_temp_path("t81-check-sys-entropy-arity-bad", ".t81");
  write_source(bad_sys_entropy_arity_path, bad_sys_entropy_arity_program);

  [[maybe_unused]] std::ostringstream bad_sys_entropy_arity_captured;
  old_buf = std::cerr.rdbuf(bad_sys_entropy_arity_captured.rdbuf());
  [[maybe_unused]] int bad_sys_entropy_arity_rc =
      t81::cli::check_syntax(bad_sys_entropy_arity_path);
  std::cerr.rdbuf(old_buf);

  if (bad_sys_entropy_arity_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on std.sys.entropy bad arity\n";
    return 1;
  }
  [[maybe_unused]] std::string bad_sys_entropy_arity_output = bad_sys_entropy_arity_captured.str();
  assert(bad_sys_entropy_arity_output.find(bad_sys_entropy_arity_path.string()) !=
         std::string::npos);
  assert(bad_sys_entropy_arity_output.find("sys_entropy expects no arguments.") !=
         std::string::npos);
  fs::remove(bad_sys_entropy_arity_path);

  const std::string bad_sys_reflect_arity_program = R"(
        fn main() -> i32 {
            std.sys.reflect(1);
            return 0;
        }
    )";

  [[maybe_unused]] auto bad_sys_reflect_arity_path =
      make_temp_path("t81-check-sys-reflect-arity-bad", ".t81");
  write_source(bad_sys_reflect_arity_path, bad_sys_reflect_arity_program);

  [[maybe_unused]] std::ostringstream bad_sys_reflect_arity_captured;
  old_buf = std::cerr.rdbuf(bad_sys_reflect_arity_captured.rdbuf());
  [[maybe_unused]] int bad_sys_reflect_arity_rc =
      t81::cli::check_syntax(bad_sys_reflect_arity_path);
  std::cerr.rdbuf(old_buf);

  if (bad_sys_reflect_arity_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on std.sys.reflect bad arity\n";
    return 1;
  }
  [[maybe_unused]] std::string bad_sys_reflect_arity_output = bad_sys_reflect_arity_captured.str();
  assert(bad_sys_reflect_arity_output.find(bad_sys_reflect_arity_path.string()) !=
         std::string::npos);
  assert(bad_sys_reflect_arity_output.find("sys_reflect expects no arguments.") !=
         std::string::npos);
  fs::remove(bad_sys_reflect_arity_path);

  const std::string bad_runtime_aliases_arity_program = R"(
        fn main() -> i32 {
            let p: T81String = std.sys.proof(1);
            let s: T81String = std.io.stream(1);
            let t: T81String = std.async.thread(1);
            let g: i32 = std.collections.graph(1);
            let _ = p;
            let _s = s;
            let _t = t;
            let _g = g;
            return 0;
        }
    )";

  [[maybe_unused]] auto bad_runtime_aliases_arity_path =
      make_temp_path("t81-check-runtime-aliases-arity-bad", ".t81");
  write_source(bad_runtime_aliases_arity_path, bad_runtime_aliases_arity_program);

  [[maybe_unused]] std::ostringstream bad_runtime_aliases_arity_captured;
  old_buf = std::cerr.rdbuf(bad_runtime_aliases_arity_captured.rdbuf());
  [[maybe_unused]] int bad_runtime_aliases_arity_rc =
      t81::cli::check_syntax(bad_runtime_aliases_arity_path);
  std::cerr.rdbuf(old_buf);

  if (bad_runtime_aliases_arity_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on runtime handle alias bad arity\n";
    return 1;
  }
  [[maybe_unused]] std::string bad_runtime_aliases_arity_output =
      bad_runtime_aliases_arity_captured.str();
  assert(bad_runtime_aliases_arity_output.find(bad_runtime_aliases_arity_path.string()) !=
         std::string::npos);
  assert(bad_runtime_aliases_arity_output.find("expects no arguments") != std::string::npos);
  fs::remove(bad_runtime_aliases_arity_path);

  const std::string bad_collections_map_helper_program = R"(
        fn main() -> i32 {
            let m: Vector[T81String] = std.collections.map();
            let out: Vector[T81String] = std.collections.map_put(m, "k", 7);
            let _ = out;
            return 0;
        }
    )";

  [[maybe_unused]] auto bad_collections_map_helper_path =
      make_temp_path("t81-check-collections-map-helper-bad-types", ".t81");
  write_source(bad_collections_map_helper_path, bad_collections_map_helper_program);

  [[maybe_unused]] std::ostringstream bad_collections_map_helper_captured;
  old_buf = std::cerr.rdbuf(bad_collections_map_helper_captured.rdbuf());
  [[maybe_unused]] int bad_collections_map_helper_rc =
      t81::cli::check_syntax(bad_collections_map_helper_path);
  std::cerr.rdbuf(old_buf);

  if (bad_collections_map_helper_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on std.collections map helper alias\n";
    return 1;
  }
  [[maybe_unused]] std::string bad_collections_map_helper_output =
      bad_collections_map_helper_captured.str();
  assert(bad_collections_map_helper_output.find(bad_collections_map_helper_path.string()) !=
         std::string::npos);
  assert(bad_collections_map_helper_output.find(
             "std.collections.map_put expects T81String key/value arguments.") !=
         std::string::npos);
  fs::remove(bad_collections_map_helper_path);

  const std::string transcendental_math_program = R"(
        fn main() -> i32 {
            let a1: T81Float = std.math.asin(0.5);
            let a2: T81Float = std.math.acos(0.5);
            let a3: T81Float = std.math.atan(1.0);
            let h1: T81Float = std.math.sinh(1.0);
            let h2: T81Float = std.math.cosh(1.0);
            let h3: T81Float = std.math.tanh(1.0);
            let _a1 = a1;
            let _a2 = a2;
            let _a3 = a3;
            let _h1 = h1;
            let _h2 = h2;
            let _h3 = h3;
            return 0;
        }
    )";

  [[maybe_unused]] auto transcendental_math_path =
      make_temp_path("t81-check-math-transcendental", ".t81");
  write_source(transcendental_math_path, transcendental_math_program);

  [[maybe_unused]] std::ostringstream transcendental_math_captured;
  old_buf = std::cerr.rdbuf(transcendental_math_captured.rdbuf());
  [[maybe_unused]] int transcendental_math_rc = t81::cli::check_syntax(transcendental_math_path);
  std::cerr.rdbuf(old_buf);

  if (transcendental_math_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on std.math transcendental alias input\n";
    std::cerr << transcendental_math_captured.str() << "\n";
    return 1;
  }
  fs::remove(transcendental_math_path);

  const std::string clamp_collections_program = R"(
        fn first[T, U](a: T, b: U) -> T {
            return a;
        }
        fn main() -> i32 {
            let clamped: T81Float = std.math.clamp(-2.0, 0.0, 1.0);
            let ints: Vector[i32] = [1, 2, 3];
            let n: i32 = std.collections.len(ints);
            let e: bool = std.collections.is_empty(ints);
            let f: i32 = std.collections.first(ints);
            let l: i32 = std.collections.last(ints);
            let pushed: Vector[i32] = std.collections.push(ints, 4);
            let popped: Vector[i32] = std.collections.pop(pushed);
            let partial: i32 = first[i32](7, "tail");
            let _c = clamped;
            let _n = n;
            let _e = e;
            let _f = f;
            let _l = l;
            let _p = popped;
            let _partial = partial;
            return 0;
        }
    )";

  [[maybe_unused]] auto clamp_collections_path =
      make_temp_path("t81-check-clamp-collections", ".t81");
  write_source(clamp_collections_path, clamp_collections_program);

  [[maybe_unused]] std::ostringstream clamp_collections_captured;
  old_buf = std::cerr.rdbuf(clamp_collections_captured.rdbuf());
  [[maybe_unused]] int clamp_collections_rc = t81::cli::check_syntax(clamp_collections_path);
  std::cerr.rdbuf(old_buf);

  if (clamp_collections_rc != 0) {
    std::cerr << "Expected `t81 check` to succeed on std.math.clamp/std.collections input\n";
    std::cerr << clamp_collections_captured.str() << "\n";
    return 1;
  }
  fs::remove(clamp_collections_path);

  const std::string generic_explicit_bad_program = R"(
        fn id[T](x: T) -> T {
            return x;
        }
        fn main() -> i32 {
            let out: i32 = id[i32]("oops");
            return out;
        }
    )";

  [[maybe_unused]] auto generic_explicit_bad_path =
      make_temp_path("t81-check-generic-explicit-bad", ".t81");
  write_source(generic_explicit_bad_path, generic_explicit_bad_program);

  [[maybe_unused]] std::ostringstream generic_explicit_bad_captured;
  old_buf = std::cerr.rdbuf(generic_explicit_bad_captured.rdbuf());
  [[maybe_unused]] int generic_explicit_bad_rc = t81::cli::check_syntax(generic_explicit_bad_path);
  std::cerr.rdbuf(old_buf);

  if (generic_explicit_bad_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on explicit generic type argument mismatch\n";
    return 1;
  }
  [[maybe_unused]] std::string generic_explicit_bad_output = generic_explicit_bad_captured.str();
  assert(generic_explicit_bad_output.find(generic_explicit_bad_path.string()) != std::string::npos);
  assert(generic_explicit_bad_output.find("expects 'i32' but got 'T81String'") !=
         std::string::npos);
  fs::remove(generic_explicit_bad_path);

  const std::string generic_explicit_arity_bad_program = R"(
        fn id[T](x: T) -> T {
            return x;
        }
        fn main() -> i32 {
            let out: i32 = id[i32, T81String](7);
            return out;
        }
    )";

  [[maybe_unused]] auto generic_explicit_arity_bad_path =
      make_temp_path("t81-check-generic-explicit-arity-bad", ".t81");
  write_source(generic_explicit_arity_bad_path, generic_explicit_arity_bad_program);

  [[maybe_unused]] std::ostringstream generic_explicit_arity_bad_captured;
  old_buf = std::cerr.rdbuf(generic_explicit_arity_bad_captured.rdbuf());
  [[maybe_unused]] int generic_explicit_arity_bad_rc =
      t81::cli::check_syntax(generic_explicit_arity_bad_path);
  std::cerr.rdbuf(old_buf);

  if (generic_explicit_arity_bad_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on explicit generic type argument arity mismatch\n";
    return 1;
  }
  [[maybe_unused]] std::string generic_explicit_arity_bad_output =
      generic_explicit_arity_bad_captured.str();
  assert(generic_explicit_arity_bad_output.find(generic_explicit_arity_bad_path.string()) !=
         std::string::npos);
  assert(generic_explicit_arity_bad_output.find(
             "expects 1 explicit type arguments at most but got 2") != std::string::npos);
  fs::remove(generic_explicit_arity_bad_path);

  const std::string generic_unresolved_inference_bad_program = R"(
        fn none_of[T]() -> Option[T] {
            return None;
        }
        fn main() -> i32 {
            let out = none_of();
            return 0;
        }
    )";

  [[maybe_unused]] auto generic_unresolved_inference_bad_path =
      make_temp_path("t81-check-generic-unresolved-inference-bad", ".t81");
  write_source(generic_unresolved_inference_bad_path, generic_unresolved_inference_bad_program);

  [[maybe_unused]] std::ostringstream generic_unresolved_inference_bad_captured;
  old_buf = std::cerr.rdbuf(generic_unresolved_inference_bad_captured.rdbuf());
  [[maybe_unused]] int generic_unresolved_inference_bad_rc =
      t81::cli::check_syntax(generic_unresolved_inference_bad_path);
  std::cerr.rdbuf(old_buf);

  if (generic_unresolved_inference_bad_rc == 0) {
    std::cerr
        << "Expected `t81 check` to fail on unresolved generic inference for call return type\n";
    return 1;
  }
  [[maybe_unused]] std::string generic_unresolved_inference_bad_output =
      generic_unresolved_inference_bad_captured.str();
  assert(generic_unresolved_inference_bad_output.find(
             generic_unresolved_inference_bad_path.string()) != std::string::npos);
  assert(generic_unresolved_inference_bad_output.find(
             "Cannot infer generic parameter 'T' for function 'none_of'.") != std::string::npos);
  fs::remove(generic_unresolved_inference_bad_path);

  const std::string generic_unresolved_multiple_inference_bad_program = R"(
        fn none_pair[T, U]() -> Option[Result[T, U]] {
            return None;
        }
        fn main() -> i32 {
            let out = none_pair();
            return 0;
        }
    )";

  [[maybe_unused]] auto generic_unresolved_multiple_inference_bad_path =
      make_temp_path("t81-check-generic-unresolved-multiple-inference-bad", ".t81");
  write_source(generic_unresolved_multiple_inference_bad_path,
               generic_unresolved_multiple_inference_bad_program);

  [[maybe_unused]] std::ostringstream generic_unresolved_multiple_inference_bad_captured;
  old_buf = std::cerr.rdbuf(generic_unresolved_multiple_inference_bad_captured.rdbuf());
  [[maybe_unused]] int generic_unresolved_multiple_inference_bad_rc =
      t81::cli::check_syntax(generic_unresolved_multiple_inference_bad_path);
  std::cerr.rdbuf(old_buf);

  if (generic_unresolved_multiple_inference_bad_rc == 0) {
    std::cerr << "Expected `t81 check` to fail on unresolved multiple generic inference for call\n";
    return 1;
  }
  [[maybe_unused]] std::string generic_unresolved_multiple_inference_bad_output =
      generic_unresolved_multiple_inference_bad_captured.str();
  assert(generic_unresolved_multiple_inference_bad_output.find(
             generic_unresolved_multiple_inference_bad_path.string()) != std::string::npos);
  assert(generic_unresolved_multiple_inference_bad_output.find(
             "Cannot infer generic parameters 'T', 'U' for function 'none_pair'.") !=
         std::string::npos);
  fs::remove(generic_unresolved_multiple_inference_bad_path);

  std::cout << "CliCheckTest passed!" << std::endl;
  return 0;
}
