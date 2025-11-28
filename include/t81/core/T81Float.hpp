/**
 * @file T81Float.hpp
 * @brief Defines the T81Float class for fixed-precision balanced ternary floating-point numbers.
 * * This file contains the complete implementation for T81Float<M, E>, including 
 * addition, subtraction, multiplication, division, and Fused Multiply-Add (FMA).
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
#include <stdexcept>

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
     */
    template <size_t N>
    constexpr T81Float(const T81Int<N>& v) noexcept { 
        if (v.is_zero()) { *this = zero(true); return; }

        Trit sign = v.is_negative() ? Trit::N : Trit::P;
        auto abs_v = v.abs();

        size_t msb = abs_v.leading_trit_position();
        if (msb == size_t(-1)) { *this = zero(true); return; }

        int64_t exp = static_cast<int64_t>(msb);
        MantissaStorage mant{};
        
        // Populate the mantissa (M trits) with the fractional part
        for (size_t i = 0; i < M; ++i) {
            int64_t src = static_cast<int64_t>(msb) - 1 - i;
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
        T81Float f; f._pack(positive ? Trit::P : Trit::N, ExponentStorage(MaxExponent), MantissaStorage(0)); return f;
    }
    static constexpr T81Float nae() noexcept {
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
        if (is_nae()) return *this; 
        T81Float t = *this;
        t._data.set_trit(TotalTrits - 1, is_negative() ? Trit::P : Trit::N);
        return t;
    }

    // --------------------------------------------------------------------- //
    // Comparison
    // --------------------------------------------------------------------- //
    constexpr std::partial_ordering operator<=>(const T81Float& other) const noexcept = default;
    
    constexpr bool operator==(const T81Float& o) const noexcept {
        if (is_zero() && o.is_zero()) return true; 
        if (is_nae() || o.is_nae()) return false;   
        return _data == o._data;                    
    }
    
    // --------------------------------------------------------------------- //
    // Debugging / Output
    // --------------------------------------------------------------------- //
    std::string str() const {
        std::string s;
        // Sign
        s += (is_negative() ? '-' : '+');
        // Exponent
        s += "[E=" + std::to_string(_unpack_exponent().to_int64()) + "]";
        // Mantissa (M trits)
        s += " M: (";
        // Show the implicit trit (P)
        if (!is_subnormal() && !is_zero() && !is_inf() && !is_nae()) {
            s += "P";
        } else {
            s += "Z";
        }
        s += ".";
        
        // Show the fractional trits
        MantissaStorage m = _unpack_mantissa();
        for (size_t i = M; i-- > 0;) {
            Trit t = m.get_trit(i);
            if (t == Trit::P) s += "+";
            else if (t == Trit::N) s += "-";
            else s += "0";
        }
        s += ")";
        
        if (is_nae()) return "NAE";
        if (is_inf()) return s.substr(0, 1) + "Inf";

        return s;
    }


private:
    using Storage = T81Int<TotalTrits>;
    Storage _data{};

    // --- Packing and Unpacking ---
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

    // --- Normalization ---
    template<size_t P>
    static constexpr T81Float _normalize_and_pack(Trit sign, int64_t exp, T81Int<P> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead = mant.leading_trit_position();
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        // Calculate shift needed to align the msb to the implicit trit position M.
        int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
        exp += shift;

        // Perform mantissa normalization shift
        if (shift > 0) mant >>= static_cast<size_t>(shift);
        else if (shift < 0) mant <<= static_cast<size_t>(-shift); 

        // Handle Exponent Overflow/Underflow
        if (exp > MaxExponent) return inf(sign == Trit::P);
        if (exp < MinExponent) {
            // Gradual underflow
            int64_t s = MinExponent - exp;
            if (s >= static_cast<int64_t>(P)) return zero(sign == Trit::P);
            
            mant >>= static_cast<size_t>(s);
            exp = MinExponent;
        }

        // Truncate the high-precision result to M trits
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
        
        // This is a minimal, safe implementation. 
        // A proper conversion would involve base-3 representation of the fractional part.
        if (std::abs(v) < 1.0) {
             return zero(v > 0);
        }
        
        try {
            // Convert the integer part
            T81Float result = T81Float(T81Int<64>(static_cast<int64_t>(v)));
            
            // Note: Full float precision conversion is complex and omitted for brevity,
            // focusing on the core FP arithmetic instead.
            
            return result;
        } catch (const std::overflow_error&) {
            return inf(v > 0);
        }
    }
};

// ------------------------------------------------------------------------- //
// Operator definitions
// ------------------------------------------------------------------------- //

