#include "t81/crypto/sha3.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <string>

namespace t81::crypto {

namespace {

constexpr uint64_t kKeccakfRoundConstants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

constexpr int kKeccakfRotc[] = {
     1,  3,  6, 10, 15, 21, 28, 36,
    45, 55,  2, 14, 27, 41, 56,  8,
    25, 43, 62, 18, 39, 61, 20, 44
};

constexpr int kKeccakfPiln[] = {
    10,  7, 11, 17, 18,  3,  5, 16,
     8, 21, 24,  4, 15, 23, 19, 13,
    12,  2, 20, 14, 22,  9,  6,  1
};

inline uint64_t rol(uint64_t value, int offset) noexcept {
    return (value << offset) | (value >> (64 - offset));
}

void keccakf(uint64_t state[25]) noexcept {
    for (int round = 0; round < 24; ++round) {
        uint64_t bc[5];
        for (int i = 0; i < 5; ++i) {
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }
        for (int i = 0; i < 5; ++i) {
            uint64_t temp = bc[(i + 4) % 5] ^ rol(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5) {
                state[j + i] ^= temp;
            }
        }
        uint64_t temp = state[1];
        for (int i = 0; i < 24; ++i) {
            int j = kKeccakfPiln[i];
            uint64_t t = state[j];
            state[j] = rol(temp, kKeccakfRotc[i]);
            temp = t;
        }
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 25; j += 5) {
                uint64_t a = state[j + i];
                uint64_t b = state[j + ((i + 1) % 5)];
                state[j + i] = a ^ ((~b) & state[j + ((i + 2) % 5)]);
            }
        }
        state[0] ^= kKeccakfRoundConstants[round];
    }
}

inline uint64_t load64(const uint8_t* data) noexcept {
    uint64_t value;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

inline void store64(uint8_t* out, uint64_t value) noexcept {
    std::memcpy(out, &value, sizeof(value));
}

}  // namespace

std::array<uint8_t, 64> sha3_512(std::span<const uint8_t> input) noexcept {
    constexpr size_t rate = 72;
    uint64_t state[25] = {};
    size_t offset = 0;
    while (offset + rate <= input.size()) {
        for (size_t lane = 0; lane < rate / 8; ++lane) {
            state[lane] ^= load64(input.data() + offset + lane * 8);
        }
        keccakf(state);
        offset += rate;
    }
    uint8_t block[rate];
    std::memset(block, 0, rate);
    const size_t remaining = input.size() - offset;
    if (remaining > 0) {
        std::memcpy(block, input.data() + offset, remaining);
    }
    block[remaining] = 0x06;
    block[rate - 1] |= 0x80;

    for (size_t lane = 0; lane < rate / 8; ++lane) {
        state[lane] ^= load64(block + lane * 8);
    }
    keccakf(state);

    std::array<uint8_t, 64> digest{};
    size_t produced = 0;
    while (produced < digest.size()) {
        for (size_t lane = 0; lane < rate / 8 && produced < digest.size(); ++lane) {
            uint8_t lane_buffer[8];
            store64(lane_buffer, state[lane]);
            for (size_t i = 0; i < 8 && produced < digest.size(); ++i) {
                digest[produced++] = lane_buffer[i];
            }
        }
        if (produced < digest.size()) {
            keccakf(state);
        }
    }
    return digest;
}

std::string sha3_512_hex(std::span<const uint8_t> input) {
    auto digest = sha3_512(input);
    static constexpr char hex_map[] = "0123456789abcdef";
    std::string out;
    out.reserve(digest.size() * 2);
    for (uint8_t byte : digest) {
        out.push_back(hex_map[byte >> 4]);
        out.push_back(hex_map[byte & 0x0f]);
    }
    return out;
}

}  // namespace t81::crypto
