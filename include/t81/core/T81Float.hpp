/**
 * @file T81Float.hpp
 * @brief Defines the T81Float class for fixed-precision balanced ternary floating-point numbers.
 * * This file contains the implementation for T81Float<M, E>, which uses the T81Int<N> 
 * class for internal mantissa (M trits) and exponent (E trits) storage.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <limits>
#include <tuple>
#include <compare>
#include <string>
#include <ostream>
#include <algorithm>

#include "t81/core/T81Int.hpp"

namespace t81::core {

namespace detail {
// Helper for compile-time power calculation
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
T81Float<M, E> operator+(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
T81Float<M, E> operator-(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
T81Float<M, E> operator*(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
T81Float<M, E> operator/(const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
std::ostream& operator<<(std::ostream&, const T81Float<M, E>&);
template <size_t M, size_t E>
T81Float<M, E> fma(const T81Float<M, E>&, const T81Float<M, E>&, const T81Float<M, E>&);
template <size_t M, size_t E>
T81Float<M, E> nextafter(const T81Float<M, E>&, const T81Float<M, E>&);

template <size_t M, size_t E>
class T81Float {
public:
    static_assert(M > 0 && E > 1, "M (Mantissa) and E (Exponent) must be positive and E > 1.");

    using ExponentStorage = T81Int<E>;
    using MantissaStorage = T81Int<M>;

    static constexpr size_t MantissaTrits = M;
    static constexpr size_t ExponentTrits = E;
    static constexpr size_t TotalTrits = 1 + E + M; // 1 (Sign) + E (Exponent) + M (Mantissa)

    // Calculate the max/min exponent value based on E trits
    static constexpr int64_t MaxExponent = (detail::ipow(3, E) - 1) / 2;
    static constexpr int64_t MinExponent = -MaxExponent;

    // --------------------------------------------------------------------- //
    // Construction
    // --------------------------------------------------------------------- //
    constexpr T81Float() noexcept { 
        // Default constructor initializes to positive zero (MinExponent, Zero Mantissa)
        _pack(Trit::P, ExponentStorage(MinExponent), MantissaStorage(0)); 
    }

    explicit T81Float(double v) { *this = from_double(v); }

    /**
     * @brief Constructs a T81Float from a T81Int.
     * Implements normalization: determines exponent, shifts mantissa, handles overflow.
     */
    template <size_t N>
    constexpr T81Float(const T81Int<N>& v) noexcept { 
        if (v.is_zero()) { *this = zero(true); return; }

        Trit sign = v.is_negative() ? Trit::N : Trit::P;
        auto abs_v = v.abs();

        // msb is the position of the leading non-zero trit in the T81Int
        size_t msb = abs_v.leading_trit_position();
        
        // If msb is the sentinel value for zero, return zero.
        if (msb == size_t(-1)) { *this = zero(true); return; }

        int64_t exp = static_cast<int64_t>(msb);
        MantissaStorage mant{};
        
        // Populate the mantissa (M trits) with the fractional part
        // The most significant trit (at msb) is the implicit '1' (or Trit::P) and is NOT stored.
        for (size_t i = 0; i < M; ++i) {
            // src is the index in the large T81Int corresponding to the fractional trits
            int64_t src = static_cast<int64_t>(msb) - 1 - i;
            // The fractional part is stored in descending order (highest frac trit at M-1)
            mant.set_trit(M - 1 - i, (src >= 0) ? abs_v.get_trit(static_cast<size_t>(src)) : Trit::Z);
        }

        if (exp > MaxExponent) *this = inf(sign == Trit::P);
        else if (exp < MinExponent) *this = zero(sign == Trit::P);
        else _pack(sign, ExponentStorage(exp), mant);
    }

    // --------------------------------------------------------------------- //
    // Special values
    // --------------------------------------------------------------------- //
    static constexpr T81Float zero(bool positive = true) noexcept {
        T81Float f; f._pack(positive ? Trit::P : Trit::N, ExponentStorage(MinExponent), MantissaStorage(0)); return f;
    }
    static constexpr T81Float inf(bool positive = true) noexcept {
        // Exponent is MaxExponent, Mantissa is zero
        T81Float f; f._pack(positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0)); return f;
    }
    static constexpr T81Float nae() noexcept {
        // Exponent is MaxExponent, Mantissa is non-zero (set LSB to P)
        T81Float f; MantissaStorage m; m.set_trit(0, Trit::P);
        f._pack(Trit::P, ExponentStorage(MaxExponent), m); return f;
    }

    // --------------------------------------------------------------------- //
    // Queries
    // --------------------------------------------------------------------- //
    constexpr bool is_zero()       const noexcept { auto [s,e,m] = _unpack(); return e.to_int64() == MinExponent && m.is_zero(); }
    constexpr bool is_negative() const noexcept { return _unpack_sign() == Trit::N; }
    constexpr bool is_inf()        const noexcept { auto [s,e,m] = _unpack(); return e.to_int64() == MaxExponent && m.is_zero(); }
    constexpr bool is_nae()        const noexcept { auto [s,e,m] = _unpack(); return e.to_int64() == MaxExponent && !m.is_zero(); }
    constexpr bool is_subnormal()  const noexcept { auto [s,e,m] = _unpack(); return e.to_int64() == MinExponent && !m.is_zero(); }

    constexpr T81Float abs() const noexcept { T81Float t = *this; t._data.set_trit(TotalTrits - 1, Trit::P); return t; }
    constexpr T81Float operator-() const noexcept {
        if (is_nae()) return *this; // Negating NAE returns NAE
        T81Float t = *this;
        // Flip the sign bit (which is Trit::P or Trit::N)
        t._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return t;
    }

    // --------------------------------------------------------------------- //
    // Comparison
    // --------------------------------------------------------------------- //
    // Default three-way comparison uses the underlying Storage's comparison.
    constexpr std::partial_ordering operator<=>(const T81Float& other) const noexcept = default;
    
    /**
     * @brief Custom equality check, handles signed zero and NAE rules.
     */
    constexpr bool operator==(const T81Float& o) const noexcept {
        if (is_zero() && o.is_zero()) return true; // Positive and negative zero are equal
        if (is_nae() || o.is_nae()) return false;   // NAE is never equal to anything, including itself
        return _data == o._data;                    // Otherwise, rely on packed data comparison
    }
    
    // --------------------------------------------------------------------- //
    // Debugging / Output
    // --------------------------------------------------------------------- //
    /**
     * @brief Converts the T81Float to a string showing its packed, raw trit representation.
     * @return A string representation (e.g., "+---000++")
     */
    std::string str() const {
        return _data.str();
    }

    // --------------------------------------------------------------------- //
    // Friends
    // --------------------------------------------------------------------- //
    friend T81Float operator+ <>(const T81Float&, const T81Float&);
    friend T81Float operator- <>(const T81Float&, const T81Float&);
    friend T81Float operator* <>(const T81Float&, const T81Float&);
    friend T81Float operator/ <>(const T81Float&, const T81Float&);
    friend T81Float fma<>(const T81Float&, const T81Float&, const T81Float&);
    friend T81Float nextafter<>(const T81Float&, const T81Float&);
    friend class std::numeric_limits<T81Float<M, E>>;

