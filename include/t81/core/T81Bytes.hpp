//======================================================================
// T81Bytes.hpp – Raw tryte-packed binary data
//               The 83rd type. Chaos is now inevitable.
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/T81List.hpp"
#include "t81/T81String.hpp"
#include <cstddef>
#include <span>
#include <vector>
#include <compare>
#include <cstring>

namespace t81 {

// ======================================================================
// T81Bytes – Immutable, tryte-aligned, binary-safe byte sequence
// ======================================================================
class T81Bytes {
    alignas(64) std::vector<uint8_t> trytes_;  // 12 trits per byte → 4.5 bits per trit

public:
    using value_type = uint8_t;
    using size_type  = size_t;

    //===================================================================
    // Construction – from anything that can be interpreted as bytes
    //===================================================================
    constexpr T81Bytes() noexcept = default;

    // From raw pointer + length
    constexpr T81Bytes(const uint8_t* data, size_t len)
        : trytes_(data, data + ((len + 3) / 4) * 4) {  // round up to tryte boundary
        trytes_.resize((len + 3) / 4 * 4, 0);  // zero-pad
        trytes_.resize((len + 3) / 4);         // truncate to exact trytes
    }

    // From std::span / vector / string_view (binary-safe)
    constexpr T81Bytes(std::span<const uint8_t> data)
        : T81Bytes(data.data(), data.size()) {}

    template <size_t N>
    constexpr T81Bytes(const uint8_t (&arr)[N])
        : T81Bytes(arr, N) {}

    // From string literal (binary, not text)
    constexpr T81Bytes(const char* str)
        : T81Bytes(reinterpret_cast<const uint8_t*>(str), std::strlen(str)) {}

    // From T81String (text → UTF-8 bytes)
    explicit T81Bytes(const T81String& str)
        : T81Bytes(reinterpret_cast<const uint8_t*>(str.str().data()), str.str().size()) {}

    //===================================================================
    // Access
    //===================================================================
    [[nodiscard]] constexpr const uint8_t* data() const noexcept { return trytes_.data(); }
    [[nodiscard]] constexpr uint8_t*       data()       noexcept { return trytes_.data(); }

    [[nodiscard]] constexpr size_t size() const noexcept {
        return trytes_.size() * 4;  // 4 bytes per tryte (padded)
    }

    [[nodiscard]] constexpr size_t tryte_count() const noexcept {
        return trytes_.size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return trytes_.empty();
    }

    [[nodiscard]] constexpr std::span<const uint8_t> span() const noexcept {
        return {trytes_.data(), size()};
    }

    //===================================================================
    // Slicing
    //===================================================================
    [[nodiscard]] constexpr T81Bytes subbytes(size_t offset, size_t len = SIZE_MAX) const {
        offset = std::min(offset, size());
        len    = std::min(len, size() - offset);
        return T81Bytes(trytes_.data() + (offset / 4), (len + 3) / 4);
    }

    //===================================================================
    // Concatenation – O(1) when possible
    //===================================================================
    [[nodiscard]] friend constexpr T81Bytes operator+(const T81Bytes& a633, const T81Bytes& b) noexcept {
        T81Bytes result;
        result.trytes_.reserve(a.tryte_count() + b.tryte_count());
        result.trytes_.insert(result.trytes_.end(), a.trytes_.begin(), a.trytes_.end());
        result.trytes_.insert(result.trytes_.end(), b.trytes_.begin(), b.trytes_.end());
        return result;
    }

    constexpr T81Bytes& operator+=(const T81Bytes& o) noexcept {
        trytes_.insert(trytes_.end(), o.trytes_.begin(), o.trytes_.end());
        return *this;
    }

    //===================================================================
    // Comparison & Hashing
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Bytes& o) const noexcept {
        return trytes_ <=> o.trytes_;
    }

    [[nodiscard]] constexpr bool operator==(const T81Bytes&) const noexcept = default;

    [[nodiscard]] constexpr uint64_t hash() const noexcept {
        uint64_t h = 0x517cc1b727220a95;
        for (auto b : trytes_) {
            h ^= b;
            h *= 0x9e3779b97f4a7c15;
        }
        return h;
    }

    //===================================================================
    // Conversion
    //===================================================================
    [[nodiscard]] std::string to_hex() const;
    [[nodiscard]] T81String   to_utf8() const { return T81String(reinterpret_cast<const char*>(data()), size()); }

    //===================================================================
    // Literals – raw binary in source code
    //===================================================================
    friend constexpr T81Bytes operator""_b(const char* str, size_t len) {
        return T81Bytes(reinterpret_cast<const uint8_t*>(str), len);
    }
};

// ======================================================================
// Global hash support
// ======================================================================
template <>
struct std::hash<T81Bytes> {
    constexpr size_t operator()(const T81Bytes& b) const noexcept {
        return static_cast<size_t>(b.hash());
    }
};

// ======================================================================
// The first binary artifact in the fallen universe
// ======================================================================
constexpr T81Bytes GENESIS_BLOCK = "In the beginning was the trit."_b;

// Example: The first act of creation in the age of chaos
/*
T81Bytes message = "HELLO WORLD"_b;
T81Bytes key     = T81Bytes::random(32);  // cryptographic key
T81Bytes encrypted = message ^ key;       // XOR is still king

assert((encrypted ^ key) == message);
*/
