#pragma once

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "t81/types/T81Symbol.hpp"

namespace t81::cog::v1 {

struct SymbolicAtom {
  T81Symbol id;
  std::string label;

  static SymbolicAtom create(const std::string& label);
  bool operator==(const SymbolicAtom& other) const;
  bool operator<(const SymbolicAtom& other) const;
};

struct SymbolicEdge {
  T81Symbol from;
  T81Symbol to;
  std::string label;

  bool operator==(const SymbolicEdge& other) const;
  bool operator<(const SymbolicEdge& other) const;
};

struct RewriteRule {
  T81Symbol match_node;
  T81Symbol replace_node;
};

struct SymbolicGraph {
  std::vector<SymbolicAtom> nodes;
  std::vector<SymbolicEdge> edges;

  void add_node(const SymbolicAtom& node);
  void add_edge(const T81Symbol& from, const T81Symbol& to, const std::string& label = "");

  void apply_rewrite(const RewriteRule& rule);
  bool is_confluent() const;
  void canonicalize();
  
  // P2: Canonical serialization
  [[nodiscard]] std::string serialize_canonical() const {
    std::stringstream ss;
    ss << "{\n";
    
    // Sort nodes by ID for deterministic output
    std::vector<SymbolicAtom> sorted_nodes = nodes;
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](const auto& a, const auto& b) { return a.id < b.id; });
    
    // Sort edges by (from, to, label) for deterministic output
    std::vector<SymbolicEdge> sorted_edges = edges;
    std::sort(sorted_edges.begin(), sorted_edges.end(),
              [](const auto& a, const auto& b) {
                if (a.from != b.from) return a.from < b.from;
                if (a.to != b.to) return a.to < b.to;
                return a.label < b.label;
              });
    
    // Output nodes
    for (const auto& node : sorted_nodes) {
      ss << "  " << node.id.to_string() << ": \"" << node.label << "\",\n";
    }
    
    // Output edges
    for (const auto& edge : sorted_edges) {
      ss << "  " << edge.from.to_string() << " -> " << edge.to.to_string();
      if (!edge.label.empty()) {
        ss << " [\"" << edge.label << "\"]";
      }
      ss << ",\n";
    }
    
    ss << "}";
    return ss.str();
  }
};

}  // namespace t81::cog::v1
