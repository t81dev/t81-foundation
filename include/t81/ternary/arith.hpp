// include/t81/ternary/arith.hpp
#pragma once
#include "t81/ternary.hpp"
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <limits>

namespace t81::ternary {

inline int trit_to_int(Trit t) noexcept {
    return static_cast<int>(t);  // Neg=-1, Zero=0, Pos=1
}

inline Trit int_to_trit(int v) noexcept {
    return v < 0 ? Trit::Neg : v > 0 ? Trit::Pos : Trit::Zero;
}

inline void normalize(std::vector<Trit>& ds) noexcept {
    while (ds.size() > 1 && ds.back() == Trit::Zero) ds.pop_back();
}

// Encode int64_t → balanced ternary (LSB first)
inline std::vector<Trit> encode_i64(int64_t x) {
    if (x == 0) return {Trit::Zero};

    bool negative = x < 0;
    uint64_t abs_x = negative ? static_cast<uint64_t>(-(x + 1)) + 1 : static_cast<uint64_t>(x);

    std::vector<Trit> result;
    result.reserve(64);

    while (abs_x > 0) {
        uint64_t rem = abs_x % 3;
        abs_x /= 3;

        if (rem == 2) {
            result.push_back(Trit::Neg);
            abs_x += 1;
        } else {
            result.push_back(rem == 1 ? Trit::Pos : Trit::Zero);
        }
    }

    if (negative) {
        for (auto& t : result) {
            t = (t == Trit::Pos) ? Trit::Neg : (t == Trit::Neg) ? Trit::Pos : Trit::Zero;
        }
    }

    normalize(result);
    return result;
}

// Decode balanced ternary → int64_t (safe, no __int128)
inline int64_t decode_i64(const std::vector<Trit>& ds) {
    if (ds.empty()) return 0;

    int64_t result = 0;
    int64_t power = 1;

    for (Trit t : ds) {
        int64_t value = trit_to_int(t);
        int64_t next = result + value * power;

        // Overflow detection
        if (value > 0 && next < result) return INT64_MAX;
        if (value < 0 && next > result) return INT64_MIN;

        result = next;
        if (power > INT64_MAX / 3) {
            return value > 0 ? INT64_MAX : INT64_MIN;
        }
        power *= 3;
    }

    return result;
}

// Full balanced-ternary addition (vector-based, LSB first)
inline std::vector<Trit> add(const std::vector<Trit>& a, const std::vector<Trit>& b) {
    const size_t max_size = std::max(a.size(), b.size());
    std::vector<Trit> result;
    result.reserve(max_size + 2);

    int carry = 0;

    for (size_t i = 0; i < max_size || carry != 0; ++i) {
        int sum = carry;
        if (i < a.size()) sum += trit_to_int(a[i]);
        if (i < b.size()) sum += trit_to_int(b[i]);

        if (sum >= 2) {
            result.push_back(int_to_trit(sum - 3));
            carry = 1;
        } else if (sum <= -2) {
            result.push_back(int_to_trit(sum + 3));
            carry = -1;
        } else {
            result.push_back(int_to_trit(sum));
            carry = 0;
        }
    }

    normalize(result);
    return result;
}

} // namespace t81::ternary
