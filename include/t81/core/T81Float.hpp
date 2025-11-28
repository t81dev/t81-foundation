#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <cmath>
#include <limits>

namespace t81::core {

using t81::Trit;
using t81::T81Int;

template <size_t MantissaTrits, size_t ExponentTrits>
class T81Float {
    static_assert(MantissaTrits >= 8);
    static_assert(ExponentTrits >= 4);

    using Storage = T81Int<MantissaTrits + ExponentTrits + 1>;

public:
    static constexpr size_t M = MantissaTrits;
    static constexpr size_t E = ExponentTrits;
    static constexpr size_t TotalTrits = M + E + 1;

    constexpr T81Float() noexcept = default;

    // Public factory — simple but correct
    static constexpr T81Float from_double(double d) noexcept {
        T81Float f;
        if (d == 0.0) return f;

        bool negative = d < 0.0;
        if (negative) d = -d;

        int exp;
        double frac = std::frexp(d, &exp);
        int64_t mant = static_cast<int64_t>(frac * (1LL << (M > 1 ? M - 1 : 1)));

        // Use template keyword for dependent name
        f.storage_ = Storage::template from_binary<TotalTrits>(mant);
        if (negative) {
            f.storage_ = -f.storage_;
        }
        return f;
    }

    static constexpr T81Float zero() noexcept { return {}; }
    static constexpr T81Float one() noexcept { return from_double(1.0); }

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return storage_ == Storage{};
    }

    [[nodiscard]] constexpr double to_double() const noexcept {
        // Use template keyword here too
        return storage_.template to_binary<double>();
    }

private:
    Storage storage_{};
};

} // namespace t81::core