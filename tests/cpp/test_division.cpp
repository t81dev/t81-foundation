#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    using I = T81Int<32>;

    assert(I(10) / I(3) == I(3));
    assert(I(-10) / I(3) == I(-3));
    assert(I(10) / I(-3) == I(-3));
    assert(I(-10) / I(-3) == I(3));

    bool ok = false;
    try { I(1) / I(0); } catch (...) { ok = true; }
    assert(ok);

    std::cout << "Division tests passed!\n";
}