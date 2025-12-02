#include "t81/core/T81Limb.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

using t81::core::T81Limb54;

constexpr int kTrials = 4096;

T81Limb54 random_limb(std::mt19937_64& rng) {
    T81Limb54 limb;
    std::uniform_int_distribution<int> dist(-13, 13);
    for (int i = 0; i < T81Limb54::TRYTES; ++i) {
        limb.set_tryte(i, static_cast<int8_t>(dist(rng)));
    }
    return limb;
}

int main() {
    std::mt19937_64 rng(0xCAFEBEEF);
    for (int t = 0; t < kTrials; ++t) {
        auto a = random_limb(rng);
        auto b = random_limb(rng);
        auto expected = T81Limb54::reference_mul(a, b);
        auto actual = T81Limb54::booth_mul(a, b);
        if (expected.to_trits() != actual.to_trits()) {
            std::cerr << "Booth mul mismatch" << std::endl;
            return 1;
        }
    }
    std::cout << "booth_mul matches reference_mul" << std::endl;
    return 0;
}