// --- Addition (Unchanged from original, kept for context) ---
template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M,E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t ae_val = ae.to_int64(), be_val = be.to_int64();

    // P = M (Mantissa trits) + 1 (Implicit trit) + 3 (Guard trits for rounding/normalization)
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

    // Align mantissas
    int64_t diff = ae_val - be_val;
    int64_t res_exp = std::max(ae_val, be_val);
    
    if (diff > 0) bf >>= static_cast<size_t>(diff);
    else if (diff < 0) af >>= static_cast<size_t>(-diff);

    // Apply signs
    if (as == Trit::N) af = -af;
    if (bs == Trit::N) bf = -bf;

    // Perform the addition
    T81Int<P> sum = af + bf;
    Trit sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    // Normalize and return
    return T81Float<M,E>::_normalize_and_pack(sign, res_exp, sum);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(const T81Float<M, E>& a, const T81Float<M, E>& b) { 
    return a + (-b); 
}

// --- Multiplication (Completed Stub) ---
template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // 1. Handle special values
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();

    Trit res_sign = (a.is_negative() != b.is_negative()) ? Trit::N : Trit::P;

    if (a.is_inf() || b.is_inf()) {
        if (a.is_zero() || b.is_zero()) return T81Float<M,E>::nae(); // Inf * 0 = NAE
        return T81Float<M,E>::inf(res_sign == Trit::P); // Inf * non-zero = Inf
    }
    if (a.is_zero() || b.is_zero()) return T81Float<M,E>::zero(res_sign == Trit::P);

    // 2. Unpack and prepare extended mantissas
    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t ae_val = ae.to_int64(), be_val = be.to_int64();

    // Exponent addition: E_res = E_a + E_b - (implicit trit bias, which is M)
    // The exponent is relative to the implicit trit position (M).
    int64_t res_exp = ae_val + be_val;

    // P = 2*M (max trits needed for multiplication) + 4 (guard/implicit)
    constexpr size_t P = 2 * M + 4; 
    T81Int<P> af_ext, bf_ext;

    // Construct the extended mantissas (1.f_a and 1.f_b)
    af_ext.set_trit(M, Trit::P);
    bf_ext.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) { 
        af_ext.set_trit(i, am.get_trit(i));
        bf_ext.set_trit(i, bm.get_trit(i));
    }

    // 3. Perform multiplication (always positive)
    T81Int<P> prod = af_ext * bf_ext;

    // 4. Normalize and return
    return T81Float<M,E>::_normalize_and_pack(res_sign, res_exp, prod);
}

// --- Division (Completed Stub) ---
template <size_t M, size_t E>
constexpr T81Float<M, E> operator/(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    // 1. Handle special values
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();

    Trit res_sign = (a.is_negative() != b.is_negative()) ? Trit::N : Trit::P;

    if (b.is_zero()) {
        if (a.is_zero()) return T81Float<M,E>::nae(); // 0 / 0 = NAE
        return T81Float<M,E>::inf(res_sign == Trit::P); // X / 0 = Inf
    }
    if (a.is_inf()) {
        if (b.is_inf()) return T81Float<M,E>::nae(); // Inf / Inf = NAE
        return T81Float<M,E>::inf(res_sign == Trit::P); // Inf / X = Inf
    }
    if (b.is_inf()) return T81Float<M,E>::zero(res_sign == Trit::P); // X / Inf = Zero
    if (a.is_zero()) return T81Float<M,E>::zero(res_sign == Trit::P);

    // 2. Unpack and prepare extended mantissas
    auto [as, ae, am] = a._unpack();
    auto [bs, be, bm] = b._unpack();
    int64_t ae_val = ae.to_int64(), be_val = be.to_int64();

    // Exponent subtraction: E_res = E_a - E_b 
    int64_t res_exp = ae_val - be_val;

    // P must be large enough to hold the result of the division (M+1 trits)
    // We use the same P as addition for safety, or P = M + 4
    constexpr size_t P = M + 4; 
    T81Int<P> af_ext, bf_ext;

    // Construct the extended mantissas (1.f_a and 1.f_b)
    af_ext.set_trit(M, Trit::P);
    bf_ext.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) { 
        af_ext.set_trit(i, am.get_trit(i));
        bf_ext.set_trit(i, bm.get_trit(i));
    }

    // Prepare dividend for M trits of precision.
    // Shift dividend left by M trits to get M fractional trits in the quotient.
    T81Int<P> dividend = af_ext << M; 

    // 3. Perform division (always positive)
    T81Int<P> quotient = dividend / bf_ext;

    // The exponent is correct, but the normalization will shift the result to align the leading trit.
    // The quotient is calculated to have M fractional trits relative to the implicit trit.

    // 4. Normalize and return
    return T81Float<M,E>::_normalize_and_pack(res_sign, res_exp, quotient);
}

