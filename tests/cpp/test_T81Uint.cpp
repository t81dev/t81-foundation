#include "t81/core/T81Uint.hpp"
#include <cassert>

using namespace t81;

int main() {
    // Running T81Uint tests...

    using U = T81UInt<28>;  // 28-trit unsigned integer (must be multiple of 4)

    // Construction
    U zero;
    U one(1);
    U large(100);

    assert(zero.to_signed().to_int64() == 0);
    assert(one.to_signed().to_int64() == 1);
    assert(large.to_signed().to_int64() == 100);

    // Arithmetic
    U sum = one + one;
    assert(sum.to_signed().to_int64() == 2);

    U diff = U(5) - U(3);
    assert(diff.to_signed().to_int64() == 2);

    U prod = U(3) * U(4);
    assert(prod.to_signed().to_int64() == 12);

    // Comparison
    assert(one < U(2));
    assert(U(5) > U(3));
    assert(one == one);

    // Bitwise operations
    U a(5);
    U b(3);
    assert((a & b).to_signed().to_int64() == 1);
    assert((a | b).to_signed().to_int64() == 7);
    assert((a ^ b).to_signed().to_int64() == 6);

    // All T81Uint tests PASSED!
    return 0;
}

