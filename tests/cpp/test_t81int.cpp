#include <cassert>
#include <t81/core/T81Int.hpp>
#include <limits>
#include <stdexcept>

using t81::core::T81Int;

void test_conversions() {
    T81Int<8> a{5};
    assert(a.to_binary<int64_t>() == 5);

    T81Int<8> b{-3};
    assert(b.to_binary<int64_t>() == -3);

    T81Int<8> c{0};
    assert(c.to_binary<int64_t>() == 0);

    // Test a larger number
    T81Int<16> d{12345};
    assert(d.to_binary<int64_t>() == 12345);
}

void test_arithmetic() {
    T81Int<8> a{5};
    T81Int<8> b{-3};

    // Addition
    T81Int<8> sum = a + b;
    assert(sum.to_binary<int64_t>() == 2);

    // Subtraction
    T81Int<8> diff = a - b;
    assert(diff.to_binary<int64_t>() == 8);

    // Multiplication
    T81Int<8> prod = a * b;
    assert(prod.to_binary<int64_t>() == -15);

    // Negation
    T81Int<8> neg_a = -a;
    assert(neg_a.to_binary<int64_t>() == -5);
}

void test_edge_cases() {
    // Zero tests
    T81Int<8> zero{0};
    T81Int<8> five{5};
    assert((zero + five).to_binary<int64_t>() == 5);
    assert((five - five).to_binary<int64_t>() == 0);
    assert((zero * five).to_binary<int64_t>() == 0);
    assert((five * zero).to_binary<int64_t>() == 0);

    // Overflow test (8 trits max value is 3280)
    T81Int<8> max_val{3280};
    T81Int<8> one{1};
    T81Int<8> overflow_sum = max_val + one;
    assert(overflow_sum.to_binary<int64_t>() == -3280); // Wraps around

    // Boundary value conversions
    T81Int<64> int64_max{std::numeric_limits<int64_t>::max()};
    assert(int64_max.to_binary<int64_t>() == std::numeric_limits<int64_t>::max());

    T81Int<64> int64_min{std::numeric_limits<int64_t>::min()};
    assert(int64_min.to_binary<int64_t>() == std::numeric_limits<int64_t>::min());

    // Overflow detection
    T81Int<8> overflow_val{128};
    bool thrown = false;
    try {
        overflow_val.to_binary<int8_t>();
    } catch (const std::overflow_error& e) {
        thrown = true;
    }
    assert(thrown);
}

int main() {
    test_conversions();
    test_arithmetic();
    test_edge_cases();
    return 0;
}