private:
    using Storage = T81Int<TotalTrits>;
    Storage _data{}; // The underlying storage for sign, exponent, and mantissa

    // --- Packing and Unpacking ---
    constexpr void _pack(Trit s, const ExponentStorage& e, const MantissaStorage& m) noexcept {
        // Layout: [Sign | Exponent (E trits) | Mantissa (M trits)]
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

    // --- Normalization ---
    /**
     * @brief Normalizes the resulting mantissa and exponent, performs rounding (truncation), 
     * and packs the result into a T81Float.
     * @tparam P The precision of the input mantissa (M + guard trits).
     */
    template<size_t P>
    static constexpr T81Float _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);

        // Find the most significant trit (msb)
        size_t lead = mant.leading_trit_position();
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        // Calculate shift needed to align the msb to the implicit trit position M.
        int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
        exp += shift;

        // Perform mantissa normalization shift
        if (shift > 0) mant >>= static_cast<size_t>(shift);
        else if (shift < 0) mant <<= static_cast<size_t>(-shift); // Should not happen after addition, but safe to include

        // Handle Exponent Overflow/Underflow
        if (exp > MaxExponent) return inf(sign == Trit::P);
        if (exp < MinExponent) {
            // Gradual underflow: shift the mantissa right until the exponent is MinExponent
            int64_t s = MinExponent - exp;
            if (s >= static_cast<int64_t>(P)) return zero(sign == Trit::P); // Result is entirely zero
            
            // Shift into the subnormal range
            mant >>= static_cast<size_t>(s);
            exp = MinExponent;
        }

        // Truncate the high-precision result to M trits (round-to-zero)
        MantissaStorage final_m;
        for (size_t i = 0; i < M; ++i) final_m.set_trit(i, mant.get_trit(i));

        T81Float f;
        f._pack(sign, ExponentStorage(exp), final_m);
        return f;
    }
    
    /**
     * @brief Stub implementation for converting double to T81Float.
     */
    static T81Float from_double(double v) {
        if (std::isnan(v)) return nae();
        if (std::isinf(v)) return inf(v > 0);
        
        // Simplified conversion: convert double to int64_t, then use the int constructor
        if (std::abs(v) < 1.0) {
             // For small values, return zero as a placeholder for full float conversion logic
             return zero(v > 0);
        }
        
        try {
            return T81Float(T81Int<64>(static_cast<int64_t>(v)));
        } catch (const std::overflow_error&) {
            // If the integer conversion overflows the 64 trits, return inf
            return inf(v > 0);
        }
    }
};

