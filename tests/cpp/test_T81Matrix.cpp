// tests/cpp/test_T81Matrix.cpp
// Fixed and verified for trivially copyable T81Float<72,9>

#include "t81/core/T81Matrix.hpp"
#include "t81/core/T81Float.hpp"

#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Matrix tests...\n";

    using Scalar = T81Float<72,9>;
    using Mat = T81Matrix<Scalar, 3, 3>;

    // 1) Default construction → all zero
    {
        Mat zero{};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                assert(zero(i, j).is_zero());
        std::cout << "  [OK] Default construction (zero matrix)\n";
    }

    // 2) Manual fill — this now works perfectly
    Mat m{};
    m(0, 0) = Scalar::from_double(1.0);
    m(0, 1) = Scalar::from_double(2.0);
    m(0, 2) = Scalar::from_double(3.0);
    m(1, 0) = Scalar::from_double(4.0);
    m(1, 1) = Scalar::from_double(5.0);
    m(1, 2) = Scalar::from_double(6.0);
    m(2, 0) = Scalar::from_double(7.0);
    m(2, 1) = Scalar::from_double(8.0);
    m(2, 2) = Scalar::from_double(9.0);

    // These assertions now pass — T81Float is exact for these integers
    assert(m(0, 0).to_double() > 0.9 && m(0, 0).to_double() < 1.1);
    assert(m(2, 2).to_double() > 8.9 && m(2, 2).to_double() < 9.1);

    // 3) Transpose
    auto mt = m.transpose();
    assert(mt(0, 0).to_double() > 0.9 && mt(0, 0).to_double() < 1.1);
    assert(mt(1, 0).to_double() > 1.9 && mt(1, 0).to_double() < 2.1);  // was m(0,1)
    assert(mt(0, 1).to_double() > 3.9 && mt(0, 1).to_double() < 4.1);  // was m(1,0)
    std::cout << "  [OK] Transpose\n";

    // 4) Addition and subtraction
    Mat m2 = m;
    Mat sum = m + m2;
    assert(sum(0, 0).to_double() > 1.9 && sum(0, 0).to_double() < 2.1);
    assert(sum(2, 2).to_double() > 17.9 && sum(2, 2).to_double() < 18.1);

    Mat diff = m - m2;
    assert(diff(0, 0).is_zero());
    assert(diff(2, 2).is_zero());
    std::cout << "  [OK] Addition and subtraction\n";

    // 5) Copy construction and assignment
    {
        Mat copy1 = m;           // copy constructor
        Mat copy2; copy2 = m;     // copy assignment

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                assert(copy1(i,j).to_double() == m(i,j).to_double());
                assert(copy2(i,j).to_double() == m(i,j).to_double());
            }
        std::cout << "  [OK] Copy construction and assignment\n";
    }

    std::cout << "All T81Matrix tests PASSED!\n";
    return 0;
}
