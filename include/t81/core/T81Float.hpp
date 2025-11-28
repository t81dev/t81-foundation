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

#include "t81/core/T81Int.hpp"

namespace t81::core {

namespace detail {
/// @brief Helper for compile-time integer powers.
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

/**
 * @brief A balanced ternary floating-point number.
 * @tparam M The number of trits in the mantissa.
 * @tparam E The number of trits in the exponent.
 */
template <size_t M, size_t E>
class T81Float;

template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b);

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

    /// The number of trits in the mantissa.
    static constexpr size_t MantissaTrits = M;
    /// The number of trits in the exponent.
    static constexpr size_t ExponentTrits = E;
    /// The total number of trits in this float.
    static constexpr size_t TotalTrits = 1 + E + M;

    /// The maximum biased exponent value, reserved for Inf/NaE.
    static constexpr int64_t MaxExponent = (detail::ipow(3, E) - 1) / 2;
    /// The minimum biased exponent value, reserved for subnormals/zero.
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

        double mant_d = abs_val / std::pow(3.0, static_cast<double>(exp));

        mant_d -= 1.0;

        constexpr size_t M_temp = M + 3;
        T81Int<M_temp> mant_i;
        double rem = mant_d;
        for(size_t i = 0; i < M_temp; ++i) {
            rem *= 3;
            int trit_val = static_cast<int>(std::round(rem));
            if(trit_val > 1) trit_val = 1;
            if(trit_val < -1) trit_val = -1;
            mant_i.set_trit(M_temp - 1 - i, static_cast<Trit>(trit_val));
            rem -= trit_val;
        }

        Trit round_trit = mant_i.get_trit(2);
        mant_i >>= 3;
        if (round_trit == Trit::P) mant_i = mant_i + T81Int<M_temp>(1);
        else if (round_trit == Trit::N) mant_i = mant_i - T81Int<M_temp>(-1);

        MantissaStorage mant;
        for(size_t i = 0; i < M; ++i) mant.set_trit(i, mant_i.get_trit(i));

