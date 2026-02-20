#include "t81/cog/tier1/symbolic.hpp"

namespace t81::cog::v1 {

SymbolicAtom SymbolicAtom::create(const std::string& label) {
  SymbolicAtom atom;
  atom.id = T81Symbol::intern(label);
  atom.label = label;
  return atom;
}

void SymbolicGraph::add_node(const SymbolicAtom& node) { nodes.push_back(node); }

void SymbolicGraph::add_edge(const T81Symbol& from, const T81Symbol& to) {
  edges.push_back({from, to});
}

}  // namespace t81::cog::v1
