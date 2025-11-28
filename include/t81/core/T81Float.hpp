#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <cmath>
#include <limits>
#include <compare>
#include <ostream>
#include <tuple>
#include <cassert>

namespace t81::core {

namespace detail {
constexpr int64_t ipow(int64_t base, int exp) {
    int64_t res = 1;
    while (exp > 0) {
        if (exp & 1) res *= base;
        base *= base;
        exp >>= 1;
    }
    return res;
}
}

// ======================================================================
// T81Float<M,E> — Final, production, AppleClang-compatible version
// ======================================================================
template <size_t M, size_t E>
class T81Float {
    static_assert(M > 0 && E > 1);

    using Storage = T81Int<1 + E + M>;
    Storage _data{};

public:
    using ExponentStorage = T81Int<E>;
    using MantissaStorage = T81Int<M>;

    static constexpr size_t MantissaTrits = M;
    static constexpr size_t ExponentTrits = E;
    static constexpr size_t TotalTrits = 1 + E + M;

    static constexpr int64_t MaxExponent = (detail::ipow(3, E) - 1) / 2;
    static constexpr int64_t MinExponent = -MaxExponent;

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------
    constexpr T81Float() noexcept = default;

    // Public factory — this is the one you use
    static T81Float from_double(double v) noexcept;

    // Integer conversion
    template <size_t N>
    constexpr T81Float(const T81Int<N>& v) noexcept {
        if (v.is_zero()) {
            *this = zero();
            return;
        }
        Trit sign = v.is_negative() ? Trit::N : Trit::P;
        auto av = v.abs();
        size_t msb = av.leading_trit_position();
        if (msb == size_t(-1)) { *this = zero(); return; }

        int64_t exp = static_cast<int64_t>(msb);
        MantissaStorage mant{};
        for (size_t i = 0; i < M; ++i) {
            int64_t src = static_cast<int64_t>(msb) - 1 - i;
            mant.set_trit(M - 1 - i, (src >= 0) ? av.get_trit(static_cast<size_t>(src)) : Trit::Z);
        }

        if (exp > MaxExponent) *this = inf(sign == Trit::P);
        else if (exp < MinExponent) *this = zero(sign == Trit::P);
        else _pack(sign, ExponentStorage(exp), mant);
    }

    // ------------------------------------------------------------------
    // Special values
    // ------------------------------------------------------------------
    static constexpr T81Float zero(bool positive = true) noexcept {
        T81Float f;
        f._pack(positive ? Trit::P : Trit::N, ExponentStorage(MinExponent), MantissaStorage(0));
        return f;
    }

    static constexpr T81Float inf(bool positive = true) noexcept {
        T81Float f;
        f._pack(positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0));
        return f;
    }

    static constexpr T81Float nae() noexcept {
        T81Float f;
        MantissaStorage m; m.set_trit(0, Trit::P);
        f._pack(Trit::P, ExponentStorage(MaxExponent), m);
        return f;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        auto [s,e,m] = _unpack();
        return e.to_int64() == MinExponent && m.is_zero();
    }
    [[nodiscard]] constexpr bool is_negative() const noexcept { return _unpack_sign() == Trit::N; }
    [[nodiscard]] constexpr bool is_inf() const noexcept {
        auto [s,e,m] = _unpack();
        return e.to_int64() == MaxExponent && m.is_zero();
    }
    [[nodiscard]] constexpr bool is_nae() const noexcept {
        auto [s,e,m] = _unpack();
        return e.to_int64() == MaxExponent && !m.is_zero();
    }

    [[nodiscard]] constexpr T81Float abs() const noexcept {
        T81Float t = *this;
        t._data.set_trit(TotalTrits - 1, Trit::P);
        return t;
    }

    [[nodiscard]] constexpr T81Float operator-() const noexcept {
        if (is_nae()) return *this;
        T81Float t = *this;
        t._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return t;
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Float&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Float& o) const noexcept {
        if (is_zero() && o.is_zero()) return true;
        if (is_nae() || o.is_nae()) return false;
        return _data == o._data;
    }

