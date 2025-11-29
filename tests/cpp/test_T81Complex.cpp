#include "t81/core/T81Complex.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81;

int main() {
    std::cout << "Running T81Complex tests...\n";

    using C = T81Complex<18>;

    // Basic construction
    C zero = C::zero();
    C one = C::one();
    C i = C::i();

    assert(zero.re.is_zero());
    assert(zero.im.is_zero());
    assert(one.re.to_double() > 0.9 && one.re.to_double() < 1.1);
    assert(one.im.is_zero());
    assert(i.re.is_zero());
    assert(i.im.to_double() > 0.9 && i.im.to_double() < 1.1);

    // Arithmetic
    C a(one.re, C::FloatType::from_double(2.0));
    C b(C::FloatType::from_double(3.0), C::FloatType::from_double(4.0));
    
    C sum = a + b;
    assert(sum.re.to_double() > 3.9 && sum.re.to_double() < 4.1);
    assert(sum.im.to_double() > 5.9 && sum.im.to_double() < 6.1);

    C diff = b - a;
    assert(diff.re.to_double() > 1.9 && diff.re.to_double() < 2.1);
    assert(diff.im.to_double() > 1.9 && diff.im.to_double() < 2.1);

    // Multiplication (i * i = -1)
    C i_squared = i * i;
    assert(i_squared.re.to_double() < -0.9 && i_squared.re.to_double() > -1.1);
    assert(std::abs(i_squared.im.to_double()) < 0.1);

    // Conjugate
    C conj_b = b.conj();
    assert(conj_b.re.to_double() > 2.9 && conj_b.re.to_double() < 3.1);
    assert(conj_b.im.to_double() < -3.9 && conj_b.im.to_double() > -4.1);

    std::cout << "All T81Complex tests PASSED!\n";
    return 0;
}

