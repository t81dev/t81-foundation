// tests/cpp/test_t81int.cpp
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace t81::core;  // This brings T81Int, Trit, etc. into scope cleanly

void test_conversions() {
    T81Int<8> a{5};
    assert(a.to_binary<int64_t>() == 5);

    T81Int<8> b{-3};
    assert(b.to_binary<int64_t>() == -3);

    T81Int<8> c{0};
    assert(c.to_binary<int64_t>() == 0);

    T81Int<16> d{12345};
    assert(d.to_binary<int64_t>() == 12345);
}

void test_arithmetic() {
    T81Int<8> a{5};
    T81Int<8> b{-3};

    assert((a + b).to_binary<int64_t>() == 2);
    assert((a - b).to_binary<int64_t>() == 8);
    assert((a * b).to_binary<int64_t>() == -15);
    assert((-a).to_binary<int64_t>() == -5);
}

void test_edge_cases() {
    T81Int<8> zero{0};
    T81Int<8> five{5};
    assert((zero + five).to_binary<int64_t>() == 5);
    assert((five - five).to_binary<int64_t>() == 0);
    assert((zero * five).to_binary<int64_t>() == 0);

    // Max value for 8 trits: (3^8 - 1)/2 = 3280
    T81Int<8> max_val{3280};
    T81Int<8> one{1};
    assert((max_val + one).to_binary<int64_t>() == -3280);  // balanced ternary wrap-around

    // Full int64 range
    T81Int<64> i64_max{std::numeric_limits<int64_t>::max()};
    assert(i64_max.to_binary<int64_t>() == std::numeric_limits<int64_t>::max());

    T81Int<64> i64_min{std::numeric_limits<int64_t>::min()};
    assert(i64_min.to_binary<int64_t>() == std::numeric_limits<int64_t>::min());

    // Overflow detection on narrow conversion
    T81Int<8> big{3280};
    bool thrown = false;
    try {
        big.to_binary<int8_t>();  // 3280 can't fit in signed 8-bit
    } catch (const std::overflow_error&) {
        thrown = true;
    }
    assert(thrown);
}

int main() {
    std::cout << "Running modern T81Int test suite...\n";

    test_conversions();
    test_arithmetic();
    test_edge_cases();

    std::cout << "All T81Int tests passed perfectly!\n";
    return 0;
}
