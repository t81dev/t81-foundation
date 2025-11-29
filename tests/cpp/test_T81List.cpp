#include "t81/core/T81List.hpp"
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81List tests...\n";

    // Construction
    T81List<T81Int<27>> empty;
    assert(empty.empty());
    assert(empty.size() == 0);

    T81List<T81Int<27>> list{T81Int<27>(1), T81Int<27>(2), T81Int<27>(3)};
    assert(list.size() == 3);
    assert(list[0].to_int64() == 1);
    assert(list[1].to_int64() == 2);
    assert(list[2].to_int64() == 3);

    // Push back
    list.push_back(T81Int<27>(4));
    assert(list.size() == 4);
    assert(list.back().to_int64() == 4);

    // Front/back
    assert(list.front().to_int64() == 1);
    assert(list.back().to_int64() == 4);

    // Concatenation
    T81List<T81Int<27>> other{T81Int<27>(5), T81Int<27>(6)};
    T81List<T81Int<27>> combined = list + other;
    assert(combined.size() == 6);
    assert(combined[4].to_int64() == 5);
    assert(combined[5].to_int64() == 6);

    // Comparison
    T81List<T81Int<27>> same{T81Int<27>(1), T81Int<27>(2), T81Int<27>(3), T81Int<27>(4)};
    assert(list == same);
    assert(list != other);

    // Hash
    assert(list.hash() == same.hash());

    std::cout << "All T81List tests PASSED!\n";
    return 0;
}

