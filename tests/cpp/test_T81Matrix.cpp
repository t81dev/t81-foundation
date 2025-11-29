#include "t81/core/T81Matrix.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Matrix tests...\n";

    using Mat = T81Matrix<T81Float<72,9>, 3, 3>;

    // Construction
    Mat zero;
    assert(zero(0, 0).is_zero());

    Mat m;
    m(0, 0) = T81Float<72,9>::from_double(1.0);
    m(0, 1) = T81Float<72,9>::from_double(2.0);
    m(0, 2) = T81Float<72,9>::from_double(3.0);
    m(1, 0) = T81Float<72,9>::from_double(4.0);
    m(1, 1) = T81Float<72,9>::from_double(5.0);
    m(1, 2) = T81Float<72,9>::from_double(6.0);
    m(2, 0) = T81Float<72,9>::from_double(7.0);
    m(2, 1) = T81Float<72,9>::from_double(8.0);
    m(2, 2) = T81Float<72,9>::from_double(9.0);

    assert(m(0, 0).to_double() > 0.9 && m(0, 0).to_double() < 1.1);
    assert(m(2, 2).to_double() > 8.9 && m(2, 2).to_double() < 9.1);

    // Transpose
    auto mt = m.transpose();
    assert(mt(0, 0).to_double() > 0.9 && mt(0, 0).to_double() < 1.1);
    assert(mt(1, 0).to_double() > 1.9 && mt(1, 0).to_double() < 2.1);  // was m(0,1)
    assert(mt(0, 1).to_double() > 3.9 && mt(0, 1).to_double() < 4.1);  // was m(1,0)

    // Arithmetic
    Mat m2 = m;
    Mat sum = m + m2;
    assert(sum(0, 0).to_double() > 1.9 && sum(0, 0).to_double() < 2.1);

    Mat diff = m - m2;
    assert(diff(0, 0).is_zero());

    std::cout << "All T81Matrix tests PASSED!\n";
    return 0;
}

