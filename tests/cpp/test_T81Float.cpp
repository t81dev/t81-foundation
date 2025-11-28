#include <cassert>
#include <iostream>
#include <limits>
#include <cmath>

#include "t81/core/T81Float.hpp"
#include "t81/core/T81Int.hpp"

using namespace t81::core;

int main() {
    std::cout << "Running T81Float manual tests...\n";

    using Float = T81Float<18, 9>;

    // Special values
    auto z   = Float::zero();
    auto pz  = Float::zero(true);
    auto nz  = Float::zero(false);
    auto pi  = Float::inf(true);
    auto ni  = Float::inf(false);
    auto nae = Float::nae();

    assert(z.is_zero() && pz.is_zero() && nz.is_zero());
    assert(pi.is_inf() && !pi.is_negative());
    assert(ni.is_inf() && ni.is_negative());
    assert(nae.is_nae());

    // Basic construction from integers
    auto one     = Float(T81Int<12>(1));
    auto two     = Float(T81Int<12>(2));
    auto three   = Float(T81Int<12>(3));
    auto neg_one = Float(T81Int<12>(-1));

    assert(one > z);
    assert(neg_one < z);
    assert(two > one);
    assert(-one == neg_one);
    assert(-neg_one == one);
    assert(one.abs() == one);
    assert(neg_one.abs() == one);

    // Arithmetic
    assert((one + one) == two);
    assert((two + one) == three);
    assert((one + neg_one).is_zero());
    assert((two - one) == one);
    assert((one - two) == neg_one);

    // Multiplication
    auto five = Float(T81Int<12>(5));
    auto seven = Float(T81Int<12>(7));
    assert((five * seven) == Float(T81Int<24>(35)));

    // Division
    auto six = Float(T81Int<12>(6));
    assert((six / three) == two);

    // FMA
    assert(fma(two, three, five) == Float(T81Int<24>(11))); // 2*3 + 5 = 11

    // Double round-trip (use larger format for full double fidelity)
    using DoubleFloat = T81Float<52, 11>;
    for (double x : {0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 3.141592653589793, 1e100, -1e100}) {
        DoubleFloat f = DoubleFloat::from_double(x);
        double back = f.to_double();
        if (std::isnan(x)) {
            assert(f.is_nae());
        } else if (std::isinf(x)) {
            assert(f.is_inf());
            assert(std::signbit(back) == std::signbit(x));
        } else {
            assert(back == x);
        }
    }

    // Subnormals
    Float tiny = Float::from_double(std::numeric_limits<double>::denorm_min());
    assert(tiny.is_subnormal());
    assert(tiny.to_double() > 0.0);

    std::cout << "All T81Float tests passed!\n";
    return 0;
}
