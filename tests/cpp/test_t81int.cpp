#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "T81Int tests...\n";

    T81Int<8> a{5}, b{-3};
    assert((a + b).to_binary<int>() == 2);
    assert((a - b).to_binary<int>() == 8);

    T81Int<32> x{10}, y{3};
    assert(x / y == T81Int<32>(3));
    assert(x % y == T81Int<32>(1));

    bool threw = false;
    try { T81Int<32>(1) / T81Int<32>(0); }
    catch (...) { threw = true; }
    assert(threw);

    std::cout << "All T81Int tests passed!\n";
}