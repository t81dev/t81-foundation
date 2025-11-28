#include "t81/core/T81Float.hpp"
#include <cassert>
#include <iostream>

using namespace t81::core;

int main() {
    std::cout << "Running T81Float tests...\n";

    assert(t81_nextafter(one, two) > one);
}

    F a = F::from_double(1.0);
    F b = F::from_double(2.0);
    F z = F::zero();

    assert(a.to_double() != 0.0);
    assert(b.to_double() > a.to_double());
    assert(z.is_zero());
    assert(!a.is_zero());

    std::cout << "All T81Float tests PASSED!\n";
    return 0;
}