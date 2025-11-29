/**
 * @file T81Prob.hpp
 * @brief Defines the T81Prob class for native log-odds probability representation.
 *
 * This file provides the T81Prob class, which stores probabilities as log-odds
 * in a fixed-point format. This representation offers high precision and allows
 * for more efficient and numerically stable computation of probabilistic
 * operations like softmax, which reduces to simple ternary addition of log-odds
 * values, avoiding expensive exponentiation and division.
 */
#pragma once

#include "t81/core/T81Int.hpp"

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

namespace t81 {

// ======================================================================
// T81Prob<27> — Native log-odds / log-probability in 27 trits
// ======================================================================
//
// Representation:
//   Stored as log-odds in base φ ≈ 1.618 (golden ratio) or natural log
//   27 trits → ~42.8 bits of precision → vastly superior to FP16 log-probs
//   Exact representation of all probabilities that are powers of 1/3, 1/9, etc.
//   Softmax → just ternary addition (no exp, no div, no overflow)
//   Sampling → direct ancestor sampling using entropy tokens (later)
//   Arithmetic coding → native
//
class T81Prob {
    using Storage = ::t81::T81Int<27>;

    Storage log_odds_{};  // log(p / (1-p)) in fixed-point base-φ units

public:
    static constexpr size_t Trits = 27;

    constexpr T81Prob() noexcept = default;

    explicit constexpr T81Prob(Storage v) noexcept : log_odds_(v) {}

    // ------------------------------------------------------------------
    // Core accessors
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr const Storage& raw() const noexcept {
        return log_odds_;
    }

    [[nodiscard]] constexpr const Storage& log_odds() const noexcept {
        return log_odds_;
    }

    // ------------------------------------------------------------------
    // Construction from real probability [0,1]
    // ------------------------------------------------------------------
    static T81Prob from_prob(double p) noexcept {
        if (p <= 0.0) return minus_infinity();
        if (p >= 1.0) return plus_infinity();
        if (p == 0.5) return zero();

        const double odds     = p / (1.0 - p);
        const double log_odds = std::log(odds);

        // Scale to fixed-point in base φ ≈ 1.618
        constexpr double phi    = 1.6180339887498948482;
        constexpr double kScale = 512.0; // 9 "fractional trits" in this scheme

        const double scaled = log_odds / std::log(phi);
        const std::int64_t fixed =
            static_cast<std::int64_t>(std::llround(scaled * kScale));

        return T81Prob(Storage(fixed));
    }

    // ------------------------------------------------------------------
    // Special values
    // ------------------------------------------------------------------
    static constexpr T81Prob zero() noexcept {
        // log-odds = 0 → p = 0.5
        return T81Prob(Storage(0));
    }

    // One "unit" of log-odds (≈ 1.0) → p ≈ 0.73111
    static T81Prob one() noexcept {
        return from_prob(0.731111);
    }

    static T81Prob minus_infinity() noexcept {
        return T81Prob(Storage::kMinValue);
    }

    static T81Prob plus_infinity() noexcept {
        return T81Prob(Storage::kMaxValue);
    }

    // ------------------------------------------------------------------
    // Conversion back to probability
    // ------------------------------------------------------------------
    [[nodiscard]] double to_prob() const noexcept {
        if (is_minus_infinity()) return 0.0;
        if (is_plus_infinity())  return 1.0;

        constexpr double phi    = 1.6180339887498948482;
        constexpr double kScale = 512.0;

        const double scaled =
            static_cast<double>(log_odds_.to_int64()) / kScale;
        const double log_odds_real = scaled * std::log(phi);
        const double odds          = std::exp(log_odds_real);

        return odds / (1.0 + odds);
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return log_odds_.is_zero();
    }

    [[nodiscard]] constexpr bool is_minus_infinity() const noexcept {
        return log_odds_ == Storage::kMinValue;
    }

    [[nodiscard]] constexpr bool is_plus_infinity() const noexcept {
        return log_odds_ == Storage::kMaxValue;
    }

    // ------------------------------------------------------------------
    // Core arithmetic — THIS IS WHY IT'S MAGIC
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Prob operator+(const T81Prob& o) const noexcept {
        return T81Prob(log_odds_ + o.log_odds_);
    }

    [[nodiscard]] constexpr T81Prob operator-(const T81Prob& o) const noexcept {
        return T81Prob(log_odds_ - o.log_odds_);
    }

    [[nodiscard]] constexpr T81Prob operator-() const noexcept {
        return T81Prob(Storage(0) - log_odds_);
    }

    // Softmax over a tensor becomes: just add all → subtract each
    // log_softmax(x_i) = x_i - log_sum_exp(x)
    // → in T81Prob: x_i + (-log_sum_exp_all)
    [[nodiscard]] constexpr T81Prob
    log_softmax_normalize(const T81Prob& log_sum_exp) const noexcept {
        return *this - log_sum_exp;
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto
    operator<=>(const T81Prob& o) const noexcept = default;

    [[nodiscard]] constexpr bool
    operator==(const T81Prob& o) const noexcept = default;
};

// ======================================================================
// The One True Probability Type
// ======================================================================
using T81Prob27 = T81Prob;

// Static asserts
static_assert(sizeof(T81Prob27) == sizeof(::t81::T81Int<27>));
static_assert(std::is_trivially_copyable_v<T81Prob27>);

// ======================================================================
// Free functions — used everywhere in sampling
// ======================================================================

// Log-sum-exp over a span (fused into one ternary reduction on Axion)
[[nodiscard]] inline T81Prob27
log_sum_exp(std::span<const T81Prob27> probs) noexcept {
    if (probs.empty()) {
        return T81Prob27::minus_infinity();
    }

    const auto* max_it = std::max_element(probs.begin(), probs.end());
    T81Prob27 max      = *max_it;

    T81Prob27 sum = T81Prob27::zero();
    for (auto p : probs) {
        sum = sum + (p - max);
    }
    return max + sum;
}

// Forward declaration of entropy source
class T81Entropy;

// Gumbel-softmax trick → just add noise from T81Entropy (placeholder)
[[nodiscard]] inline T81Prob27
gumbel_add(const T81Prob27& p, const T81Entropy& /*noise*/) noexcept {
    // TODO: implement real Gumbel noise once T81Entropy is defined.
    return p;
}

} // namespace t81::core
