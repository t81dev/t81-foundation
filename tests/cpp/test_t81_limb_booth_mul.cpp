#include "t81/core/T81Limb.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

using t81::core::T81Limb;

constexpr int kTrials = 4096;

T81Limb random_limb(std::mt19937_64& rng) {
    T81Limb limb;
    std::uniform_int_distribution<int> dist(-13, 13);
    for (int i = 0; i < T81Limb::TRYTES; ++i) {
        limb.set_tryte(i, static_cast<int8_t>(dist(rng)));
    }
    return limb;
}

int main() {
    std::mt19937_64 rng(0xCAFEBEEF);
    for (int t = 0; t < kTrials; ++t) {
        auto a = random_limb(rng);
        auto b = random_limb(rng);
        auto expected = T81Limb::reference_mul(a, b);
        auto actual = T81Limb::booth_mul(a, b);
        if (std::memcmp(&expected, &actual, sizeof(T81Limb)) != 0) {
            std::cerr << "Booth mul mismatch" << std::endl;
            return 1;
        }
    }
    std::cout << "booth_mul matches reference_mul" << std::endl;
    return 0;
}
