#include "t81/t81.hpp"
#include "t81/conversion.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    using namespace t81;

    core::T81Limb classic;
    for (int idx = 0; idx < core::T81Limb::TRYTES; ++idx) {
        classic.set_tryte(idx, static_cast<int8_t>((idx % 27) - 13));
    }

    auto native = from_classic(classic);
    auto roundtrip = to_classic(native);
    assert(std::memcmp(&classic, &roundtrip, sizeof(classic)) == 0);

    std::cout << "Conversion round-trip (classic ↔ native) OK\n";
    return 0;
}
