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
T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b);

template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b);

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

    T81Float() {
        _pack(Trit::P, ExponentStorage(MinExponent), MantissaStorage(0));
    }

    explicit T81Float(double value) {
        if (std::isnan(value)) *this = nae();
        else if (std::isinf(value)) *this = inf(value > 0);
        else *this = zero(!std::signbit(value));
    }

    template <size_t N>
    explicit T81Float(const T81Int<N>& value) {
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
        // Correctly copy the M trits *below* the MST into the mantissa
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
    T81Int<N> to_int() const { return T81Int<N>(0); }

    static T81Float zero(bool is_positive = true) {
        T81Float f;
        f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MinExponent), MantissaStorage(0));
        return f;
    }

    static T81Float inf(bool is_positive = true) {
        T81Float f;
        f._pack(is_positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0));
        return f;
    }

    static T81Float nae() {
        T81Float f;
        MantissaStorage nae_mantissa;
        nae_mantissa.set_trit(0, Trit::P);
        f._pack(Trit::P, ExponentStorage(MaxExponent), nae_mantissa);
        return f;
    }

    bool is_zero() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MinExponent && mant.is_zero();
    }
    
    bool is_negative() const { return _unpack_sign() == Trit::N; }

    bool is_inf() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MaxExponent && mant.is_zero();
    }
    
    bool is_nae() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MaxExponent && !mant.is_zero();
    }

    bool is_subnormal() const {
        auto [sign, exp, mant] = _unpack();
        return exp.to_int64() == MinExponent && !mant.is_zero();
    }

    T81Float abs() const {
        if (is_negative()) {
            T81Float result = *this;
            result._data.set_trit(TotalTrits - 1, Trit::P);
            return result;
        }
        return *this;
    }

    std::partial_ordering operator<=>(const T81Float& other) const {
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

    bool operator==(const T81Float& other) const {
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

    T81Float operator-() const {
        if (is_nae()) return *this; // NaE has no sign
        T81Float result = *this;
        result._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return result;
    }

    friend T81Float<M, E> operator+<>(const T81Float<M, E>& a, const T81Float<M, E>& b);

private:
    using Storage = T81Int<TotalTrits>;

    void _pack(Trit sign, const ExponentStorage& exp, const MantissaStorage& mant) {
        _data.set_trit(TotalTrits - 1, sign);
        for (size_t i = 0; i < E; ++i) _data.set_trit(M + i, exp.get_trit(i));
        for (size_t i = 0; i < M; ++i) _data.set_trit(i, mant.get_trit(i));
    }

    Trit _unpack_sign() const { return _data.get_trit(TotalTrits - 1); }
    
    ExponentStorage _unpack_exponent() const {
        ExponentStorage exp;
        for (size_t i = 0; i < E; ++i) exp.set_trit(i, _data.get_trit(M + i));
        return exp;
    }

    MantissaStorage _unpack_mantissa() const {
        MantissaStorage mant;
        for (size_t i = 0; i < M; ++i) mant.set_trit(i, _data.get_trit(i));
        return mant;
    }

    std::tuple<Trit, ExponentStorage, MantissaStorage> _unpack() const {
        return {_unpack_sign(), _unpack_exponent(), _unpack_mantissa()};
    }

    Storage _data;
};

template <size_t M, size_t E>
T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M, E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M, E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    auto [a_sign, a_exp_st, a_mant] = a._unpack();
    auto [b_sign, b_exp_st, b_mant] = b._unpack();

    int64_t a_exp = a_exp_st.to_int64();
    int64_t b_exp = b_exp_st.to_int64();

    T81Int<M + 3> a_full, b_full;

    bool a_normalized = a_exp > T81Float<M, E>::MinExponent;
    bool b_normalized = b_exp > T81Float<M, E>::MinExponent;

    if (a_normalized) a_full.set_trit(M, Trit::P);
    if (b_normalized) b_full.set_trit(M, Trit::P);

    for (size_t i = 0; i < M; ++i) {
        a_full.set_trit(i, a_mant.get_trit(i));
        b_full.set_trit(i, b_mant.get_trit(i));
    }

    int64_t exp_diff = a_exp - b_exp;
    if (exp_diff > 0) {
        b_full >>= static_cast<size_t>(exp_diff);
    } else if (exp_diff < 0) {
        a_full >>= static_cast<size_t>(-exp_diff);
    }
    int64_t res_exp = std::max(a_exp, b_exp);

    if (a_sign == Trit::N) a_full = -a_full;
    if (b_sign == Trit::N) b_full = -b_full;

    T81Int<M + 3> sum = a_full + b_full;

    if (sum.is_zero()) {
        return T81Float<M, E>::zero();
    }

    Trit res_sign = Trit::P;
    if (sum.is_negative()) {
        res_sign = Trit::N;
        sum = -sum;
    }

    size_t lead_pos = 0;
    for (size_t i = M + 2; i > 0; --i) {
        if (sum.get_trit(i) != Trit::Z) {
            lead_pos = i;
            break;
        }
    }

    int64_t shift = static_cast<int64_t>(lead_pos) - static_cast<int64_t>(M);
    res_exp += shift;

    if (shift > 0) {
        sum >>= static_cast<size_t>(shift);
    } else if (shift < 0) {
        sum <<= static_cast<size_t>(-shift);
    }
    
    if (res_exp > T81Float<M, E>::MaxExponent) {
        return T81Float<M, E>::inf(res_sign == Trit::P);
    }
    if (res_exp < T81Float<M, E>::MinExponent) {
        return T81Float<M, E>::zero(res_sign == Trit::P);
    }
    
    typename T81Float<M, E>::MantissaStorage res_mant;
    for (size_t i = 0; i < M; ++i) {
        res_mant.set_trit(i, sum.get_trit(i));
    }

    T81Float<M, E> result;
    result._pack(res_sign, typename T81Float<M, E>::ExponentStorage(res_exp), res_mant);
    return result;
}

template <size_t M, size_t E>
T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    return a + (-b);
}

template <size_t M, size_t E>
T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    return T81Float<M, E>::nae();
}

template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    return T81Float<M, E>::nae();
}

} // namespace t81::core