        if (exp > MaxExponent) *this = inf(sign == Trit::P);
        else if (exp < MinExponent) *this = zero(sign == Trit::P);
        else _pack(sign, ExponentStorage(exp), mant);
    }

    template <size_t N>
    constexpr explicit T81Float(const T81Int<N>& value) {
        if (value.is_zero()) {
            *this = zero(true);
            return;
        }

        Trit sign = value.is_negative() ? Trit::N : Trit::P;
        T81Int<N> abs_value = value.abs();

        size_t mst_pos = N;
        for (size_t i = N; i-- > 0;) {
            if (abs_value.get_trit(i) != Trit::Z) {
                mst_pos = i;
                break;
            }
        }
        
        if (mst_pos == N) { 
            *this = zero(true);
            return;
        }

        int64_t exp = static_cast<int64_t>(mst_pos);
        
        MantissaStorage mant;
        for (size_t i = 0; i < M; ++i) {
            int64_t src_idx = static_cast<int64_t>(mst_pos) - 1 - i;
            if (src_idx >= 0) {
                mant.set_trit(M - 1 - i, abs_value.get_trit(static_cast<size_t>(src_idx)));
            } else {
                mant.set_trit(M - 1 - i, Trit::Z);
            }
        }
        
        if (exp > MaxExponent) {
            *this = inf(sign == Trit::P);
        } else if (exp < MinExponent) {
            *this = zero(sign == Trit::P);
        } else {
            _pack(sign, ExponentStorage(exp), mant);
        }
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
        for(size_t i=0; i<M; ++i) full_mant.set_trit(i, mant_st.get_trit(i));

        if (exp >= N) return T81Int<N>(0);

        full_mant >>= (M - exp);

        T81Int<N> result;
        for(size_t i=0; i<N && i<=exp; ++i) result.set_trit(i, full_mant.get_trit(i));

        if (is_negative()) return -result;
        return result;
    }

    T81Float<M, E> round() const {
        return *this + T81Float<M,E>(is_negative() ? -0.5 : 0.5);
    }
    T81Float<M, E> trunc() const {
        auto [sign, exp_st, mant_st] = _unpack();
        int64_t exp = exp_st.to_int64();
        if (exp < 0) return zero(is_negative());

        T81Float<M,E> result = *this;
        for(int64_t i = exp; i >= 0; --i) {
            result._data.set_trit(static_cast<size_t>(i), Trit::Z);
        }
        return result;
    }
    T81Float<M, E> floor() const {
        if (is_negative() && !to_int<M>().is_zero()) {
            return (trunc() - T81Float<M,E>(T81Int<1>(1)));
        }
        return trunc();
    }

    double to_double() const {
        if (is_nae()) return std::numeric_limits<double>::quiet_NaN();
        if (is_inf()) return is_negative() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        if (is_zero()) return is_negative() ? -0.0 : 0.0;

        auto [sign, exp_st, mant_st] = _unpack();

        double mant_d = 0.0;
        if (!is_subnormal()) mant_d = 1.0;

        double p3 = 1.0/3.0;
        for (size_t i = 0; i < M; ++i) {
            mant_d += static_cast<double>(static_cast<int8_t>(mant_st.get_trit(M - 1 - i))) * p3;
            p3 /= 3.0;
        }

        double result = mant_d * std::pow(3.0, static_cast<double>(exp_st.to_int64()));
        return (sign == Trit::N) ? -result : result;
    }

    static constexpr T81Float zero(bool is_positive = true) {
        T81Float f;
        f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MinExponent), MantissaStorage(0));
        return f;
    }

    static constexpr T81Float inf(bool is_positive = true) {
        T81Float f;
        f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0));
        return f;
    }

    static constexpr T81Float nae() {
        T81Float f;
        MantissaStorage nae_mantissa;
        nae_mantissa.set_trit(0, Trit::P);
        f._pack(Trit::P, ExponentStorage(MaxExponent), nae_mantissa);
        return f;
    }

    constexpr bool is_zero() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MinExponent && mant.is_zero();
    }
    
    constexpr bool is_negative() const { return _unpack_sign() == Trit::N; }
    constexpr bool signbit() const { return _unpack_sign() == Trit::N; }

    constexpr bool is_inf() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MaxExponent && mant.is_zero();
    }
    
    constexpr bool is_nae() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MaxExponent && !mant.is_zero();
    }

    constexpr bool is_subnormal() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MinExponent && !mant.is_zero();
    }

    constexpr T81Float abs() const {
        if (is_negative()) {
            T81Float result = *this;
            result._data.set_trit(TotalTrits - 1, Trit::P);
            return result;
        }
        return *this;
    }

    constexpr std::partial_ordering operator<=>(const T81Float& other) const {
        if (is_nae() || other.is_nae()) return std::partial_ordering::unordered;
        if (is_zero() && other.is_zero()) return std::partial_ordering::equivalent;
        Trit self_sign = _unpack_sign();
        Trit other_sign = other._unpack_sign();
        if (self_sign != other_sign) return self_sign == Trit::P ? std::partial_ordering::greater : std::partial_ordering::less;
        auto s_exp = _unpack_exponent();
        auto o_exp = other._unpack_exponent();
        auto exp_cmp = s_exp <=> o_exp;
        if (exp_cmp != 0) return self_sign == Trit::P ? exp_cmp : (0 <=> exp_cmp);
        auto s_mant = _unpack_mantissa();
        auto o_mant = other._unpack_mantissa();
        auto mant_cmp = s_mant <=> o_mant;
        return self_sign == Trit::P ? mant_cmp : (0 <=> mant_cmp);
    }

    constexpr bool operator==(const T81Float& other) const {
        if (is_zero() && other.is_zero()) return true;
        if (is_nae() || other.is_nae()) return false;
        return _data == other._data;
    }

    bool operator!=(const T81Float& other) const {
        return !(*this == other);
    }
    
    const T81Int<TotalTrits>& internal_data() const { return _data; }
    
    std::string str() const {
        if (is_nae()) return "NaE";
        if (is_inf()) return is_negative() ? "-Inf" : "+Inf";
        if (is_zero()) return is_negative() ? "-0" : "+0";

        auto [sign, exp, mant] = _unpack();
        std::string s = (sign == Trit::P) ? "+" : "-";
        s += " exp(" + std::to_string(exp.to_int64()) + ")";
        if (is_subnormal()) {
            s += " mant(0." + mant.str() + ")";
        } else {
            s += " mant(1." + mant.str() + ")";
        }
        return s;
    }

    template<size_t M_FRIEND, size_t E_FRIEND>
    friend T81Float<M_FRIEND, E_FRIEND> make_float(Trit sign, int64_t exp_val, const T81Int<M_FRIEND>& mant_val);

    constexpr T81Float operator-() const {
        if (is_nae()) return *this;
        T81Float result = *this;
        result._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return result;
    }

    friend constexpr T81Float<M, E> operator+ (const T81Float<M, E>& a, const T81Float<M, E>& b);
    friend constexpr T81Float<M, E> operator- (const T81Float<M, E>& a, const T81Float<M, E>& b);
    friend constexpr T81Float<M, E> operator* (const T81Float<M, E>& a, const T81Float<M, E>& b);
    friend T81Float<M, E> operator/ (const T81Float<M, E>& a, const T81Float<M, E>& b);
    friend T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c);
    friend T81Float<M, E> nextafter(const T81Float<M, E>& from, const T81Float<M, E>& to);
    friend class std::numeric_limits<T81Float<M, E>>;

