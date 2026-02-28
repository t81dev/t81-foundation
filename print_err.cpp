#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include <iostream>

using namespace t81::frontend;

int main() {
  const char* source = R"(
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
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, "print_err");
  auto stmts = parser.parse();
  if (parser.had_error()) return 1;
  SemanticAnalyzer analyzer(stmts, "print_err");
  analyzer.analyze();
  for (const auto& diagnostic : analyzer.diagnostics()) {
    std::cout << diagnostic.line << ":" << diagnostic.column << ": " << diagnostic.message << "\n";
  }
  return 0;
}
