#include <cassert>
#include <iostream>
#include "t81/types/T81Graph.hpp"

using namespace t81;

void test_basic_operations() {
  T81Graph<10, 5> g;
  assert(g.nodes() == 10);
  assert(g.max_degree() == 5);
  g.add_edge(0, 1, Weight81(0.5));
  g.add_edge(0, 2, Weight81(1.0));
  assert(std::abs(g.weight(0, 1).to_double() - 0.5) < 1e-6);
  assert(std::abs(g.weight(0, 2).to_double() - 1.0) < 1e-6);
  [[maybe_unused]] auto outgoing = g.outgoing(0);
  assert(outgoing.size() == 2);
  std::cout << "test_basic_operations PASSED\n";
}

void test_pagerank() {
  T81Graph<3, 3> g;
  g.add_edge(0, 1);
  g.add_edge(1, 2);
  g.add_edge(2, 0);
  auto pr = pagerank(g, 10);
  assert(std::abs(pr(0).to_double() - 0.333) < 0.1);
  (void)pr;
  std::cout << "test_pagerank PASSED\n";
}

void test_symbolic_graph() {
  using SymGraph = T81Graph<5, 2, T81Symbol>;
  SymGraph g;

  auto loves = T81Symbol::intern("LOVES");
  auto hates = T81Symbol::intern("HATES");

  g.add_edge(0, 1, loves);
  g.add_edge(1, 0, hates);

  assert(g.weight(0, 1) == loves);
  assert(g.weight(1, 0) == hates);
  // Default constructed symbol should be empty/null
  assert(g.weight(0, 2) == T81Symbol());

  [[maybe_unused]] auto outgoing = g.outgoing(0);
  assert(outgoing.size() == 1);
  assert(outgoing[0].first == 1);
  assert(outgoing[0].second == loves);

  std::cout << "test_symbolic_graph PASSED\n";
}

int main() {
  test_basic_operations();
  test_pagerank();
  test_symbolic_graph();
  return 0;
}
