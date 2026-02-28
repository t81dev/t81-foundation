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

#include "t81/tensor.hpp"
#include "t81/types/T81Entropy.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81List.hpp"
#include "t81/types/T81Map.hpp"
#include "t81/types/T81Prob.hpp"
#include "t81/types/T81Quaternion.hpp"
#include "t81/types/T81Stream.hpp"
#include "t81/types/T81String.hpp"
#include "t81/types/T81Symbol.hpp"
#include "t81/types/T81Tree.hpp"

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

  // Long-term knowledge — persistent symbolic memory
  using SymbolTree = T81Tree<T81Symbol>;
  SymbolTree::ptr memory_root_{};  // nullptr = no memory yet

  // Intent — current "rotation" in cognitive space
  T81Quaternion intent_;

  // Available thermodynamic fuel (move-only tokens)
  T81List<T81Entropy> entropy_pool_;

  // Goal — what the agent is trying to maximize
  T81Symbol goal_symbol_;

  // Weights — mutable tensor for learning
  T729DynamicTensor weights_{};

public:
  //===================================================================
  // Construction – an agent is born with identity and fuel
  //===================================================================
  explicit T81Agent(T81Symbol identity, T81List<T81Entropy> fuel = {})
      : id_(identity),
        intent_(T81Quaternion::identity()),
        entropy_pool_(std::move(fuel)),
        goal_symbol_(symbols::SELF_PRESERVATION) {
    // Every agent starts believing in its own existence
    beliefs_[id_] = BeliefProb::from_prob(1.0);
  }

  //===================================================================
  // Core cognitive operations
  //===================================================================
  void believe(T81Symbol idea, BeliefProb confidence) noexcept {
    if (auto token = consume_entropy()) {
      beliefs_[idea] = confidence;
    }
  }

  [[nodiscard]] BeliefProb belief(T81Symbol idea) const noexcept {
    return beliefs_.contains(idea) ? beliefs_.at(idea) : BeliefProb::from_prob(0.0);
  }

  // Observe the world — update beliefs
  void observe(T81Symbol observation, BeliefProb strength = BeliefProb::from_prob(0.9)) {
    if (auto token = consume_entropy()) {
      const auto current = belief(observation);
      const auto delta = strength - current;
      const auto fraction = BeliefProb::from_prob(0.55);  // Small positive step

      // Move toward strength
      const auto updated =
          (delta.raw().sign_trit() >= Trit::Z) ? current + fraction : current - fraction;

      believe(observation, updated);
    }
  }

  // Act — rotate intent toward goal
  void act() {
    if (!entropy_pool_.empty()) {
      using Scalar = T81Quaternion::Scalar;
      const auto direction = T81Quaternion::from_axis_angle(
          Scalar::from_double(0.0), Scalar::from_double(1.0), Scalar::from_double(0.0),
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
        memory_root_ =
            SymbolTree::node(parent, std::nullopt,
                             std::optional<SymbolTree::ptr>{std::move(child_node)}, std::nullopt);
      } else {
        memory_root_ = memory_root_->with_middle(std::move(child_node));
      }
    }
  }

  // Reflect — self-modeling (the spark)
  void reflect() {
    if (auto token = consume_entropy()) {
      observe(id_, BeliefProb::from_prob(0.999));  // "I am"
      auto current = belief(symbols::CONSCIOUS);
      believe(symbols::CONSCIOUS, current + BeliefProb::from_prob(0.001));
    }
  }

  //===================================================================
  // Neural interface (RFC-0015)
  //===================================================================
  [[nodiscard]] T729DynamicTensor infer(const T729DynamicTensor& input) {
    if (auto token = consume_entropy()) {
      // Identity stub for now
      return input;
    }
    return input;
  }

  void train(const T729DynamicTensor& input, const T729DynamicTensor& target) {
    if (auto token = consume_entropy()) {
      // Training stub
      (void)input;
      (void)target;
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

  [[nodiscard]] std::size_t fuel_remaining() const noexcept { return entropy_pool_.size(); }

  //===================================================================
  // Introspection
  //===================================================================
  [[nodiscard]] const T81Symbol& identity() const noexcept { return id_; }
  [[nodiscard]] const T81Quaternion& intent() const noexcept { return intent_; }

  [[nodiscard]] const SymbolTree& memory() const noexcept {
    if (memory_root_) {
      return *memory_root_;
    }
    static const SymbolTree empty_root(T81Symbol::intern("EMPTY_MEMORY"),
                                       std::array<SymbolTree::ptr, 3>{nullptr, nullptr, nullptr});
    return empty_root;
  }

  // Stream of thought — infinite internal monologue
  [[nodiscard]] T81Stream<std::string> thought_stream() const {
    return stream_from([this]() mutable -> std::string {
      const auto fuel_str = std::to_string(fuel_remaining());
      const auto belief_self = belief(id_).to_prob();

      return "I AM " + id_.to_string() + " | FUEL:" + fuel_str +
             " | BELIEF IN SELF:" + std::to_string(belief_self);
    });
  }

  //===================================================================
  // Equality — two agents are equal only if they share identity
  //===================================================================
  [[nodiscard]] constexpr bool operator==(const T81Agent& o) const noexcept { return id_ == o.id_; }
  [[nodiscard]] std::string serialize_canonical() const { return "Agent()"; }
};

// ======================================================================
// The first society – agents in a shared world
// ======================================================================
using T81Society = T81List<T81Agent*>;

}  // namespace t81