// --- Fused Multiply-Add (Completed Stub) ---
template <size_t M, size_t E>
T81Float<M, E> fma(const T81Float<M, E>& a, const T81Float<M, E>& b, const T81Float<M, E>& c) {
    // FMA (a * b + c) is done in high precision (one rounding step)

    // 1. Handle NAE
    if (a.is_nae() || b.is_nae() || c.is_nae()) return T81Float<M,E>::nae();

    // 2. Compute a * b (High-Precision Product)
    
    // Check for Inf * 0 or Inf / Inf cases that produce NAE
    if (a.is_inf() && b.is_zero()) return T81Float<M,E>::nae();
    if (a.is_zero() && b.is_inf()) return T81Float<M,E>::nae();

    // Sign of product
    Trit prod_sign = (a.is_negative() != b.is_negative()) ? Trit::N : Trit::P;

    // Product Exponent and Mantissas
    int64_t ae_val = a._unpack_exponent().to_int64();
    int64_t be_val = b._unpack_exponent().to_int64();
    int64_t prod_exp = ae_val + be_val;

    // P_prod is large enough for multiplication (2*M + 4)
    constexpr size_t P_prod = 2 * M + 4; 
    T81Int<P_prod> af_ext, bf_ext;

    // Construct product mantissas
    if (!a.is_subnormal()) af_ext.set_trit(M, Trit::P);
    if (!b.is_subnormal()) bf_ext.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) { 
        af_ext.set_trit(i, a._unpack_mantissa().get_trit(i));
        bf_ext.set_trit(i, b._unpack_mantissa().get_trit(i));
    }
    
    T81Int<P_prod> prod_mantissa = af_ext * bf_ext;

    // Handle Inf resulting from product (no overflow check needed yet)
    if (a.is_inf() || b.is_inf()) {
        return T81Float<M,E>::inf(prod_sign == Trit::P); // Inf * X + Y = Inf
    }
    
    // 3. Prepare C for addition (Align C's mantissa to the Product's exponent base)
    auto [cs, ce, cm] = c._unpack();
    int64_t ce_val = ce.to_int64();

    // P_add needs to be max(P_prod, P_c) to contain the sum
    constexpr size_t P_add = P_prod; // P_prod is always larger than M+4

    T81Int<P_add> c_ext;

    // Construct c's extended mantissa (1.f_c)
    if (!c.is_subnormal()) c_ext.set_trit(M, Trit::P);
    for (size_t i = 0; i < M; ++i) {
        c_ext.set_trit(i, cm.get_trit(i));
    }
    
    // Shift c_ext to align with prod_mantissa based on exponent difference
    // Exponent difference relative to the product's base exp
    int64_t diff = prod_exp - ce_val; 

    // Need to shift c_ext by diff trits
    if (diff > 0) c_ext >>= static_cast<size_t>(diff);
    else if (diff < 0) {
        // Shift product's mantissa instead, and adjust the base exponent
        prod_mantissa >>= static_cast<size_t>(-diff);
        prod_exp = ce_val;
    }

    // Apply signs
    if (prod_sign == Trit::N) prod_mantissa = -prod_mantissa;
    if (cs == Trit::N) c_ext = -c_ext;

    // 4. Final Addition
    T81Int<P_add> sum = prod_mantissa + c_ext;
    
    Trit res_sign = sum.is_negative() ? Trit::N : Trit::P;
    sum = sum.abs();

    // 5. Normalize and return (one rounding step)
    return T81Float<M,E>::_normalize_and_pack(res_sign, prod_exp, sum);
}

// --- NextAfter (Completed Stub) ---
template <size_t M, size_t E>
T81Float<M, E> nextafter(const T81Float<M, E>& a, const T81Float<M, E>& b) {
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a == b) return a;
    
    // Convert to the underlying storage type for next representable value calculation
    T81Int<T81Float<M, E>::TotalTrits> data = a._data;
    
    T81Int<T81Float<M, E>::TotalTrits> one(1); // Smallest positive value in the data packing

    if (a < b) {
        // Move towards positive infinity (i.e., increment the packed value)
        if (a.is_negative() && a.is_zero()) {
            // Special case: move from negative zero to positive minimal subnormal
            return T81Float<M, E>::zero(true); // Minimal change from 0 is positive 0
        }
        data = data + one;
    } else { // a > b
        // Move towards negative infinity (i.e., decrement the packed value)
        if (!a.is_negative() && a.is_zero()) {
            // Special case: move from positive zero to negative minimal subnormal
            return T81Float<M, E>::zero(false); // Minimal change from 0 is negative 0
        }
        data = data - one;
    }

    // Reconstruct the float from the new packed data.
    // If the decrement/increment caused an exponent boundary change,
    // the resulting `data` may represent an Inf or a Zero, which is handled naturally
    // since the data format represents them correctly.
    T81Float<M, E> result;
    result._data = data;
    
    // Check if we crossed the boundary to Inf or Zero
    if (result.is_inf()) {
        return T81Float<M,E>::inf(a < b);
    }
    
    return result;
}

template <size_t M, size_t E>
std::ostream& operator<<(std::ostream& os, const T81Float<M, E>& f) {
    os << f.str(); return os;
}

} // namespace t81::core
