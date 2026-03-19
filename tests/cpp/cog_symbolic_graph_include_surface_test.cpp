#include "t81/cog/v1/symbolic_graph.hpp"

int main() {
  t81::cog::v1::SymbolicGraph graph;
  auto atom = t81::cog::v1::SymbolicAtom::create("include_surface");
  graph.add_node(atom);
  graph.canonicalize();
  return graph.nodes.size() == 1 ? 0 : 1;
}
