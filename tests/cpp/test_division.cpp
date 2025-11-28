// tests/cpp/test_division.cpp
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace t81::core;

int main() {
    std::cout << "Running T81Int division & remainder tests...\n";

    using I = T81Int<32>;

    // Positive ÷ positive
    assert((I(10) / I(3)).to_binary<int64_t>() == 3);
    assert((I(10) % I(3)).to_binary<int64_t>() == 1);

    // Negative ÷ positive
    assert((I(-10) / I(3)).to_binary<int64_t>() == -3);
    assert((I(-10) % I(3)).to_binary<int64_t>() == -1);

    // Positive ÷ negative
    assert((I(10) / I(-3)).to_binary<int64_t>() == -3);
    assert((I(10) % I(-3)).to_binary<int64_t>() == 1);

    // Negative ÷ negative
    assert((I(-10) / I(-3)).to_binary<int64_t>() == 3);
    assert((I(-10) % I(-3)).to_binary<int64_t>() == -1);

    // Zero dividend
    assert((I(0) / I(5)).to_binary<int64_t>() == 0);
    assert((I(0) % I(5)).to_binary<int64_t>() == 0);

    // Division by zero → throws std::domain_error
    bool threw = false;
    try {
        I(42) / I(0);
    } catch (const std::domain_error&) {
        threw = true;
    }
    assert(threw);

    // div_mod correctness: a = q*b + r  and  |r| < |b|
    I a{123456789};
    I b{-123};
    auto [q, r] = div_mod(a, b);
    assert(q * b + r == a);
    assert(r.abs() < b.abs() || r.is_zero());

    std::cout << "All division and remainder tests passed perfectly!\n";
    return 0;
}
