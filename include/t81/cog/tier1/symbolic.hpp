#pragma once

#include <string>
#include <vector>
#include "t81/core/T81Symbol.hpp"

namespace t81::cog::v1 {

struct SymbolicAtom {
  T81Symbol id;
  std::string label;

  static SymbolicAtom create(const std::string& label);
};

struct SymbolicEdge {
  T81Symbol from;
  T81Symbol to;
};

struct SymbolicGraph {
  std::vector<SymbolicAtom> nodes;
  std::vector<SymbolicEdge> edges;

  void add_node(const SymbolicAtom& node);
  void add_edge(const T81Symbol& from, const T81Symbol& to);
};

}  // namespace t81::cog::v1
