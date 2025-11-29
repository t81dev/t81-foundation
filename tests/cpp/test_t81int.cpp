//
// Comprehensive Test for t81::T81Int
//

#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <string>

// Use a specific, non-trivial size for most tests.
using TestInt = t81::T81Int<11>; // Range: -29524 to 29524

// Helper to check for thrown exceptions
template<typename Func>
void assert_throws(Func&& func) {
    bool threw = false;
    try {
        func();
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);
}

void test_constructors_and_assignment() {
    std::cout << "  - Testing Constructors & Assignment...\n";

    // Default constructor
    TestInt zero;
    assert(zero.is_zero());
    assert(zero.to_int64() == 0);

    // Construct from integer
    TestInt a(42);
    assert(a.to_int64() == 42);

    TestInt b(-121);
    assert(b.to_int64() == -121);

    // Copy constructor and assignment
    TestInt c = a;
    assert(c.to_int64() == 42);
    TestInt d;
    d = b;
    assert(d.to_int64() == -121);

    // Test overflow on construction
    // 3^11 = 177147. Max value is (3^11 - 1) / 2 = 88573
    t81::T81Int<11> max_val_11(88573);
    assert(max_val_11.to_int64() == 88573);
    assert_throws([]{ t81::T81Int<11> too_big(88574); });

    t81::T81Int<11> min_val_11(-88573);
    assert(min_val_11.to_int64() == -88573);
    assert_throws([]{ t81::T81Int<11> too_small(-88574); });
}

void test_conversions() {
    std::cout << "  - Testing Conversions...\n";

    // to_int64
    TestInt a(100);
    assert(a.to_int64() == 100);
    assert(TestInt(-100).to_int64() == -100);

    // to_binary
    assert(a.to_binary<int>() == 100);
    assert(a.to_binary<short>() == 100);
    assert(a.to_binary<std::int8_t>() == 100);

    TestInt big(130);
    assert_throws([&big]{ (void)big.to_binary<std::int8_t>(); }); // 130 does not fit in int8_t
    TestInt small(-130);
    assert_throws([&small]{ (void)small.to_binary<std::int8_t>(); }); // -130 does not fit in int8_t


    // to_trit_string
    // 4 = 1*3 + 1 = 0++ (little-endian: ++0)
    t81::T81Int<3> four(4);
    assert(four.to_trit_string() == "0++");
    // 13 = 1*9 + 1*3 + 1 = +++
    t81::T81Int<3> thirteen(13);
    assert(thirteen.to_trit_string() == "+++");
    // -5 = -++
    t81::T81Int<3> neg_five(-5);
    assert(neg_five.to_trit_string() == "-++");
}

void test_arithmetic() {
    std::cout << "  - Testing Arithmetic...\n";

    TestInt a(100);
    TestInt b(17);
    TestInt neg_b(-17);
    TestInt zero;

    // Addition
    assert((a + b).to_int64() == 117);
    assert((a + neg_b).to_int64() == 83);
    assert((a + zero) == a);

    // Subtraction
    assert((a - b).to_int64() == 83);
    assert((b - a).to_int64() == -83);
    assert((a - zero) == a);
    assert((a - a).is_zero());

    // Multiplication
    assert((b * TestInt(10)).to_int64() == 170);
    assert((b * neg_b).to_int64() == -289);
    assert((a * zero).is_zero());
    assert((a * TestInt(1)) == a);

    // Division and Modulo
    assert((a / b).to_int64() == 5); // 100 / 17 = 5
    assert((a % b).to_int64() == 15); // 100 % 17 = 15
    assert_throws([]{ (void)(TestInt(1) / TestInt(0)); });
    assert_throws([]{ (void)(TestInt(1) % TestInt(0)); });

    // Unary minus
    assert((-b).to_int64() == -17);
    assert(-(-b) == b);

    // Increment/Decrement
    TestInt i(10);
    assert((++i).to_int64() == 11);
    assert((i++).to_int64() == 11);
    assert(i.to_int64() == 12);
    assert((--i).to_int64() == 11);
    assert((i--).to_int64() == 11);
    assert(i.to_int64() == 10);
}

void test_comparison() {
    std::cout << "  - Testing Comparison...\n";

    TestInt a(100);
    TestInt b(101);
    TestInt a_copy(100);

    assert(a == a_copy);
    assert(a != b);
    assert(a < b);
    assert(b > a);
    assert(a <= b);
    assert(a <= a_copy);
    assert(b >= a);
    assert(a >= a_copy);

    TestInt neg_a(-100);
    TestInt neg_b(-101);

    assert(neg_a > neg_b);
    assert(neg_b < neg_a);
}

void test_indexing_and_trits() {
    std::cout << "  - Testing Indexing & Trits...\n";
    t81::T81Int<5> val; // Zero

    val[0] = t81::Trit::P; // 1
    val[1] = t81::Trit::P; // 3
    // 1*1 + 1*3 = 4
    assert(val.to_int64() == 4);
    assert(val[0] == t81::Trit::P);
    assert(val[1] == t81::Trit::P);
    assert(val[2] == t81::Trit::Z);

    val[2] = t81::Trit::N; // -9
    // 4 + (-9) = -5
    assert(val.to_int64() == -5);

    // sign_trit
    assert(TestInt(100).sign_trit() == t81::Trit::P);
    assert(TestInt(-100).sign_trit() == t81::Trit::N);
    assert(TestInt(0).sign_trit() == t81::Trit::Z);
}

void test_shifts() {
    std::cout << "  - Testing Shifts...\n";

    t81::T81Int<8> a(10); // 10 = 1 * 9 + 1 * 1 = P0P0 in trits

    // Left shift (multiplies by 3^k)
    t81::T81Int<8> a_shl_1 = a << 1;
    assert(a_shl_1.to_int64() == 30);

    t81::T81Int<8> a_shl_2 = a << 2;
    assert(a_shl_2.to_int64() == 90);

    // Right shift (divides by 3^k)
    t81::T81Int<8> a_shr_1 = a >> 1;
    assert(a_shr_1.to_int64() == 3); // 10 / 3 = 3

    t81::T81Int<8> a_shr_2 = a >> 2;
    assert(a_shr_2.to_int64() == 1); // 10 / 9 = 1

    // Shift into zero
    assert((a << 8).is_zero());
    assert((a >> 8).is_zero());
}


int main() {
    std::cout << "Running T81Int tests...\n";

    test_constructors_and_assignment();
    test_conversions();
    test_arithmetic();
    test_comparison();
    test_indexing_and_trits();
    test_shifts();

    std::cout << "All T81Int tests passed!\n";
}