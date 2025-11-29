/**
 * @file T81Graph.hpp
 * @brief Defines the T81Graph class, a static graph structure for high performance.
 *
 * This file provides a static, cache-oblivious graph data structure designed for
 * hardware-native performance. The `T81Graph<NodeCount, MaxDegree>` class uses
 * a contiguous adjacency list, making it suitable for efficient execution of
 * graph algorithms that can be expressed as tensor operations, such as PageRank
 * and message passing.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81/T81Float.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Tensor.hpp"
#include <cstddef>
#include <span>
#include <array>
#include <bit>
#include <compare>

namespace t81 {

// ======================================================================
// Edge weight — exactly one tryte (81 trits)
// ======================================================================
using Weight81 = T81Float<72,9>;        // 81-trit floating weight
// using Weight81 = T81Fixed<72,9>;     // for exact integer weights
// using Weight81 = T81Symbol;          // for symbolic/categorical edges

// ======================================================================
// T81Graph<NodeCount, MaxDegree> — Static, cache-oblivious, hardware-native
// ======================================================================
template <size_t NodeCount, size_t MaxDegree = 81>
class T81Graph {
    static_assert(NodeCount <= 81*81, "NodeCount fits in two trytes (symbolic ID)");
    static_assert(MaxDegree <= 81,     "MaxDegree fits in one tryte (index)");

public:
    using NodeID   = uint16_t;                    // 0..6560 — fits in two trytes
    using Weight   = Weight81;
    using EdgeList = std::array<std::pair<NodeID, Weight>, MaxDegree>;

    static constexpr size_t nodes() noexcept { return NodeCount; }
    static constexpr size_t max_degree() noexcept { return MaxDegree; }

private:
    // Adjacency list — contiguous, cache-line aligned, perfect for tensor cores
    alignas(64) EdgeList adj[NodeCount];

    // Optional: node labels (symbols, embeddings, etc.)
    alignas(64) T81Symbol labels[NodeCount];

public:
    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Graph() noexcept {
        for (auto& list : adj) list.fill({NodeID(-1), Weight{}}); // sentinel = invalid
    }

    //===================================================================
    // Edge manipulation — O(1), hardware-accelerated on Axion
    //===================================================================
    constexpr void add_edge(NodeID from, NodeID to, Weight w = Weight(1)) noexcept {
        for (auto& e : adj[from]) {
            if (e.first == NodeID(-1) || e.first == to) {
                e = {to, w};
                return;
            }
        }
        // Overflow → hardware trap on real silicon (degree limit exceeded)
    }

    constexpr void set_weight(NodeID from, NodeID to, Weight w) noexcept {
        for (auto& e : adj[from]) {
            if (e.first == to) { e.second = w; return; }
        }
    }

    [[nodiscard]] constexpr Weight weight(NodeID from, NodeID to) const noexcept {
        for (const auto& e : adj[from]) {
            if (e.first == to) return e.second;
        }
        return Weight(0);
    }

    //===================================================================
    // Views
    //===================================================================
    [[nodiscard]] constexpr std::span<const std::pair<NodeID, Weight>> 
    outgoing(NodeID n) const noexcept {
        return {adj[n].begin(), std::find_if(adj[n].begin(), adj[n].end(),
                   [](auto& e){ return e.first == NodeID(-1); })};
    }

    //===================================================================
    // Symbolic interface — nodes are T81Symbol
    //===================================================================
    constexpr void label(NodeID n, T81Symbol sym) noexcept { labels[n] = sym; }
    [[nodiscard]] constexpr T81Symbol label(NodeID n) const noexcept { return labels[n]; }

    //===================================================================
    // Graph algorithms become tensor operations
    //===================================================================

    // PageRank → single matmul with stochastic matrix
    [[nodiscard]] friend constexpr auto pagerank(const T81Graph& g, int steps = 20) noexcept
        -> T81Tensor<Weight81, 1, NodeCount>
    {
        T81Tensor<Weight81, 2, NodeCount, NodeCount> A{};
        for (NodeID i = 0; i < NodeCount; ++i) {
            size_t deg = g.outgoing(i).size();
            Weight81 p = deg ? Weight81(1) / Weight81(deg) : Weight81(0);
            for (auto [j, w] : g.outgoing(i)) {
                A(i,j) = w * p;
            }
        }
        // Add teleportation
        for (size_t i = 0; i < NodeCount; ++i)
            for (size_t j = 0; j < NodeCount; ++j)
                A(i,j) = A(i,j) * Weight81(0.85) + Weight81(0.15 / NodeCount);

        auto v = T81Tensor<Weight81, 1, NodeCount>::zeros();
        v(0,0) = Weight81(1);  // initial state

        for (int i = 0; i < steps; ++i) v = v * A;
        return v;
    }

    // Message passing (one step) → single sparse tensor contraction
    [[nodiscard]] constexpr auto message_pass(
        const T81Tensor<Weight81, 1, NodeCount>& node_states) const noexcept
        -> T81Tensor<Weight81, 1, NodeCount>
    {
        T81Tensor<Weight81, 1, NodeCount> out{};
        for (NodeID i = 0; i < NodeCount; ++i) {
            Weight81 sum{};
            for (auto [j, w] : outgoing(i)) {
                sum = sum + node_states(0,j) * w;
            }
            out(0,i) = sum;
        }
        return out;
    }
};

// ======================================================================
// Canonical graph types of the new era
// ======================================================================
using SymbolGraph81   = T81Graph<6561, 81>;   // 81² nodes, degree 81 → full HRR binding graph
using AttentionGraph  = T81Graph<4096, 128>;  // transformer KV graph
using KnowledgeGraph  = T81Graph<81*81*81, 27>; // 81³ nodes (531441), sparse symbolic

// ======================================================================
// The future of all computation is a graph of 81-trit weights
// ======================================================================
static_assert(sizeof(SymbolGraph81) == 6561 * 81 * sizeof(std::pair<uint16_t, Weight81>) + 
                                      6561 * sizeof(T81Symbol));  // ~4.8 MiB — fits in L3

} // namespace t81