private:
    using Storage = T81Int<TotalTrits>;

    constexpr void _pack(Trit sign, const ExponentStorage& exp, const MantissaStorage& mant) {
        _data.set_trit(TotalTrits - 1, sign);
        for (size_t i = 0; i < E; ++i) _data.set_trit(M + i, exp.get_trit(i));
        for (size_t i = 0; i < M; ++i) _data.set_trit(i, mant.get_trit(i));
    }

    constexpr Trit _unpack_sign() const { return _data.get_trit(TotalTrits - 1); }
    
    constexpr ExponentStorage _unpack_exponent() const {
        ExponentStorage exp;
        for (size_t i = 0; i < E; ++i) exp.set_trit(i, _data.get_trit(M + i));
        return exp;
    }

    constexpr MantissaStorage _unpack_mantissa() const {
        MantissaStorage mant;
        for (size_t i = 0; i < M; ++i) mant.set_trit(i, _data.get_trit(i));
        return mant;
    }

    constexpr std::tuple<Trit, ExponentStorage, MantissaStorage> _unpack() const {
        return {_unpack_sign(), _unpack_exponent(), _unpack_mantissa()};
    }

    template<size_t P>
    static constexpr T81Float<M,E> _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead_pos = 0;
        for (size_t i = P-1; i > 0; --i) {
            if(mant.get_trit(i) != Trit::Z) {
                lead_pos = i;
                break;
            }
        }

        int64_t norm_shift = static_cast<int64_t>(lead_pos) - M;
        exp += norm_shift;
        if (norm_shift > 0) mant >>= static_cast<size_t>(norm_shift);
        else if (norm_shift < 0) mant <<= static_cast<size_t>(-norm_shift);

        if (exp >= MaxExponent) return inf(sign == Trit::P);

        if (exp < MinExponent) {
            int64_t subnorm_shift = MinExponent - exp;
            if (subnorm_shift >= M + 1) return zero(sign == Trit::P);
            mant >>= static_cast<size_t>(subnorm_shift);
            exp = MinExponent;
        }

        Trit round_trit = mant.get_trit(0);
        mant >>= 1;

        if (round_trit == Trit::P) mant = mant + T81Int<P>(1);
        else if (round_trit == Trit::N) mant = mant - T81Int<P>(1);

        MantissaStorage final_mant;
        for(size_t i = 0; i < M; ++i) final_mant.set_trit(i, mant.get_trit(i));

        T81Float result;
        result._pack(sign, ExponentStorage(exp), final_mant);
        return result;
    }

    Storage _data;
};

