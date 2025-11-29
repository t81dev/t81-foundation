#include "t81/core/T81Fixed.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81::core;

int main() {
    std::cout << "Running T81Fixed tests...\n";

    using Fixed = T81Fixed<18, 9>;  // 18 integer trits, 9 fractional trits

    // Construction
    Fixed zero;  // Default constructor creates zero
    Fixed one = Fixed::from_double(1.0);
    Fixed half = Fixed::from_double(0.5);

    assert(zero.is_zero());
    assert(!one.is_zero());
    assert(one.to_double() > 0.9 && one.to_double() < 1.1);
    assert(half.to_double() > 0.4 && half.to_double() < 0.6);

    // Arithmetic
    Fixed sum = one + half;
    assert(sum.to_double() > 1.4 && sum.to_double() < 1.6);

    Fixed diff = one - half;
    assert(diff.to_double() > 0.4 && diff.to_double() < 0.6);

    Fixed prod = half * Fixed::from_double(2.0);
    assert(prod.to_double() > 0.9 && prod.to_double() < 1.1);

    // Comparison
    assert(one > half);
    assert(half < one);
    assert(one == one);

    // Negation
    Fixed neg = -one;
    assert(neg.to_double() < -0.9 && neg.to_double() > -1.1);

    std::cout << "All T81Fixed tests PASSED!\n";
    return 0;
}

