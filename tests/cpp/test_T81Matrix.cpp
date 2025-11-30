// tests/cpp/test_T81Matrix.cpp
// Matrix tests that are robust to T81Float<72,9> internal representation.

#include "t81/core/T81Matrix.hpp"
#include "t81/core/T81Float.hpp"

#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Matrix tests...\n";

    using Scalar = T81Float<72,9>;
    using Mat    = T81Matrix<Scalar, 3, 3>;

    // Pre-constructed scalars so equality is purely "same ternary value"
    const Scalar one   = Scalar::from_double(1.0);
    const Scalar two   = Scalar::from_double(2.0);
    const Scalar three = Scalar::from_double(3.0);
    const Scalar four  = Scalar::from_double(4.0);
    const Scalar five  = Scalar::from_double(5.0);
    const Scalar six   = Scalar::from_double(6.0);
    const Scalar seven = Scalar::from_double(7.0);
    const Scalar eight = Scalar::from_double(8.0);
    const Scalar nine  = Scalar::from_double(9.0);

    // 1) Default construction → all zero
    {
        Mat zero{};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                assert(zero(i, j).is_zero());
        std::cout << "  [OK] Default construction (zero matrix)\n";
    }

    // 2) Manual fill
    Mat m{};
    m(0, 0) = one;
    m(0, 1) = two;
    m(0, 2) = three;
    m(1, 0) = four;
    m(1, 1) = five;
    m(1, 2) = six;
    m(2, 0) = seven;
    m(2, 1) = eight;
    m(2, 2) = nine;

    // Check that the assigned values are exactly what we stored
    assert(m(0, 0) == one);
    assert(m(0, 1) == two);
    assert(m(0, 2) == three);
    assert(m(1, 0) == four);
    assert(m(1, 1) == five);
    assert(m(1, 2) == six);
    assert(m(2, 0) == seven);
    assert(m(2, 1) == eight);
    assert(m(2, 2) == nine);
    std::cout << "  [OK] Manual fill and element access\n";

    // 3) Transpose – purely structural check
    Mat mt = m.transpose();
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            assert(mt(i, j) == m(j, i));
    std::cout << "  [OK] Transpose\n";

    // 4) Addition and subtraction – check with algebraic identities
    Mat m2  = m;
    Mat sum = m + m2;
    Mat diff = m - m2;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            // sum should be element-wise double of m
            Scalar expected_sum = m(i, j) + m2(i, j);
            assert(sum(i, j) == expected_sum);

            // diff should be zero everywhere
            assert(diff(i, j).is_zero());
        }
    }
    std::cout << "  [OK] Addition and subtraction\n";

    // 5) Copy construction and assignment
    {
        Mat copy1 = m;   // copy constructor
        Mat copy2; 
        copy2 = m;       // copy assignment

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                assert(copy1(i, j) == m(i, j));
                assert(copy2(i, j) == m(i, j));
            }
        std::cout << "  [OK] Copy construction and assignment\n";
    }

    std::cout << "All T81Matrix tests PASSED!\n";
    return 0;
}
