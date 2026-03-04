#include <cassert>
#include <iostream>
#include "t81/experimental/cog/tier1/symbolic.hpp"

using namespace t81::cog::v1;

int main() {
    std::cout << "Testing SymbolicGraph serialize_canonical..." << std::endl;

    // Create a symbolic graph
    SymbolicGraph graph;
    
    // Add nodes
    auto node_a = SymbolicAtom::create("NodeA");
    auto node_b = SymbolicAtom::create("NodeB");
    auto node_c = SymbolicAtom::create("NodeC");
    
    graph.add_node(node_a);
    graph.add_node(node_b);
    graph.add_node(node_c);
    
    // Add edges
    graph.add_edge(node_a.id, node_b.id, "connects");
    graph.add_edge(node_b.id, node_c.id, "leads_to");
    graph.add_edge(node_a.id, node_c.id, "reaches");
    
    // Test serialization
    std::string serialized = graph.serialize_canonical();
    std::cout << "Serialized graph:" << std::endl;
    std::cout << serialized << std::endl;
    
    // Verify the serialization contains expected elements
    assert(serialized.find("{") != std::string::npos);
    assert(serialized.find("}") != std::string::npos);
    assert(serialized.find("NodeA") != std::string::npos);
    assert(serialized.find("NodeB") != std::string::npos);
    assert(serialized.find("NodeC") != std::string::npos);
    assert(serialized.find("->") != std::string::npos);
    assert(serialized.find("connects") != std::string::npos);
    assert(serialized.find("leads_to") != std::string::npos);
    assert(serialized.find("reaches") != std::string::npos);
    
    std::cout << "✅ SymbolicGraph serialize_canonical test PASSED!" << std::endl;
    return 0;
}
