#include "t81/core/T81Bytes.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Bytes tests...\n";

    // Empty construction
    T81Bytes empty;
    assert(empty.empty());
    assert(empty.size() == 0);

    // From array
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    T81Bytes bytes(data);
    assert(bytes.size() == 4);
    assert(bytes[0] == 0x01);
    assert(bytes[3] == 0x04);

    // From C string
    T81Bytes from_str("Hello");
    assert(from_str.size() == 5);

    // Concatenation
    uint8_t data2[] = {0x05, 0x06};
    T81Bytes bytes2(data2);
    T81Bytes combined = bytes + bytes2;
    assert(combined.size() == 6);
    assert(combined[4] == 0x05);
    assert(combined[5] == 0x06);

    // Slice
    T81Bytes slice = bytes.slice(1, 3);
    assert(slice.size() == 2);
    assert(slice[0] == 0x02);
    assert(slice[1] == 0x03);

    // Hash
    T81Bytes same(data);
    assert(bytes.hash() == same.hash());

    std::cout << "All T81Bytes tests PASSED!\n";
    return 0;
}

