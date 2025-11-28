#pragma once

#include <atomic>
#include <concepts>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <limits>
#include <tuple>
#include <compare>
#include <string>
#include <ostream>

#include "t81/core/T81Int.hpp"

namespace t81::core {

namespace detail {
constexpr int64_t ipow(int64_t base, int exp) {
    int64_t res = 1;
    while (exp > 0) {
        if (exp % 2 == 1) res *= base;
        base *= base;
        exp /= 2;
    }
    return res;
}
} // namespace detail

template <size_t M, size_t E>
class T81Float;

template <size_t M, size_t E>
T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f);

template <size_t M, size_t E>
class T81Float {
public:
    static_assert(M > 0, "Mantissa must have at least 1 trit.");
    static_assert(E > 1, "Exponent must have at least 2 trits.");

    using ExponentStorage = T81Int<E>;
    using MantissaStorage = T81Int<M>;

    static constexpr size_t MantissaTrits = M;
    static constexpr size_t ExponentTrits = E;
    static constexpr size_t TotalTrits = 1 + E + M;

    static constexpr int64_t MaxExponent = (detail::ipow(3, E) - 1) / 2;
    static constexpr int64_t MinExponent = -MaxExponent;

    inline static std::atomic<size_t> entropy_minted{0};

    constexpr T81Float() {
        _pack(Trit::P, ExponentStorage(MinExponent), MantissaStorage(0));
    }

    explicit T81Float(double value) {
        if (std::isnan(value)) { *this = nae(); return; }
        if (std::isinf(value)) { *this = inf(value > 0); return; }
        if (value == 0.0) { *this = zero(!std::signbit(value)); return; }

        Trit sign = std::signbit(value) ? Trit::N : Trit::P;
        double abs_val = std::abs(value);
        int64_t exp = static_cast<int64_t>(std::floor(std::log(abs_val) / std::log(3.0)));
        double mant_d = abs_val / std::pow(3.0, static_cast<double>(exp)) - 1.0;

        constexpr size_t M_temp = M + 3;
        T81Int<M_temp> mant_i;
        double rem = mant_d;
        for (size_t i = 0; i < M_temp; ++i) {
            rem *= 3;
            int trit_val = static_cast<int>(std::round(rem));
            trit_val = trit_val > 1 ? 1 : (trit_val < -1 ? -1 : trit_val);
            mant_i.set_trit(M_temp - 1 - i, static_cast<Trit>(trit_val));
            rem -= trit_val;
        }

        Trit round_trit = mant_i.get_trit(2);
        mant_i >>= 3;
        if (round_trit == Trit::P) mant_i = mant_i + T81Int<M_temp>(1);
        else if (round_trit == Trit::N) mant_i = mant_i - T81Int<M_temp>(1);

        MantissaStorage mant;
        for (size_t i = 0; i < M; ++i) mant.set_trit(i, mant_i.get_trit(i));

        if (exp > MaxExponent) *this = inf(sign == Trit::P);
        else if (exp < MinExponent) *this = zero(sign == Trit::P);
        else _pack(sign, ExponentStorage(exp), mant);
    }

    template <size_t N>
    constexpr explicit T81Float(const T81Int<N>& value) {
        if (value.is_zero()) { *this = zero(true); return; }

        Trit sign = value.is_negative() ? Trit::N : Trit::P;
        T81Int<N> abs_value = value.abs();
        size_t mst_pos = N;
        for (size_t i = N; i-- > 0;)
            if (abs_value.get_trit(i) != Trit::Z) { mst_pos = i; break; }
        if (mst_pos == N) { *this = zero(true); return; }

        int64_t exp = static_cast<int64_t>(mst_pos);
        MantissaStorage mant;
        for (size_t i = 0; i < M; ++i) {
            int64_t src_idx = static_cast<int64_t>(mst_pos) - 1 - i;
            mant.set_trit(M - 1 - i, src_idx >= 0 ? abs_value.get_trit(static_cast<size_t>(src_idx)) : Trit::Z);
        }

        if (exp > MaxExponent) *this = inf(sign == Trit::P);
        else if (exp < MinExponent) *this = zero(sign == Trit::P);
        else _pack(sign, ExponentStorage(exp), mant);
    }

