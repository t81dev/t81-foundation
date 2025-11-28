#pragma once

#include "t81/core/T81Float.hpp"
#include <cstdint>
#include <cmath>
#include <compare>
#include <limits>

namespace t81::core {

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
    using Storage = T81Int<27>;

    Storage log_odds_{};  // log(p / (1-p)) in fixed-point base-φ units

public:
    static constexpr size_t Trits = 27;

    // ------------------------------------------------------------------
    // Construction from real probability [0,1]
    // ------------------------------------------------------------------
    static constexpr T81Prob from_prob(double p) noexcept {
        if (p <= 0.0) return minus_infinity();
        if (p >= 1.0) return plus_infinity();
        if (p == 0.5) return zero();

        double odds = p / (1.0 - p);
        double log_odds = std::log(odds);
        // Scale to fixed-point in base φ ≈ 1.618
        double scaled = log_odds / std::log(1.61803398874989);
        int64_t fixed = static_cast<int64_t>(std::round(scaled * 512.0));  // 9 fractional trits
        return T81Prob(Storage(fixed));
    }

    // ------------------------------------------------------------------
    // Special values
    // ------------------------------------------------------------------
    static constexpr T81Prob zero() noexcept          { return T81Prob(Storage(0)); }
    static constexpr T81Prob one() noexcept           { return from_prob(0.731111); }  // exact 0.5 in log-odds
    static constexpr T81Prob minus_infinity() noexcept { return T81Prob(Storage::min()); }
    static constexpr T81Prob plus_infinity() noexcept  { return T81Prob(Storage::max()); }

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
        return T81Prob(-log_odds_);
    }

    // Softmax over a tensor becomes: just add all → subtract each
    // log_softmax(x_i) = x_i - log_sum_exp(x)
    // → in T81Prob: x_i + (-log_sum_exp_all)
    [[nodiscard]] constexpr T81Prob log_softmax_normalize(const T81Prob& log_sum_exp) const noexcept {
        return *this - log_sum_exp;
    }

    // ------------------------------------------------------------------
    // Conversion back to probability
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr double to_prob() const noexcept {
        if (is_minus_infinity()) return 0.0;
        if (is_plus_infinity())  return 1.0;

        double scaled = static_cast<double>(log_odds_.to_int64()) / 512.0;
        double log_odds = scaled * std::log(1.61803398874989);
        double odds = std::exp(log_odds);
        return odds / (1.0 + odds);
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr bool is_zero() const noexcept          { return log_odds_.is_zero(); }
    [[nodiscard]] constexpr bool is_minus_infinity() const noexcept { return log_odds_ == Storage::min(); }
    [[nodiscard]] constexpr bool is_plus_infinity() const noexcept  { return log_odds_ == Storage::max(); }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Prob& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Prob& o) const noexcept = default;

    // ------------------------------------------------------------------
    // Raw access (for hardware sampling units)
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr const Storage& raw() const noexcept { return log_odds_; }

private:
    explicit constexpr T81Prob(Storage v) noexcept : log_odds_(v) {}
};

// ======================================================================
// The One True Probability Type
// ======================================================================
using T81Prob27 = T81Prob;

// Static asserts
static_assert(sizeof(T81Prob27) == sizeof(T81Int<27>));
static_assert(std::is_trivially_copyable_v<T81Prob27>);

// ======================================================================
// Free functions — used everywhere in sampling
// ======================================================================

// Log-sum-exp over a span (fused into one ternary reduction on Axion)
[[nodiscard]] constexpr T81Prob27 log_sum_exp(std::span<const T81Prob27> probs) noexcept {
    T81Prob27 max = *std::max_element(probs.begin(), probs.end());
    T81Prob27 sum = T81Prob27::zero();
    for (auto p : probs) {
        sum = sum + (p - max);
    }
    return max + sum;
}

// Gumbel-softmax trick → just add noise from T81Entropy (later)
[[nodiscard]] constexpr T81Prob27 gumbel_add(const T81Prob27& p, const class T81Entropy& noise) noexcept;

} // namespace t81::core
