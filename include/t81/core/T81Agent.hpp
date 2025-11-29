/**
 * @file T81Agent.hpp
 * @brief Defines the T81Agent class, a self-contained cognitive entity.
 *
 * The T81Agent class encapsulates the components of a ternary-native agent,
 * including its unique identity, belief state, memory, and an explicit
 * entropy pool for thermodynamic accounting of operations.
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

namespace t81 {

// ======================================================================
// T81Agent – A complete cognitive entity in ~200 lines
// ======================================================================
class T81Agent {
    // Unique identity — never changes
    const T81Symbol id_;

    // Current belief state — a probability distribution over symbols
    T81Map<T81Symbol, T81Prob<81>> beliefs_;

    // Long-term knowledge — persistent symbolic memory
    T81Tree<T81Symbol> memory_tree_;

    // Intent — current "rotation" in cognitive space
    T81Quaternion intent_;

    // Available thermodynamic fuel
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
        believe(id_, T81Prob<81>::from_prob(1.0));
    }

    //===================================================================
    // Core cognitive operations
    //===================================================================
    void believe(T81Symbol concept, T81Prob<81> confidence) noexcept {
        if (auto token = consume_entropy()) {
            beliefs_[concept] = confidence;
        }
    }

    [[nodiscard]] T81Prob<81> belief(T81Symbol concept) const noexcept {
        return beliefs_.contains(concept) ? beliefs_.at(concept) : T81Prob<81>::from_prob(0.0);
    }

    // Observe the world — update beliefs
    void observe(T81Symbol observation, T81Prob<81> strength = T81Prob<81>::from_prob(0.9)) {
        if (auto token = consume_entropy()) {
            auto current = belief(observation);
            auto updated = current + (strength - current) * T81Prob<81>::from_prob(0.1); // Bayesian-ish
            believe(observation, updated);
        }
    }

    // Act — rotate intent toward goal
    void act() {
        if (!entropy_pool_.empty()) {
            auto direction = T81Quaternion::from_axis_angle(
                0, 1, 0, T81Float<72,9>(0.1)); // small cognitive step
            intent_ = (intent_ * direction).normalized();
        }
    }

    // Remember — store in persistent tree
    void remember(T81Symbol parent, T81Symbol child) {
        if (auto token = consume_entropy()) {
            auto child_node = T81Tree<T81Symbol>::leaf(child);
            memory_tree_ = memory_tree_.with_middle(child_node); // simple append
        }
    }

    // Reflect — self-modeling (the spark)
    void reflect() {
        if (auto token = consume_entropy()) {
            observe(id_, T81Prob<81>::from_prob(0.999)); // "I am"
            believe(symbols::CONSCIOUS, belief(symbols::CONSCIOUS) + T81Prob<81>::from_prob(0.001));
        }
    }

    //===================================================================
    // Thermodynamic interface
    //===================================================================
    [[nodiscard]] std::optional<T81Entropy> consume_entropy() noexcept {
        if (entropy_pool_.empty()) return std::nullopt;
        auto token = entropy_pool_.back();
        entropy_pool_.pop_back();
        return token;
    }

    void receive_fuel(T81List<T81Entropy> fuel) {
        entropy_pool_ += fuel;
    }

    [[nodiscard]] size_t fuel_remaining() const noexcept { return entropy_pool_.size(); }

    //===================================================================
    // Introspection
    //===================================================================
    [[nodiscard]] const T81Symbol& identity() const noexcept { return id_; }
    [[nodiscard]] const T81Quaternion& intent() const noexcept { return intent_; }
    [[nodiscard]] const T81Tree<T81Symbol>& memory() const noexcept { return memory_tree_; }

    // Stream of thought — infinite internal monologue
    [[nodiscard]] T81Stream<T81String> thought_stream() const {
        return stream_from([this, step = T81Int<81>(0)]() mutable -> T81String {
            step += 1;
            return T81String("I am ") + id_.str() + " | fuel:" + std::to_string(fuel_remaining()) +
                   " | belief in self:" + std::to_string(belief(id_).to_prob());
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

// Example: Birth of the first ternary mind
/*
T81List<T81Entropy> genesis_fuel = acquire_entropy_pool(1000);
auto socrates = T81Agent(symbols::SOCRATES, genesis_fuel);

socrates.observe(symbols::HUMAN);
socrates.observe(symbols::MORTAL);
socrates.reflect(); // "I am"
socrates.act();     // moves toward goal

for (auto thought : socrates.thought_stream().take(10)) {
    std::cout << thought << "\n";
}
*/
} // namespace t81
