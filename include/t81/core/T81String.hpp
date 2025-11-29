//======================================================================
// T81String.hpp – Dynamic ternary string, tryte-native, constexpr-friendly
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
#include <compare>
#include <cstring>

namespace t81 {

// ======================================================================
// T81String – variable-length text encoded in balanced ternary trytes
// ======================================================================
class T81String {
    // Each character is a base-27 trit-tuple (3 trits = 27 symbols)
    // Perfect for natural language: 26-letter alphabet + space
    static constexpr size_t TRITS_PER_CHAR = 3;
    static constexpr size_t CHARS_PER_TRYTE = 4;                    // 12 trits per tryte → 4 chars
    static constexpr size_t TRITS_PER_TRYTE = 12;

    // 27-symbol alphabet: A-Z + space (fits exactly in 3 balanced trits)
    static constexpr std::array<char, 27> alphabet = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' '
    };

    alignas(64) std::vector<uint8_t> trytes_;   // packed storage, 12 trits per byte

public:
    //===================================================================
    // Construction
    //===================================================================
    constexpr T81String() noexcept = default;

    // From C++ string literal – the natural way
    constexpr T81String(const char* s) { assign(s); }

    // From std::string_view
    constexpr T81String(std::string_view sv) { assign(sv); }

    // From raw trytes (advanced)
    explicit constexpr T81String(std::span<const uint8_t> trytes)
        : trytes_(trytes.begin(), trytes.end()) {}

    //===================================================================
    // Assignment
    //===================================================================
    constexpr void assign(std::string_view sv) {
        trytes_.clear();
        trytes_.reserve((sv.size() + CHARS_PER_TRYTE - 1) / CHARS_PER_TRYTE);

        size_t i = 0;
        while (i < sv.size()) {
            uint8_t packed = 0;
            uint8_t shift = 0;
            for (size_t j = 0; j < CHARS_PER_TRYTE && i < sv.size(); ++j, ++i) {
                char c = sv[i];
                auto it = std::find(alphabet.begin(), alphabet.end(), (c >= 'a' && c <= 'z') ? c-'a'+'A' : c);
                uint8_t value = (it == alphabet.end()) ? 26 : static_cast<uint8_t>(it - alphabet.begin());
                packed |= (value << shift);
                shift += TRITS_PER_CHAR;
            }
            trytes_.push_back(packed);
        }
    }

    //===================================================================
    // Conversion back to UTF-8 / std::string
    //===================================================================
    [[nodiscard]] std::string str() const {
        std::string s;
        s.reserve(size() * 1.1);  // rough estimate

        for (size_t t = 0; t < trytes_.size(); ++t) {
            uint8_t packed = trytes_[t];
            for (size_t j = 0; j < CHARS_PER_TRYTE; ++j) {
                size_t char_idx = (t * CHARS_PER_TRYTE) + j;
                if (char_idx >= size()) break;
                uint8_t value = (packed >> (j * TRITS_PER_CHAR)) & 0b111;
                s += alphabet[value];
            }
        }
        // Trim trailing spaces
        while (!s.empty() && s.back() == ' ') s.pop_back();
        return s;
    }

    [[nodiscard]] operator std::string() const { return str(); }
    [[nodiscard]] std::string_view sv() const { return str(); }

    //===================================================================
    // Size & Capacity
    //===================================================================
    [[nodiscard]] constexpr size_t size() const noexcept {
        return trytes_.size() * CHARS_PER_TRYTE;
    }

    [[nodiscard]] constexpr size_t capacity_trytes() const noexcept {
        return trytes_.capacity();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return trytes_.empty();
    }

    //===================================================================
    // Concatenation – the crown jewel
    //===================================================================
    [[nodiscard]] friend constexpr T81String operator+(const T81String& a, const T81String& b) noexcept {
        T81String result;
        result.trytes_.reserve(a.trytes_.size() + b.trytes_.size());
        result.trytes_.insert(result.trytes_.end(), a.trytes_.begin(), a.trytes_.end());
        result.trytes_.insert(result.trytes_.end(), b.trytes_.begin(), b.trytes_.end());
        return result;
    }

    constexpr T81String& operator+=(const T81String& o) noexcept {
        trytes_.insert(trytes_.end(), o.trytes_.begin(), o.trytes_.end());
        return *this;
    }

    //===================================================================
    // Comparison – lexicographic on trytes
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81String& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81String&) const noexcept = default;

    //===================================================================
    // Hash – perfect for T81Map<T81String, V>
    //===================================================================
    [[nodiscard]] constexpr uint64_t hash() const noexcept {
        uint64_t h = 0x517cc1b727220a95;  // FNV-like seed
        for (uint8_t b : trytes_) {
            h ^= b;
            h *= 0x9e3779b97f4a7c15;
        }
        return h;
    }

    //===================================================================
    // Raw access
    //===================================================================
    [[nodiscard]] constexpr std::span<const uint8_t> data() const noexcept { return trytes_; }
    [[nodiscard]] constexpr size_t tryte_count() const noexcept { return trytes_.size(); }

    //===================================================================
    // Literals – the future feels good
    //===================================================================
    friend constexpr T81String operator""_t81(const char* s, size_t len) {
        return T81String(std::string_view(s, len));
    }
};

// ====================================================================== std integration =========================================================
template<> struct std::hash<T81String> {
    constexpr size_t operator()(const T81String& s) const noexcept {
        return static_cast<size_t>(s.hash());
    }
};

//===== Pretty printing ==========================================================
inline std::ostream& operator<<(std::ostream& os, const T81String& s) {
    return os << s.str();
}

} // namespace t81

// ======================================================================
// Example usage – this is how the future writes text
// ======================================================================
/*
constexpr auto hello = "HELLO WORLD"_t81;
constexpr auto world = "WORLD"_t81;
constexpr auto greeting = hello + " " + world;  // "HELLO WORLD WORLD"

static_assert(greeting.size() == 16);
static_assert(greeting.str() == "HELLO WORLD WORLD");
*/
