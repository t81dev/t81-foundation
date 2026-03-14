#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <array>
#include "t81/native.hpp"
#include "t81/types/cell.hpp"

#define TEST_CHECK(cond)                                                                      \
  do {                                                                                        \
    if (!(cond)) {                                                                            \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1);                                                                           \
    }                                                                                         \
  } while (0)

using namespace t81;
using namespace t81::core;

// Scalar reference for a single trit negation
int8_t scalar_negate(int8_t trit) {
    return static_cast<int8_t>(-trit);
}

// Scalar reference for a single trit addition
std::pair<int8_t, int8_t> scalar_add(int8_t a, int8_t b, int8_t cin) {
    int sum = a + b + cin;
    int8_t cout = 0;
    if (sum > 1) {
        cout = 1;
        sum -= 3;
    } else if (sum < -1) {
        cout = -1;
        sum += 3;
    }
    return {static_cast<int8_t>(sum), cout};
}

void test_negation_invariants() {
    std::cout << "Testing T81 SIMD negation invariants..." << std::endl;

    // Test all 81 valid byte combinations (3^4 = 81)
    for (int i = 0; i < 256; ++i) {
        uint8_t byte = static_cast<uint8_t>(i);
        bool valid = true;
        std::array<int8_t, 4> trits;
        for (int j = 0; j < 4; ++j) {
            uint8_t code = (byte >> (j * 2)) & 0x3;
            if (code == 3) {
                valid = false;
                break;
            }
            trits[j] = (code == 0) ? -1 : (code == 1 ? 0 : 1);
        }

        if (!valid) continue;

        std::array<uint8_t, 32> data{};
        data.fill(byte);
        T81 val(data);
        T81 negated = -val;

        for (int b = 0; b < 32; ++b) {
            uint8_t neg_byte = negated.data[b];
            for (int j = 0; j < 4; ++j) {
                uint8_t code = (neg_byte >> (j * 2)) & 0x3;
                int8_t res_trit = (code == 0) ? -1 : (code == 1 ? 0 : 1);
                TEST_CHECK(res_trit == scalar_negate(trits[j]));
            }
        }
    }

    // Test double negation identity: -(-v) == v
    std::mt19937_64 rng(0x12345);
    for (int i = 0; i < 100; ++i) {
        std::array<uint8_t, 32> data;
        for (auto& b : data) {
            uint8_t byte = 0;
            for (int j = 0; j < 4; ++j) {
                byte |= (rng() % 3) << (j * 2);
            }
            b = byte;
        }
        T81 v(data);
        T81 double_neg = -(-v);
        for (int j = 0; j < 32; ++j) {
            TEST_CHECK(double_neg.data[j] == v.data[j]);
        }
    }
}

void test_addition_correctness() {
    std::cout << "Testing T81 SIMD addition correctness..." << std::endl;

    std::mt19937_64 rng(0x54321);
    auto rand_t81 = [&]() {
        std::array<uint8_t, 32> data;
        for (auto& b : data) {
            uint8_t byte = 0;
            for (int j = 0; j < 4; ++j) {
                byte |= (rng() % 3) << (j * 2);
            }
            b = byte;
        }
        return T81(data);
    };

    for (int iter = 0; iter < 1000; ++iter) {
        T81 a = rand_t81();
        T81 b = rand_t81();
        T81 sum = a + b;

        std::array<int8_t, 128> a_trits, b_trits, sum_trits;
        T81::UnpackDigits(a.data, a_trits);
        T81::UnpackDigits(b.data, b_trits);
        T81::UnpackDigits(sum.data, sum_trits);

        int8_t carry = 0;
        for (int i = 0; i < 128; ++i) {
            auto [s, cout] = scalar_add(a_trits[i], b_trits[i], carry);
            if (sum_trits[i] != s) {
                std::cerr << "Addition mismatch at trit " << i << " iter " << iter << std::endl;
                std::cerr << "a=" << (int)a_trits[i] << " b=" << (int)b_trits[i] << " cin=" << (int)carry << std::endl;
                std::cerr << "expected=" << (int)s << " got=" << (int)sum_trits[i] << std::endl;
                TEST_CHECK(sum_trits[i] == s);
            }
            carry = cout;
        }
        // Note: T81 addition currently ignores the final carry-out beyond 128 trits,
        // which is consistent with the current implementation.
    }
}

int main() {
    test_negation_invariants();
    test_addition_correctness();
    std::cout << "All SIMD correctness tests passed." << std::endl;
    return 0;
}
