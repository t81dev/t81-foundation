#include "t81/core/T81Limb.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <random>

using t81::core::T81Limb;

constexpr int kTrials = 4096;
constexpr int kWideTrits = T81Limb::TRITS * 2;

static void normalize(std::array<int, kWideTrits>& acc) {
    for (int i = 0; i + 1 < kWideTrits; ++i) {
        int carry = (acc[i] + (acc[i] >= 0 ? 1 : -1)) / 3;
        acc[i] -= carry * 3;
        acc[i + 1] += carry;
    }
    int carry = (acc[kWideTrits - 1] + (acc[kWideTrits - 1] >= 0 ? 1 : -1)) / 3;
    acc[kWideTrits - 1] -= carry * 3;
}

static T81Limb canonical_high_half(const T81Limb& a, const T81Limb& b) {
    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();
    std::array<int, kWideTrits> accum{};
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        for (int j = 0; j < T81Limb::TRITS; ++j) {
            accum[i + j] += static_cast<int>(a_trits[i]) * static_cast<int>(b_trits[j]);
        }
    }
    normalize(accum);
    normalize(accum);
    normalize(accum);

    std::array<int8_t, kWideTrits> trits{};
    for (int i = 0; i < kWideTrits; ++i) {
        int value = accum[i];
        if (value > 1) value = 1;
        else if (value < -1) value = -1;
        trits[i] = static_cast<int8_t>(value);
    }

    std::array<int8_t, T81Limb::TRITS> high{};
    std::copy_n(trits.begin() + T81Limb::TRITS, T81Limb::TRITS, high.begin());
    return T81Limb::from_trits(high);
}

int main() {
    std::mt19937_64 rng(0xC001CAFE);
    std::uniform_int_distribution<int> dist(-13, 13);

    for (int trial = 0; trial < kTrials; ++trial) {
        T81Limb a;
        T81Limb b;
        for (int i = 0; i < T81Limb::TRYTES; ++i) {
            a.set_tryte(i, static_cast<int8_t>(dist(rng)));
            b.set_tryte(i, static_cast<int8_t>(dist(rng)));
        }

        auto [low, high] = T81Limb::mul_wide(a, b);
        auto expected_high = canonical_high_half(a, b);
        if (std::memcmp(&high, &expected_high, sizeof(T81Limb)) != 0) {
            std::cerr << "wide high mismatch on trial " << trial << std::endl;
            return 1;
        }
    }

    std::cout << "mul_wide high half matches canonical high half" << std::endl;
    return 0;
}
