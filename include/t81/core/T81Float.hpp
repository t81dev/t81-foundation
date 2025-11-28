#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <cmath>
#include <limits>

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
} // namespace detail

// Forward declarations
template <size_t M, size_t E> class T81Float;
template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
constexpr T81Float<M, E> operator/(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
constexpr T81Float<M, E> fma(const T81Float<M, E>&, const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
constexpr T81Float<M, E> t81_nextafter(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
std::ostream& operator<<(std::ostream&, const T81Float<M, E>&);


template <size_t M, size_t E>
class T81Float {
public:
    static_assert(M > 0 && E > 1);

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

        size_t msb = _msb_pos(abs_v);
        if (msb == size_t(-1)) { *this = zero(true); return; }

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

    constexpr const T81Int<TotalTrits>& internal_data() const { return _data; }

    std::string str() const {
        if (is_nae()) return "NaE";
        if (is_inf()) return (is_negative() ? "-" : "+") + std::string("Inf");
        if (is_zero()) return (is_negative() ? "-0" : "+0");
        // TODO: A real implementation would convert the value to a string.
        return "<T81Float>";
    }

    // --------------------------------------------------------------------- //
    // Friends
    // --------------------------------------------------------------------- //
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> operator+(const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> operator-(const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> operator*(const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> operator/(const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> fma(const T81Float<M_, E_>&, const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    template<size_t M_, size_t E_>
    friend constexpr T81Float<M_, E_> t81_nextafter(const T81Float<M_, E_>&, const T81Float<M_, E_>&);
    friend class std::numeric_limits<T81Float<M, E>>;

private:
    using Storage = T81Int<TotalTrits>;
    Storage _data{};

    constexpr void _pack(Trit s, const ExponentStorage& e, const MantissaStorage& m) noexcept {
        _data.set_trit(TotalTrits - 1, s);
        for (size_t i = 0; i < E; ++i) _data.set_trit(M + i, e.get_trit(i));
        for (size_t i = 0; i < M; ++i) _data.set_trit(i, m.get_trit(i));
    }
    constexpr Trit _unpack_sign() const noexcept { return _data.get_trit(TotalTrits - 1); }
    constexpr ExponentStorage _unpack_exponent() const noexcept {
        ExponentStorage e;
        for (size_t i = 0; i < E; ++i) e.set_trit(i, _data.get_trit(M + i));
        return e;
    }
    constexpr MantissaStorage _unpack_mantissa() const noexcept {
        MantissaStorage m;
        for (size_t i = 0; i < M; ++i) m.set_trit(i, _data.get_trit(i));
        return m;
    }
    constexpr auto _unpack() const noexcept { return std::tuple{_unpack_sign(), _unpack_exponent(), _unpack_mantissa()}; }

    // The one true normalization — with proper round-to-nearest, ties-to-even.
    template<size_t P>
    static constexpr T81Float _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead = _msb_pos(mant);
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        // We need 3 extra trits for rounding: Guard, Round, Sticky
        constexpr int extra_trits = 3;
        int64_t shift = static_cast<int64_t>(lead) - (static_cast<int64_t>(M) + extra_trits);
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

        // --- Rounding Logic ---
        Trit guard = mant.get_trit(extra_trits - 1);
        Trit round = mant.get_trit(extra_trits - 2);

        bool sticky = false;
        for (size_t i = 0; i < extra_trits - 2; ++i) {
            if (mant.get_trit(i) != Trit::Z) {
                sticky = true;
                break;
            }
        }

        mant >>= extra_trits;

        bool round_up = false;
        if (guard == Trit::P) {
            if (round == Trit::P || sticky) {
                round_up = true;
            } else { // Tie-breaking: round to even (LSB is Z)
                if (mant.get_trit(0) == Trit::P) {
                    round_up = true;
                }
            }
        }

        if (round_up) {
            mant = mant + T81Int<P>(1);
            if (_msb_pos(mant) > M) {
                mant >>= 1;
                exp += 1;
                if (exp > MaxExponent) return inf(sign == Trit::P);
            }
        }
        // --- End Rounding Logic ---

        MantissaStorage final_m;
        for (size_t i = 0; i < M; ++i) final_m.set_trit(i, mant.get_trit(i));

        T81Float f;
        f._pack(sign, ExponentStorage(exp), final_m);
        return f;
    }

    // double → T81Float
    static T81Float from_double(double v) {
        if (v == 0.0) return zero(std::copysign(1.0, v) > 0);
        if (std::isinf(v)) return inf(v > 0);
        if (std::isnan(v)) return nae();

        Trit sign = (v > 0) ? Trit::P : Trit::N;
        double abs_v = std::abs(v);

        double exp3_d = std::log(abs_v) / std::log(3.0);
        int64_t exp = static_cast<int64_t>(std::floor(exp3_d));

        constexpr size_t P = M + 16;
        T81Int<P> mant;
        double remainder = abs_v / std::pow(3.0, static_cast<double>(exp));

        for (int i = static_cast<int>(P) - 1; i >= 0; --i) {
            remainder *= 3.0;
            int trit_val = static_cast<int>(std::round(remainder - 1.5));
            Trit t = (trit_val == 1) ? Trit::P : ((trit_val == -1) ? Trit::N : Trit::Z);
            mant.set_trit(i, t);
            remainder -= static_cast<double>(trit_val);
        }

        return _normalize_and_pack(sign, exp, mant);
    }

    static constexpr T81Float from_internal_data(const T81Int<TotalTrits>& d) noexcept {
        T81Float f;
        f._data = d;
        return f;
    }

    template<size_t N_Int>
    static constexpr size_t _msb_pos(const T81Int<N_Int>& v) {
        for (size_t i = N_Int; i-- > 0;) {
            if (v.get_trit(i) != Trit::Z) {
                return i;
            }
        }
        return size_t(-1);
    }
};

// ------------------------------------------------------------------------- //
// Operator definitions — constexpr allowed ONLY here
// ------------------------------------------------------------------------- //
template <size_t M, size_t E>
inline constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
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
    for (size_t i = 0; i < M; ++i) { af.set_trit(i, am.get_trit(i)); bf.set_trit(i, bm.get_trit(i)); }

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
inline constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) { return a + (-b); }

template <size_t M, size_t E>
inline constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_zero() || b.is_zero()) return T81Float<M, E>::zero(a.is_negative() != b.is_negative());
    if (a.is_inf() || b.is_inf()) return T81Float<M, E>::inf(a.is_negative() != b.is_negative());

    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();

    int64_t res_exp = ae.to_int64() + be.to_int64();
    Trit res_sign = (as == bs) ? Trit::P : Trit::N;

    constexpr size_t P = 2 * M + 2;
    T81Int<P> m_a, m_b;
    if (!a.is_subnormal()) m_a.set_trit(M, Trit::P);
    if (!b.is_subnormal()) m_b.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) {
        m_a.set_trit(i, am.get_trit(i));
        m_b.set_trit(i, bm.get_trit(i));
    }

    T81Int<P> prod = m_a * m_b;
    res_exp -= M;

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, prod);
}

template <size_t M, size_t E>
inline constexpr T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf()) { // inf / anything
        if (b.is_inf()) return T81Float<M, E>::nae();
        return T81Float<M, E>::inf(a.is_negative() == b.is_negative());
    }
    if (b.is_zero()) { // anything / zero
        if (a.is_zero()) return T81Float<M, E>::nae();
        return T81Float<M, E>::inf(a.is_negative() == b.is_negative());
    }
    if (b.is_inf()) return T81Float<M, E>::zero(a.is_negative() == b.is_negative()); // anything / inf
    if (a.is_zero()) return T81Float<M, E>::zero(a.is_negative() == b.is_negative());

    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();

    int64_t res_exp = ae.to_int64() - be.to_int64();
    Trit res_sign = (as == bs) ? Trit::P : Trit::N;

    constexpr size_t P = M + 16;
    T81Int<P> num;
    if (!a.is_subnormal()) num.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) num.set_trit(i, am.get_trit(i));

    T81Int<P> den;
    if (!b.is_subnormal()) den.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) den.set_trit(i, bm.get_trit(i));

    constexpr size_t precision_shift = 8;
    num <<= precision_shift;
    res_exp -= precision_shift;

    T81Int<P> quot(0);
    T81Int<P> rem(num);

    for (int i = M + precision_shift; i >= 0; --i) {
        T81Int<P> trial = rem - (den << i);
        if (!trial.is_negative()) {
            rem = trial;
            quot.set_trit(i, Trit::P);
        }
    }

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, quot);
}

template <size_t M, size_t E>
inline constexpr T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c) {
    return a * b + c; // TODO: Implement fused multiply-add
}

template <size_t M, size_t E>
inline constexpr T81Float<M, E> t81_nextafter(const T81Float<M, E>& from, const T81Float<M, E>& to) {
    if (from.is_nae() || to.is_nae()) return T81Float<M, E>::nae();
    if (from.is_zero() && to.is_zero()) return to;
    if (from == to) return to;

    if (from < to) { // increment
        T81Int<T81Float<M, E>::TotalTrits> repr(from.internal_data());
        repr = repr + T81Int<T81Float<M, E>::TotalTrits>(1);
        return T81Float<M, E>::from_internal_data(repr);
    } else { // from > to, decrement
        T81Int<T81Float<M, E>::TotalTrits> repr(from.internal_data());
        repr = repr - T81Int<T81Float<M, E>::TotalTrits>(1);
        return T81Float<M, E>::from_internal_data(repr);
    }
}

template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f) {
    os << f.str(); return os;
}

} // namespace t81::core
