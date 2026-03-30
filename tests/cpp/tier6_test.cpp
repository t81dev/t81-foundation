#include <cassert>
#include <iostream>
#include <vector>

#include "t81/experimental/cog/tier6/distributed_monad.hpp"
#include "t81/tracing/canonhash.hpp"

using namespace t81::cog::v6;

void test_empty_mesh() {
  MeshReflector reflector;
  std::vector<MonadState> monads;

  auto result = reflector.mesh_reflect(monads);
  assert(result.converged);
  assert(result.convergence_hash == "EMPTY_MESH");
  assert(result.iterations == 0);
  std::cout << "Empty mesh test passed.\n";
}

void test_mesh_reflection_convergence() {
  MeshReflector reflector;
  std::vector<MonadState> monads;

  MonadState m1;
  m1.address.node_hash = t81::hash::hash_string("hash1");
  m1.label = "node1";

  MonadState m2;
  m2.address.node_hash = t81::hash::hash_string("hash2");
  m2.label = "node2";

  m1.peers.push_back(m2.address);
  m2.peers.push_back(m1.address);

  monads.push_back(m1);
  monads.push_back(m2);

  auto result = reflector.mesh_reflect(monads);
  assert(!result.converged);
  assert(result.iterations == 1);
  assert(monads[0].label == "node1+node2");
  assert(monads[1].label == "node2+node1");
  assert(monads[0].reflection_depth == 1);
  assert(monads[1].reflection_depth == 1);

  auto result2 = reflector.mesh_reflect(monads);
  assert(!result2.converged);
  assert(result2.iterations == 1);
  assert(monads[0].label == "node1+node2+node2+node1");
  assert(monads[1].label == "node2+node1+node1+node2");

  std::cout << "Mesh reflection test passed.\n";
}

void test_entropy_containment() {
  MeshReflector reflector(2);  // Set max depth to 2
  std::vector<MonadState> monads;

  MonadState m1;
  m1.address.node_hash = t81::hash::hash_string("hash1");
  m1.label = "node1";

  MonadState m2;
  m2.address.node_hash = t81::hash::hash_string("hash2");
  m2.label = "node2";

  m1.peers.push_back(m2.address);
  m2.peers.push_back(m1.address);

  monads.push_back(m1);
  monads.push_back(m2);

  auto r1 = reflector.mesh_reflect(monads);
  assert(!r1.converged);
  assert(monads[0].entropy_contained);

  auto r2 = reflector.mesh_reflect(monads);
  assert(!r2.converged);
  assert(!monads[0].entropy_contained);
  assert(!monads[1].entropy_contained);

  auto r3 = reflector.mesh_reflect(monads);
  assert(!r3.converged);
  assert(r3.convergence_hash == "ENTROPY_VIOLATION");

  std::cout << "Entropy containment test passed.\n";
}

int main() {
  test_empty_mesh();
  test_mesh_reflection_convergence();
  test_entropy_containment();
  std::cout << "All tier 6 tests passed.\n";
  return 0;
}
