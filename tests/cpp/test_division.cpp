#include <cassert>
#include <t81/core/T81Int.hpp>
#include <limits>
#include <stdexcept>

using t81::core::T81Int;

void test_comparisons() {
    T81Int<8> a{5};
    T81Int<8> b{-3};
    T81Int<8> c{5};

    assert(a == c);
    assert(a != b);
    assert(b < a);
    assert(a > b);
    assert(b <= a);
    assert(a >= b);
    assert(a >= c);
    assert(a <= c);
}

void test_abs() {
    T81Int<8> a{5};
    T81Int<8> b{-3};
    T81Int<8> c{0};

    assert(a.abs().to_binary<int>() == 5);
    assert(b.abs().to_binary<int>() == 3);
    assert(c.abs().to_binary<int>() == 0);
}

void test_division() {
    assert((T81Int<32>( 10) / T81Int<32>(3)).to_binary<int64_t>() ==  3);
    assert((T81Int<32>( 10) % T81Int<32>(3)).to_binary<int64_t>() ==  1);
    assert((T81Int<32>(-10) / T81Int<32>(3)).to_binary<int64_t>() == -3);
    assert((T81Int<32>(-10) % T81Int<32>(3)).to_binary<int64_t>() == -1);
    assert((T81Int<32>( 10) / T81Int<32>(-3)).to_binary<int64_t>() == -3);
    assert((T81Int<32>( 10) % T81Int<32>(-3)).to_binary<int64_t>() ==  1);
    assert((T81Int<32>(-10) / T81Int<32>(-3)).to_binary<int64_t>() ==  3);
    assert((T81Int<32>(-10) % T81Int<32>(-3)).to_binary<int64_t>() == -1);

    assert((T81Int<32>(0) / T81Int<32>(5)).to_binary<int64_t>() == 0);

    // Division by zero must throw
    bool threw = false;
    try { T81Int<32>(1) / T81Int<32>(0); }
    catch (const std::domain_error&) { threw = true; }
    assert(threw);

    // Identity check
    T81Int<32> a{10};
    T81Int<32> b{3};
    auto [q, r] = div_mod(a, b);
    assert((q * b + r) == a);
    assert((r.abs() < b.abs()) || r.is_zero());
}

int main() {
    test_comparisons();
    test_abs();
    test_division();
    return 0;
}
