#include <iostream>
#include <stdexcept>

#include "t81/cog/v1/symbolic_graph.hpp"

namespace {
using t81::T81Symbol;
using t81::cog::v1::ConfluenceReport;
using t81::cog::v1::RewriteProgram;
using t81::cog::v1::RewriteRule;
using t81::cog::v1::SymbolicAtom;
using t81::cog::v1::SymbolicGraph;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

SymbolicGraph make_linear_graph() {
  SymbolicGraph g;
  auto a = SymbolicAtom::create("a");
  auto b = SymbolicAtom::create("b");
  auto c = SymbolicAtom::create("c");
  g.add_node(a);
  g.add_node(b);
  g.add_node(c);
  g.add_edge(a.id, b.id, "rw");
  g.add_edge(b.id, c.id, "rw");
  g.canonicalize();
  return g;
}

void test_deterministic_canonical_rule_order() {
  const auto a = T81Symbol::intern("a");
  const auto b = T81Symbol::intern("b");
  const auto c = T81Symbol::intern("c");

  RewriteProgram unsorted_rules;
  unsorted_rules.max_passes = 1;
  unsorted_rules.canonical_rule_order = true;
  unsorted_rules.rules = {{b, c}, {a, b}};

  RewriteProgram sorted_rules = unsorted_rules;
  sorted_rules.rules = {{a, b}, {b, c}};

  RewriteProgram input_order_rules = unsorted_rules;
  input_order_rules.canonical_rule_order = false;

  auto g1 = make_linear_graph();
  auto g2 = make_linear_graph();
  auto g3 = make_linear_graph();

  const auto r1 = g1.apply_program(unsorted_rules);
  const auto r2 = g2.apply_program(sorted_rules);
  const auto r3 = g3.apply_program(input_order_rules);

  require(r1.rewrites_applied == 2, "expected 2 rewrites for canonical unsorted rules");
  require(r2.rewrites_applied == 2, "expected 2 rewrites for canonical sorted rules");
  require(g1.serialize_canonical() == g2.serialize_canonical(),
          "canonical rule order should produce identical graph regardless of input rule order");
  require(g1.serialize_canonical() != g3.serialize_canonical(),
          "input-order rewriting should differ from canonical rewriting for this rule set");
  require(!r1.converged, "expected non-converged result at max_passes=1 with active rewrites");
  require(!r3.converged, "expected non-converged result at max_passes=1 with active rewrites");
}

void test_rule_confluence_conflict_detection() {
  const auto a = T81Symbol::intern("a");
  const auto b = T81Symbol::intern("b");
  const auto c = T81Symbol::intern("c");

  RewriteProgram p;
  p.rules = {{a, b}, {a, c}};

  const ConfluenceReport report = SymbolicGraph::check_rule_confluence(p);
  require(!report.confluent, "conflicting same-match rewrite rules must be non-confluent");
  require(report.left_rule_index.has_value(), "conflict report must include left rule index");
  require(report.right_rule_index.has_value(), "conflict report must include right rule index");
}

void test_rule_confluence_cycle_detection() {
  const auto a = T81Symbol::intern("a");
  const auto b = T81Symbol::intern("b");

  RewriteProgram p;
  p.rules = {{a, b}, {b, a}};

  const ConfluenceReport report = SymbolicGraph::check_rule_confluence(p);
  require(!report.confluent, "direct rewrite cycles must be non-confluent");
  require(report.left_rule_index.has_value(), "cycle report must include left rule index");
  require(report.right_rule_index.has_value(), "cycle report must include right rule index");
}

void test_rule_confluence_ok() {
  const auto a = T81Symbol::intern("a");
  const auto b = T81Symbol::intern("b");
  const auto c = T81Symbol::intern("c");

  RewriteProgram p;
  p.rules = {{a, b}, {b, c}};

  const ConfluenceReport report = SymbolicGraph::check_rule_confluence(p);
  require(report.confluent, "compatible chained rewrites should be confluent under bounded checks");
}
}  // namespace

int main() {
  test_deterministic_canonical_rule_order();
  test_rule_confluence_conflict_detection();
  test_rule_confluence_cycle_detection();
  test_rule_confluence_ok();
  std::cout << "tier1_rewrite_confluence_test passed\n";
  return 0;
}
