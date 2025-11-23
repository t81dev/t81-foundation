#include <t81/bigint/divmod.hpp>
#include <cassert>

namespace t81 {

DivModResult divmod(const T243BigInt& a, const T243BigInt& b) {
    assert(!b.is_zero() && "divmod: divisor must be non-zero");

    // Handle simple case: a == 0
    if (a.is_zero()) {
        return DivModResult{T243BigInt(0), T243BigInt(0)};
    }

    const bool a_neg = a.is_negative();
    const bool b_neg = b.is_negative();

    T243BigInt ua = a_neg ? a.abs() : a;
    T243BigInt ub = b_neg ? b.abs() : b;

    T243BigInt uq;
    T243BigInt ur;

    // Core magnitude division: ua = ub * uq + ur, with 0 <= ur < ub
    T243BigInt::divmod_nonneg_(ua, ub, uq, ur);

    // Sign of the "truncated" quotient:
    // If signs differ, quotient is negative; otherwise non-negative.
    if (a_neg ^ b_neg) {
        uq = -uq;
    }

    // Now adjust to Euclidean remainder if necessary.
    //
    // We currently have:
    //   a = (a_neg ? -ua : ua) = (b_neg ? -ub : ub) * uq + (sign?)*ur
    //
    // But we want:
    //   a = b * q_e + r_e with 0 <= r_e < |b|
    //
    // A standard way: compute provisional r, then fix up if r < 0.
    T243BigInt q = uq;
    T243BigInt r = a - b * q;

    if (r.is_negative()) {
        // Adjust: r_e = r + |b|, q_e = q - sign(b)
        // where sign(b) is +1 if b > 0, -1 if b < 0.
        T243BigInt abs_b = b_neg ? b.abs() : b;
        r += abs_b;

        if (b_neg) {
            q += T243BigInt(1);   // b < 0 => subtracting sign(b) == -1
        } else {
            q -= T243BigInt(1);   // b > 0 => subtracting sign(b) == +1
        }
    }

    // At this point:
    //   a = b * q + r
    //   and 0 <= r < |b|
    return DivModResult{q, r};
}

} // namespace t81