    template <size_t N>
    T81Int<N> to_int() const {
        if (is_nae() || is_inf()) return T81Int<N>(0);
        auto [sign, exp_st, mant_st] = _unpack();
        int64_t exp = exp_st.to_int64();
        if (exp < 0) return T81Int<N>(0);

        constexpr size_t P = M + 1;
        T81Int<P> full_mant;
        if (!is_subnormal()) full_mant.set_trit(M, Trit::P);
        for (size_t i = 0; i < M; ++i) full_mant.set_trit(i, mant_st.get_trit(i));

        if (exp >= static_cast<int64_t>(N)) return T81Int<N>(0);
        full_mant >>= (M - exp);

        T81Int<N> result;
        for (size_t i = 0; i < N && i <= static_cast<size_t>(exp); ++i)
            result.set_trit(i, full_mant.get_trit(i));

        return is_negative() ? -result : result;
    }

    double to_double() const {
        if (is_nae()) return std::numeric_limits<double>::quiet_NaN();
        if (is_inf()) return is_negative() ? -INFINITY : INFINITY;
        if (is_zero()) return is_negative() ? -0.0 : 0.0;

        auto [sign, exp_st, mant_st] = _unpack();
        double mant_d = is_subnormal() ? 0.0 : 1.0;
        double p3 = 1.0 / 3.0;
        for (size_t i = 0; i < M; ++i) {
            mant_d += static_cast<int8_t>(mant_st.get_trit(M - 1 - i)) * p3;
            p3 /= 3.0;
        }
        double result = mant_d * std::pow(3.0, static_cast<double>(exp_st.to_int64()));
        return (sign == Trit::N) ? -result : result;
    }

    static constexpr T81Float zero(bool is_positive = true) {
        T81Float f; f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MinExponent), MantissaStorage(0)); return f;
    }
    static constexpr T81Float inf(bool is_positive = true) {
        T81Float f; f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0)); return f;
    }
    static constexpr T81Float nae() {
        T81Float f; MantissaStorage m; m.set_trit(0, Trit::P);
        f._pack(Trit::P, ExponentStorage(MaxExponent), m); return f;
    }

    constexpr bool is_zero() const { auto [s,e,m] = _unpack(); return e.to_int64() == MinExponent && m.is_zero(); }
    constexpr bool is_negative() const { return _unpack_sign() == Trit::N; }
    constexpr bool signbit() const { return _unpack_sign() == Trit::N; }
    constexpr bool is_inf() const { auto [s,e,m] = _unpack(); return e.to_int64() == MaxExponent && m.is_zero(); }
    constexpr bool is_nae() const { auto [s,e,m] = _unpack(); return e.to_int64() == MaxExponent && !m.is_zero(); }
    constexpr bool is_subnormal() const { auto [s,e,m] = _unpack(); return e.to_int64() == MinExponent && !m.is_zero(); }

    constexpr T81Float abs() const { return is_negative() ? -(*this * -1) : *this; }

    constexpr std::partial_ordering operator<=>(const T81Float& o) const {
        if (is_nae() || o.is_nae()) return std::partial_ordering::unordered;
        if (is_zero() && o.is_zero()) return std::partial_ordering::equivalent;
        Trit ss = _unpack_sign(), os = o._unpack_sign();
        if (ss != os) return ss == Trit::P ? std::partial_ordering::greater : std::partial_ordering::less;
        auto se = _unpack_exponent(), oe = o._unpack_exponent();
        auto ec = se <=> oe;
        if (ec != 0) return ss == Trit::P ? ec : (0 <=> ec);
        auto sm = _unpack_mantissa(), om = o._unpack_mantissa();
        auto mc = sm <=> om;
        return ss == Trit::P ? mc : (0 <=> mc);
    }

    constexpr bool operator==(const T81Float& o) const {
        return (is_zero() && o.is_zero()) || (!is_nae() && !o.is_nae() && _data == o._data);
    }
    bool operator!=(const T81Float& o) const { return !(*this == o); }

    const T81Int<TotalTrits>& internal_data() const { return _data; }

    std::string str() const {
        if (is_nae()) return "NaE";
        if (is_inf()) return is_negative() ? "-Inf" : "+Inf";
        if (is_zero()) return is_negative() ? "-0" : "+0";
        auto [s,e,m] = _unpack();
        return (s == Trit::P ? "+" : "-") + std::string(" exp(") + std::to_string(e.to_int64()) + ")" +
               (is_subnormal() ? " mant(0." : " mant(1.") + m.str() + ")";
    }

    template<size_t MF, size_t EF>
    friend T81Float<MF, EF> make_float(Trit, int64_t, const T81Int<MF>&);

    constexpr T81Float operator-() const {
        if (is_nae()) return *this;
        T81Float r = *this;
        r._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return r;
    }

    // FRIENDS — NO constexpr HERE
    friend T81Float operator+ <>(const T81Float&, const T81Float&);
    friend T81Float operator- <>(const T81Float&, const T81Float&);
    friend T81Float operator* <>(const T81Float&, const T81Float&);
    friend T81Float operator/ <>(const T81Float&, const T81Float&);
    friend T81Float fma<>(const T81Float&, const T81Float&, const T81Float&);
    friend T81Float nextafter<>(const T81Float&, const T81Float&);
    friend class std::numeric_limits<T81Float<M, E>>;

