#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Int::to_binary tests...\n";

    T81Int<16> a{42};
    assert(a.to_binary<int>() == 42);
    assert(a.to_binary<long>() == 42L);

    T81Int<16> b{-100};
    assert(b.to_binary<int>() == -100);

    // Test overflow
    T81Int<16> c{300}; // Fits in int, but not in int8_t
    bool threw = false;
    try {
        (void)c.to_binary<std::int8_t>();
    } catch (const std::overflow_error&) {
        threw = true;
    }
    assert(threw);

    std::cout << "All T81Int::to_binary tests passed!\n";
    return 0;
}
