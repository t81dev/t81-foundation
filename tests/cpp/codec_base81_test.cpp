#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <t81/codec/base81.hpp>
#include <t81/core/base81.hpp>

int main() {
    using namespace t81::codec::base81;

    // Roundtrip bytes (leading zeros are not preserved by the canonical integer codec)
    {
        std::vector<std::uint8_t> bytes = {0xFFu, 0x10u};
        auto enc = encode_bytes(bytes);
        std::vector<std::uint8_t> dec;
        bool ok = decode_bytes(enc, dec);
        assert(ok);
        assert(dec == bytes);
        assert(t81::core::is_base81(enc));
    }

    // Empty input
    {
        std::vector<std::uint8_t> out;
        bool ok = decode_bytes("", out);
        assert(ok);
        assert(out.empty());
    }

    // Invalid character
    {
        std::vector<std::uint8_t> out;
        bool ok = decode_bytes("~", out); // '~' not in canonical alphabet
        assert(!ok);
        assert(!t81::core::is_base81("~"));
    }

    // Non-canonical leading zero should fail
    {
        std::vector<std::uint8_t> out;
        bool ok = decode_bytes("00", out);
        assert(!ok);
    }

    // Multi-byte codepoint correctness (uses UTF-8 symbols from the alphabet)
    {
        std::vector<std::uint8_t> bytes = {0x12u, 0x34u};
        auto enc = encode_bytes(bytes);
        assert(!enc.empty()); // sanity check

        std::vector<std::uint8_t> dec;
        bool ok = decode_bytes(enc, dec);
        assert(ok);
        assert(dec == bytes);
    }

    return 0;
}
