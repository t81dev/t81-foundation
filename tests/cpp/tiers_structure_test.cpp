#include <cassert>
#include <iostream>

#include "t81/cog/v1/symbolic_graph.hpp"
#include "t81/experimental/cog/tier2/reflective.hpp"
#include "t81/experimental/cog/tier3/recursive.hpp"
#include "t81/experimental/cog/tier5/infinite.hpp"
#include "t81/experimental/distributed/distributed.hpp"

// Simple test to ensure all tier structures can be instantiated and basic methods called.
int main() {
  // Tier 1: Symbolic
  {
    t81::cog::v1::SymbolicGraph graph;
    auto atom_a = t81::cog::v1::SymbolicAtom::create("test_atom_a");
    auto atom_b = t81::cog::v1::SymbolicAtom::create("test_atom_b");
    auto atom_c = t81::cog::v1::SymbolicAtom::create("test_atom_c");
    graph.add_node(atom_a);
    graph.add_node(atom_b);
    graph.add_node(atom_c);
    assert(graph.nodes.size() == 3);

    graph.add_edge(atom_a.id, atom_b.id, "rewrite");
    graph.add_edge(atom_a.id, atom_c.id, "rewrite");
    assert(!graph.is_confluent());

    graph.apply_rewrite({atom_c.id, atom_b.id});
    assert(graph.is_confluent());
    assert(graph.nodes.size() == 2);

    std::cout << "Tier 1 (Symbolic) test passed." << std::endl;
  }

  // Tier 2: Reflective
  {
    t81::cog::v2::ReflectiveFrame frame;
    frame.capture_state(0, {}, "Initial State");
    frame.capture_state(1, {}, "Reasoning step 1");
    assert(frame.justification.steps.size() == 2);
    std::cout << "Tier 2 (Reflective) test passed." << std::endl;
  }

  // Tier 3: Recursive
  {
    t81::cog::v3::Recursor recursor;
    t81::cog::v3::ContractionProof proof;
    proof.verified = true;
    proof.initial_entropy = 10.0;
    proof.final_entropy = 5.0;  // Valid contraction

    assert(recursor.can_recurse());
    recursor.push_frame(proof);
    assert(recursor.current_depth == 1);
    recursor.pop_frame();
    assert(recursor.current_depth == 0);
    std::cout << "Tier 3 (Recursive) test passed." << std::endl;
  }

  // Tier 4: Distributed
  {
    t81::cog::v4::NodeState node;
    node.vector.global_tick = 100;
    node.update_tick();
    assert(node.vector.global_tick == 101);

    t81::cog::v4::CoherenceVector other_vec;
    other_vec.global_tick = 101;
    assert(node.vector.is_coherent(other_vec));
    std::cout << "Tier 4 (Distributed) test passed." << std::endl;
  }

  // Tier 5: Infinite
  {
    t81::cog::v5::InfiniteCanonicalForm form;
    assert(!form.is_convergent);
    form.collapse();
    assert(form.is_convergent);
    assert(form.convergence_signature == "COLLAPSED_INFINITY");

    t81::cog::v5::CollapseSignature sig = t81::cog::v5::CollapseSignature::generate(form);
    // Signature should be non-empty (based on dummy implementation)
    assert(!sig.hash.empty());
    std::cout << "Tier 5 (Infinite) test passed." << std::endl;
  }

  return 0;
}
