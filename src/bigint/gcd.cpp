#include <t81/bigint/gcd.hpp>
#include <t81/bigint/divmod.hpp>

namespace t81 {

T243BigInt gcd(T243BigInt a, T243BigInt b) {
    // Ensure non-negative inputs
    if (a.is_negative()) a = a.abs();
    if (b.is_negative()) b = b.abs();

    // Handle trivial cases explicitly
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    // Euclidean algorithm using Euclidean remainder
    while (!b.is_zero()) {
        DivModResult dm = divmod(a, b);
        a = b;
        b = dm.r;  // remainder always satisfies 0 <= r < |b|
    }

    // a is now gcd, and is guaranteed non-negative by construction
    return a;
}

} // namespace t81
