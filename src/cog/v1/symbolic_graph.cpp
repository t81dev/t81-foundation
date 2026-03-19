#include "t81/cog/v1/symbolic_graph.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace t81::cog::v1 {

namespace {
std::vector<RewriteRule> ordered_rules(const RewriteProgram& program) {
  std::vector<RewriteRule> rules = program.rules;
  if (!program.canonical_rule_order) {
    return rules;
  }
  std::sort(rules.begin(), rules.end());
  rules.erase(std::unique(rules.begin(), rules.end()), rules.end());
  return rules;
}
}  // namespace

SymbolicAtom SymbolicAtom::create(const std::string& label) {
  SymbolicAtom atom;
  atom.id = T81Symbol::intern(label);
  atom.label = label;
  return atom;
}

bool SymbolicAtom::operator==(const SymbolicAtom& other) const { return id == other.id; }

bool SymbolicAtom::operator<(const SymbolicAtom& other) const { return id < other.id; }

bool SymbolicEdge::operator==(const SymbolicEdge& other) const {
  return from == other.from && to == other.to && label == other.label;
}

bool SymbolicEdge::operator<(const SymbolicEdge& other) const {
  if (from != other.from) return from < other.from;
  if (to != other.to) return to < other.to;
  return label < other.label;
}

bool RewriteRule::operator==(const RewriteRule& other) const {
  return match_node == other.match_node && replace_node == other.replace_node;
}

bool RewriteRule::operator<(const RewriteRule& other) const {
  if (match_node != other.match_node) return match_node < other.match_node;
  return replace_node < other.replace_node;
}

void SymbolicGraph::add_node(const SymbolicAtom& node) { nodes.push_back(node); }

void SymbolicGraph::add_edge(const T81Symbol& from, const T81Symbol& to, const std::string& label) {
  edges.push_back({from, to, label});
}

void SymbolicGraph::apply_rewrite(const RewriteRule& rule) {
  if (!rule.match_node.is_valid()) {
    return;
  }
  if (rule.match_node == rule.replace_node) {
    return;
  }

  bool touched = false;
  for (auto& node : nodes) {
    if (node.id == rule.match_node) {
      node.id = rule.replace_node;
      node.label = rule.replace_node.to_string();
      touched = true;
    }
  }
  for (auto& edge : edges) {
    if (edge.from == rule.match_node) {
      edge.from = rule.replace_node;
      touched = true;
    }
    if (edge.to == rule.match_node) {
      edge.to = rule.replace_node;
      touched = true;
    }
  }

  if (touched) {
    canonicalize();
  }
}

RewriteExecutionResult SymbolicGraph::apply_program(const RewriteProgram& program) {
  RewriteExecutionResult result;
  const std::vector<RewriteRule> rules = ordered_rules(program);

  if (program.max_passes == 0 || rules.empty()) {
    result.converged = true;
    return result;
  }

  for (std::size_t pass = 0; pass < program.max_passes; ++pass) {
    bool changed_this_pass = false;
    ++result.passes_executed;

    for (const auto& rule : rules) {
      const std::string before = serialize_canonical();
      apply_rewrite(rule);
      const std::string after = serialize_canonical();
      if (before != after) {
        changed_this_pass = true;
        ++result.rewrites_applied;
      }
    }

    if (!changed_this_pass) {
      result.converged = true;
      return result;
    }
  }

  result.converged = false;
  return result;
}

bool SymbolicGraph::is_confluent() const {
  // Tier-1 check: graph is confluent when rewrite traversal is deterministic.
  // We enforce:
  // 1) unique node identities
  // 2) all edges refer to existing nodes
  // 3) for any (from,label), there is at most one destination.
  std::unordered_set<T81Symbol> node_ids;
  node_ids.reserve(nodes.size());
  for (const auto& node : nodes) {
    if (!node.id.is_valid()) {
      return false;
    }
    if (!node_ids.insert(node.id).second) {
      return false;
    }
  }

  struct TransitionKey {
    T81Symbol from;
    std::string label;

    bool operator==(const TransitionKey& other) const {
      return from == other.from && label == other.label;
    }
  };

  struct TransitionKeyHash {
    std::size_t operator()(const TransitionKey& k) const noexcept {
      std::size_t h1 = std::hash<T81Symbol>{}(k.from);
      std::size_t h2 = std::hash<std::string>{}(k.label);
      return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
  };

  std::unordered_map<TransitionKey, T81Symbol, TransitionKeyHash> transition_targets;
  transition_targets.reserve(edges.size());
  for (const auto& edge : edges) {
    if (!edge.from.is_valid() || !edge.to.is_valid()) {
      return false;
    }
    if (!node_ids.contains(edge.from) || !node_ids.contains(edge.to)) {
      return false;
    }

    TransitionKey key{edge.from, edge.label};
    auto it = transition_targets.find(key);
    if (it == transition_targets.end()) {
      transition_targets.emplace(std::move(key), edge.to);
      continue;
    }
    if (it->second != edge.to) {
      return false;
    }
  }

  return true;
}

ConfluenceReport SymbolicGraph::check_rule_confluence(const RewriteProgram& program) {
  ConfluenceReport report;
  if (program.rules.empty()) {
    report.confluent = true;
    report.reason = "empty rule set";
    return report;
  }

  std::unordered_map<T81Symbol, std::pair<T81Symbol, std::size_t>> replacement_for_match;
  replacement_for_match.reserve(program.rules.size());
  for (std::size_t i = 0; i < program.rules.size(); ++i) {
    const auto& rule = program.rules[i];
    if (!rule.match_node.is_valid() || !rule.replace_node.is_valid()) {
      report.confluent = false;
      report.left_rule_index = i;
      report.reason = "invalid symbol in rewrite rule";
      return report;
    }

    auto it = replacement_for_match.find(rule.match_node);
    if (it == replacement_for_match.end()) {
      replacement_for_match.emplace(rule.match_node, std::make_pair(rule.replace_node, i));
      continue;
    }
    if (it->second.first != rule.replace_node) {
      report.confluent = false;
      report.left_rule_index = it->second.second;
      report.right_rule_index = i;
      report.reason = "conflicting replacements for same match node";
      return report;
    }
  }

  for (std::size_t i = 0; i < program.rules.size(); ++i) {
    const auto& left = program.rules[i];
    for (std::size_t j = i + 1; j < program.rules.size(); ++j) {
      const auto& right = program.rules[j];
      if (left.match_node == right.replace_node && left.replace_node == right.match_node &&
          left.match_node != left.replace_node) {
        report.confluent = false;
        report.left_rule_index = i;
        report.right_rule_index = j;
        report.reason = "rewrite cycle between rule pair";
        return report;
      }
    }
  }

  report.confluent = true;
  report.reason = "rule set locally confluent under bounded checks";
  return report;
}

void SymbolicGraph::canonicalize() {
  std::sort(nodes.begin(), nodes.end());
  std::sort(edges.begin(), edges.end());
  nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
}

}  // namespace t81::cog::v1
