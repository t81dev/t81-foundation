#include "t81/experimental/cog/tier6/distributed_monad.hpp"

#include <algorithm>
#include <sstream>

namespace t81::cog::v6 {

bool MeshReflector::entropy_check(const std::vector<MonadState>& monads) const {
  for (const auto& m : monads) {
    if (m.reflection_depth >= max_depth_) {
      return false;  // Θ₇ violated — entropy not contained.
    }
  }
  return true;
}

ReflectionResult MeshReflector::mesh_reflect(std::vector<MonadState>& monads) {
  ReflectionResult result;

  if (monads.empty()) {
    result.converged = true;
    result.convergence_hash = "EMPTY_MESH";
    return result;
  }

  // Θ₇ pre-check: refuse to iterate if any monad already exceeds max_depth_.
  if (!entropy_check(monads)) {
    result.converged = false;
    result.convergence_hash = "ENTROPY_VIOLATION";
    return result;
  }

  // One round: each monad reflects its peer labels and increments depth.
  bool any_changed = false;
  for (auto& m : monads) {
    const std::string prev_label = m.label;
    // Incorporate peer labels into this monad's label (deterministic fold).
    std::string composite = m.label;
    for (const auto& peer_addr : m.peers) {
      // Find the peer monad by node_hash comparison.
      auto it = std::find_if(monads.begin(), monads.end(), [&](const MonadState& p) {
        return p.address.node_hash.bytes == peer_addr.node_hash.bytes;
      });
      if (it != monads.end()) {
        composite += "+" + it->label;
      }
    }
    m.reflection_depth++;
    m.label = composite;
    if (m.label != prev_label) any_changed = true;

    // Θ₇ post-step: mark entropy violation if depth limit reached.
    if (m.reflection_depth >= max_depth_) {
      m.entropy_contained = false;
    }
  }

  result.iterations++;

  // Compute a stable convergence signature: XOR of all label hashes.
  std::string combined;
  for (const auto& m : monads) combined += m.label;
  auto hash = t81::hash::hash_string(combined);
  result.convergence_hash = hash.to_string();
  result.converged = !any_changed;

  return result;
}

}  // namespace t81::cog::v6
