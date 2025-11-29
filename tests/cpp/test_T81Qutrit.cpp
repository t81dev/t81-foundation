#include "t81/core/T81Qutrit.hpp"
#include <cassert>
#include <iostream>

using namespace t81;
using namespace t81::qutrit;  // ZERO, ONE, TWO

int main() {
    std::cout << "Running T81Qutrit tests...\n";

    T81Qutrit q0 = ZERO;  // |0⟩   →  0
    T81Qutrit q1 = ONE;   // |1⟩   → +1
    T81Qutrit q2 = TWO;   // |2⟩   → -1 (balanced -1)

    // Basic integer mapping (via underlying T81Int<2>)
    assert(q0.to_int64() == 0);
    assert(q1.to_int64() == 1);
    assert(q2.to_int64() == -1);

    // Distinct basis states
    assert(q0 != q1);
    assert(q0 != q2);
    assert(q1 != q2);

    // Copy / assign sanity
    T81Qutrit q0_copy = q0;
    T81Qutrit q1_copy = q1;
    T81Qutrit q2_copy = q2;

    assert(q0_copy == q0);
    assert(q1_copy == q1);
    assert(q2_copy == q2);

    std::cout << "All T81Qutrit tests PASSED!\n";
    return 0;
}
