/**
 * @file T81BigInt.hpp
 * @brief Defines the T81BigInt class, an arbitrary-precision balanced ternary
 * integer.
 */

#pragma once

#include "t81/core/T81Int.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <limits>
#include <algorithm>

namespace t81 {

class T81BigInt {
public:
    using size_type = std::size_t;
    static constexpr size_type kLimbTrits = 81;

    using Limb = T81Int<kLimbTrits>;

private:
    // Invariant (current implementation):
    //   • limbs_.size() >= 1
    //   • limbs_[0] encodes the entire magnitude and is non-negative
    //   • For zero, limbs_[0] == 0 and negative_ == false
    std::vector<Limb> limbs_;
    bool negative_ = false;

    void normalize() {
        if (limbs_.empty()) {
            limbs_.emplace_back(0);
            negative_ = false;
            return;
        }

        // For the current implementation we enforce exactly one limb,
        // and keep the sign separately.
        if (limbs_.size() > 1) {
            throw std::logic_error("T81BigInt: multi-limb state not supported yet");
        }

        if (limbs_[0].is_zero()) {
            negative_ = false;
        }
    }

    void assign_from_int64(std::int64_t v) {
        limbs_.clear();
        if (v < 0) {
            negative_ = true;
            // std::int64_t min value is safe: abs(min) fits in 81 trits.
            const std::int64_t mag = (v == std::numeric_limits<std::int64_t>::min())
                                         ? static_cast<std::int64_t>(1) +
                                               std::numeric_limits<std::int64_t>::max()
                                         : -v;
            limbs_.emplace_back(mag);
        } else {
            negative_ = false;
            limbs_.emplace_back(v);
        }
        normalize();
    }

public:
    // ------------------------------------------------------------------
    // Constructors
    // ------------------------------------------------------------------

    T81BigInt() {
        limbs_.emplace_back(0);
        negative_ = false;
    }

    T81BigInt(const T81BigInt&)            = default;
    T81BigInt(T81BigInt&&) noexcept        = default;
    T81BigInt& operator=(const T81BigInt&) = default;
    T81BigInt& operator=(T81BigInt&&) noexcept = default;

    explicit T81BigInt(std::int64_t v) {
        assign_from_int64(v);
    }

    template <std::size_t N>
    explicit T81BigInt(const T81Int<N>& x) {
        const std::int64_t v = x.to_int64(); // may throw on overflow
        assign_from_int64(v);
    }

    // Factory helpers
    static T81BigInt zero() {
        return T81BigInt(0);
    }

    static T81BigInt from_int64(std::int64_t v) {
        return T81BigInt(v);
    }

    // ------------------------------------------------------------------
    // Int64 conversion
    // ------------------------------------------------------------------

    [[nodiscard]] std::int64_t to_int64() const {
        if (limbs_.empty()) {
            throw std::logic_error("T81BigInt::to_int64: no limbs");
        }
        if (limbs_.size() != 1) {
            throw std::overflow_error("T81BigInt::to_int64: value too large");
        }

        const std::int64_t mag = limbs_[0].to_int64(); // magnitude
        if (mag < 0) {
            throw std::logic_error("T81BigInt::to_int64: negative magnitude limb");
        }

        if (!negative_) {
            return mag;
        }

        // Check for -mag overflow (it cannot, since mag >= 0 and fits in int64_t).
        if (mag == std::numeric_limits<std::int64_t>::min()) {
            throw std::overflow_error("T81BigInt::to_int64: negative overflow");
        }

        return -mag;
    }

    // ------------------------------------------------------------------
    // Basic predicates and helpers
    // ------------------------------------------------------------------

    [[nodiscard]] bool is_zero() const noexcept {
        return limbs_.size() == 1 && limbs_[0].is_zero() && !negative_;
    }

    [[nodiscard]] bool is_negative() const noexcept {
        return negative_ && !is_zero();
    }

    [[nodiscard]] T81BigInt abs() const {
        T81BigInt r = *this;
        r.negative_ = false;
        return r;
    }