// ------------------------------------------------------------------------- //
// Operator definitions — constexpr allowed ONLY here
// ------------------------------------------------------------------------- //
template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // 1. Handle special values (NAE, Inf, Zero)
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M,E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    // 2. Unpack and prepare extended mantissas
    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t ae_val = ae.to_int64(), be_val = be.to_int64();

    // P = M (Mantissa trits) + 1 (Implicit trit) + 3 (Guard trits) = M + 4
    constexpr size_t P = M + 4; 
    T81Int<P> af, bf;
    
    // Inject the explicit '1' (or Trit::P) trit for normalized numbers
    if (!a.is_subnormal()) af.set_trit(M, Trit::P);
    if (!b.is_subnormal()) bf.set_trit(M, Trit::P);
    
    // Copy the mantissa trits
    for (size_t i = 0; i < M; ++i) { 
        af.set_trit(i, am.get_trit(i)); 
        bf.set_trit(i, bm.get_trit(i)); 
    }

    // 3. Align mantissas
    int64_t diff = ae_val - be_val;
    int64_t res_exp = std::max(ae_val, be_val);
    
    if (diff > 0) bf >>= static_cast<size_t>(diff);
    else if (diff < 0) af >>= static_cast<size_t>(-diff);

    // 4. Apply signs to the extended mantissas
    if (as == Trit::N) af = -af;
    if (bs == Trit::N) bf = -bf;

    // 5. Perform the addition
    T81Int<P> sum = af + bf;
    Trit sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    // 6. Normalize and return
    return T81Float<M,E>::_normalize_and_pack(sign, res_exp, sum);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) { 
    // Subtraction is implemented as addition with negation
    return a + (-b); 
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // Placeholder for multiplication
    return T81Float<M, E>::nae();
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // Placeholder for division
    if (b.is_zero()) return T81Float<M,E>::inf(a.is_negative() != b.is_negative());
    if (a.is_zero()) return T81Float<M,E>::zero(a.is_negative() != b.is_negative());
    return T81Float<M, E>::nae();
}

template <size_t M, size_t E>
T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c) {
    // Placeholder for Fused Multiply-Add
    return T81Float<M, E>::nae();
}

template <size_t M, size_t E>
T81Float<M, E> nextafter(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // Placeholder for nextafter
    return T81Float<M, E>::nae();
}

template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f) {
    os << f.str(); return os;
}

} // namespace t81::core
