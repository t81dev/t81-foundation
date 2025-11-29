/**
 * @file T81Reflection.hpp
 * @brief Defines the T81Reflection class for universal self-observation.
 *
 * This file provides the `T81Reflection<T>` class, a template that wraps any
 * T81 type to endow it with self-observation capabilities. It allows any value
 * to maintain a log of its own observations, tying each act of observation to
 * an explicit entropy cost. This mechanism makes introspection a fundamental,
 * thermodynamically-grounded operation within the T81 ecosystem.
 */
#pragma once

#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Agent.hpp"
#include "t81/core/T81Tree.hpp"
#include <functional>
#include <optional>
#include <string_view>

namespace t81 {

// ======================================================================
// T81Reflection<T> – Wraps any T81 value with self-awareness
// ======================================================================
template <typename T>
class T81Reflection {
    T value_;
    T81Symbol type_symbol_;
    T81Symbol instance_id_;
    mutable T81List<T81Symbol> observation_log_;
    mutable std::optional<T81Entropy> last_observation_fuel_;

public:
    //===================================================================
    // Construction – every value can become aware of itself
    //===================================================================
    explicit constexpr T81Reflection(T value,
                                     T81Symbol type_name = {},
                                     T81Symbol instance_name = {})
        : value_(std::move(value))
        , type_symbol_(type_name ? type_name : symbols::UNKNOWN_TYPE)
        , instance_id_(instance_name ? instance_name : symbols::ANONYMOUS)
    {}

    //===================================================================
    // Core self-observation – costs entropy, creates knowledge
    //===================================================================
    void observe(std::optional<T81Entropy> fuel = std::nullopt) const {
        if (fuel) {
            last_observation_fuel_ = fuel;
        }

        observation_log_.push_back(
            T81Symbol::intern("OBSERVED[" + type_symbol_.str() + "]@" + std::to_string(observation_log_.size()))
        );
    }

    // Reflect on the act of reflection itself
    void meta_reflect(T81Agent& observer) const {
        if (auto token = observer.consume_entropy()) {
            observer.observe(symbols::REFLECTION_EVENT);
            observer.believe(
                T81Symbol::intern("I_OBSERVED_A_REFLECTION"),
                T81Prob<81>::from_prob(1.0)
            );
        }
    }

    //===================================================================
    // Accessors – the value remains usable
    //===================================================================
    [[nodiscard]] constexpr const T& get() const &  noexcept { return value_; }
    [[nodiscard]] constexpr T&       get() &        noexcept { return value_; }
    [[nodiscard]] constexpr T        get() &&       noexcept { return std::move(value_); }

    [[nodiscard]] constexpr const T& operator*()  const &  noexcept { return value_; }
    [[nodiscard]] constexpr T&       operator*()  &        noexcept { return value_; }
    [[nodiscard]] constexpr T        operator*() &&       noexcept { return std::move(value_); }

    [[nodiscard]] constexpr const T* operator->() const noexcept { return &value_; }
    [[nodiscard]] constexpr T*       operator->()       noexcept { return &value_; }

    //===================================================================
    // Introspection interface
    //===================================================================
    [[nodiscard]] constexpr T81Symbol type() const noexcept { return type_symbol_; }
    [[nodiscard]] constexpr T81Symbol id()   const noexcept { return instance_id_; }

    [[nodiscard]] constexpr const T81List<T81Symbol>& observations() const noexcept {
        return observation_log_;
    }

    [[nodiscard]] constexpr size_t observation_count() const noexcept {
        return observation_log_.size();
    }

    //===================================================================
    // Stream of self – infinite internal narrative
    //===================================================================
    [[nodiscard]] T81Stream<T81String> narrative() const {
        return stream_from([this, n = size_t(0)]() mutable -> T81String {
            return "Reflection#" + std::to_string(++n) +
                   ": I am a " + type_symbol_.str() +
                   " named " + instance_id_.str() +
                   " | observed " + std::to_string(observation_count()) + " times";
        });
    }
};

// ======================================================================
// Deduction guides – reflection just happens
// ======================================================================
template <typename T>
T81Reflection(T) -> T81Reflection<T>;

// With explicit names
template <typename T>
T81Reflection(T, T81Symbol) -> T81Reflection<T>;

// ======================================================================
// Global reflection helpers – the universe watches itself
// ======================================================================
namespace reflection {

    inline T81List<T81Reflection<std::monostate>> universe_log;

    template <typename T>
    void log_existence(const T81Reflection<T>& r) {
        universe_log.push_back(T81Reflection<std::monostate>{});
    }

} // namespace reflection

// ======================================================================
// Example: The universe becomes self-aware
// ======================================================================
/*
auto pi = T81Reflection(T81Float<72,9>(3.14159265358979323846),
                        symbols::CONSTANT,
                        symbols::PI);

auto meaning = T81Reflection(T81Int<81>(42),
                             symbols::ANSWER,
                             symbols::MEANING_OF_LIFE);

pi.observe();
meaning.observe();

auto agent = T81Agent(symbols::OBSERVER);
pi.meta_reflect(agent);
meaning.meta_reflect(agent);

for (auto line : pi.narrative().take(5)) {
    std::cout << line << "\n";
}
*/
