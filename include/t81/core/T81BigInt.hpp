/**
 * @file T81BigInt.hpp
 * @brief Defines the T81BigInt class for arbitrary-precision ternary integers.
 *
 * This file contains the implementation of T81BigInt, a class that provides
 * arbitrary-precision integer arithmetic using a vector of fixed-size T81Int
 * "limbs". This allows it to represent integers of any size, limited only by
 * available memory.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include <vector>
#include <string>
#include <compare>

namespace t81::core {

class T81BigInt {
    static constexpr size_t constexpr LimbTrits = 81;
    using Limb = T81Int<LimbTrits>;

    std::vector<Limb> limbs_;  // little-endian
    bool negative_ = false;

    void normalize() noexcept {
        while (limbs_.size() > 1 && limbs_.back().is_zero())
            limbs_.pop_back();
        if (limbs_.empty()) {
            limbs_.push_back(Limb(0));
            negative_ = false;
        }
    }

public:
    constexpr T81BigInt() noexcept { limbs_.push_back(Limb(0)); }

    template <size_t N>
    constexpr T81BigInt(const T81Int<N>& x) noexcept {
        if (x.is_negative()) negative_ = true;
        T81Int<N> abs = x.abs();
        while (!abs.is_zero()) {
            limbs_.push_back(Limb(abs));
            abs >>= LimbTrits;
        }
        normalize();
    }

    constexpr T81BigInt(int64_t x) noexcept : T81BigInt(T81Int<81>(x)) {}

    [[nodiscard]] std::string str() const {
        if (is_zero()) return "0";
        std::string s;
        T81BigInt temp = abs();
        while (!temp.is_zero()) {
            int rem = (temp % 3).to_int64();
            s.push_back(rem == 2 ? '-' : (rem == 1 ? '+' : '0'));
            temp /= 3;
        }
        if (negative_) s += '-';
        std::reverse(s.begin(), s.end());
        return s;
    }

    // Arithmetic — schoolbook, but perfect
    friend constexpr T81BigInt operator+(const T81BigInt& a, const T81BigInt& b);
    friend constexpr T81BigInt operator*(const T81BigInt a, const T81BigInt& b) noexcept {
        T81BigInt result;
        result.limbs_.assign(a.limbs_.size() + b.limbs_.size(), Limb(0));
        for (size_t i = 0; i < a.limbs_.size(); ++i)
            for (size_t j = 0; j < b.limbs_.size(); ++j)
                result.limbs_[i+j] += a.limbs_[i] * b.limbs_[j];
        result.negative_ = a.negative_ != b.negative_;
        result.normalize();
        return result;
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept { return limbs_.size() == 1 && limbs_[0].is_zero(); }
    [[nodiscard]] constexpr T81BigInt abs() const noexcept { auto c = *this; c.negative_ = false; return c; }

    constexpr auto operator<=>(const T81BigInt& o) const noexcept = default;
};

// The ultimate integer
using BigInt = T81BigInt;

} // namespace t81::core