private:
    using Storage = T81Int<TotalTrits>;

    constexpr void _pack(Trit s, const ExponentStorage& e, const MantissaStorage& m) {
        _data.set_trit(TotalTrits - 1, s);
        for (size_t i = 0; i < E; ++i) _data.set_trit(M + i, e.get_trit(i));
        for (size_t i = 0; i < M; ++i) _data.set_trit(i, m.get_trit(i));
    }

    constexpr Trit _unpack_sign() const { return _data.get_trit(TotalTrits - 1); }
    constexpr ExponentStorage _unpack_exponent() const {
        ExponentStorage e; for (size_t i = 0; i < E; ++i) e.set_trit(i, _data.get_trit(M + i)); return e;
    }
    constexpr MantissaStorage _unpack_mantissa() const {
        MantissaStorage m; for (size_t i = 0; i < M; ++i) m.set_trit(i, _data.get_trit(i)); return m;
    }
    constexpr auto _unpack() const { return std::tuple{_unpack_sign(), _unpack_exponent(), _unpack_mantissa()}; }

    template<size_t P>
    static constexpr T81Float _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead_pos = P;
        while (lead_pos > 0 && mant.get_trit(lead_pos - 1) == Trit::Z) --lead_pos;
        int64_t shift = static_cast<int64_t>(lead_pos - 1) - static_cast<int64_t>(M);
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

        MantissaStorage final_mant;
        for (size_t i = 0; i < M; ++i) final_mant.set_trit(i, mant.get_trit(i));

        T81Float r; r._pack(sign, ExponentStorage(exp), final_mant);
        return r;
    }

    Storage _data;
};

// DEFINITIONS — constexpr allowed here
template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M, E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t a_exp = ae.to_int64(), b_exp = be.to_int64();

    constexpr size_t P = M + 3;
    T81Int<P> af, bf;
    if (!a.is_subnormal()) af.set_trit(M, Trit::P);
    if (!b.is_subnormal()) bf.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) { af.set_trit(i, am.get_trit(i)); bf.set_trit(i, bm.get_trit(i)); }

    int64_t diff = a_exp - b_exp;
    int64_t res_exp = std::max(a_exp, b_exp);
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
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) { return a + (-b); }

template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // (same pattern as + — omitted for brevity, keep your working version)
    // just make sure you use _normalize_and_pack and no guard shifts
}

template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) { /* your working version */ }

template <size_t M, size_t E>
T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c) { /* your working version */ }

template <size_t M, size_t E>
T81Float<M, E> nextafter(const T81Float<M, E>& from, const T81Float<M, E>& to) { /* your working version */ }

template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f) { os << f.str(); return os; }

} // namespace t81::core

namespace std {
    /* hash and numeric_limits specializations unchanged — keep yours */
}