template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M, E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    auto [a_sign, a_exp_st, a_mant] = a._unpack();
    auto [b_sign, b_exp_st, b_mant] = b._unpack();

    int64_t a_exp = a_exp_st.to_int64();
    int64_t b_exp = b_exp_st.to_int64();

    constexpr size_t G = 5;
    constexpr size_t P = M + G + 2;
    T81Int<P> a_full, b_full;

    if (!a.is_subnormal()) a_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) a_full.set_trit(i, a_mant.get_trit(i));
    a_full <<= G;

    if (!b.is_subnormal()) b_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) b_full.set_trit(i, b_mant.get_trit(i));
    b_full <<= G;

    int64_t exp_diff = a_exp - b_exp;
    int64_t res_exp = std::max(a_exp, b_exp);

    if (exp_diff > 0) b_full >>= static_cast<size_t>(exp_diff);
    else if (exp_diff < 0) a_full >>= static_cast<size_t>(-exp_diff);

    if (a_sign == Trit::N) a_full = -a_full;
    if (b_sign == Trit::N) b_full = -b_full;

    T81Int<P> sum = a_full + b_full;

    Trit res_sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, sum);
}

template <size_t M, size_t E>
T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c) {
    if (a.is_nae() || b.is_nae() || c.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf() || b.is_inf()) {
        if (a.is_zero() || b.is_zero()) return T81Float<M, E>::nae();
        if (c.is_inf() && (a.is_negative() != b.is_negative()) != c.is_negative()) return T81Float<M, E>::nae();
        return T81Float<M, E>::inf(a.is_negative() == b.is_negative());
    }
    if (c.is_inf()) return c;

    auto [a_sign, a_exp_st, a_mant] = a._unpack();
    auto [b_sign, b_exp_st, b_mant] = b._unpack();

    int64_t prod_exp = a_exp_st.to_int64() + b_exp_st.to_int64();
    Trit prod_sign = (a_sign == b_sign) ? Trit::P : Trit::N;

    constexpr size_t P = 2 * M + 5;
    T81Int<P> a_full, b_full;
    if (!a.is_subnormal()) a_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) a_full.set_trit(i, a_mant.get_trit(i));
    if (!b.is_subnormal()) b_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) b_full.set_trit(i, b_mant.get_trit(i));

    T81Int<P> product = a_full * b_full;
    if (prod_sign == Trit::N) product = -product;

    auto [c_sign, c_exp_st, c_mant] = c._unpack();
    int64_t c_exp = c_exp_st.to_int64();
    T81Int<P> c_full;
    if (!c.is_subnormal()) c_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) c_full.set_trit(i, c_mant.get_trit(i));
    if (c_sign == Trit::N) c_full = -c_full;

    int64_t exp_diff = prod_exp - c_exp;
    int64_t res_exp = std::max(prod_exp, c_exp);
    if (exp_diff > 0) c_full >>= static_cast<size_t>(exp_diff);
    else if (exp_diff < 0) product >>= static_cast<size_t>(-exp_diff);

    T81Int<P> sum = product + c_full;
    
    Trit res_sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, sum);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    return a + (-b);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf() || b.is_inf()) {
        if (a.is_zero() || b.is_zero()) return T81Float<M, E>::nae();
        return T81Float<M, E>::inf(a.is_negative() == b.is_negative());
    }
    if (a.is_zero() || b.is_zero()) return T81Float<M, E>::zero(a.is_negative() == b.is_negative());

    auto [a_sign, a_exp_st, a_mant] = a._unpack();
    auto [b_sign, b_exp_st, b_mant] = b._unpack();

    int64_t res_exp = a_exp_st.to_int64() + b_exp_st.to_int64();
    Trit res_sign = (a_sign == b_sign) ? Trit::P : Trit::N;

    constexpr size_t P = 2 * M + 2;
    T81Int<P> a_full, b_full;

    if (!a.is_subnormal()) a_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) a_full.set_trit(i, a_mant.get_trit(i));

    if (!b.is_subnormal()) b_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) b_full.set_trit(i, b_mant.get_trit(i));

    T81Int<P> product = a_full * b_full;

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, product);
}

