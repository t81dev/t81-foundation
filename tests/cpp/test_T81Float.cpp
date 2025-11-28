// tests/cpp/test_T81Float.cpp
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>
#include <limits>

using namespace t81::core;

int main() {
    std::cout << "T81Float smoke tests running...\n";

    using F = T81Float<18, 9>;

    // Special values
    assert(F::zero().is_zero());
    assert(F::inf(true).is_inf() && !F::inf(true).is_negative());
    assert(F::inf(false).is_inf() && F::inf(false).is_negative());
    assert(F::nae().is_nae());

    // Construction from integers
    F one(T81Int<20>(1));
    F two(T81Int<20>(2));
    F three(T81Int<20>(3));
    F neg_one(T81Int<20>(-1));

    assert(one > F::zero());
    assert(neg_one < F::zero());
    assert(two > one);
    assert(-one == neg_one);
    assert(one.abs() == one);

    // Basic arithmetic
    assert(one + one == two);
    assert(two + one == three);
    assert(one + neg_one == F::zero());
    assert(two - one == one);
    assert(one - two == neg_one);

    // Multiplication & division
    F five(T81Int<20>(5));
    F seven(T81Int<20>(7));
    assert(five * seven == F(T81Int<40>(35)));
    assert(F(T81Int<20>(6)) / three == two);

    // FMA
    assert(fma(two, three, five) == F(T81Int<40>(11))); // 2×3 + 5 = 11

    // Double round-trip (use the double-sized format)
    using D = T81Float<52, 11>;
    for (double v : {0.0, 1.0, -1.0, 0.5, 3.141592653589793, 1e30, -1e30}) {
        D f = D::from_double(v);
        assert(f.to_double() == v);
    }

    // Subnormal
    F tiny = F::from_double(std::numeric_limits<double>::denorm_min());
    assert(tiny.is_subnormal());

    std::cout << "All T81Float smoke tests passed!\n";
}
