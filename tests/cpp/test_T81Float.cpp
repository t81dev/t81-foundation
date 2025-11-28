#include <cassert>
#include <iostream>
#include <limits>
#include <cmath>
#include <compare>

#include "t81/core/T81Float.hpp"

using namespace t81::core;

// A helper to make floats from bit patterns for testing.
// This is a friend of T81Float, so it can access private members.
template<size_t M, size_t E>
T81Float<M, E> make_float(Trit sign, int64_t exp_val, const T81Int<M>& mant_val) {
    T81Float<M, E> f;
    f._pack(sign, T81Int<E>(exp_val), mant_val);
    return f;
}

void test_special_values() {
    using Float = T81Float<18, 9>;
    
    auto zero = Float::zero();
    auto p_inf = Float::inf(true);
    auto n_inf = Float::inf(false);
    auto nae = Float::nae();

    assert(zero.is_zero());
    assert(!zero.is_inf());
    assert(!zero.is_nae());
    assert(!zero.is_negative());

    assert(p_inf.is_inf());
    assert(!p_inf.is_zero());
    assert(!p_inf.is_nae());
    assert(!p_inf.is_negative());

    assert(n_inf.is_inf());
    assert(!n_inf.is_zero());
    assert(!n_inf.is_nae());
    assert(n_inf.is_negative());

    assert(nae.is_nae());
    assert(!nae.is_inf());
    assert(!nae.is_zero());
}

void test_comparison() {
    using Float = T81Float<18, 9>;

    auto zero = Float::zero();
    using Int = T81Int<10>;
    auto p_one = Float(Int(1));
    auto n_one = Float(Int(-1));
    auto p_two = Float(Int(2));
    auto p_inf = Float::inf(true);
    auto n_inf = Float::inf(false);

    assert(zero == zero);
    assert(p_one == p_one);
    assert(p_one != p_two);
    assert(p_one != n_one);
    
    assert(p_one > zero);
    assert(n_one < zero);
    assert(p_two > p_one);

    assert(p_inf > p_two);
    assert(n_inf < n_one);

    // NaE comparisons
    auto nae = Float::nae();
    assert(!(nae == nae));
    assert(nae != nae);
    assert(!(nae < nae));
    assert(!(nae > nae));
    assert(!(nae <= nae));
    assert(!(nae >= nae));
}

void test_abs_and_negation() {
    using Float = T81Float<18, 9>;

    auto zero = Float::zero();
    auto p_one = Float(T81Int<4>(1));
    auto n_one = Float(T81Int<4>(-1));
    auto p_inf = Float::inf(true);
    auto n_inf = Float::inf(false);

    assert(p_one.abs() == p_one);
    assert(n_one.abs() == p_one);
    assert(p_inf.abs() == p_inf);
    assert(n_inf.abs() == p_inf);

    assert((-p_one) == n_one);
    assert((-n_one) == p_one);
    assert((-p_inf) == n_inf);
    assert((-n_inf) == p_inf);
    assert((-zero) == zero);
}

void test_int_conversion() {
    using Float = T81Float<18, 9>;
    using Int = T81Int<10>;

    Float from_int_one(Int(1));
    auto one_data = from_int_one.internal_data();
    auto one_sign = one_data.get_trit(Float::TotalTrits - 1);
    
    T81Int<Float::ExponentTrits> one_exp;
    for (size_t i = 0; i < Float::ExponentTrits; ++i) {
        one_exp.set_trit(i, one_data.get_trit(Float::MantissaTrits + i));
    }
    
    assert(one_sign == Trit::P);
    assert(one_exp.to_int64() == 0);

    Float from_int_zero(Int(0));
    assert(from_int_zero.is_zero());
}

void test_arithmetic() {
    using Float = T81Float<18, 9>;
    using Int = T81Int<10>;

    Float one(Int(1));
    Float two(Int(2));
    Float three(Int(3));
    Float neg_one(Int(-1));

    // 1.0 + 1.0 == 2.0
    auto sum = one + one;
    if (sum != two) {
        std::cout << "DEBUG: one: " << one.str() << std::endl;
        std::cout << "DEBUG: two: " << two.str() << std::endl;
        std::cout << "DEBUG: one + one: " << sum.str() << std::endl;
    }
    assert(sum == two);
    
    // 2.0 + 1.0 == 3.0
    assert((two + one) == three);

    // 1.0 + (-1.0) == 0.0
    assert((one + neg_one).is_zero());

    // Commutativity
    assert((one + two) == (two + one));

    // Subtraction
    assert((two - one) == one);
    assert((three - one) == two);
    assert((three - two) == one);
    assert((one - one).is_zero());
    assert((one - two) == neg_one);
}


int main() {
    test_special_values();
    test_comparison();
    test_abs_and_negation();
    test_int_conversion();
    test_arithmetic();

    return 0;
}
