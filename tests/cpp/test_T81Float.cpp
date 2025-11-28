#include <cassert>
#include <iostream>
#include <limits>
#include <cmath>

#include "t81/core/T81Float.hpp"
#include "t81/core/T81Int.hpp"

using namespace t81::core;

void test_special_values() {
    using Float = T81Float<18, 9>;

    auto z  = Float::zero();
    auto pz = Float::zero(true);
    auto nz = Float::zero(false);
    auto pi = Float::inf(true);
    auto ni = Float::inf(false);
    auto nae = Float::nae();

    assert(z.is_zero() && pz.is_zero() && nz.is_zero());
    assert(pi.is_inf() && !pi.is_negative());
    assert(ni.is_inf() &&  ni.is_negative());
    assert(nae.is_nae());
}

void test_comparison() {
    using Float = T81Float<18, 9>;

    auto zero = Float::zero();
    auto one  = Float(T81Int<10>(1));
    auto two  = Float(T81Int<10>(2));
    auto neg_one = Float(T81Int<10>(-1));
    auto p_inf = Float::inf(true);
    auto n_inf = Float::inf(false);
    auto nae   = Float::nae();

    assert(zero == zero);
    assert(one  == one);
    assert(one  != two);
    assert(one  > zero);
    assert(neg_one < zero);
    assert(two  > one);
    assert(p_inf > two);
    assert(n_inf < neg_one);

    // NAE poisons everything
    assert(!(nae == nae));
    assert(nae != nae);
    assert(std::isunordered(nae, nae));
}

void test_abs_and_negation() {
    using Float = T81Float<18, 9>;

    auto zero = Float::zero();
    auto one  = Float(T81Int<4>(1));
    auto neg_one = Float(T81Int<4>(-1));
    auto p_inf = Float::inf(true);
    auto n_inf = Float::inf(false);
    auto nae   = Float::nae();

    assert(one.abs()   == one);
    assert(neg_one.abs() == one);
    assert(p_inf.abs() == p_inf);
    assert(n_inf.abs() == p_inf);

    assert(-one   == neg_one);
    assert(-neg_one == one);
    assert(-p_inf == n_inf);
    assert(-n_inf == p_inf);
    assert(-zero  == zero);
    assert(-nae   == nae);
}

void test_int_conversion() {
    using Float = T81Float<18, 9>;

    Float f1(T81Int<12>(1));
    Float f0(T81Int<12>(0));
    Float fn(T81Int<12>(-1));

    assert(!f1.is_zero() && !f1.is_negative());
    assert( f0.is_zero());
    assert(!fn.is_zero() &&  fn.is_negative());

    // 3⁵ = 243 → needs exponent > 0
    Float f243(T81Int<20>(243));
    assert(f243.to_double() == 243.0);
}

void test_arithmetic() {
    using Float = T81Float<18, 9>;

    Float one (T81Int<10>( 1));
    Float two (T81Int<10>( 2));
    Float three(T81Int<10>( 3));
    Float neg_one(T81Int<10>(-1));

    assert((one  + one)   == two);
    assert((two  + one)   == three);
    assert((one  + neg_one).is_zero());
    assert((two  - one)   == one);
    assert((three - two)  == one);
    assert((one  - two)   == neg_one);
}

void test_multiplication() {
    using Float = T81Float<18, 9>;

    Float a(T81Int<10>(5));
    Float b(T81Int<10>(7));

    assert((a * b) == Float(T81Int<20>(35)));
}

void test_division() {
    using Float = T81Float<18, 9>;

    Float six (T81Int<10>(6));
    Float three(T81Int<10>(3));
    Float two  (T81Int<10>(2));

    assert((six / three) == two);
    assert((Float(T81Int<10>(1)) / two) == Float::from_double(0.5));
}

void test_fma() {
    using Float = T81Float<18, 9>;

    Float a(T81Int<10>(2));
    Float b(T81Int<10>(3));
    Float c(T81Int<10>(4));

    // 2*3 + 4 = 10
    assert(fma(a, b, c) == Float(T81Int<12>(10)));
}

void test_roundtrip_double() {
    using Float = T81Float<52, 11>;   // t81d size

    for (double x : {
            0.0, -0.0, 1.0, -1.0, 3.141592653589793,
            1e10, -1e10, 1e-10, std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity()
         }) {
        Float f = Float::from_double(x);
        double back = f.to_double();

        if (std::isnan(x)) {
            assert(f.is_nae());
        } else if (std::isinf(x)) {
            assert(f.is_inf());
            assert(std::signbit(back) == std::signbit(x));
        } else {
            assert(back == x);   // exact for all normal values in range
        }
    }
}

void test_subnormals() {
    using Float = T81Float<18, 9>;

    Float tiny = Float::from_double(std::numeric_limits<double>::denorm_min());
    assert(tiny.is_subnormal());
    assert(tiny.to_double() > 0.0);
}

int main() {
    std::cout << "Running T81Float manual tests...\n";

    test_special_values();
    test_comparison();
    test_abs_and_negation();
    test_int_conversion();
    test_arithmetic();
    test_multiplication();
    test_division();
    test_fma();
    test_roundtrip_double();
    test_subnormals();

    std::cout << "All tests passed!\n";
    return 0;
}
