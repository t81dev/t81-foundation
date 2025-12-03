#include "t81/core/T81Limb.hpp"

#include <cstring>
#include <iostream>
#include <random>
#include <string>

using namespace t81::core;

constexpr int TRIALS = 4096;

T81Limb RandomLimb(std::mt19937_64& rng, std::uniform_int_distribution<int>& dist) {
    T81Limb limb;
    for (int i = 0; i < T81Limb::TRYTES; ++i)
        limb.set_tryte(i, static_cast<int8_t>(dist(rng)));
    return limb;
}

std::string PrintTrits(const T81Limb& limb) {
    auto trits = limb.to_trits();
    std::string out;
    out.reserve(T81Limb::TRITS * 3);
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        out += std::to_string(static_cast<int>(trits[i]));
        if (i + 1 < T81Limb::TRITS) out += ' ';
    }
    return out;
}

int main() {
    std::mt19937_64 rng(0xB0BE110B);
    std::uniform_int_distribution<int> dist(-13, 13);

    for (int t = 0; t < TRIALS; ++t) {
        T81Limb a = RandomLimb(rng, dist);
        T81Limb b = RandomLimb(rng, dist);

        T81Limb expected = T81Limb::reference_mul(a, b);
        T81Limb actual = a * b;

        if (std::memcmp(&expected, &actual, sizeof(T81Limb)) != 0) {
            std::cerr << "Multiplication mismatch at trial " << t << '\n';
            std::cerr << "a        : " << PrintTrits(a) << '\n';
            std::cerr << "b        : " << PrintTrits(b) << '\n';
            std::cerr << "expected : " << PrintTrits(expected) << '\n';
            std::cerr << "actual   : " << PrintTrits(actual) << '\n';
            return 1;
        }
    }

    std::cout << "bohemian_mul matches operator* on " << TRIALS << " random trials — multiplication verified\n";
    return 0;
}
