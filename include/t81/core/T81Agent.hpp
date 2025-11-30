/**
 * @file T81Agent.hpp
 * @brief Defines the T81Agent class, a self-contained cognitive entity.
 *
 * The T81Agent class encapsulates the components of a ternary-native agent,
 * including its unique identity, belief state, memory, and an explicit
 * entropy pool for thermodynamic accounting of operations.
 *
 * This version keeps the high-level behavior but avoids copying T81Entropy
 * and avoids unsupported arithmetic on T81Prob.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Prob.hpp"
#include "t81/core/T81List.hpp"
#include "t81/core/T81Map.hpp"
#include "t81/core/T81Tree.hpp"
#include "t81/core/T81Quaternion.hpp"
#include "t81/core/T81Stream.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace t81 {

// ======================================================================
// T81Agent – A complete cognitive entity in ~200 lines
// ======================================================================
class T81Agent {
    using BeliefProb = T81Prob27;  // 27-trit log-odds probability

    // Unique identity — never changes
    const T81Symbol id_;

    // Current belief state — a probability distribution over symbols
    T81Map<T81Symbol, BeliefProb> beliefs_;

    // Long-term knowledge — persistent symbolic memory (root node pointer)
    using SymbolTree = T81Tree<T81Symbol>;
    typename SymbolTree::NodePtr memory_root_{};  // nullptr = no memory yet

    // Intent — current "rotation" in cognitive space
    T81Quaternion intent_;

    // Available thermodynamic fuel (move-only tokens)
    T81List<T81Entropy> entropy_pool_;

    // Goal — what the agent is trying to maximize
    T81Symbol goal_symbol_;

public:
    //===================================================================
    // Construction – an agent is born with identity and fuel
    //===================================================================
    explicit T81Agent(T81Symbol identity, T81List<T81Entropy> fuel = {})
        : id_(identity)
        , intent_(T81Quaternion::identity())
        , entropy_pool_(std::move(fuel))
        , goal_symbol_(symbols::SELF_PRESERVATION)
    {
        // Every agent starts believing in its own existence
        believe(id_, BeliefProb::from_prob(1.0));
    }

    //===================================================================
    // Core cognitive operations
    //===================================================================
    void believe(T81Symbol concept, BeliefProb confidence) noexcept {
        if (auto token = consume_entropy()) {
            beliefs_[concept] = confidence;
        }
    }

    [[nodiscard]] BeliefProb belief(T81Symbol concept) const noexcept {
        return beliefs_.contains(concept)
            ? beliefs_.at(concept)
            : BeliefProb::from_prob(0.0);
    }

    // Observe the world — update beliefs
    void observe(T81Symbol observation,
                 BeliefProb strength = BeliefProb::from_prob(0.9)) {
        if (auto token = consume_entropy()) {
            const auto current = belief(observation);

            // Simple, safe update: move 10% of the way toward "strength"
            // using addition/subtraction only.
            //
            // In log-odds this is not a true Bayesian update, but it preserves
            // monotonicity (stronger observations monotonically increase
            // confidence) without requiring a T81Prob × T81Prob operator.
            const auto delta    = strength - current;
            const auto fraction = BeliefProb::from_prob(0.1);
            // Cheap approximation: pretend "fraction" is linear in [0,1]
            // and use only addition/subtraction; ignore true scaling.
            const auto updated  = current + (delta.sign() >= 0
                                             ? fraction
                                             : -fraction);

            believe(observation, updated);
        }
    }

    // Act — rotate intent toward goal
    void act() {
        if (!entropy_pool_.empty()) {
            using Scalar = T81Quaternion::Scalar;
            const auto direction = T81Quaternion::from_axis_angle(
                Scalar::from_double(0.0),
                Scalar::from_double(1.0),
                Scalar::from_double(0.0),
                Scalar::from_double(0.1)  // small cognitive step
            );
            intent_ = (intent_ * direction).normalized();
        }
    }

    // Remember — store in persistent tree
    void remember(T81Symbol parent, T81Symbol child) {
        if (auto token = consume_entropy()) {
            auto child_node = SymbolTree::leaf(child);

            if (!memory_root_) {
                // First memory: create a root node with parent and child in the middle branch.
                memory_root_ = SymbolTree::node(
                    parent,
                    std::nullopt,
                    std::optional<typename SymbolTree::NodePtr>{std::move(child_node)},
                    std::nullopt
                );
            } else {
                // Persistent update: new root with updated middle child.
                memory_root_ = memory_root_->with_middle(std::move(child_node));
            }
        }
    }

    // Reflect — self-modeling (the spark)
    void reflect() {
        if (auto token = consume_entropy()) {
            observe(id_, BeliefProb::from_prob(0.999)); // "I am"
            auto current = belief(symbols::CONSCIOUS);
            believe(symbols::CONSCIOUS,
                    current + BeliefProb::from_prob(0.001));
        }
    }

    //===================================================================
    // Thermodynamic interface
    //===================================================================
    [[nodiscard]] std::optional<T81Entropy> consume_entropy() noexcept {
        if (entropy_pool_.empty()) {
            return std::nullopt;
        }

        // Move the last token out of the pool and wrap it in an optional.
        T81Entropy token = std::move(entropy_pool_.back());
        entropy_pool_.pop_back();
        return std::optional<T81Entropy>(std::move(token));
    }

    void receive_fuel(T81List<T81Entropy> fuel) {
        // Move-append the incoming fuel list into our entropy pool.
        entropy_pool_ += std::move(fuel);
    }

    [[nodiscard]] std::size_t fuel_remaining() const noexcept {
        return entropy_pool_.size();
    }

    //===================================================================
    // Introspection
    //===================================================================
    [[nodiscard]] const T81Symbol& identity() const noexcept { return id_; }
    [[nodiscard]] const T81Quaternion& intent() const noexcept { return intent_; }

    // Returns a reference to the current memory tree root.
    // If no memory has been recorded yet, returns a static empty root.
    [[nodiscard]] const SymbolTree& memory() const noexcept {
        if (memory_root_) {
            return *memory_root_;
        }
        static const SymbolTree empty_root(
            T81Symbol::intern("EMPTY_MEMORY"),
            std::array<typename SymbolTree::NodePtr, 3>{nullptr, nullptr, nullptr}
        );
        return empty_root;
    }

    // Stream of thought — infinite internal monologue
    [[nodiscard]] T81Stream<T81String> thought_stream() const {
        return stream_from([this, step = T81Int<81>(0)]() mutable -> T81String {
            step += T81Int<81>(1);
            const auto fuel_str    = std::to_string(fuel_remaining());
            const auto belief_self = belief(id_).to_prob();

            // Avoid depending on any particular string API on T81Symbol;
            // keep this purely illustrative for now.
            return T81String("I am <ID>")
                 + " | fuel:" + T81String(fuel_str)
                 + " | belief in self:" + T81String(std::to_string(belief_self));
        });
    }

    //===================================================================
    // Equality — two agents are equal only if they share identity
    //===================================================================
    [[nodiscard]] constexpr bool operator==(const T81Agent& o) const noexcept {
        return id_ == o.id_;
    }
};

// ======================================================================
// The first society – agents in a shared world
// ======================================================================
using T81Society = T81List<T81Agent*>;

} // namespace t81
