/**
 * @file T81Symbol.hpp
 * @brief T81Symbol — 81-trit interned symbolic token.
 *
 * Each symbol is represented as an 81-trit balanced-ternary integer (T81Int<81>)
 * and is intended for fast equality comparison and hashing. In the current
 * implementation, symbols are backed by a monotonically increasing 64-bit
 * integer ID stored in the underlying T81Int, which keeps them safely within
 * the T81Int::to_int64() domain.
 */

#pragma once

#include "t81/core/T81Int.hpp"

#include <atomic>
#include <cstdint>
#include <compare>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>

namespace t81::core {

class T81Symbol {
public:
    using Raw       = T81Int<81>;
    using size_type = std::size_t;

    static constexpr size_type kTrits = 81;

    // ------------------------------------------------------------------
    // Constructors
    // ------------------------------------------------------------------

    constexpr T81Symbol() noexcept = default;

    // Construct directly from raw 81-trit value (primarily for intern table).
    explicit constexpr T81Symbol(const Raw& raw) noexcept : value_(raw) {}

    // ------------------------------------------------------------------
    // Public factories — user-visible entry points
    // ------------------------------------------------------------------

    // Intern a string into a unique symbol (current implementation uses
    // a simple global counter; future versions will use a ternary hash map).
    static T81Symbol intern(std::string_view sv) noexcept;

    // Construct from a 64-bit ID (used by the current intern implementation).
    static constexpr T81Symbol from_u64(std::uint64_t x) noexcept {
        return T81Symbol(Raw(static_cast<std::int64_t>(x)));
    }

    // Construct from an existing raw value.
    static constexpr T81Symbol from_raw(const Raw& r) noexcept {
        return T81Symbol(r);
    }

    // ------------------------------------------------------------------
    // Core properties
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr Raw raw() const noexcept { return value_; }

    [[nodiscard]] bool is_zero() const noexcept {
        return value_.is_zero();
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr auto operator<=>(const T81Symbol&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Symbol&) const noexcept  = default;

    // ------------------------------------------------------------------
    // Hash
    // ------------------------------------------------------------------

    // Hash of the underlying 64-bit ID (current implementation).
    [[nodiscard]] std::uint64_t hash() const {
        // to_int64() is safe because the only constructors we use for now
        // (from_u64 / intern) store values that fit in 64 bits.
        std::uint64_t v = static_cast<std::uint64_t>(value_.to_int64());

        // Simple 64-bit mixing (SplitMix64 style)
        v += 0x9E3779B97F4A7C15ull;
        v = (v ^ (v >> 30)) * 0xBF58476D1CE4E5B9ull;
        v = (v ^ (v >> 27)) * 0x94D049BB133111EBull;
        v ^= (v >> 31);
        return v;
    }

    // ------------------------------------------------------------------
    // Debugging / representation helpers
    // ------------------------------------------------------------------

    // Balanced-ternary debug form (uses T81Int’s string representation).
    [[nodiscard]] std::string str() const {
        return value_.to_string();
    }

    // Hex representation of the underlying 64-bit ID.
    [[nodiscard]] std::string hex() const {
        std::uint64_t v = static_cast<std::uint64_t>(value_.to_int64());
        static constexpr char kHex[] = "0123456789abcdef";

        char buf[16];
        for (int i = 15; i >= 0; --i) {
            buf[i] = kHex[v & 0xF];
            v >>= 4;
        }

        return std::string(buf, buf + 16);
    }

private:
    Raw value_{}; // 81-trit payload
};

// ======================================================================
// Global intern table — placeholder implementation
// ======================================================================

inline T81Symbol T81Symbol::intern(std::string_view /*sv*/) noexcept {
    // Real implementation: map normalized input → ternary symbol using
    // a lock-free hash table in ternary memory.
    //
    // Current implementation: monotonic 64-bit counter stored in T81Int<81>.
    struct InternTable {
        alignas(64) std::atomic<std::uint64_t> counter{0};
    };

    static InternTable table{};
    const std::uint64_t id =
        table.counter.fetch_add(1, std::memory_order_relaxed);

    return T81Symbol::from_u64(id);
}

// ======================================================================
// Predefined symbols — conventional IDs in the current scheme
// ======================================================================

namespace symbols {
inline constexpr T81Symbol EOS  = T81Symbol::from_u64(0); // End-of-sequence
inline constexpr T81Symbol PAD  = T81Symbol::from_u64(1); // Padding
inline constexpr T81Symbol BOS  = T81Symbol::from_u64(2); // Begin-of-sequence
inline constexpr T81Symbol UNK  = T81Symbol::from_u64(3); // Unknown token
inline constexpr T81Symbol MASK = T81Symbol::from_u64(4); // Mask token
} // namespace symbols

} // namespace t81::core

// ======================================================================
// std integration
// ======================================================================

namespace std {
template <>
struct hash<t81::core::T81Symbol> {
    size_t operator()(const t81::core::T81Symbol& s) const noexcept {
        return static_cast<size_t>(s.hash());
    }
};
} // namespace std

// ======================================================================
// Stream output
// ======================================================================

inline std::ostream& operator<<(std::ostream& os, t81::core::T81Symbol s) {
    return os << "§" << std::hex << s.hex();
}
