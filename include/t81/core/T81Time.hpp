/**
 * @file T81Time.hpp
 * @brief Defines the T81Time class for a physical, causal, and thermodynamic timeline.
 *
 * This file provides the T81Time class, which represents a moment in a timeline
 * that is physical, causal, and thermodynamically-grounded. Each new moment in
 * time is created by an explicit, irreversible act that consumes an entropy
 * token. This design ensures that time is monotonic and that every event in the
 * system's history is auditable and tied to a specific thermodynamic cost.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Reflection.hpp"

#include <compare>
#include <optional>
#include <utility>
#include <vector>

namespace t81 {

// ======================================================================
// T81Time – The arrow of existence itself
// ======================================================================
class T81Time {
    // Global logical clock — one ternary tick per irreversible act in the universe
    static inline T81Int<81> global_tick{0};

    T81Int<81>              tick_;      // local snapshot of global time
    T81Entropy              witness_;   // entropy token consumed at this instant
    T81Symbol               event_id_;  // symbolic name of the event that caused this tick
    T81Reflection<T81Time>* observer_{nullptr};  // who saw this moment (optional)

public:
    //===================================================================
    // The birth of a new moment – only way to create valid time
    //===================================================================

    // Creates a new moment, consuming the provided entropy token.
    static T81Time now(T81Entropy&& fuel,
                       T81Symbol event = T81Symbol::intern("TICK")) {
        global_tick += T81Int<81>(1);
        return T81Time(global_tick, std::move(fuel), event);
    }

    // The very first moment – the Big Bang of the ternary universe
    static T81Time genesis() {
        return T81Time(
            T81Int<81>(0),
            T81Entropy::acquire(),
            T81Symbol::intern("GENESIS")
        );
    }

    // Only the universe can create time
    T81Time(T81Int<81> t, T81Entropy&& w, T81Symbol e)
        : tick_(std::move(t))
        , witness_(std::move(w))
        , event_id_(e) {}

    // Copy constructor: cloning a time re-acquires entropy, preserving
    // the invariant that each instance has its own token.
    T81Time(const T81Time& other)
        : tick_(other.tick_)
        , witness_(T81Entropy::acquire())
        , event_id_(other.event_id_)
        , observer_(other.observer_) {}

    T81Time(T81Time&& other) noexcept = default;

    T81Time& operator=(const T81Time& other) {
        if (this != &other) {
            tick_     = other.tick_;
            witness_  = T81Entropy::acquire();
            event_id_ = other.event_id_;
            observer_ = other.observer_;
        }
        return *this;
    }

    T81Time& operator=(T81Time&& other) noexcept = default;

    //===================================================================
    // Observers – time knows it is being observed
    //===================================================================
    void observed_by(T81Reflection<T81Time>& observer) {
        observer_ = &observer;
        observer.observe(std::optional<T81Entropy>(witness_));
    }

    //===================================================================
    // Causal ordering – the only comparison that matters
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Time& o) const noexcept {
        return tick_ <=> o.tick_;
    }

    [[nodiscard]] constexpr bool operator==(const T81Time& o) const noexcept {
        return tick_ == o.tick_;
    }

    //===================================================================
    // Physical properties
    //===================================================================
    [[nodiscard]] constexpr const T81Int<81>& tick() const noexcept { return tick_; }
    [[nodiscard]] constexpr const T81Entropy& witness() const noexcept { return witness_; }
    [[nodiscard]] constexpr T81Symbol event() const noexcept { return event_id_; }

    [[nodiscard]] constexpr bool is_genesis() const noexcept {
        return tick_.is_zero();
    }

    //===================================================================
    // Duration between moments
    //===================================================================
    [[nodiscard]] friend constexpr T81Int<81>
    operator-(const T81Time& later, const T81Time& earlier) noexcept {
        return later.tick_ - earlier.tick_;
    }

    //===================================================================
    // Human-readable narrative – time speaks
    //===================================================================
    [[nodiscard]] T81String narrate() const {
        return T81String("At tick ")
             + T81String(tick_.to_string())
             + T81String(" the event '")
             + T81String(event_id_.to_string())
             + T81String("' consumed entropy and brought a new moment into being.");
    }

    //===================================================================
    // The final reflection – time looks back at itself
    //===================================================================
    [[nodiscard]] T81Reflection<T81Time> reflect() const {
        return T81Reflection<T81Time>(
            *this,
            T81Symbol::intern("TIME"),
            event_id_
        );
    }
};

// ======================================================================
// Global timeline – the memory of the universe
// ======================================================================
inline std::vector<T81Time> cosmic_history;

// Record a moment forever
inline void record(T81Time&& t) {
    cosmic_history.emplace_back(std::move(t));
}

// ======================================================================
// The first moment – executed exactly once at universe startup
// ======================================================================
namespace genesis {
    inline T81Time BEGINNING = [] {
        return T81Time::genesis();
    }();
}

// ======================================================================
// Example: The first three moments of existence
// ======================================================================
/*
auto t1 = T81Time::now(T81Entropy::acquire(), symbols::THOUGHT);
auto t2 = T81Time::now(T81Entropy::acquire(), symbols::REFLECTION);
auto t3 = T81Time::now(T81Entropy::acquire(), symbols::PROOF);

assert(t1 < t2 && t2 < t3);
assert((t3 - t1) == T81Int<81>(3));

std::cout << t2.reflect().narrative().head().value_or(""_t81) << "\n";
// → "Reflection#1: I am a TIME named REFLECTION | observed 1 times"
*/
} // namespace t81
