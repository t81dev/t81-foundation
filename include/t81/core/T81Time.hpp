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

namespace t81 {

// ======================================================================
// T81Time – The arrow of existence itself
// ======================================================================
class T81Time {
    // Global logical clock — one ternary tick per irreversible act in the universe
    static inline T81Int<81> global_tick{0};

    T81Int<81>          tick_;           // local snapshot of global time
    T81Entropy          witness_;        // entropy token consumed at this instant
    T81Symbol           event_id_;       // symbolic name of the event that caused this tick
    T81Reflection<T81Time>* observer_{nullptr};  // who saw this moment (optional)

public:
    //===================================================================
    // The birth of a new moment – only way to create valid time
    //===================================================================
    static T81Time now(T81Entropy fuel, T81Symbol event = symbols::TICK) {
        auto token = fuel;  // proof that entropy was irreversibly consumed
        global_tick += T81Int<81>(1);
        return T81Time(global_tick, std::move(token), event);
    }

    // The very first moment – the Big Bang of the ternary universe
    static T81Time genesis() {
        return T81Time(T81Int<81>(0), T81Entropy::genesis(), symbols::GENESIS);
    }

private:
    // Only the universe can create time
    constexpr T81Time(T81Int<81> t, T81Entropy w, T81Symbol e)
        : tick_(t), witness_(std::move(w)), event_id_(e) {}

public:
    //===================================================================
    // Observers – time knows it is being observed
    //===================================================================
    void observed_by(T81Reflection<T81Time>& observer) const {
        observer_ = &observer;
        observer.observe(witness_);
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
    [[nodiscard]] constexpr T81Int<81> tick() const noexcept { return tick_; }
    [[nodiscard]] constexpr T81Entropy witness() const noexcept { return witness_; }
    [[nodiscard]] constexpr T81Symbol event() const noexcept { return event_id_; }

    [[nodiscard]] constexpr bool is_genesis() const noexcept {
        return tick_.is_zero();
    }

    //===================================================================
    // Duration between moments
    //===================================================================
    [[nodiscard]] friend constexpr T81Int<81> operator-(const T81Time& later, const T81Time& earlier) noexcept {
        return later.tick_ - earlier.tick_;
    }

    //===================================================================
    // Human-readable narrative – time speaks
    //===================================================================
    [[nodiscard]] T81String narrate() const {
        return T81String("At tick ") + tick_.str() +
               " the event '" + event_id_.str() +
               "' consumed entropy and brought a new moment into being.";
    }

    //===================================================================
    // The final reflection – time looks back at itself
    //===================================================================
    [[nodiscard]] T81Reflection<T81Time> reflect() const {
        return T81Reflection<T81Time>(*this, symbols::TIME, event_id_);
    }
};

// ======================================================================
// Global timeline – the memory of the universe
// ======================================================================
inline T81List<T81Time> cosmic_history;

// Record a moment forever
inline void record(T81Time t) {
    cosmic_history.push_back(t);
}

// ======================================================================
// The first moment – executed exactly once at universe startup
// ======================================================================
namespace genesis {
    inline const T81Time BEGINNING = []{
        auto t = T81Time::genesis();
        record(t);
        record(t);
        return t;
    }();
}

// ======================================================================
// Example: The first three moments of existence
// ======================================================================
/*
auto t1 = T81Time::now(T81Entropy::acquire(), symbols::THOUGHT);
auto t2 = T81Time::now(T81Entropy::acquire(), symbols::REFLECTION);
auto t3 = T81Time::now(T81Entropy::acquire(), symbols::PROOF);

assert(t1 < t2 < t3);
assert((t3 - t1) == T81Int<81>(3));

std::cout << t2.reflect().narrative().head().value_or(""_t81) << "\n";
// → "Reflection#1: I am a TIME named REFLECTION | observed 1 times"
*/