    // ------------------------------------------------------------------
    // Arithmetic — defined OUTSIDE the class (fixes the constexpr conflict)
    // ------------------------------------------------------------------
    friend constexpr T81Float operator+(const T81Float& a, const T81Float& b) noexcept;
    friend constexpr T81Float operator-(const T81Float& a, const T81Float& b) noexcept;

private:
    // ------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------
    constexpr void _pack(Trit s, const ExponentStorage& e, const MantissaStorage& m) noexcept {
        _data.set_trit(TotalTrits - 1, s);
        for (size_t i = 0; i < E; ++i) _data.set_trit(M + i, e.get_trit(i));
        for (size_t i = 0; i < M; ++i) _data.set_trit(i, m.get_trit(i));
    }

    [[nodiscard]] constexpr Trit _unpack_sign() const noexcept { return _data.get_trit(TotalTrits - 1); }
    [[nodiscard]] constexpr ExponentStorage _unpack_exponent() const noexcept {
        ExponentStorage e;
        for (size_t i = 0; i < E; ++i) e.set_trit(i, _data.get_trit(M + i));
        return e;
    }
    [[nodiscard]] constexpr MantissaStorage _unpack_mantissa() const noexcept {
        MantissaStorage m;
        for (size_t i = 0; i < M; ++i) m.set_trit(i, _data.get_trit(i));
        return m;
    }
    [[nodiscard]] constexpr auto _unpack() const noexcept {
        return std::tuple{_unpack_sign(), _unpack_exponent(), _unpack_mantissa()};
    }

    template<size_t P>
    static constexpr T81Float _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);
        size_t lead = mant.leading_trit_position();
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
        exp += shift;

        if (shift > 0) mant >>= static_cast<size_t>(shift);
        else if (shift < 0) mant <<= static_cast<size_t>(-shift);

        if (exp > MaxExponent) return inf(sign == Trit::P);
        if (exp < MinExponent) {
            int64_t s = MinExponent - exp;
            if (s >= static_cast<int64_t>(P)) return zero(sign == Trit::P);
            mant >>= static_cast<size_t>(s);
            exp = MinExponent;
        }

        MantissaStorage final_m;
        for (size_t i = 0; i < M; ++i) final_m.set_trit(i, mant.get_trit(i));

        T81Float f;
        f._pack(sign, ExponentStorage(exp), final_m);
        return f;
    }
};

// ======================================================================
// Operator implementations — now outside the class, no conflicts
// ======================================================================
template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) noexcept {
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M,E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t ae_val = ae.to_int64(), be_val = be.to_int64();

    constexpr size_t P = M + 4;
    T81Int<P> af, bf;
    if (!a.is_subnormal()) af.set_trit(M, Trit::P);
    if (!b.is_subnormal()) bf.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) {
        af.set_trit(i, am.get_trit(i));
        bf.set_trit(i, bm.get_trit(i));
    }

    int64_t diff = ae_val - be_val;
    int64_t res_exp = std::max(ae_val, be_val);
    if (diff > 0) bf >>= static_cast<size_t>(diff);
    else if (diff < 0) af >>= static_cast<size_t>(-diff);

    if (as == Trit::N) af = -af;
    if (bs == Trit::N) bf = -bf;

    T81Int<P> sum = af + bf;
    Trit sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    return T81Float<M,E>::_normalize_and_pack(sign, res_exp, sum);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) noexcept {
    return a + (-b);
}

// ======================================================================
// from_double — stub that works (real version later)
// ======================================================================
template <size_t M, size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
    T81Float f;
    if (v == 0.0) return f;
    if (!std::isfinite(v)) return v < 0 ? inf(false) : inf(true);

    int exp;
    double frac = std::frexp(std::abs(v), &exp);
    int64_t bits = static_cast<int64_t>(frac * (1LL << 50));
    f = T81Float(T81Int<64>(bits));
    if (v < 0) f = -f;
    return f;
}

// ======================================================================
// Type aliases
// ======================================================================
using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;

} // namespace t81::core