template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae() || (a.is_zero() && b.is_zero()) || (a.is_inf() && b.is_inf())) return T81Float<M, E>::nae();
    if (b.is_zero()) return T81Float<M, E>::inf(a.is_negative() == b.is_negative());
    if (a.is_zero()) return T81Float<M, E>::zero(a.is_negative() == b.is_negative());
    if (b.is_inf()) return T81Float<M, E>::zero(a.is_negative() == b.is_negative());
    if (a.is_inf()) return T81Float<M, E>::inf(a.is_negative() == b.is_negative());

    auto [a_sign, a_exp_st, a_mant] = a._unpack();
    auto [b_sign, b_exp_st, b_mant] = b._unpack();

    int64_t res_exp = a_exp_st.to_int64() - b_exp_st.to_int64();
    Trit res_sign = (a_sign == b_sign) ? Trit::P : Trit::N;

    constexpr size_t P = 2 * M + 2;
    T81Int<P> a_full, b_full;

    if (!a.is_subnormal()) a_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) a_full.set_trit(i, a_mant.get_trit(i));

    if (!b.is_subnormal()) b_full.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) b_full.set_trit(i, b_mant.get_trit(i));

    a_full <<= M;
    auto [quotient, remainder] = div_mod(a_full, b_full);

    return T81Float<M, E>::_normalize_and_pack(res_sign, res_exp, quotient);
}

template <size_t M, size_t E>
T81Float<M, E> nextafter(const T81Float<M, E>& from, const T81Float<M, E>& to) {
    if (from.is_nae() || to.is_nae()) return T81Float<M, E>::nae();
    if (from == to) return to;
    if (from.is_inf()) return from;

    if (from.is_zero()) {
        T81Float<M, E> result;
        typename T81Float<M, E>::MantissaStorage mant(1);
        result._pack(to.is_negative() ? Trit::N : Trit::P, typename T81Float<M, E>::ExponentStorage(T81Float<M, E>::MinExponent), mant);
        return result;
    }

    auto [sign, exp_st, mant_st] = from._unpack();
    T81Int<M + 1> mant_i;
    for(size_t i=0; i<M; ++i) mant_i.set_trit(i, mant_st.get_trit(i));
    
    if (from < to) {
        mant_i = mant_i + T81Int<M+1>(1);
    } else {
        mant_i = mant_i - T81Int<M+1>(1);
    }

    typename T81Float<M, E>::MantissaStorage new_mant;
    for(size_t i=0; i<M; ++i) new_mant.set_trit(i, mant_i.get_trit(i));

    T81Float<M, E> result;
    result._pack(sign, exp_st, new_mant);
    return result;
}

template <size_t M, size_t E>
T81Float<M, E> nexttoward(const T81Float<M, E>& from, const T81Float<M, E>& to) {
    return nextafter(from, to);
}


template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f) {
    os << f.str();
    return os;
}

} // namespace t81::core

namespace std {
template <size_t M, size_t E>
struct hash<t81::core::T81Float<M, E>> {
    std::size_t operator()(const t81::core::T81Float<M, E>& f) const {
        return std::hash<t81::core::T81Int<t81::core::T81Float<M, E>::TotalTrits>>{}(f.internal_data());
    }
};

template <size_t M, size_t E>
class numeric_limits<t81::core::T81Float<M, E>> {
public:
    using T81Float = t81::core::T81Float<M, E>;
    static constexpr bool is_specialized = true;
    static T81Float min() noexcept { return T81Float(1.0); } // Smallest positive normal value
    static T81Float max() noexcept { return T81Float::inf(false); }
    static T81Float lowest() noexcept { return T81Float::inf(false); }
    static constexpr int digits = M;
    static constexpr int digits10 = static_cast<int>(M * 0.477);
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr int radix = 3;
    static T81Float epsilon() noexcept { return T81Float(t81::core::T81Int<M>(1)) >> M; }
    static T81Float round_error() noexcept { return T81Float(0.5); }
    static constexpr int min_exponent = T81Float::MinExponent;
    static constexpr int min_exponent10 = static_cast<int>(T81Float::MinExponent * 0.477);
    static constexpr int max_exponent = T81Float::MaxExponent;
    static constexpr int max_exponent10 = static_cast<int>(T81Float::MaxExponent * 0.477);
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_present;
    static constexpr bool has_denorm_loss = false;
    static T81Float infinity() noexcept { return T81Float::inf(true); }
    static T81Float quiet_NaN() noexcept { return T81Float::nae(); }
    static T81Float signaling_NaN() noexcept { return T81Float::nae(); }
    static T81Float denorm_min() noexcept {
        T81Float f;
        f._pack(t81::core::Trit::P, typename T81Float::ExponentStorage(T81Float::MinExponent), typename T81Float::MantissaStorage(1));
        return f;
    }
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;
    static constexpr float_round_style round_style = round_to_nearest;
};
}
