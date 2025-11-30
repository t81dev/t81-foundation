/**
 * @file T81String.hpp
 * @brief T81String — variable-length text for the T81 stack.
 *
 * Design (current implementation):
 *   • Logical alphabet: 27 symbols (A–Z plus space).
 *   • Storage: normalized ASCII string (A–Z + ' '), future-compatible with
 *     ternary/tryte packing.
 *   • Provides construction from C-style strings and std::string_view,
 *     concatenation, comparison, hashing, and a "_t81" user-defined literal.
 *
 * Notes:
 *   • All input is normalized to uppercase and non-[A–Z ] characters are
 *     mapped to space.
 *   • The previous bit-packed implementation had multiple correctness issues
 *     (insufficient bits per symbol, dangling string_view). This version
 *     prioritizes correctness and clean semantics; ternary packing can be
 *     added beneath the same API later.
 */

#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace t81 {

// ======================================================================
// T81String – normalized ASCII over a 27-symbol alphabet
// ======================================================================
class T81String {
public:
    using size_type = std::size_t;

private:
    // 27-symbol alphabet: A–Z + space.
    static constexpr std::array<char, 27> kAlphabet = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' '
    };

    // Invariant:
    //   • storage_ contains only characters from kAlphabet (A–Z and ' ').
    std::string storage_;

    static constexpr char normalize_char(char c) noexcept {
        // Uppercase if ASCII alpha.
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }

        // Accept uppercase A–Z and space directly.
        if ((c >= 'A' && c <= 'Z') || c == ' ') {
            return c;
        }

        // Fallback: map everything else to space.
        return ' ';
    }

public:
    //===================================================================
    // Construction
    //===================================================================

    T81String() = default;

    // From C string
    explicit T81String(const char* s) {
        if (s != nullptr) {
            assign(std::string_view{s});
        }
    }

    // From std::string_view
    explicit T81String(std::string_view sv) {
        assign(sv);
    }

    //===================================================================
    // Assignment
    //===================================================================

    void assign(std::string_view sv) {
        storage_.clear();
        storage_.reserve(sv.size());

        for (char c : sv) {
            storage_.push_back(normalize_char(c));
        }
    }

    //===================================================================
    // Conversion back to std::string / string_view
    //===================================================================

    [[nodiscard]] std::string str() const {
        return storage_;
    }

    [[nodiscard]] operator std::string() const {
        return storage_;
    }

    [[nodiscard]] std::string_view sv() const noexcept {
        return std::string_view{storage_};
    }

    //===================================================================
    // Size & Capacity
    //===================================================================

    [[nodiscard]] size_type size() const noexcept {
        return storage_.size();
    }

    // std::string-compatible alias used by tests
    [[nodiscard]] size_type length() const noexcept {
        return size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return storage_.empty();
    }

    [[nodiscard]] size_type capacity_chars() const noexcept {
        return storage_.capacity();
    }

    // Raw byte view, for low-level integrations.
    [[nodiscard]] std::span<const std::uint8_t> data() const noexcept {
        return std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(storage_.data()),
            storage_.size()
        );
    }

    [[nodiscard]] size_type tryte_count() const noexcept {
        // Placeholder: when ternary packing is introduced, this will reflect
        // the actual tryte count. For now, count bytes.
        return storage_.size();
    }

    //===================================================================
    // Concatenation
    //===================================================================

    [[nodiscard]] friend T81String operator+(const T81String& a,
                                             const T81String& b) {
        T81String result;
        result.storage_.reserve(a.storage_.size() + b.storage_.size());
        result.storage_.append(a.storage_);
        result.storage_.append(b.storage_);
        return result;
    }

    T81String& operator+=(const T81String& o) {
        storage_.append(o.storage_);
        return *this;
    }

    //===================================================================
    // Comparison – lexicographic on normalized storage
    //===================================================================

    [[nodiscard]] auto operator<=>(const T81String& o) const noexcept = default;
    [[nodiscard]] bool operator==(const T81String& o) const noexcept = default;

    //===================================================================
    // Hash – FNV-like mixing over normalized bytes
    //===================================================================

    [[nodiscard]] std::uint64_t hash() const noexcept {
        std::uint64_t h = 0x517cc1b727220a95ull; // seed
        for (unsigned char b : storage_) {
            h ^= static_cast<std::uint64_t>(b);
            h *= 0x9e3779b97f4a7c15ull; // golden ratio-ish
        }
        return h;
    }

    //===================================================================
    // Alphabet utilities
    //===================================================================

    [[nodiscard]] static constexpr std::array<char, 27> alphabet() noexcept {
        return kAlphabet;
    }

    [[nodiscard]] static bool is_valid_char(char c) noexcept {
        c = normalize_char(c);
        return (c >= 'A' && c <= 'Z') || c == ' ';
    }

    //===================================================================
    // Literals – convenient construction
    //===================================================================

    friend T81String operator""_t81(const char* s, std::size_t len) {
        return T81String(std::string_view{s, len});
    }
};

// ======================================================================
// std integration
// ======================================================================

} // namespace t81

namespace std {
template <>
struct hash<t81::T81String> {
    size_t operator()(const t81::T81String& s) const noexcept {
        return static_cast<size_t>(s.hash());
    }
};
} // namespace std

// ======================================================================
// Pretty printing
// ======================================================================

inline std::ostream& operator<<(std::ostream& os, const t81::T81String& s) {
    return os << s.sv();
}
