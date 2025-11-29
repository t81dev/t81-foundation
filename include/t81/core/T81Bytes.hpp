/**
 * @file T81Bytes.hpp
 * @brief T81Bytes — immutable-style, binary-safe byte sequence.
 *
 * Design (current implementation):
 *   • Stores raw bytes in a cache-friendly std::vector<uint8_t>.
 *   • No padding/rounding logic; size() is the exact byte length.
 *   • Value semantics: concatenation and slicing return new T81Bytes.
 *
 * Notes:
 *   • The API is shaped so the internal representation can later move to a
 *     true tryte-aligned ternary buffer without breaking user code.
 */

#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/T81String.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace t81 {

// ======================================================================
// T81Bytes – binary-safe byte sequence
// ======================================================================
class T81Bytes {
public:
    using value_type = std::uint8_t;
    using size_type  = std::size_t;

private:
    alignas(64) std::vector<std::uint8_t> bytes_;

public:
    //===================================================================
    // Construction – from anything that can be interpreted as bytes
    //===================================================================

    T81Bytes() = default;

    // From raw pointer + length
    T81Bytes(const std::uint8_t* data, size_type len)
        : bytes_(data, data + len) {}

    // From std::span
    T81Bytes(std::span<const std::uint8_t> data)
        : bytes_(data.begin(), data.end()) {}

    // From fixed-size array
    template <std::size_t N>
    T81Bytes(const std::uint8_t (&arr)[N])
        : bytes_(arr, arr + N) {}

    // From C string (treat as raw bytes, excluding terminating '\0')
    T81Bytes(const char* str)
        : T81Bytes(reinterpret_cast<const std::uint8_t*>(str),
                   str ? std::strlen(str) : 0) {}

    // From T81String (text → UTF-8-ish bytes)
    explicit T81Bytes(const T81String& str) {
        const std::string s = str.str();
        bytes_.assign(reinterpret_cast<const std::uint8_t*>(s.data()),
                      reinterpret_cast<const std::uint8_t*>(s.data()) + s.size());
    }

    //===================================================================
    // Access
    //===================================================================

    [[nodiscard]] const std::uint8_t* data() const noexcept {
        return bytes_.data();
    }

    [[nodiscard]] std::uint8_t* data() noexcept {
        return bytes_.data();
    }

    [[nodiscard]] size_type size() const noexcept {
        return bytes_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return bytes_.empty();
    }

    [[nodiscard]] std::span<const std::uint8_t> span() const noexcept {
        return std::span<const std::uint8_t>(bytes_.data(), bytes_.size());
    }

    [[nodiscard]] size_type tryte_count() const noexcept {
        // Placeholder: when tryte-packing is introduced, this can reflect
        // the real tryte count. For now, 1 byte ≈ 1 logical slot.
        return bytes_.size();
    }

    //===================================================================
    // Slicing
    //===================================================================

    [[nodiscard]] T81Bytes subbytes(size_type offset,
                                    size_type len = static_cast<size_type>(-1)) const {
        const size_type n = size();
        if (offset > n) {
            offset = n;
        }
        const size_type max_len = n - offset;
        if (len > max_len) {
            len = max_len;
        }
        return T81Bytes(bytes_.data() + offset, len);
    }

    //===================================================================
    // Concatenation
    //===================================================================

    [[nodiscard]] friend T81Bytes operator+(const T81Bytes& a,
                                            const T81Bytes& b) {
        T81Bytes result;
        result.bytes_.reserve(a.bytes_.size() + b.bytes_.size());
        result.bytes_.insert(result.bytes_.end(), a.bytes_.begin(), a.bytes_.end());
        result.bytes_.insert(result.bytes_.end(), b.bytes_.begin(), b.bytes_.end());
        return result;
    }

    T81Bytes& operator+=(const T81Bytes& o) {
        bytes_.insert(bytes_.end(), o.bytes_.begin(), o.bytes_.end());
        return *this;
    }

    //===================================================================
    // Comparison & Hashing
    //===================================================================

    [[nodiscard]] auto operator<=>(const T81Bytes& o) const noexcept = default;
    [[nodiscard]] bool operator==(const T81Bytes& o) const noexcept  = default;

    [[nodiscard]] std::uint64_t hash() const noexcept {
        std::uint64_t h = 0x517cc1b727220a95ull; // seed
        for (std::uint8_t b : bytes_) {
            h ^= static_cast<std::uint64_t>(b);
            h *= 0x9e3779b97f4a7c15ull;
        }
        return h;
    }

    //===================================================================
    // Conversion
    //===================================================================

    [[nodiscard]] std::string to_hex() const {
        static constexpr char kHex[] = "0123456789abcdef";
        std::string out;
        out.reserve(bytes_.size() * 2);

        for (std::uint8_t b : bytes_) {
            out.push_back(kHex[(b >> 4) & 0x0F]);
            out.push_back(kHex[b & 0x0F]);
        }
        return out;
    }

    // Interpret bytes as UTF-8/ASCII and normalize via T81String.
    [[nodiscard]] T81String to_utf8() const {
        return T81String(
            std::string_view(reinterpret_cast<const char*>(data()), size()));
    }

    //===================================================================
    // Literals – raw binary in source code
    //===================================================================

    friend T81Bytes operator""_b(const char* str, std::size_t len) {
        return T81Bytes(reinterpret_cast<const std::uint8_t*>(str), len);
    }
};

} // namespace t81

// ======================================================================
// Global hash support
// ======================================================================

namespace std {
template <>
struct hash<t81::T81Bytes> {
    size_t operator()(const t81::T81Bytes& b) const noexcept {
        return static_cast<size_t>(b.hash());
    }
};
} // namespace std

// ======================================================================
// Well-known constant – Genesis block
// ======================================================================

namespace t81 {
inline const T81Bytes GENESIS_BLOCK = "In the beginning was the trit."_b;
} // namespace t81

// Example usage:
/*
using namespace t81;

T81Bytes message = "HELLO WORLD"_b;
T81Bytes key     = T81Bytes::from_random(32); // hypothetical future API
T81Bytes encrypted = message + key;

auto hex_repr = encrypted.to_hex();
*/
