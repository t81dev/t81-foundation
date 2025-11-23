#include <t81/codec/base81.hpp>
#include <array>
#include <stdexcept>

namespace t81::codec::base81 {

std::string_view alphabet() {
    // Pick and freeze this; do NOT change once tests & hashes depend on it.
    static constexpr char kAlphabet[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "!#$%&()*+,-./:;<=>?@[\\]^_"; // 81 chars total; adjust to taste
    static_assert(sizeof(kAlphabet) - 1 == 81, "Alphabet must have 81 chars");
    return std::string_view{kAlphabet, 81};
}

// Helper: map char -> digit index
static int char_to_digit(char c) {
    auto a = alphabet();
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] == c) return static_cast<int>(i);
    }
    return -1;
}

std::string encode_bytes(const std::vector<std::uint8_t>& data) {
    if (data.empty()) return std::string{};

    // Interpret the byte array as a big-endian integer and convert to base-81.
    // This is simple, portable, and deterministic.
    // Implementation detail: operate via arbitrary-precision division in base 256.

    // Copy to a mutable buffer of "digits" in base 256.
    std::vector<std::uint8_t> buf = data;
    std::string out;

    while (!buf.empty()) {
        std::vector<std::uint8_t> next;
        next.reserve(buf.size());

        std::uint16_t carry = 0;
        for (auto b : buf) {
            std::uint16_t cur = (carry << 8) | b; // base 256
            std::uint8_t q = static_cast<std::uint8_t>(cur / 81);
            std::uint8_t r = static_cast<std::uint8_t>(cur % 81);
            if (!next.empty() || q != 0) {
                next.push_back(q);
            }
            carry = r;
        }
        out.push_back(alphabet()[carry]);
        buf.swap(next);
    }

    // Digits were produced least significant first
    std::reverse(out.begin(), out.end());
    return out;
}

bool decode_bytes(std::string_view s, std::vector<std::uint8_t>& out) {
    out.clear();
    if (s.empty()) return true;

    // Convert base-81 string back into a big-endian base-256 byte vector.
    std::vector<std::uint8_t> buf; // base-256 digits, big-endian

    for (char c : s) {
        int d = char_to_digit(c);
        if (d < 0) {
            return false; // invalid character
        }

        std::vector<std::uint8_t> next;
        next.reserve(buf.size() + 1);

        std::uint16_t carry = static_cast<std::uint16_t>(d);
        for (auto b : buf) {
            std::uint16_t cur = static_cast<std::uint16_t>(b) * 81 + carry;
            std::uint8_t q = static_cast<std::uint8_t>(cur >> 8); // div 256
            std::uint8_t r = static_cast<std::uint8_t>(cur & 0xFF);
            if (!next.empty() || q != 0) {
                next.push_back(q);
            }
            carry = r;
        }
        if (!next.empty() || carry != 0) {
            next.push_back(static_cast<std::uint8_t>(carry));
        }

        buf.swap(next);
    }

    out = std::move(buf);
    return true;
}

} // namespace t81::codec::base81
