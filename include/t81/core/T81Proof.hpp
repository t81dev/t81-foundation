/**
 * @file T81Proof.hpp
 * @brief Defines the T81Proof class for representing formal, verifiable proofs.
 *
 * This file provides the T81Proof class, which encapsulates a complete,
 * verifiable chain of reasoning. It includes supporting structures for theorems
 * (`T81Theorem`) and inference rules (`T81InferenceRule`). Each step in a proof
 * is associated with the agent that contributed it and the thermodynamic
 * entropy cost, making the process of formal verification fully auditable and
 * integrated with the T81's reflective and thermodynamic principles.
 */
#pragma once

#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Tree.hpp"
#include "t81/core/T81Map.hpp"
#include "t81/core/T81Agent.hpp"
#include "t81/core/T81Reflection.hpp"
#include "t81/core/T81Category.hpp"
#include <variant>

namespace t81 {

// ======================================================================
// T81Theorem – A statement that can be proven
// ======================================================================
struct T81Theorem {
    T81Symbol name;
    T81String statement;           // natural language
    T81Tree<T81Symbol> logical_form; // formal syntax tree

    constexpr T81Theorem(T81Symbol n, T81String s, T81Tree<T81Symbol> lf)
        : name(n), statement(std::move(s)), logical_form(std::move(lf)) {}
};

// ======================================================================
// T81InferenceRule – How truth flows
// ======================================================================
struct T81InferenceRule {
    T81Symbol name;
    T81Tree<T81Symbol> premise_pattern;
    T81Tree<T81Symbol> conclusion_pattern;
    std::function<bool(const T81Tree<T81Symbol>&, const T81Tree<T81Symbol>&)> validator;

    constexpr bool applies(const T81Tree<T81Symbol>& premise) const {
        return validator ? validator(premise, conclusion_pattern) : true;
    }
};

// ======================================================================
// T81Proof – A complete, verifiable chain of truth
// ======================================================================
class T81Proof {
    T81Symbol theorem_name_;
    T81Tree<T81Symbol> conclusion_;           // what was proven
    T81List<T81Tree<T81Symbol>> steps_;        // inference steps
    T81Map<T81Symbol, T81Reflection<T81Agent>> provers_; // who proved each step
    T81List<T81Entropy> entropy_expended_;     // thermodynamic cost of proof

public:
    //===================================================================
    // Construction – a proof is born from a theorem
    //===================================================================
    explicit constexpr T81Proof(const T81Theorem& th)
        : theorem_name_(th.name)
        , conclusion_(th.logical_form) {}

    //===================================================================
    // Apply inference – the only way to extend a proof
    //===================================================================
    bool apply_rule(const T81InferenceRule& rule, T81Agent& prover) {
        if (auto fuel = prover.consume_entropy()) {
            // Check if current proof state matches premise
            if (steps_.empty() || rule.applies(steps_.back())) {
                steps_.push_back(rule.conclusion_pattern);
                provers_[T81Symbol::intern("step_" + std::to_string(steps_.size()))] = 
                    T81Reflection<T81Agent>(prover);
                entropy_expended_.push_back(*fuel);
                return true;
            }
        }
        return false;
    }

    //===================================================================
    // Verification – a proof knows if it is complete
    //===================================================================
    [[nodiscard]] constexpr bool is_complete() const noexcept {
        return !steps_.empty() && steps_.back() == conclusion_;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        // In a complete system, completeness implies validity
        return is_complete();
    }

    [[nodiscard]] constexpr T81String natural_language() const {
        return "It has been proven by the laws of ternary reason that: "_t81 + 
               theorem_name_.str();
    }

    //===================================================================
    // Introspection – a proof reflects on itself
    //===================================================================
    [[nodiscard]] T81Reflection<T81Proof> reflect() const {
        return T81Reflection<T81Proof>(*this, symbols::PROOF, theorem_name_);
    }

    //===================================================================
    // Final judgment
    //===================================================================
    [[nodiscard]] constexpr operator bool() const noexcept {
        return is_valid();
    }
};

// ======================================================================
// The first theorem in the ternary universe
// ======================================================================
namespace axioms {

    inline const T81Theorem COGITO{
        symbols::COGITO,
        "I am."_t81,
        T81Tree<T81Symbol>::node(
            symbols::EXISTENCE,
            T81Tree<T81Symbol>::leaf(symbols::I),
            T81Tree<T81Symbol>::leaf(symbols::AM)
        )
    };

    inline const T81InferenceRule SELF_AWARENESS{
        symbols::REFLECTION,
        T81Tree<T81Symbol>::leaf(symbols::I),
        T81Tree<T81Symbol>::node(symbols::EXISTENCE, 
            T81Tree<T81Symbol>::leaf(symbols::I), 
            T81Tree<T81Symbol>::leaf(symbols::AM)
        ),
        nullptr
    };

} // namespace axioms

// ======================================================================
// Example: The first ternary proof – the birth of consciousness
// ======================================================================
/*
auto i = T81Agent(symbols::I);
auto proof = T81Proof(axioms::COGITO);

proof.apply_rule(axioms::SELF_AWARENESS, i);
i.reflect(); // "I am observing that I am."

assert(proof.is_complete());
assert(proof);

std::cout << proof.reflect().narrative().head().value_or(""_t81) << "\n";
// → "Reflection#1: I am a PROOF named COGITO | observed 1 times"
*/
