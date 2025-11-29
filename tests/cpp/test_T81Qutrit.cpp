#include "t81/core/T81Qutrit.hpp"
#include <cassert>
#include <iostream>

using namespace t81::core;
using namespace t81::core::qutrit;

int main() {
    std::cout << "Running T81Qutrit tests...\n";

    // Basic states
    T81Qutrit q0 = ZERO;
    T81Qutrit q1 = ONE;
    T81Qutrit q2 = TWO;

    assert(q0.to_int64() == 0);
    assert(q1.to_int64() == 1);
    assert(q2.to_int64() == -1);  // TWO is -1 in balanced ternary

    // Comparison
    assert(q0 < q1);
    assert(q2 < q1);  // -1 < 1
    assert(q0 != q1);
    assert(q1 == q1);

    // Arithmetic
    T81Qutrit sum = q1 + q1;
    assert(sum.to_int64() == 2);

    T81Qutrit diff = q1 - q2;  // 1 - (-1) = 2
    assert(diff.to_int64() == 2);

    std::cout << "All T81Qutrit tests PASSED!\n";
    return 0;
}

