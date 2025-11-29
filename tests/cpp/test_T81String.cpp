#include "t81/core/T81String.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81String tests...\n";

    // Construction
    T81String empty;
    assert(empty.empty());
    assert(empty.length() == 0);

    T81String hello("Hello");
    assert(!hello.empty());
    assert(hello.length() == 5);

    // Construction from string
    T81String lit("World");
    assert(lit.length() == 5);

    // Concatenation
    T81String space(" ");
    T81String combined = hello + space + lit;
    assert(combined.length() == hello.length() + 1 + lit.length());

    // Comparison
    T81String hello2("Hello");
    assert(hello == hello2);
    assert(hello != lit);

    // Hash
    assert(hello.hash() == hello2.hash());
    assert(hello.hash() != lit.hash());

    // String view access
    std::string_view sv = hello.sv();
    assert(sv == "HELLO");  // Should be normalized to uppercase

    // String operations
    T81String str("ABC");
    assert(str.length() == 3);

    std::cout << "All T81String tests PASSED!\n";
    return 0;
}