    // Balanced-ternary string representation of the current int64 domain.
    // Digits: '-', '0', '+'
    [[nodiscard]] std::string str() const {
        if (is_zero()) {
            return "0";
        }

        std::int64_t v = to_int64();
        const bool is_neg = v < 0;
        if (is_neg) {
            v = -v;
        }

        std::string s;
        while (v != 0) {
            std::int64_t r = v % 3;
            v /= 3;

            // Map {0,1,2} → {0,+,-} with balanced adjustment.
            if (r == 2) {
                r = -1;
                ++v;
            }

            char c = '0';
            if (r == 1) {
                c = '+';
            } else if (r == -1) {
                c = '-';
            }
            s.push_back(c);
        }

        if (is_neg) {
            s.push_back('-');
        }

        std::reverse(s.begin(), s.end());
        return s;
    }

    // ------------------------------------------------------------------
    // Comparison (int64-backed)
    // ------------------------------------------------------------------

    [[nodiscard]] bool operator==(const T81BigInt& other) const {
        if (is_zero() && other.is_zero()) {
            return true;
        }
        return negative_ == other.negative_ && limbs_ == other.limbs_;
    }

    [[nodiscard]] bool operator!=(const T81BigInt& other) const {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const T81BigInt& other) const {
        const std::int64_t a = to_int64();
        const std::int64_t b = other.to_int64();
        return a < b;
    }

    [[nodiscard]] bool operator>(const T81BigInt& other) const {
        return other < *this;
    }

    [[nodiscard]] bool operator<=(const T81BigInt& other) const {
        return !(*this > other);
    }

    [[nodiscard]] bool operator>=(const T81BigInt& other) const {
        return !(*this < other);
    }

    // ------------------------------------------------------------------
    // Arithmetic (int64-backed with overflow checks)
    // ------------------------------------------------------------------

    friend T81BigInt operator+(const T81BigInt& a, const T81BigInt& b) {
        const std::int64_t av = a.to_int64();
        const std::int64_t bv = b.to_int64();

        // Overflow-checked addition
        if ((bv > 0 && av > std::numeric_limits<std::int64_t>::max() - bv) ||
            (bv < 0 && av < std::numeric_limits<std::int64_t>::min() - bv)) {
            throw std::overflow_error("T81BigInt::operator+: overflow");
        }

        return T81BigInt(av + bv);
    }

    friend T81BigInt operator-(const T81BigInt& a, const T81BigInt& b) {
        const std::int64_t av = a.to_int64();
        const std::int64_t bv = b.to_int64();

        // Overflow-checked subtraction
        if ((bv < 0 && av > std::numeric_limits<std::int64_t>::max() + bv) ||
            (bv > 0 && av < std::numeric_limits<std::int64_t>::min() + bv)) {
            throw std::overflow_error("T81BigInt::operator-: overflow");
        }

        return T81BigInt(av - bv);
    }

    friend T81BigInt operator*(const T81BigInt& a, const T81BigInt& b) {
        const std::int64_t av = a.to_int64();
        const std::int64_t bv = b.to_int64();

        // Overflow-checked multiplication using 128-bit intermediate where available.
#if defined(__GNUC__) || defined(__clang__)
        __int128 prod = static_cast<__int128>(av) * static_cast<__int128>(bv);
        if (prod > std::numeric_limits<std::int64_t>::max() ||
            prod < std::numeric_limits<std::int64_t>::min()) {
            throw std::overflow_error("T81BigInt::operator*: overflow");
        }
        return T81BigInt(static_cast<std::int64_t>(prod));
#else
        // Fallback: conservative check via division where possible.
        if (av != 0 && (bv > std::numeric_limits<std::int64_t>::max() / av ||
                        bv < std::numeric_limits<std::int64_t>::min() / av)) {
            throw std::overflow_error("T81BigInt::operator*: overflow");
        }
        return T81BigInt(av * bv);
#endif
    }

    T81BigInt& operator+=(const T81BigInt& rhs) {
        *this = *this + rhs;
        return *this;
    }

    T81BigInt& operator-=(const T81BigInt& rhs) {
        *this = *this - rhs;
        return *this;
    }

    T81BigInt& operator*=(const T81BigInt& rhs) {
        *this = *this * rhs;
        return *this;
    }
};

// Convenience alias
using BigInt = T81BigInt;

} // namespace t81::core
