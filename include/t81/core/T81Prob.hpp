/**
 * @file T81Prob.hpp
 * @brief Defines the T81Prob class for native log-odds probability
 * representation.
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
// T81Prob<Trits> — Native log-odds / log-probability
// ======================================================================
//
// Representation:
//   Stored as log-odds in base φ ≈ 1.618 (golden ratio) or natural log.
//   Trits controls the underlying T81Int width:
//     Trits = 27 → ~42.8 bits of precision → vastly superior to FP16 log-probs
//   Exact representation of many structured probabilities (powers of 1/3, 1/9,…).
//   Softmax → ternary addition (no exp, no div, no overflow in the hot path).
//   Sampling / arithmetic coding can be layered on top later.
//
// NOTE: The scale (kScale) is currently independent of Trits. Wider Trits simply
//       allow a larger dynamic range of log-odds before saturation.
// ======================================================================
template <std::size_t Trits>
class T81Prob {
public:
    using Storage = T81Int<Trits>;

private:
    Storage log_odds_{};  // log(p / (1-p)) in fixed-point base-φ units

public:
    static constexpr std::size_t kTrits = Trits;

    // ------------------------------------------------------------------
    // Constructors
    // ------------------------------------------------------------------

    T81Prob() noexcept = default;

    explicit T81Prob(Storage v) noexcept : log_odds_(v) {}

    // ------------------------------------------------------------------
    // Core accessors
    // ------------------------------------------------------------------

    [[nodiscard]] const Storage& raw() const noexcept {
        return log_odds_;
    }

    [[nodiscard]] const Storage& log_odds() const noexcept {
        return log_odds_;
    }

    // ------------------------------------------------------------------
    // Construction from real probability [0,1]
    // ------------------------------------------------------------------
    //
    // Not constexpr: depends on libm (log / exp / llround).
    //
    [[nodiscard]] static T81Prob from_prob(double p) noexcept {
        if (p <= 0.0) return minus_infinity();
        if (p >= 1.0) return plus_infinity();
        if (p == 0.5) return zero();

        const double odds     = p / (1.0 - p);
        const double log_odds = std::log(odds);

        // Scale to fixed-point in base φ ≈ 1.618
        constexpr double phi    = 1.6180339887498948482;
        constexpr double kScale = 512.0; // ~9 "fractional trits" in this scheme

        const double scaled = log_odds / std::log(phi);
        const std::int64_t fixed =
            static_cast<std::int64_t>(std::llround(scaled * kScale));

        return T81Prob(Storage(fixed));
    }

    // ------------------------------------------------------------------
    // Special values
    // ------------------------------------------------------------------

    // log-odds = 0 → p = 0.5
    [[nodiscard]] static T81Prob zero() noexcept {
        return T81Prob(Storage(0));
    }

    // One "unit" of log-odds (≈ 1.0) → p ≈ 0.73111
    [[nodiscard]] static T81Prob one() noexcept {
        return from_prob(0.731111);
    }

    [[nodiscard]] static T81Prob minus_infinity() noexcept {
        return T81Prob(Storage::kMinValue);
    }

    [[nodiscard]] static T81Prob plus_infinity() noexcept {
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

    [[nodiscard]] bool is_zero() const noexcept {
        return log_odds_.is_zero();
    }

    [[nodiscard]] bool is_minus_infinity() const noexcept {
        return log_odds_ == Storage::kMinValue;
    }

    [[nodiscard]] bool is_plus_infinity() const noexcept {
        return log_odds_ == Storage::kMaxValue;
    }

    // ------------------------------------------------------------------
    // Core arithmetic — THIS IS WHY IT'S MAGIC
    // ------------------------------------------------------------------

    [[nodiscard]] T81Prob operator+(const T81Prob& o) const noexcept {
        return T81Prob(log_odds_ + o.log_odds_);
    }

    [[nodiscard]] T81Prob operator-(const T81Prob& o) const noexcept {
        return T81Prob(log_odds_ - o.log_odds_);
    }

    [[nodiscard]] T81Prob operator-() const noexcept {
        return T81Prob(Storage(0) - log_odds_);
    }

    // Softmax over a tensor becomes: just add all → subtract each
    // log_softmax(x_i) = x_i - log_sum_exp(x)
    // → in T81Prob: x_i + (-log_sum_exp_all)
    [[nodiscard]] T81Prob
    log_softmax_normalize(const T81Prob& log_sum_exp) const noexcept {
        return *this - log_sum_exp;
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------

    [[nodiscard]] auto operator<=>(const T81Prob& o) const noexcept = default;
    [[nodiscard]] bool operator==(const T81Prob& o) const noexcept = default;
};

// ======================================================================
// Canonical 27-trit probability type
// ======================================================================

using T81Prob27 = T81Prob<27>;

// Static asserts (27-trit specialization matches T81Int<27> size / triviality)
static_assert(sizeof(T81Prob27) == sizeof(T81Int<27>));
static_assert(std::is_trivially_copyable_v<T81Prob27>);

// ======================================================================
// Free functions — used everywhere in sampling (for T81Prob27)
// ======================================================================

// Log-sum-exp over a span (fused into one ternary reduction on Axion)
[[nodiscard]] inline T81Prob27
log_sum_exp(std::span<const T81Prob27> probs) noexcept {
    if (probs.empty()) {
        return T81Prob27::minus_infinity();
    }

    const auto max_it = std::max_element(probs.begin(), probs.end());
    const T81Prob27 max = *max_it;

    T81Prob27 sum = T81Prob27::zero();
    for (const auto& p : probs) {
        sum = sum + (p - max);
    }
    return max + sum;
}

// Forward declaration of entropy source
class T81Entropy;

// Gumbel-softmax trick → just add noise from T81Entropy (placeholder)
[[nodiscard]] inline T81Prob27
gumbel_add(const T81Prob27& p, const T81Entropy& /*noise*/) noexcept {
    // TODO: implement real Gumbel noise once T81Entropy is fully specified.
    return p;
}

} // namespace t81
